/* 
   gcc -Wall -Werror -fPIC `pkg-config ecore --cflags --libs` `pkg-config ecore-imf --cflags --libs` `pkg-config elementary --cflags --libs` -c ecore_imf_uim.c 
   
   gcc -Wall -Werror -shared -Wl,-soname,uim.so -o uim.so ./ecore_imf_uim.o -luim -luim-scm
 */
#include <Eina.h>
#include <Ecore.h>
#include <Ecore_Input.h>
#include <Ecore_IMF.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <langinfo.h>
#include <assert.h>
#include <uim/uim.h>
#include <uim/uim-util.h>
#include <uim/uim-im-switcher.h>
#include <locale.h>
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#define __UNUSED__ __attribute__ ((unused))

#define UIM_DEFAULT_ENCODING "UTF-8"

typedef struct _Ecore_IMF_Context_Data Ecore_IMF_Context_Data;
struct _Ecore_IMF_Context_Data
{
    uim_context uim_ctx;
    Eina_List *preedit_segments;
    int prev_preedit_length;
    Eina_Bool use_preedit;
    void *win;
};
static Ecore_IMF_Context_Data* imf_context_data_new();
static void imf_context_data_destroy(Ecore_IMF_Context_Data *imf_context_data);

typedef struct _UIMPreedit_Segment UIMPreedit_Segment;
struct _UIMPreedit_Segment
{
    int attr;
    Eina_Unicode *str;
};
static UIMPreedit_Segment* uimpreedit_segment_new(int attr, const char *str);
static void uimpreedit_segment_destroy(UIMPreedit_Segment *preedit_segment);
static int uimpreedit_segment_length(Eina_List *preedit_segments);

static void
commit_cb(void *ptr, const char *str)
{
   EINA_LOG_DBG("in");
   Ecore_IMF_Context *ctx = (Ecore_IMF_Context *)ptr;
   ecore_imf_context_commit_event_add(ctx, str);
}

static void
preedit_clear_cb(void *ptr)
{
   EINA_LOG_DBG("in");
   Ecore_IMF_Context *ctx = (Ecore_IMF_Context *)ptr; 
   Ecore_IMF_Context_Data *imf_context_data = ecore_imf_context_data_get(ctx);
   UIMPreedit_Segment *ps;

   EINA_LIST_FREE(imf_context_data->preedit_segments, ps) {
      uimpreedit_segment_destroy(ps);
   }
   imf_context_data->preedit_segments = NULL;
}

static void
preedit_pushback_cb(void *ptr, int attr, const char *str)
{
   EINA_LOG_DBG("in");
   int ret = EINA_TRUE;
   Ecore_IMF_Context *ctx = (Ecore_IMF_Context *)ptr; 
   Ecore_IMF_Context_Data *imf_context_data = ecore_imf_context_data_get(ctx);
   UIMPreedit_Segment *preedit_segment = NULL;

   if(!strcmp(str, "") &&
      !(attr & (UPreeditAttr_Cursor | UPreeditAttr_Separator)))
    return;

   preedit_segment =  uimpreedit_segment_new(attr, str);
   if(!preedit_segment) {
      ret = EINA_FALSE;
      goto done;
   }
   imf_context_data->preedit_segments =
       eina_list_append(imf_context_data->preedit_segments, preedit_segment);

 done:
   if(ret == EINA_TRUE) {
      ;
   } else {
      uimpreedit_segment_destroy(preedit_segment);
   }
}

static void
preedit_update_cb(void *ptr)
{
   EINA_LOG_DBG("in");
   Ecore_IMF_Context *ctx = (Ecore_IMF_Context *)ptr; 
   Ecore_IMF_Context_Data *imf_context_data = ecore_imf_context_data_get(ctx);
   int prev_preedit_length = imf_context_data->prev_preedit_length;
   int preedit_length;
   prev_preedit_length = imf_context_data->prev_preedit_length;
   preedit_length = 
       uimpreedit_segment_length(imf_context_data->preedit_segments);

   if(prev_preedit_length == 0 && preedit_length) {
      ecore_imf_context_preedit_start_event_add(ctx);      
   } 

   ecore_imf_context_preedit_changed_event_add(ctx);

   if(prev_preedit_length && preedit_length == 0)
       ecore_imf_context_preedit_end_event_add(ctx);

   imf_context_data->prev_preedit_length = preedit_length;
}

static void
prop_list_update_cb(void *ptr __UNUSED__, const char *str __UNUSED__)
{
   EINA_LOG_DBG("in");
}

static void
configuration_changed_cb(void *ptr __UNUSED__)
{
   EINA_LOG_DBG("in");
}

static void
switch_app_global_im_cb(void *ptr __UNUSED__, const char *name __UNUSED__)
{
   EINA_LOG_DBG("in");
}

