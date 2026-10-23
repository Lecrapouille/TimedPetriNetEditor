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
TARGET_NAME := TimedPetriNet
TARGET_DESCRIPTION := Library manipulating Petri nets
include $(M)/project/Makefile

###################################################
# Set MQTT Library.
#
ifeq ($(OS),Linux)
    INCLUDES += $(THIRD_PARTIES_DIR)/MQTT/include
    VPATH += $(THIRD_PARTIES_DIR)/MQTT/src
    DEFINES += -DMQTT_BROKER_ADDR=\"localhost\"
    DEFINES += -DMQTT_BROKER_PORT=1883
    PKG_LIBS += libmosquitto
endif

###################################################
# Set json ibrary.
#
INCLUDES += $(THIRD_PARTIES_DIR)/json/include

###################################################
# Set xml Library.
#
VPATH += $(THIRD_PARTIES_DIR)/tinyxml2
LIB_FILES += $(THIRD_PARTIES_DIR)/tinyxml2/tinyxml2.cpp
USER_CXXFLAGS := -Wno-old-style-cast -Wno-sign-conversion
USER_CXXFLAGS += -Wno-nan-infinity-disabled -Wno-format-nonliteral -Wno-double-promotion

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

###################################################
# Make the list of compiled files for the library
#
LIB_FILES += $(call rwildcard,$(P)/src/PetriNet,*.cpp)
LIB_FILES += $(call rwildcard,$(P)/src/PetriNet,*.c)
USER_CXXFLAGS += -Wno-cast-qual -Wno-float-equal
USER_CCFLAGS += -Wno-sign-conversion -Wno-float-equal -Wno-nan-infinity-disabled
# The core relies on NaN and infinities: never with -ffast-math.
USER_CXXFLAGS += -fno-fast-math
USER_CCFLAGS += -fno-fast-math

###################################################
# Public headers installed under
# $(PREFIX)/include/TimedPetriNetEditor/$(PROJECT_VERSION)/.
#
HEADER_FILES += $(call rwildcard,$(P)/src/PetriNet,*.hpp)

###################################################
# Generic Makefile rules
#
include $(M)/rules/Makefile