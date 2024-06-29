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
TARGET_NAME := TimedPetriJulia
TARGET_DESCRIPTION := Timed Petri Net Editor interface for Julia
include $(M)/project/Makefile

###################################################
# Inform Makefile where to find *.cpp files
#
VPATH += $(P)/src $(P)/src/Utils $(P)/src/Net
VPATH += $(P)/src/Net/Imports VPATH += $(P)/src/Net/Exports

###################################################
# Inform Makefile where to find header files
#
INCLUDES += $(P)/include $(P)/src

###################################################
# Internal libraries since we can call the GUI from Julia REPL
# We force compilation order.
LIB_TPNE_NET := $(call internal-lib,TimedPetriNet)
LIB_TPNE_EDITOR := $(call internal-lib,TimedPetriGUI)
INTERNAL_LIBS := $(LIB_TPNE_EDITOR) $(LIB_TPNE_NET)
#DIRS_WITH_MAKEFILE := $(P)/src/Net $(P)/src/Editor

#$(LIB_TPNE_NET): $(P)/src/Net

#$(LIB_TPNE_EDITOR): $(P)/src/Editor

#$(P)/src/Editor: $(P)/src/Net

###################################################
# Make the list of compiled files for the library
#
LIB_FILES += $(P)/src/julia/Julia.cpp

USER_CXXFLAGS := -Wno-cast-qual -Wno-undef

###################################################
# GUI
#
include $(abspath $(P)/src/Editor/DearImGui/Backends/Makefile)

###################################################
# Generic Makefile rules
#
include $(M)/rules/Makefile