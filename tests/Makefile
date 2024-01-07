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
INCLUDES += -I$(P)/include -I$(P)/src -I$(P)/external

###################################################
# Inform Makefile where to find *.cpp and *.o files
#
VPATH += $(P)/include $(P)/src $(P)/src/Utils $(P)/src/Net
VPATH += $(P)/src/Net/Imports VPATH += $(P)/src/Net/Exports
VPATH += $(P)/src/Application $(P)/src/Editor $(P)/src/Editor/DearImGui

###################################################
# Set json Library.
#
INCLUDES += -I$(THIRDPART)/json/include

###################################################
# Set xml Library.
#
VPATH += $(THIRDPART)/tinyxml2
OBJS += tinyxml2.o

###################################################
# Make the list of compiled files for tests
#
OBJS += Path.o Howard.o Utils.o TimedTokens.o Receptivities.o
OBJS += PetriNet.o Algorithms.o Simulation.o
OBJS += ExportGrafcetCpp.o ExportJSON.o ExportSymfony.o ExportPnEditor.o
OBJS += ExportPetriLaTeX.o ExportJulia.o ExportGraphviz.o ExportDrawIO.o
OBJS += ExportPNML.o Exports.o ImportPNML.o ImportJSON.o Imports.o
OBJS += PetriNetTests.o LoadJSONTests.o HowardTests.o EventGraphTests.o
OBJS += SparseMatrixTests.o TimedTokensTests.o main.o

###################################################
# MacOS X
#
ifeq ($(ARCHI),Darwin)
LINKER_FLAGS += -framework CoreFoundation
endif

###################################################
# Set Libraries. For knowing which libraries
# is needed please read the external/README.md file.
#
PKG_LIBS += gtest gmock

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
