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

### Library raylib
print-compile raylib
(
 cd raylib/src

 # Need to patch TakeScreenshot() function because it only accepts file name instead
 # of file path which is the poorest idea ever!
 # Note: Concerning .backup see https://www.themoderncoder.com/fix-sed-i-error-macos/
 sed -i.backup 's/ExportImage(image, path)/ExportImage(image, fileName)/g' rcore.c

 mkdir -p $OS
 call-make clean
 if [ "$OS" != "Emscripten" ]; then
   call-make PLATFORM=PLATFORM_DESKTOP RAYLIB_RELEASE_PATH=$OS RAYLIB_BUILD_MODE=DEBUG
 else
   call-make PLATFORM=PLATFORM_WEB RAYLIB_RELEASE_PATH=$OS
 fi
)
