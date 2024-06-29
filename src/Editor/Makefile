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
TARGET_NAME := TimedPetriGUI
TARGET_DESCRIPTION := Library Editor for Petri nets
include $(M)/project/Makefile

###################################################
# For the moment we only use Dear ImGui
#
include ./DearImGui/Backends/Makefile

###################################################
# Make the list of compiled files for the library
#
INCLUDES += $(P)/src $(P)/include
LIB_FILES += $(GUI_FILES)
THIRDPART_LIBS := $(GUI_THIRDPART_LIBS)

###################################################
# Generic Makefile rules
#
include $(M)/rules/Makefile