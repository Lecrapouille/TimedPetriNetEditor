# SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
# Copyright (c) 2020-2026 Quentin Quadrat
#
# This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
# for users who cannot use the GPL, under a commercial license.
# See LICENSING.md for details.

###################################################
# Location of the project directory and Makefiles
#
P := ..
M := $(P)/.makefile

###################################################
# Project definition
#
include $(P)/Makefile.common
TARGET_NAME := $(PROJECT_NAME)-UnitTest
TARGET_DESCRIPTION := Unit tests for $(PROJECT_NAME)
BUILD_TYPE = test
include $(M)/project/Makefile

###################################################
# Set json ibrary.
#
INCLUDES += $(THIRD_PARTIES_DIR)/json/include

###################################################
# Set xml Library.
#
VPATH += $(THIRD_PARTIES_DIR)/tinyxml2
SRC_FILES += $(THIRD_PARTIES_DIR)/tinyxml2/tinyxml2.cpp
USER_CXXFLAGS := -Wno-old-style-cast -Wno-sign-conversion

###################################################
# Inform Makefile where to find *.cpp files
#
VPATH += $(P)/src $(P)/src/PetriNet $(P)/src/MaxPlus
VPATH += $(P)/src/PetriNet/Imports $(P)/src/PetriNet/Exports

###################################################
# Inform Makefile where to find header files
#
INCLUDES += $(P)/src $(THIRD_PARTIES_DIR)

###################################################
# Make the list of compiled files for the library
#
SRC_FILES += $(call rwildcard,$(P)/src/PetriNet,*.cpp)
SRC_FILES += $(call rwildcard,$(P)/src/PetriNet,*.c)
SRC_FILES += $(call rwildcard,$(P)/src/MaxPlus,*.cpp)
SRC_FILES += $(call rwildcard,$(P)/src/MaxPlus,*.c)
SRC_FILES += $(call rwildcard,$(P)/tests,*.cpp)

# BEWARE: Disable fast math for testing, else the tests on NaN will fail.
USER_CXXFLAGS += -Wno-float-equal -fno-fast-math -Wno-nan-infinity-disabled
USER_CCFLAGS += -Wno-sign-conversion -Wno-float-equal -fno-fast-math

###################################################
# Set Libraries. For knowing which libraries
# is needed please read the external/README.md file.
#
PKG_LIBS += gtest gmock

###################################################
# Generic Makefile rules
#
include $(M)/rules/Makefile