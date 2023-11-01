###################################################
# Project definition
#
PROJECT = TimedPetriNetEditor
TARGET = $(PROJECT)
DESCRIPTION = Test RayLib
STANDARD = --std=c++14
BUILD_TYPE = debug

###################################################
# Location of the project directory and Makefiles
#
P := .
M := $(P)/.makefile
include $(M)/Makefile.header

###################################################
# Linkage
#
LINKER_FLAGS += -ldl -lpthread

###################################################
# Inform Makefile where to find header files
#
INCLUDES += -I$(P)/include -I$(P)/src -I$(P)/src/Net/Formats -I$(P)/src/Renderer

###################################################
# Inform Makefile where to find *.cpp and *.o files
#
VPATH += $(P)/src $(P)/src/Net $(P)/src/Net/Formats $(P)/src/Renderer $(P)/include

###################################################
# Project defines
#
DEFINES += -DDATADIR=\"$(DATADIR)\"

###################################################
# Reduce warnings
#
CCFLAGS += -Wno-sign-conversion -Wno-float-equal
CXXFLAGS += -Wno-undef -Wno-switch-enum -Wno-enum-compare

###################################################
# Set thirdpart Raylib
#
INCLUDES += -I$(THIRDPART)/raylib/src
THIRDPART_LIBS += $(abspath $(THIRDPART)/raylib/src/$(ARCHI)/libraylib.a)

ifeq ($(ARCHI),Emscripten)
# We tell the linker that the game/library uses GLFW3
# library internally, it must be linked automatically
# (emscripten provides the implementation)
LINKER_FLAGS += -s USE_GLFW=3
# Add this flag ONLY in case we are using ASYNCIFY code
LINKER_FLAGS += -s ASYNCIFY
# All webs need a "shell" structure to load and run the game,
# by default emscripten has a `shell.html` but we can provide
# our own.
LINKER_FLAGS += --shell-file $(THIRDPART)/raylib/src/shell.html
endif

###################################################
# Set thirdpart Dear ImGui
#
INCLUDES += -I$(THIRDPART)/imgui -I$(THIRDPART)/imgui/backends -I$(THIRDPART)/imgui/misc/cpp
VPATH += $(THIRDPART)/imgui $(THIRDPART)/imgui/backends $(THIRDPART)/imgui/misc/cpp
DEARIMGUI_OBJS += imgui_widgets.o imgui_draw.o imgui_tables.o imgui.o imgui_stdlib.o
# DEARIMGUI_OBJS += imgui_impl_glfw.o imgui_impl_opengl3.o

###################################################
# Set thirdpart Dear ImGui Plot
#
INCLUDES += -I$(THIRDPART)/implot
VPATH += $(THIRDPART)/implot
DEARIMGUI_OBJS += implot_items.o implot.o

###################################################
# Set thirdpart file dialog
INCLUDES += -I$(THIRDPART)/ImGuiFileDialog
VPATH += $(THIRDPART)/ImGuiFileDialog
DEARIMGUI_OBJS += ImGuiFileDialog.o

###################################################
# Set thirdpart Raylib with Dear ImGui
#
INCLUDES += -I$(THIRDPART)/rlImGui
VPATH += $(THIRDPART)/rlImGui
DEARIMGUI_OBJS += rlImGui.o

###################################################
# Set MQTT Library.
#
INCLUDES += -I$(THIRDPART) -I$(THIRDPART)/MQTT/include
VPATH += $(THIRDPART)/MQTT/src
DEFINES += -DMQTT_BROKER_ADDR=\"localhost\"
DEFINES += -DMQTT_BROKER_PORT=1883
PKG_LIBS += libmosquitto

###################################################
# Set json Library.
#
INCLUDES += -I$(THIRDPART)/json/include

###################################################
# Make the list of compiled files for the application
#
#DEARIMGUI_OBJS += imgui_demo.o
OBJS += $(DEARIMGUI_OBJS)
OBJS += Application.o PetriNet.o PetriEditor.o Howard.o Algorithms.o main.o
OBJS += ImportJSON.o ExportJSON.o ExportSymfony.o ExportPnEditor.o
OBJS += ExportPetriLaTeX.o ExportJulia.o ExportGraphviz.o ExportDrawIO.o
OBJS += ExportGrafcetCpp.o

###################################################
# MacOS X
#
ifeq ($(ARCHI),Darwin)
BUILD_MACOS_APP_BUNDLE = 1
APPLE_IDENTIFIER = lecrapouille
MACOS_BUNDLE_ICON = data/TimedPetriNetEditor.icns
LINKER_FLAGS += -framework CoreFoundation
endif

###################################################
# Compile the project, the static and shared libraries
.PHONY: all
all: $(TARGET)

###################################################
# Compile and launch unit tests and generate the code coverage html report.
.PHONY: unit-tests
unit-tests:
	@$(call print-simple,"Compiling unit tests")
	@$(MAKE) -C tests coverage

###################################################
# Compile and launch unit tests and generate the code coverage html report.
.PHONY: check
check: unit-tests

ifeq ($(ARCHI),Linux)
###################################################
# Install project. You need to be root.
.PHONY: install
install: $(TARGET)
	@$(call INSTALL_BINARY)
	@$(call INSTALL_DOCUMENTATION)
	@$(call INSTALL_PROJECT_LIBRARIES)
	@$(call INSTALL_PROJECT_HEADERS)
endif

###################################################
# Clean the whole project.
.PHONY: veryclean
veryclean: clean
	@rm -fr cov-int $(PROJECT).tgz *.log foo 2> /dev/null
	@(cd tests && $(MAKE) -s clean)
	@$(call print-simple,"Cleaning","$(PWD)/doc/html")
	@rm -fr $(THIRDPART)/*/ doc/html 2> /dev/null

###################################################
# Sharable informations between all Makefiles
include $(M)/Makefile.footer
