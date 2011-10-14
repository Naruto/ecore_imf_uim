/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
/* 
   gcc -Wall -g ecore_imf_test.c -o ecore_imf_test `pkg-config evas --cflags --libs` `pkg-config ecore --cflags --libs` `pkg-config ecore-imf-evas --cflags --libs` `pkg-config ecore-evas --cflags --libs`
 */
#include <stdio.h>

#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Ecore_IMF.h>
#include <Ecore_IMF_Evas.h>

#define __UNUSED__ __attribute__ ((unused))

/* Called when a key has been pressed by the user */
static void
_e_entry_key_down_cb(void *data,
                     Evas *e __UNUSED__,
                     Evas_Object *obj __UNUSED__,
                     void *event_info)
{
   EINA_LOG_DBG("%s in", __FUNCTION__);
   Ecore_IMF_Context *ctx = (Ecore_IMF_Context *)data;

   Ecore_IMF_Event_Key_Down ev;

   ecore_imf_evas_event_key_down_wrap(event_info, &ev);
   if (ecore_imf_context_filter_event(ctx,
                                      ECORE_IMF_EVENT_KEY_DOWN,
                                      (Ecore_IMF_Event *) &ev))
       return;
   fprintf(stderr, "str %s\n", ev.keyname);
}

static void
_e_entry_key_up_cb(void *data,
                   Evas *e __UNUSED__,
                   Evas_Object *obj __UNUSED__,
                   void *event_info)
{
   EINA_LOG_DBG("%s in", __FUNCTION__);
   Ecore_IMF_Context *ctx = (Ecore_IMF_Context *)data;

   Ecore_IMF_Event_Key_Up ev;

   ecore_imf_evas_event_key_up_wrap(event_info, &ev);
   if (ecore_imf_context_filter_event(ctx,
                                      ECORE_IMF_EVENT_KEY_UP,
                                      (Ecore_IMF_Event *) &ev))
       return;
}

static void
_e_entry_focus_in_cb(void *data,
                     Evas *e __UNUSED__,
                     Evas_Object *obj __UNUSED__,
                     void *event_info)
{
   EINA_LOG_DBG("%s in", __FUNCTION__);
   Ecore_IMF_Context *ctx = (Ecore_IMF_Context *)data;

   Ecore_IMF_Event_Key_Down ev;

   ecore_imf_evas_event_key_down_wrap(event_info, &ev);
   ecore_imf_context_reset(ctx);
   ecore_imf_context_focus_in(ctx);
   return;
}

static void
_e_entry_focus_out_cb(void *data,
                      Evas *e __UNUSED__,
                      Evas_Object *obj __UNUSED__,
                      void *event_info)
{
   EINA_LOG_DBG("%s in", __FUNCTION__);
   Ecore_IMF_Context *ctx = (Ecore_IMF_Context *)data;

   Ecore_IMF_Event_Key_Down ev;

   ecore_imf_evas_event_key_down_wrap(event_info, &ev);
   // ecore_imf_context_reset(ctx);
   ecore_imf_context_focus_out(ctx);
   return;
}

static Eina_Bool
_retrieve_surrounding_cb(void *data __UNUSED__,
                         Ecore_IMF_Context *ctx __UNUSED__,
                         char **text,
                         int *cursor_pos)
{
   if (text)
      *text = strdup("test surrouding");

   if (cursor_pos)
      *cursor_pos = 3;

   return EINA_TRUE;
} /* _retrieve_surrounding_cb */

static Eina_Bool
_event_commit_cb(void *data __UNUSED__,
                 int type __UNUSED__,
                 void *event)
{
   Ecore_IMF_Event_Commit *ev = event;
   fprintf(stderr, "commited %s\n", ev->str);
   return EINA_TRUE;
} /* _event_commit_cb */

