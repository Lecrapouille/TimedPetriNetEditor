# SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
# Copyright (c) 2020-2026 Quentin Quadrat
#
# This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
# for users who cannot use the GPL, under a commercial license.
# See LICENSING.md for details.

###################################################
# Location of the project directory and Makefiles
#
_MK_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
P := $(abspath $(_MK_DIR)/../..)
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
VPATH += $(P)/src $(P)/src/PetriNet $(P)/src/MaxPlus
VPATH += $(P)/src/PetriNet/Imports $(P)/src/PetriNet/Exports

###################################################
# Inform Makefile where to find header files
#
INCLUDES += $(P)/src

###################################################
# Internal libraries since we can call the GUI from Julia REPL
# We force compilation order.
LIB_TPNE_NET := $(call internal-lib,TimedPetriNet)
LIB_TPNE_MAXPLUS := $(call internal-lib,TimedPetriMaxPlus)
LIB_TPNE_EDITOR := $(call internal-lib,TimedPetriGUI)
INTERNAL_LIBS := $(LIB_TPNE_EDITOR) $(LIB_TPNE_MAXPLUS) $(LIB_TPNE_NET)
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
include $(abspath $(P)/src/Editor/Backends/Makefile)

###################################################
# Generic Makefile rules
#
include $(M)/rules/Makefile