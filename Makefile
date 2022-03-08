##=====================================================================
## TimedPetriNetEditor: A timed Petri net editor.
## Copyright 2021 Quentin Quadrat <lecrapouille@gmail.com>
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

PROJECT = TimedPetriNetEditor
TARGET = $(PROJECT)
STANDARD = --std=c++14
DESCRIPTION = Timed Petri Net Editor
BUILD_TYPE = debug

P := .
M := $(P)/.makefile
include $(M)/Makefile.header

VPATH += $(P)/src $(P)/src/utils $(P)/src/julia
INCLUDES += -I$(P)/src -I$(P)/src/utils -I$(P)
DEFINES = -DDATADIR=\"$(DATADIR)\"
DEFINES += -Wno-undef -Wno-switch-enum

LIB_OBJS += Howard.o PetriNet.o PetriEditor.o Julia.o
OBJS += $(LIB_OBJS) main.o

PKG_LIBS = sfml-graphics

all: $(TARGET)

include $(M)/Makefile.footer