static void
switch_system_global_im_cb(void *ptr __UNUSED__, const char *name __UNUSED__)
{
   EINA_LOG_DBG("in");
}

static void
_ecore_imf_context_uim_add(Ecore_IMF_Context *ctx)
{
   EINA_LOG_DBG("in");
   Ecore_IMF_Context_Data *imf_context_data = NULL;
   uim_context uim_ctx = NULL;
   const char *im_name;

   imf_context_data = imf_context_data_new();
   if(!imf_context_data) goto on_error;

   im_name = uim_get_default_im_name(setlocale(LC_CTYPE, NULL));
   uim_ctx = uim_create_context(ctx, UIM_DEFAULT_ENCODING,
                                NULL, im_name,
                                uim_iconv,
                                commit_cb);
   if(!uim_ctx) goto on_error;

   uim_set_preedit_cb(uim_ctx,
                      preedit_clear_cb, preedit_pushback_cb, preedit_update_cb);
   uim_set_prop_list_update_cb(uim_ctx, prop_list_update_cb);
   uim_set_configuration_changed_cb(uim_ctx, configuration_changed_cb);
   /* uim_set_candidate_selector_cb(uim_ctx, cand_activate_cb, cand_select_cb, */
   /*                               cand_shift_page_cb, cand_deactivate_cb); */
   uim_set_im_switch_request_cb(uim_ctx, 
                                switch_app_global_im_cb,
                                switch_system_global_im_cb);
   // uim_set_text_acquisition_cb(uim_ctx, acquire_text_cb, delete_text_cb);
   uim_prop_list_update(uim_ctx);

   imf_context_data->uim_ctx = uim_ctx;

   ecore_imf_context_data_set(ctx, imf_context_data);

   return;

 on_error:
   imf_context_data_destroy(imf_context_data);
}

static void
_ecore_imf_context_uim_del(Ecore_IMF_Context *ctx)
{
   EINA_LOG_DBG("in");
   Ecore_IMF_Context_Data *imf_context_data = ecore_imf_context_data_get(ctx);
   imf_context_data_destroy(imf_context_data);
}

static void
_ecore_imf_context_uim_client_window_set(Ecore_IMF_Context *ctx,
                                         void              *window)
{
   EINA_LOG_DBG("in");
   Ecore_IMF_Context_Data *imf_context_data = ecore_imf_context_data_get(ctx);
   imf_context_data->win = window;
}

static void
_ecore_imf_context_uim_preedit_string_get(Ecore_IMF_Context *ctx,
                                          char             **str,
                                          int               *cursor_pos)
{
   EINA_LOG_DBG("in");
   Eina_Bool ret = EINA_TRUE;
   Ecore_IMF_Context_Data *imf_context_data = ecore_imf_context_data_get(ctx);
   Eina_List *l;
   UIMPreedit_Segment *ps;
   int pos = 0;
   Eina_UStrbuf *preedit_chars = NULL;

   preedit_chars = eina_ustrbuf_new();
   EINA_LIST_FOREACH(imf_context_data->preedit_segments, l, ps) {
      if(ps->attr & UPreeditAttr_Cursor)
          pos = (int)eina_ustrbuf_length_get(preedit_chars);
      ret = eina_ustrbuf_append(preedit_chars, ps->str);
      if(ret != EINA_TRUE)
          goto on_error;
   }
   if(cursor_pos)
       *cursor_pos = pos;
   if(str) {
       Eina_Unicode *tmp = eina_ustrbuf_string_steal(preedit_chars);
       *str = eina_unicode_unicode_to_utf8(tmp, NULL);
       free(tmp);
   }

 on_error:
   eina_ustrbuf_free(preedit_chars);
}

static void
_ecore_imf_context_uim_focus_in(Ecore_IMF_Context *ctx)
{
   EINA_LOG_DBG("in");
   Ecore_IMF_Context_Data *imf_context_data = ecore_imf_context_data_get(ctx);
   uim_context uim_ctx = imf_context_data->uim_ctx;   
   uim_focus_in_context(uim_ctx);
}

static void
_ecore_imf_context_uim_focus_out(Ecore_IMF_Context *ctx)
{
   EINA_LOG_DBG("in");
   Ecore_IMF_Context_Data *imf_context_data = ecore_imf_context_data_get(ctx);
   uim_context uim_ctx = imf_context_data->uim_ctx;   
   uim_focus_out_context(uim_ctx);
}

static void
_ecore_imf_context_uim_reset(Ecore_IMF_Context *ctx)
{
   EINA_LOG_DBG("in");
   Ecore_IMF_Context_Data *imf_context_data = ecore_imf_context_data_get(ctx);
   uim_context uim_ctx = imf_context_data->uim_ctx;
   uim_reset_context(uim_ctx);
   preedit_clear_cb(ctx);
   preedit_update_cb(ctx);
}

