#!/bin/bash -e
###############################################################################
### This script is called by (cd .. && make download-external-libs). It will
### git clone thirdparts needed for this project but does not compile them.
### It replaces git submodules that I dislike.
###############################################################################

source ../.makefile/download-external-libs.sh

### Dear ImGui: Bloat-free Graphical User interface for C++ with minimal dependencies
### License: MIT
cloning ocornut/imgui -b docking
#(cd imgui && git checkout -b 719d9313041b85227a3e6deb289a313819aaeab3)

### Dear ImGui backend for use with SFML
### License: MIT
cloning SFML/imgui-sfml
#(cd imgui-sfml && git checkout -b 719d9313041b85227a3e6deb289a313819aaeab3)
cp imgui-sfml/imconfig-SFML.h imgui/imconfig.h
