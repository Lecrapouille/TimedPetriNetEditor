# SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
# Copyright (c) 2020-2026 Quentin Quadrat
#
# This file is part of TimedPetriNetEditor. See LICENSING.md for details.
#
# Third parties of the editor, and of TimedPetriViewer.

TARGETS += imgui implot ImGuiFileDialog

### Dear ImGui: Bloat-free Graphical User interface for C++. License: MIT
imgui.GIT := https://github.com/ocornut/imgui.git
imgui.VERSION := docking
imgui.TYPE := static
imgui.SOURCES := imgui.cpp imgui_draw.cpp imgui_tables.cpp imgui_widgets.cpp \
    misc/cpp/imgui_stdlib.cpp backends/imgui_impl_glfw.cpp backends/imgui_impl_opengl3.cpp
imgui.PUBLIC_INCLUDES := . backends misc/cpp
imgui.PUBLIC_DEFINES := IMGUI_DEFINE_MATH_OPERATORS
# The headers of GLFW for its backend: Emscripten has them, ExaequOS gives
# them with its package glfw.
ifeq ($(TARGET_OS),Emscripten)
    ifdef EXAEQUOS
        imgui.PKG_CONFIG := glfw
    endif
else
    imgui.PKG_CONFIG := glfw3
endif

### Immediate Mode Plotting. License: MIT
implot.GIT := https://github.com/epezent/implot.git
implot.TYPE := static
implot.SOURCES := implot.cpp implot_items.cpp
implot.PUBLIC_INCLUDES := .
implot.DEPENDS := imgui

### File Dialog for Dear ImGui. License: MIT
ImGuiFileDialog.GIT := https://github.com/aiekick/ImGuiFileDialog.git
ImGuiFileDialog.TYPE := static
ImGuiFileDialog.SOURCES := ImGuiFileDialog.cpp
ImGuiFileDialog.PUBLIC_INCLUDES := .
ImGuiFileDialog.DEPENDS := imgui
