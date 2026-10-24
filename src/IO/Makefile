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
TARGET_NAME := TimedPetriIO
TARGET_DESCRIPTION := Load and save Petri nets in many file formats
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
# Set xml Library.
#
VPATH += $(THIRD_PARTIES_DIR)/tinyxml2
LIB_FILES += $(THIRD_PARTIES_DIR)/tinyxml2/tinyxml2.cpp

###################################################
# Make the list of compiled files for the library
#
LIB_FILES += $(call rwildcard,$(P)/src/IO,*.cpp)
USER_CXXFLAGS := -Wno-old-style-cast -Wno-sign-conversion -Wno-float-equal
USER_CXXFLAGS += -Wno-nan-infinity-disabled -Wno-format-nonliteral -Wno-double-promotion
USER_CXXFLAGS += -fno-fast-math

###################################################
# Public headers installed under
# $(PREFIX)/include/TimedPetriNetEditor/$(PROJECT_VERSION)/.
#
HEADER_FILES += $(call rwildcard,$(P)/src/IO,*.hpp)

###################################################
# Generic Makefile rules
#
include $(M)/rules/Makefile
###################################################
# tinyxml2 is a third party: its warnings are not ours.
#
$(OBJS_ROOT_DIR)/external/tinyxml2/tinyxml2.o: USER_CXXFLAGS += -Wno-useless-cast
