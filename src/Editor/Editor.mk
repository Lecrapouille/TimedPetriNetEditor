# SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
# Copyright (c) 2020-2026 Quentin Quadrat
#
# This file is part of TimedPetriNetEditor. See LICENSING.md for details.
#
# The library of the editor (GLFW3 and OpenGL 3, Dear ImGui) and the editor.
# Paths are relative to this directory.

###############################################################################
# ZeroMQ, for the remote control and the debugger. Not in a browser.
#
#? 0: build the editor without ZeroMQ (no remote control, no debugger).
TPNE_ZEROMQ ?= 1
ifeq ($(TARGET_OS),Emscripten)
    TPNE_ZEROMQ := 0
endif
ifeq ($(TPNE_ZEROMQ),1)
    ifneq ($(BUILDING),)
        ifeq ($(shell $(PKG_CONFIG) --exists libzmq && echo 1),)
            $(call print-warning,libzmq not found: building with TPNE_ZEROMQ=0)
            TPNE_ZEROMQ := 0
        endif
    endif
endif

###############################################################################
# The library of the editor.
#
TARGETS += TimedPetriGUI
TimedPetriGUI.TYPE := library
TimedPetriGUI.DESCRIPTION := Library Editor for Petri nets
TimedPetriGUI.SOURCES := *.cpp Backends/GLFW3/Application.cpp \
    $(if $(filter 1,$(TPNE_ZEROMQ)),Remote/ZeroMQRemote.cpp,Remote/NullRemoteControl.cpp)
TimedPetriGUI.PUBLIC_HEADERS := *.hpp
TimedPetriGUI.INCLUDES := ..
# Editor.hpp includes "Application.hpp".
TimedPetriGUI.PUBLIC_INCLUDES := Backends/GLFW3 Remote
TimedPetriGUI.DEFINES := TPNE_USE_ZEROMQ=$(TPNE_ZEROMQ)
TimedPetriGUI.DEPENDS := TimedPetriAnalysis TimedPetriRuntime TimedPetriMaxPlus imgui implot ImGuiFileDialog
TimedPetriGUI.MERGE := imgui implot ImGuiFileDialog
TimedPetriGUI.PROJECT_INFO := 1
TimedPetriGUI.DATA := ../../data
TimedPetriGUI.CXXFLAGS := -Wno-old-style-cast -Wno-sign-conversion -Wno-float-equal \
    -Wno-cast-qual -Wno-conversion -Wno-useless-cast -Wno-nan-infinity-disabled \
    -Wno-format-nonliteral -Wno-deprecated-enum-enum-conversion -Wno-unknown-pragmas \
    -Wno-double-promotion
ifeq ($(TPNE_ZEROMQ),1)
    TimedPetriGUI.PKG_CONFIG := libzmq
endif

ifeq ($(TARGET_OS),Linux)
    TimedPetriGUI.PKG_CONFIG += glfw3
    TimedPetriGUI.LIBS := -lGL
else ifeq ($(TARGET_OS),Darwin)
    TimedPetriGUI.PKG_CONFIG += glfw3 glew
    TimedPetriGUI.PUBLIC_DEFINES := GL_SILENCE_DEPRECATION
    TimedPetriGUI.LIBS := -framework OpenGL -framework Cocoa -framework IOKit \
        -framework CoreVideo -framework CoreFoundation
else ifeq ($(TARGET_OS),Windows)
    TimedPetriGUI.PKG_CONFIG += glfw3
    TimedPetriGUI.LIBS := -lopengl32
else ifeq ($(TARGET_OS),Emscripten)
    ifdef EXAEQUOS
        PKG_CONFIG_STATIC := 1
        TimedPetriGUI.PKG_CONFIG += exa-wayland glfw
        TimedPetriGUI.LDFLAGS := -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sFULL_ES3
    endif
endif

###############################################################################
# The editor.
#
TARGETS += TimedPetriNetEditor
TimedPetriNetEditor.DESCRIPTION := Timed Petri Net Editor
TimedPetriNetEditor.SOURCES := ../main.cpp
TimedPetriNetEditor.INCLUDES := ..
TimedPetriNetEditor.DEPENDS := TimedPetriGUI TimedPetriJulia
TimedPetriNetEditor.LIBS := -ldl -lpthread
TimedPetriNetEditor.BUNDLE := 1
TimedPetriNetEditor.ICON := ../../data/TimedPetriNetEditor.icns
TimedPetriNetEditor.BUNDLE_ID := com.lecrapouille.TimedPetriNetEditor

ifeq ($(TARGET_OS),Emscripten)
    ifdef EXAEQUOS
        TimedPetriNetEditor.LDFLAGS := -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sFULL_ES3
    else
        TimedPetriNetEditor.LDFLAGS := -sUSE_GLFW=3 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sFULL_ES3 \
            --shell-file $(abspath $(HERE)Backends/GLFW3/shell.html) \
            --preload-file $(abspath $(HERE)../../data) \
            --preload-file $(abspath $(HERE)../../examples)@/examples \
            --exclude-file '*.png' --exclude-file '*.cpp' --exclude-file '*.md' \
            --exclude-file '*.args' --exclude-file '*Makefile' \
            -sFORCE_FILESYSTEM=1 -sASYNCIFY -sGL_ENABLE_GET_PROC_ADDRESS
    endif
endif