static Eina_Bool
_event_changed_cb(void *data __UNUSED__,
                  int type __UNUSED__,
                  void *event)
{
   Ecore_IMF_Event_Preedit_Changed *ev = event;
   Ecore_IMF_Context *ctx = ev->ctx;
   char *preedit_string = NULL;
   int cursor_pos = 0;

   ecore_imf_context_preedit_string_get(ctx, &preedit_string, &cursor_pos);
   if (!preedit_string) return ECORE_CALLBACK_PASS_ON;

   printf("cursor_pos:%d preedit_string:%s\n", cursor_pos, preedit_string);

   free(preedit_string);

   return ECORE_CALLBACK_DONE;
}

int main()
{
   Eina_List *ids, *l;
   const char *id;
   Ecore_Evas *ee;
   Evas *evas;
   Evas_Object *rect;
   Ecore_IMF_Context *ctx;

   fprintf(stderr, "** initializing ecore\n");
   ecore_init();
   fprintf(stderr, "** initializing ecore_evas\n");
   ecore_evas_init();
   fprintf(stderr, "** initializing ecore_imf\n");
   ecore_imf_init();

   fprintf(stderr, "** retrieving ecore_imf_context_available_ids_get\n");
   ids = ecore_imf_context_available_ids_get();
   if (ids)
     {
        // ecore_list_first_goto(ids);
        fprintf(stderr, "** available input methods:");
        EINA_LIST_FOREACH(ids, l, id) {
           fprintf(stderr, " %s", id);
        }
        fprintf(stderr, "\n");
     }
   id = ecore_imf_context_default_id_by_canvas_type_get("evas");
   if(!id)
       id = ecore_imf_context_default_id_get();

   ee = ecore_evas_software_x11_new(NULL, 0, 0, 0, 800, 480);
   ecore_evas_title_set(ee, "test");
   ecore_evas_name_class_set(ee, "test", "test");
   // ecore_evas_fullscreen_set(ee, 1);
   ecore_evas_show(ee);

   evas = ecore_evas_get(ee);
   evas_image_cache_set(evas, 8 * 1024 * 1024);

   rect = evas_object_rectangle_add(evas);
   evas_object_move(rect, 50, 50);
   evas_object_resize(rect, 50, 50);
   evas_object_show(rect);


   fprintf(stderr, "** creating context\n");
   fprintf(stderr, "** module id: %s\n", id);
   ctx = ecore_imf_context_add(id);
   if(!ctx)
     {
        fprintf(stderr, "ctx is null\n");
        return 1;
     }

   ecore_imf_context_client_window_set(ctx,
                                       (void *)ecore_evas_software_x11_window_get(ee));
   ecore_imf_context_retrieve_surrounding_callback_set(ctx, _retrieve_surrounding_cb, NULL);
   ecore_imf_context_reset(ctx);
   ecore_imf_context_show(ctx);
   ecore_event_handler_add(ECORE_IMF_EVENT_COMMIT, _event_commit_cb, NULL);
   ecore_event_handler_add(ECORE_IMF_EVENT_PREEDIT_START, NULL, NULL);
   ecore_event_handler_add(ECORE_IMF_EVENT_PREEDIT_END, NULL, NULL);
   ecore_event_handler_add(ECORE_IMF_EVENT_PREEDIT_CHANGED, _event_changed_cb,
                           NULL);
   evas_object_event_callback_add(rect, EVAS_CALLBACK_FOCUS_IN, 
                                  _e_entry_focus_in_cb, (void*)ctx);
   evas_object_event_callback_add(rect, EVAS_CALLBACK_FOCUS_OUT, 
                                  _e_entry_focus_out_cb, (void*)ctx);
   evas_object_event_callback_add(rect, EVAS_CALLBACK_KEY_DOWN,
                                  _e_entry_key_down_cb, (void *)ctx);
   evas_object_event_callback_add(rect, EVAS_CALLBACK_KEY_UP,
                                  _e_entry_key_up_cb, (void *)ctx);
   evas_object_focus_set (rect, EINA_TRUE);
  
   ecore_main_loop_begin();

   ecore_imf_context_hide(ctx);
   ecore_imf_context_del(ctx);

   ecore_imf_shutdown();
   ecore_evas_shutdown();
   ecore_shutdown();

   return 0;
} /* main */


