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

### Dear ImGui backend for use with SFML
### License: MIT
cloning SFML/imgui-sfml
cp imgui-sfml/imconfig-SFML.h imgui/imconfig.h

### Class wrapping client MQTT (mosquitto lib)
### License: MIT
cloning Lecrapouille/MQTT

### Portable GUI dialogs library, C++11, single-header
### License: WTFPL
cloning samhocevar/portable-file-dialogs

### JSON for Modern C++
### License: MIT
cloning nlohmann/json