static void
_ecore_imf_context_uim_use_preedit_set(__attribute__((unused)) Ecore_IMF_Context *ctx,
                                       __attribute__((unused)) Eina_Bool          use_preedit)
{
   EINA_LOG_DBG("in");
}


static unsigned int
_modifiers_key_get(Ecore_IMF_Keyboard_Modifiers state)
{
   unsigned int modifiers = 0;

   if(state & ECORE_IMF_KEYBOARD_MODIFIER_CTRL)
       modifiers |= UMod_Control;

   if(state & ECORE_IMF_KEYBOARD_MODIFIER_ALT)
       modifiers |= UMod_Alt;

   if(state & ECORE_IMF_KEYBOARD_MODIFIER_SHIFT)
       modifiers |= UMod_Shift;

   if(state & ECORE_IMF_KEYBOARD_MODIFIER_WIN)
       modifiers |= UMod_Super;

   return modifiers;
}

static unsigned int
_locks_key_get(Ecore_IMF_Keyboard_Locks state)
{
   unsigned int locks = 0;

   if(state & ECORE_IMF_KEYBOARD_LOCK_NUM)
       locks |= UKey_Num_Lock;

   if(state & ECORE_IMF_KEYBOARD_LOCK_CAPS)
       locks |= UKey_Caps_Lock;

   if(state & ECORE_IMF_KEYBOARD_LOCK_SCROLL)
       locks |= UKey_Scroll_Lock;

   return locks;
}

static Eina_Hash *key_map_hash = NULL;
typedef struct _Key_Map Key_Map;
struct _Key_Map
{
    char *name;
    int code;
};
static const Key_Map key_map_table[] = {
    {"Escape", UKey_Escape},
    {"Tab", UKey_Tab},
    {"BackSpace", UKey_Backspace},
    {"Delete", UKey_Delete},
    {"Insert", UKey_Insert},
    {"Return", UKey_Return},
    {"Left", UKey_Left},
    {"Up", UKey_Up} ,
    {"Right", UKey_Right},
    {"Down", UKey_Down},
    {"Page Up", UKey_Prior},
    {"Page Down", UKey_Next},
    {"Home", UKey_Home},
    {"End", UKey_End},

    /* {"", UKey_Multi_key}, */
    /* {"", UKey_Codeinput}, */
    /* {"", UKey_SingleCandidate}, */
    /* {"", UKey_MultipleCandidate},  */
    /* {"", UKey_PreviousCandidate},  */
    /* {"", UKey_Mode_switch},  */
    {"Shift_L", UKey_Shift}, 
    {"Shift_R", UKey_Shift}, 
    {"Control_L", UKey_Control},
    {"Control_R", UKey_Control},
    {"Alt_L", UKey_Alt},
    {"Alt_R", UKey_Alt},
    {"Meta_L", UKey_Meta},
    {"Meta_R", UKey_Meta},
    {"Super_L", UKey_Super},
    {"Super_R", UKey_Super},
    {"Hyper_L", UKey_Hyper},
    {"Hyper_R", UKey_Hyper},
};
static int key_map_table_num = sizeof(key_map_table) / sizeof(key_map_table[0]);

static int
_key_get(Ecore_IMF_Event_Key_Down *event)
{
  int *code;
  code = eina_hash_find(key_map_hash, event->key);
  if(code)
    return *code;
  else {
    if(strlen(event->string) == 1)
      return (int)event->string[0];
  }

   return UKey_Other;
}

static void
_keyevent_convert(Ecore_IMF_Event_Key_Down *event, int *key, int *state)
{
   *state = 0;
   *state |= _modifiers_key_get(event->modifiers);

   *key = 0;
   *key |= _locks_key_get(event->locks);
   *key |= _key_get(event);
   // printf("key:%s\n", event->key);
}

static Eina_Bool
_ecore_imf_context_uim_filter_event(Ecore_IMF_Context   *ctx,
                                    Ecore_IMF_Event_Type type,
                                    Ecore_IMF_Event     *event)
{
   EINA_LOG_DBG("in");
   int r = 0;
   Eina_Bool ret = EINA_FALSE;
   Ecore_IMF_Context_Data *imf_context_data = ecore_imf_context_data_get(ctx);
   uim_context uim_ctx = imf_context_data->uim_ctx;   
   int key = 0;
   int state = 0;

   if(type == ECORE_IMF_EVENT_KEY_DOWN || type == ECORE_IMF_EVENT_KEY_UP) {
      _keyevent_convert((Ecore_IMF_Event_Key_Down*)event, &key, &state);
      if(type == ECORE_IMF_EVENT_KEY_DOWN) 
        r = uim_press_key(uim_ctx, key, state);
      else
        r = uim_release_key(uim_ctx, key, state);
      if(r == 0) {
         ret = EINA_TRUE;
      }
   }
   return ret;
}


