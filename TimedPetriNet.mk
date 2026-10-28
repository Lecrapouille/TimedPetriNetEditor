##=====================================================================
## TimedPetriNetEditor: A timed Petri net editor.
## Copyright 2021 -- 2026 Quentin Quadrat <lecrapouille@gmail.com>
##
## This file is part of TimedPetriNetEditor.
##
## TimedPetriNetEditor is free software: you can redistribute it and/or modify it
## under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##=====================================================================

###################################################
# Use TimedPetriNet from another Makefile project, without building this
# project: define TPNE_DIR (this folder) then include this file.
#
#   TPNE_SOURCES  the library: nets, simulation, GRAFCET engine, (max,+)
#                 algorithms, imports and exports (the file formats). No GUI.
#   TPNE_CORE_SOURCES  only the nets, the firing rule, the engines: what a
#                 board running a net needs (with TPNE_RUNTIME_SOURCES).
#   TPNE_RUNTIME_SOURCES  the runtime calling functions when transitions
#                 fire (needs TPNE_SOURCES).
#   TPNE_ANALYSIS_SOURCES  properties of the nets: reachability, deadlocks,
#                 bounds, liveness, invariants (needs TPNE_SOURCES).
#   TPNE_VIEWER_SOURCES  NetViewer, a read-only view of a net drawn inside
#                 the Dear ImGui windows of the host. The host compiles
#                 Dear ImGui and gives its include folder so that
#                 "imgui/imgui.h" is found.
#
ifndef TPNE_DIR
$(error TimedPetriNet.mk: define TPNE_DIR before including this file)
endif

TPNE_INCLUDES := $(TPNE_DIR)/src
TPNE_VPATH := $(TPNE_DIR)/src
TPNE_CORE_SOURCES := $(shell find $(TPNE_DIR)/src/PetriNet -name '*.cpp' 2> /dev/null)
TPNE_SOURCES := $(shell find $(TPNE_DIR)/src/PetriNet $(TPNE_DIR)/src/IO $(TPNE_DIR)/src/MaxPlus -name '*.cpp' -o -name '*.c' 2> /dev/null)
TPNE_SOURCES += $(TPNE_DIR)/external/tinyxml2/tinyxml2.cpp
# Third parties headers are system headers: no warnings from them.
TPNE_CXXFLAGS := -isystem $(abspath $(TPNE_DIR)/external/json/include)
TPNE_CXXFLAGS += -isystem $(abspath $(TPNE_DIR)/external)
# -Wnoexcept and -Wuseless-cast are known by GCC only: clang warns about them.
TPNE_IS_CLANG := $(shell $(CXX) --version 2> /dev/null | grep -c clang)
ifeq ($(TPNE_IS_CLANG),0)
TPNE_CXXFLAGS += -Wno-noexcept
endif
# Howard.c
TPNE_CFLAGS := -Wno-sign-conversion -Wno-float-equal

TPNE_RUNTIME_SOURCES := $(shell find $(TPNE_DIR)/src/Runtime -name '*.cpp' 2> /dev/null)
TPNE_ANALYSIS_SOURCES := $(shell find $(TPNE_DIR)/src/Analysis -name '*.cpp' 2> /dev/null)
TPNE_VIEWER_SOURCES := $(TPNE_DIR)/src/Editor/NetViewer.cpp
TPNE_VIEWER_SOURCES += $(TPNE_DIR)/src/Editor/Drawable.cpp
TPNE_VIEWER_SOURCES += $(TPNE_DIR)/src/Editor/Theme.cpp
# The viewer only needs the headers of TimedPetriNet and the host Dear ImGui.
TPNE_VIEWER_CXXFLAGS := -Wno-old-style-cast -Wno-sign-conversion -Wno-float-equal
TPNE_VIEWER_CXXFLAGS += -Wno-double-promotion -Wno-conversion
ifeq ($(TPNE_IS_CLANG),0)
TPNE_VIEWER_CXXFLAGS += -Wno-useless-cast
endif
