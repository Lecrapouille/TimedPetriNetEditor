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
# Depends on TimedPetriAnalysis, TimedPetriRuntime (journals), TimedPetriMaxPlus,
# TimedPetriIO and TimedPetriNet (required when linking the macOS dylib).
#
LIB_TPNE_NET := $(call internal-lib,TimedPetriNet)
LIB_TPNE_IO := $(call internal-lib,TimedPetriIO)
LIB_TPNE_MAXPLUS := $(call internal-lib,TimedPetriMaxPlus)
LIB_TPNE_ANALYSIS := $(call internal-lib,TimedPetriAnalysis)
LIB_TPNE_RUNTIME := $(call internal-lib,TimedPetriRuntime)
INTERNAL_LIBS := $(LIB_TPNE_ANALYSIS) $(LIB_TPNE_RUNTIME) $(LIB_TPNE_MAXPLUS) $(LIB_TPNE_IO) $(LIB_TPNE_NET)
# The projects share the build folder: clean removes it once, without
# going down in the projects this one depends on.
ifeq ($(filter clean,$(MAKECMDGOALS)),)
DIRS_WITH_MAKEFILE := $(P)/src/MaxPlus
endif

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
THIRD_PARTIES_LIBS :=

###################################################
# Generic Makefile rules
#
include $(M)/rules/Makefile
