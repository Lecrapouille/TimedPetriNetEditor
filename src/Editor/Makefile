# SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
# Copyright (c) 2020-2026 Quentin Quadrat
#
# This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
# for users who cannot use the GPL, under a commercial license.
# See LICENSING.md for details.

###################################################
# Location of the project directory and Makefiles
#
P := ../..
M := $(P)/.makefile

###################################################
# Project definition
#
include $(P)/Makefile.common
TARGET_NAME := TimedPetriGUI
TARGET_DESCRIPTION := Library Editor for Petri nets
include $(M)/project/Makefile

###################################################
# Depends on TimedPetriMaxPlus, TimedPetriIO and TimedPetriNet (required when
# linking the macOS dylib).
#
LIB_TPNE_NET := $(call internal-lib,TimedPetriNet)
LIB_TPNE_IO := $(call internal-lib,TimedPetriIO)
LIB_TPNE_MAXPLUS := $(call internal-lib,TimedPetriMaxPlus)
INTERNAL_LIBS := $(LIB_TPNE_MAXPLUS) $(LIB_TPNE_IO) $(LIB_TPNE_NET)
DIRS_WITH_MAKEFILE := $(P)/src/MaxPlus

###################################################
# Set json library.
#
INCLUDES += $(THIRD_PARTIES_DIR)/json/include

###################################################
# For the moment we only use Dear ImGui
#
include ./Backends/Makefile

###################################################
# Make the list of compiled files for the library
#
INCLUDES += $(P)/src
LIB_FILES += $(GUI_FILES)
# Raylib is linked by the top-level target (see root Makefile); not merged into this .a.
THIRD_PARTIES_LIBS :=

###################################################
# Generic Makefile rules
#
include $(M)/rules/Makefile
