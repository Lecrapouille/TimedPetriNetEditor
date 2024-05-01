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
TARGET = TimedPetriNetJulia
DESCRIPTION = Timed Petri Net Editor interface for Julia

###################################################
# Location of the project directory and Makefiles
#
include $(M)/Makefile.header

###################################################
# Internal libraries since we can call the GUI from Julia REPL
#
LIB_TPNE_CORE = $(abspath $(P)/src/Net/$(BUILD)/libtimedpetrinetcore.a)
LIB_TPNE_GUI = $(abspath $(P)/src/Editor/$(BUILD)/libtimedpetrinetgui.a)
THIRDPART_LIBS += $(LIB_TPNE_GUI) $(LIB_TPNE_CORE)
PKG_LIBS += --static glfw3
LINKER_FLAGS += -lGL

###################################################
# Inform Makefile where to find *.cpp files
#
include $(P)/src/Editor/DearImGui/Makefile.imgui
VPATH += $(P)/include $(P)/src $(P)/src/Utils $(P)/src/Net
VPATH += $(P)/src/Net/Imports VPATH += $(P)/src/Net/Exports

###################################################
# Inform Makefile where to find header files
#
INCLUDES += -I$(P)/include -I$(P)/src -I$(P)/external

###################################################
# Make the list of compiled files for the library
#
LIB_OBJS += Julia.o

###################################################
# Compile the project, the static and shared libraries
.PHONY: all
all: $(STATIC_LIB_TARGET) $(SHARED_LIB_TARGET) $(PKG_FILE) $(LIB_TPNE_GUI) $(LIB_TPNE_CORE) $(P)/Makefile $(P)/Makefile.common

###################################################
# Install project. You need to be root.
.PHONY: install
install: $(STATIC_LIB_TARGET) $(SHARED_LIB_TARGET) $(PKG_FILE)
	@$(call INSTALL_PROJECT_LIBRARIES)

###################################################
# Sharable informations between all Makefiles
#
include $(M)/Makefile.footer
