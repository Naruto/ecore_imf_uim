#!/bin/bash

CC=gcc
CF="-I$(readlink -f .) -D_GNU_SOURCE=1 -Wall -Wextra -g"

UIM_INSTALL_DIR=${HOME}/.ecore/immodules
UIM_DEPS=($(pkg-config --print-requires-private ecore ecore-imf elementary uim))
#echo "DEPENDENCIES: ${UIM_DEPS[@]}"
UIM_CFLAGS="$(pkg-config --cflags ${UIM_DEPS[@]} ecore ecore-imf elementary uim)"
#echo "DEPENDENCY CFLAGS: $UIM_CFLAGS"

UIM_LIBS="$(pkg-config --libs ${UIM_DEPS[@]} ecore ecore-imf elementary uim)"
#echo "DEPENDENCY LIBS: $LIBS"

TEST_DEPS=($(pkg-config --print-requires-private evas ecore ecore-imf-evas ecore-evas))
#echo "DEPENDENCIES: ${DEPS[@]}"
TEST_CFLAGS="$(pkg-config --cflags ${TEST_DEPS[@]} evas ecore ecore-imf-evas ecore-evas)"
#echo "DEPENDENCY CFLAGS: $CFLAGS"

TEST_LIBS="$(pkg-config --libs ${TEST_DEPS[@]} evas ecore ecore-imf-evas ecore-evas)"
#echo "DEPENDENCY LIBS: $LIBS"

#echo


echo "build uim module"
${CC} ${CF} ${UIM_CFLAGS} -fPIC -c ecore_imf_uim.c 
${CC} ${CF} ${UIM_LIBS} -shared -Wl,-soname,uim.so -o uim.so ./ecore_imf_uim.o 

echo "install uim.so to ${UIM_INSTALL_DIR}"
mkdir -p ${UIM_INSTALL_DIR}
cp uim.so ${UIM_INSTALL_DIR}/uim.so

echo "build test program"
${CC} ${CF} ${TEST_CFLAGS} ${TEST_LIBS} ecore_imf_test.c -o ecore_imf_test
