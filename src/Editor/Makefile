###################################################
# Location of the project directory and Makefiles
#
P := ../..
M := $(P)/.makefile
BUILD := $(P)/build

###################################################
# Project definition
#
include $(P)/Makefile.common
TARGET = TimedPetriNetGui
DESCRIPTION = Library for the GUI of the Timed Petri Net Editor

###################################################
# Sharable informations between all Makefiles
#
include $(M)/Makefile.header

###################################################
# For the moment we only use Dear ImGui
#
include ./DearImGui/Makefile.imgui

###################################################
# Make the list of compiled files for the library
#
VPATH += $(P)/include $(P)/src $(P)/src/Utils $(P)/src/Net
VPATH += $(P)/src/Net/Imports $(P)/src/Net/Exports
INCLUDES += -I$(P)/include -I$(P)/src -I$(P)/external
LIB_OBJS += $(GUI_OBJS) Drawable.o Application.o Editor.o

###################################################
# Compile the project, the static and shared libraries
#
.PHONY: all
all: $(STATIC_LIB_TARGET) $(SHARED_LIB_TARGET) $(PKG_FILE) $(P)/Makefile $(P)/Makefile.common

###################################################
# Install project. You need to be root.
.PHONY: install
install: $(STATIC_LIB_TARGET) $(SHARED_LIB_TARGET) $(PKG_FILE)
	@$(call INSTALL_PROJECT_LIBRARIES)

###################################################
# Sharable informations between all Makefiles
#
include $(M)/Makefile.footer