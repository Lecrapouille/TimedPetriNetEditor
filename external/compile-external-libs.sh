#!/bin/bash -e
###############################################################################
### This script is called by (cd .. && make compile-external-libs). It will
### compile thirdparts cloned previously with make download-external-libs.
###
### To avoid pollution, these libraries are not installed in your operating
### system (no sudo make install is called). As consequence, you have to tell
### your project ../Makefile where to find their files. Here generic example:
###     INCLUDES += -I$(THIRDPART)/thirdpart1/path1
###        for searching heeder files.
###     VPATH += $(THIRDPART)/thirdpart1/path1
###        for searching c/c++ files.
###     THIRDPART_LIBS += $(abspath $(THIRDPART)/libXXX.a))
###        for linking your project against the lib.
###     THIRDPART_OBJS += foo.o
###        for inking your project against this file iff THIRDPART_LIBS is not
###        used (the path is not important thanks to VPATH).
###
### The last important point to avoid polution, better to compile thirdparts as
### static library rather than shared lib to avoid telling your system where to
### find them when you'll start your application.
###
### Builtin variables of this script:
### $ARCHI: architecture (i.e. Linux, Darwin, Windows, Emscripten)
### $TARGET: target (your application name)
### $CC: C compiler
### $CXX: C++ compiler
###############################################################################

source ../.makefile/compile-external-libs.sh

### Library raylib
print-compile raylib
(
 cd raylib/src

 # Fucking poor API !!!!! Need to hot patch
 sed -i 's/ExportImage(image, path)/ExportImage(image, fileName)/g' rcore.c

 mkdir -p $ARCHI
 call-make clean
 if [ "$ARCHI" != "Emscripten" ]; then
   call-make PLATFORM=PLATFORM_DESKTOP RAYLIB_RELEASE_PATH=$ARCHI RAYLIB_BUILD_MODE=DEBUG
 else
   call-make PLATFORM=PLATFORM_WEB RAYLIB_RELEASE_PATH=$ARCHI
 fi
)
