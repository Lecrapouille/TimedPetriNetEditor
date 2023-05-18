##=====================================================================
## TimedPetriNetEditor: A timed Petri net editor.
## Copyright 2021 -- 2022 Quentin Quadrat <lecrapouille@gmail.com>
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
# Project definition
#
PROJECT = TimedPetriNetEditor
TARGET = $(PROJECT)-UnitTest
DESCRIPTION = Unit tests for $(PROJECT)
BUILD_TYPE = debug
USE_COVERAGE = 1

###################################################
# Location of the project directory and Makefiles
#
P := ..
M := $(P)/.makefile
include $(M)/Makefile.header

###################################################
# Inform Makefile where to find header files
#
INCLUDES += -I$(P) -I$(P)/src -I$(P)/src/utils
INCLUDES += -I$(P)/src/Renderer
INCLUDES += -I$(P)/external/imgui -I$(P)/external/imgui-sfml
INCLUDES += -I$(P)/external/MQTT/include -I$(P)/external
INCLUDES += -I$(P)/external/portable-file-dialogs
INCLUDES += -I$(P)/external/json/include

###################################################
# Inform Makefile where to find *.cpp and *.o files
#
VPATH += $(P)/src $(P)/src/utils $(P)/src/julia
VPATH += $(P)/src/Renderer $(P)/external/MQTT/src
VPATH += $(P)/external/imgui $(P)/external/imgui-sfml

###################################################
# Reduce warnings
#
#DEFINES += -Wno-undef -Wno-switch-enum -Wno-sign-conversion -Wno-float-equal -Wno-shadow
#DEFINES += -Wno-keyword-macro -Wno-defaulted-function-deleted

###################################################
# Reduce warnings
#
DEFINES += -Wno-undef -Wno-switch-enum -Wno-sign-conversion -Wno-float-equal
DEFINES += -Wno-deprecated-copy-dtor -Wno-defaulted-function-deleted
# Aaaargf needed because of imgui warnings
DEFINES += -Wno-old-style-cast -Wno-unused-parameter
DEFINES += -Wno-conversion -Wno-cast-qual

###################################################
# Make the list of compiled files for tests
#
OBJS += Receptivities.o PetriNet.o Howard.o
OBJS += PetriNetTests.o AnimationTests.o EventGraphTests.o
OBJS += LoadJSONTests.o HowardTests.o
OBJS += main.o

###################################################
# Set Libraries. For knowing which libraries
# is needed please read the external/README.md file.
#
PKG_LIBS += sfml-graphics gtest gmock

###################################################
# Compile the project
.PHONY: all
all: $(TARGET)

###################################################
# Compile and launch unit tests and generate the code coverage html report.
.PHONY: unit-tests
.PHONY: check
unit-tests check: $(TARGET)
	@$(call print-to,"Running","$(TARGET)","$(RAPPORT)","")
	$(SANITIZER) ./$(BUILD)/$(TARGET) $(TU_OPTIONS) || (cat $(TARGET).log; return 1)

###################################################
# Sharable informations between all Makefiles
include $(M)/Makefile.footer
