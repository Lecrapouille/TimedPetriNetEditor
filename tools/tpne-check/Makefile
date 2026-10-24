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
TARGET_NAME := tpne-check
TARGET_DESCRIPTION := Check the properties of Petri nets in a terminal or a CI
include $(M)/project/Makefile

###################################################
# Depends on TimedPetriAnalysis, TimedPetriIO (loading the files) and
# TimedPetriNet.
#
LIB_TPNE_NET := $(call internal-lib,TimedPetriNet)
LIB_TPNE_IO := $(call internal-lib,TimedPetriIO)
LIB_TPNE_ANALYSIS := $(call internal-lib,TimedPetriAnalysis)
INTERNAL_LIBS := $(LIB_TPNE_ANALYSIS) $(LIB_TPNE_IO) $(LIB_TPNE_NET)
# The projects share the build folder: clean removes it once, without
# going down in the projects this one depends on.
ifeq ($(filter clean,$(MAKECMDGOALS)),)
DIRS_WITH_MAKEFILE := $(P)/src/Analysis
endif

###################################################
# MacOS X: get path inside the bundle application
#
ifeq ($(OS),Darwin)
    LINKER_FLAGS += -framework CoreFoundation
endif

###################################################
# Inform Makefile where to find *.cpp files
#
VPATH += $(P)/src $(P)/tools/tpne-check

###################################################
# Inform Makefile where to find header files
#
INCLUDES += $(P)/src $(THIRD_PARTIES_DIR)
INCLUDES += $(THIRD_PARTIES_DIR)/json/include

###################################################
# The application
#
SRC_FILES := $(P)/tools/tpne-check/main.cpp
LINKER_FLAGS += -lpthread

###################################################
# Generic Makefile rules
#
include $(M)/rules/Makefile