static const Ecore_IMF_Context_Info uim_info = {
   .id = "uim",
   .description = "A multilingual input method framework",
   .default_locales = "ko:ja:th:zh",
   .canvas_type = "evas",
   .canvas_required = 1,
};

static Ecore_IMF_Context_Class uim_class = {
   .add = _ecore_imf_context_uim_add,
   .del = _ecore_imf_context_uim_del,
   .client_window_set = _ecore_imf_context_uim_client_window_set,
   .client_canvas_set = NULL,
   .show = NULL,
   .hide = NULL,
   .preedit_string_get = _ecore_imf_context_uim_preedit_string_get,
   .focus_in = _ecore_imf_context_uim_focus_in,
   .focus_out = _ecore_imf_context_uim_focus_out,
   .reset = _ecore_imf_context_uim_reset,
   .cursor_position_set = NULL,
   .use_preedit_set = _ecore_imf_context_uim_use_preedit_set,
   .input_mode_set = NULL,
   .filter_event = _ecore_imf_context_uim_filter_event,
   .preedit_string_with_attributes_get = NULL,
   .prediction_allow_set = NULL,
   .autocapital_type_set = NULL,
   .control_panel_show = NULL,
   .control_panel_hide = NULL,
   .input_panel_layout_set = NULL,
   .input_panel_layout_get = NULL,
   .input_panel_language_set = NULL,
   .input_panel_language_get = NULL,
   .cursor_location_set = NULL,
};

static Ecore_IMF_Context *
uim_imf_module_create(void)
{
   EINA_LOG_DBG("%s in", __FUNCTION__);
   Ecore_IMF_Context *ctx = NULL;

   ctx = ecore_imf_context_new(&uim_class);
   if(!ctx)
     goto error;

   return ctx;

error:
   free(ctx);
   return NULL;
}

static Ecore_IMF_Context *
uim_imf_module_exit(void)
{
   return NULL;
}

Eina_Bool
ecore_imf_uim_init(void)
{
   EINA_LOG_DBG("%s in", __FUNCTION__);
   int i;
   eina_init();

   ecore_imf_module_register(&uim_info,
                             uim_imf_module_create,
                             uim_imf_module_exit);

   uim_init();

   
   key_map_hash = eina_hash_string_small_new(NULL);
   for(i=0; i<key_map_table_num; i++) {
     eina_hash_direct_add(key_map_hash, 
                          key_map_table[i].name, &key_map_table[i].code);
   }

   return EINA_TRUE;
}

void
ecore_imf_uim_shutdown(void)
{
   eina_hash_free(key_map_hash);

   uim_quit();
   eina_shutdown();
}

EINA_MODULE_INIT(ecore_imf_uim_init);
EINA_MODULE_SHUTDOWN(ecore_imf_uim_shutdown);

/* 
 * Internal Functions
 */
static UIMPreedit_Segment*
uimpreedit_segment_new(int attr, const char *str)
{
   UIMPreedit_Segment *preedit_segment = NULL;
   preedit_segment = calloc(1, sizeof(UIMPreedit_Segment));
   if(!preedit_segment) goto on_error;

   preedit_segment->attr = attr;
   preedit_segment->str = eina_unicode_utf8_to_unicode(str, NULL);
   if(!preedit_segment->str) goto on_error;

   return preedit_segment;

 on_error:
   uimpreedit_segment_destroy(preedit_segment);
   return NULL;
}

static void
uimpreedit_segment_destroy(UIMPreedit_Segment *preedit_segment)
{
   if(!preedit_segment) return;
   free(preedit_segment->str);
   free(preedit_segment);
}

static int
uimpreedit_segment_length(Eina_List *preedit_segments)
{
   int length = 0;
   UIMPreedit_Segment *ps;
   Eina_List *l;

   EINA_LIST_FOREACH(preedit_segments, l, ps) {
      length += eina_unicode_strlen(ps->str);
   }

   return length;
}

static Ecore_IMF_Context_Data*
imf_context_data_new()
{
   Ecore_IMF_Context_Data *imf_context_data = NULL;
   imf_context_data = calloc(1, sizeof(Ecore_IMF_Context_Data));
   if(!imf_context_data) goto on_error;

   return imf_context_data;

 on_error:
   imf_context_data_destroy(imf_context_data);
   return NULL;
}

static void
imf_context_data_destroy(Ecore_IMF_Context_Data *imf_context_data)
{
   UIMPreedit_Segment *ps;
   if(!imf_context_data) return;
   
   EINA_LIST_FREE(imf_context_data->preedit_segments, ps) {
      uimpreedit_segment_destroy(ps);
   }
   if(imf_context_data->uim_ctx)
       uim_release_context(imf_context_data->uim_ctx);
   free(imf_context_data);
}
