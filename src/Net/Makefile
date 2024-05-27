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
    INCLUDES += $(THIRDPART_DIR)/MQTT/include
    VPATH += $(THIRDPART_DIR)/MQTT/src
    DEFINES += -DMQTT_BROKER_ADDR=\"localhost\"
    DEFINES += -DMQTT_BROKER_PORT=1883
    PKG_LIBS += libmosquitto
endif

###################################################
# Set json ibrary.
#
INCLUDES += $(THIRDPART_DIR)/json/include

###################################################
# Set xml Library.
#
VPATH += $(THIRDPART_DIR)/tinyxml2
LIB_FILES += $(THIRDPART_DIR)/tinyxml2/tinyxml2.cpp
USER_CXXFLAGS := -Wno-old-style-cast -Wno-sign-conversion

###################################################
# MacOS X: get path inside the bundle application
#
ifeq ($(OS),Darwin)
    LINKER_FLAGS += -framework CoreFoundation
endif

###################################################
# Inform Makefile where to find *.cpp files
#
VPATH += $(P)/src $(P)/src/Utils $(P)/src/Net
VPATH += $(P)/src/Net/Imports $(P)/src/Net/Exports

###################################################
# Inform Makefile where to find header files
#
INCLUDES += $(P)/include $(P)/src $(THIRDPART_DIR)
INCLUDES += $(THIRDPART_DIR)/imgui

###################################################
# Make the list of compiled files for the library
#
LIB_FILES += $(call rwildcard,$(P)/src/Net,*.cpp)
LIB_FILES += $(call rwildcard,$(P)/src/Net,*.c)
LIB_FILES += $(call rwildcard,$(P)/src/Utils,*.cpp)
USER_CXXFLAGS += -Wno-cast-qual -Wno-float-equal
USER_CCFLAGS += -Wno-sign-conversion -Wno-float-equal

###################################################
# Generic Makefile rules
#
include $(M)/rules/Makefile