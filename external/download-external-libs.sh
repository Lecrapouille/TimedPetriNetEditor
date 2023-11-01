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

### Immediate Mode Plotting
### License: MIT
cloning epezent/implot

### File Dialog for Dear ImGui
### License: MIT
cloning aiekick/ImGuiFileDialog

### A simple and easy-to-use library to enjoy videogames programming
### License: Zlib
cloning raysan5/raylib

### A Raylib integration with DearImGui
### License: Zlib
cloning raylib-extras/rlImGui

### JSON for Modern C++
### License: MIT
cloning nlohmann/json

### Class wrapping client MQTT (mosquitto lib)
### License: MIT
cloning Lecrapouille/MQTT

### C++ implementation of sparse matrix using CRS (Compressed Row Storage) format
### License: none
#cloning uestla/Sparse-Matrix
