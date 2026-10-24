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
TARGET_NAME := TimedPetriAnalysis
TARGET_DESCRIPTION := Properties of Petri nets: reachability, deadlocks, bounds, liveness, invariants
include $(M)/project/Makefile

###################################################
# Depends on TimedPetriNet (the nets).
#
LIB_TPNE_NET := $(call internal-lib,TimedPetriNet)
INTERNAL_LIBS := $(LIB_TPNE_NET)
DIRS_WITH_MAKEFILE := $(P)/src/PetriNet

###################################################
# MacOS X: get path inside the bundle application
#
ifeq ($(OS),Darwin)
    LINKER_FLAGS += -framework CoreFoundation
endif

###################################################
# Inform Makefile where to find *.cpp files
#
VPATH += $(P)/src

###################################################
# Inform Makefile where to find header files
#
INCLUDES += $(P)/src $(THIRD_PARTIES_DIR)
INCLUDES += $(THIRD_PARTIES_DIR)/json/include

###################################################
# Make the list of compiled files for the library
#
LIB_FILES += $(call rwildcard,$(P)/src/Analysis,*.cpp)
USER_CXXFLAGS := -Wno-old-style-cast

###################################################
# Public headers installed under
# $(PREFIX)/include/TimedPetriNetEditor/$(PROJECT_VERSION)/.
#
HEADER_FILES += $(call rwildcard,$(P)/src/Analysis,*.hpp)

###################################################
# Generic Makefile rules
#
include $(M)/rules/Makefile