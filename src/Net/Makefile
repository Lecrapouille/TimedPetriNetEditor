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
TARGET = TimedPetriNetCore
DESCRIPTION = Library for the Core of the Timed Petri Net Editor

###################################################
# Location of the project directory and Makefiles
#
include $(M)/Makefile.header

###################################################
# Set MQTT Library.
#
ifneq ($(ARCHI),Emscripten)
INCLUDES += -I$(THIRDPART) -I$(THIRDPART)/MQTT/include
VPATH += $(THIRDPART)/MQTT/src
DEFINES += -DMQTT_BROKER_ADDR=\"localhost\"
DEFINES += -DMQTT_BROKER_PORT=1883
PKG_LIBS += libmosquitto
endif

###################################################
# Set json ibrary.
#
INCLUDES += -I$(THIRDPART)/json/include

###################################################
# Set xml Library.
#
VPATH += $(THIRDPART)/tinyxml2
LIB_OBJS += tinyxml2.o

###################################################
# MacOS X
#
ifeq ($(ARCHI),Darwin)
LINKER_FLAGS += -framework CoreFoundation
endif

###################################################
# Inform Makefile where to find *.cpp files
#
VPATH += $(P)/include $(P)/src $(P)/src/Utils $(P)/src/Net
VPATH += $(P)/src/Net/Imports $(P)/src/Net/Exports

###################################################
# Inform Makefile where to find header files
#
INCLUDES += -I$(P)/include -I$(P)/src -I$(P)/external

###################################################
# Linkage
#
LINKER_FLAGS += -ldl -lpthread

###################################################
# Make the list of compiled files for the library
#
LIB_OBJS += ImportJSON.o ImportPNML.o ImportTimedEventGraph.o ExportTimedEventGraph.o
LIB_OBJS += ExportJSON.o ExportPNML.o ExportSymfony.o ExportPnEditor.o
LIB_OBJS += ExportPetriLaTeX.o ExportJulia.o ExportGraphviz.o ExportDrawIO.o
LIB_OBJS += ExportGrafcetCpp.o ImportFlowshop.o
LIB_OBJS += Path.o Howard.o Utils.o TimedTokens.o Receptivities.o
LIB_OBJS += PetriNet.o Algorithms.o Simulation.o History.o
LIB_OBJS += Imports.o Exports.o

###################################################
# Compile the project, the static and shared libraries
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
