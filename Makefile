##=====================================================================
## TimedPetriNetEditor: A timed Petri net editor.
## Copyright 2021 -- 2023 Quentin Quadrat <lecrapouille@gmail.com>
##
## This file is part of PetriEditor.
##
## PetriEditor is free software: you can redistribute it and/or modify it
## under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with GNU Emacs.  If not, see <http://www.gnu.org/licenses/>.
##=====================================================================

###################################################
# Location of the project directory and Makefiles
#
P := .
M := $(P)/.makefile

###################################################
# Project definition
#
include $(P)/Makefile.common
TARGET_NAME := $(PROJECT_NAME)
TARGET_DESCRIPTION := Timed Petri Net Editor
include $(M)/project/Makefile

###################################################
# Standalone application
#
SRC_FILES := src/main.cpp
INCLUDES := $(P)/include
VPATH := $(P)/src
# Internal libs to compile
LIB_TPNE_NET := $(call internal-lib,TimedPetriNet)
LIB_TPNE_EDITOR := $(call internal-lib,TimedPetriEditor)
LIB_TPNE_JULIA := $(call internal-lib,TimedPetriJulia)
INTERNAL_LIBS := $(LIB_TPNE_EDITOR) $(LIB_TPNE_NET) $(LIB_TPNE_JULIA)
DIRS_WITH_MAKEFILE := $(P)/src/Net $(P)/src/Editor $(P)/src/julia

###################################################
# GUI
#
include $(abspath $(P)/src/Editor/DearImGui/Backends/Makefile)
THIRDPART_LIBS :=
LINKER_FLAGS += -ldl -lpthread
INCLUDES += $(P)/src/Editor/DearImGui
INCLUDES += $(P)/src

###################################################
# Embed assets for web version. Assets shall be
# present inside $(BUILD) folder.
#
ifeq ($(OS),Emscripten)
    ifndef EXAEQUOS
        # Install the folder data in the Emscripten filesystem
        LINKER_FLAGS += --preload-file $(PROJECT_DATA_DIR)
        LINKER_FLAGS += -s FORCE_FILESYSTEM=1
        # Add this flag ONLY in case we are using ASYNCIFY code
        LINKER_FLAGS += -s ASYNCIFY
        # For linking glfwGetProcAddress().
        LINKER_FLAGS += -s GL_ENABLE_GET_PROC_ADDRESS
    endif
endif

###################################################
# Generic Makefile rules
#
include $(M)/rules/Makefile

###################################################
# Compile internal librairies in the correct order
#

$(LIB_TPNE_NET): $(P)/src/Net

$(LIB_TPNE_EDITOR): $(P)/src/Editor

$(LIB_TPNE_JULIA): $(P)/src/julia

$(P)/src/julia: $(P)/src/Editor

$(P)/src/Editor: $(P)/src/Net

###################################################
# Copy data inside BUILD to allow emscripten to embedded them
#
.PHONY: copy-assets
copy-emscripten-assets: | $(BUILD)
ifeq ($(ARCHI),Emscripten)
	@$(call print-to,"Copying assets","$(TARGET)","$(BUILD)","")
	@mkdir -p $(BUILD)/data
	@mkdir -p $(BUILD)/examples
	@cp $(P)/data/examples/*.json $(BUILD)/examples
	@cp $(P)/data/font.ttf $(BUILD)/data
	@cp $(P)/data/imgui.ini $(BUILD)/data
endif