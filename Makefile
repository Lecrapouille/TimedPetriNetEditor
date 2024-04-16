###################################################
# Project definition
#
PROJECT = TimedPetriNetEditor
TARGET = $(PROJECT)
DESCRIPTION = Timed Petri Net Editor
STANDARD = --std=c++14
BUILD_TYPE = debug

###################################################
# Location of the project directory and Makefiles
#
P := .
M := $(P)/.makefile
include $(M)/Makefile.header

###################################################
# Editor using Dear ImGui backend.
# Select backend for dear im gui: RayLib or GLFW3
#
#DEAR_IMGUI_BACKEND ?= RayLib
DEAR_IMGUI_BACKEND ?= GLFW3
VPATH += $(P)/src/Editor/DearImGui
INCLUDES += -I$(P)/src/Editor/DearImGui

###################################################
# Check selected backend for dear im gui if compiled for html5
# Be sure to place this section after including MyMakefile
ifeq ($(ARCHI),Emscripten)
ifneq ($(DEAR_IMGUI_BACKEND),RayLib)
$(warning Force RayLib backend for compiling with Emscripten)
DEAR_IMGUI_BACKEND = RayLib
endif
endif

###################################################
# Inform Makefile where to find *.cpp files
#
VPATH += $(P)/include $(P)/src $(P)/src/Utils $(P)/src/Net
VPATH += $(P)/src/Net/Imports VPATH += $(P)/src/Net/Exports

###################################################
# Inform Makefile where to find header files
#
INCLUDES += -I$(P)/include -I$(P)/src -I$(P)/external

###################################################
# Project defines
#
DEFINES += -DDATADIR=\"$(DATADIR):$(abspath $(P))/data/:data/\"

###################################################
# Reduce warnings
#
CCFLAGS += -Wno-sign-conversion -Wno-float-equal
CXXFLAGS += -Wno-undef -Wno-switch-enum -Wno-enum-compare

###################################################
# Linkage
#
LINKER_FLAGS += -ldl -lpthread

###################################################
# Set thirdpart Raylib
#
ifeq ($(DEAR_IMGUI_BACKEND),RayLib)
INCLUDES += -I$(THIRDPART)/raylib/src
THIRDPART_LIBS += $(abspath $(THIRDPART)/raylib/src/$(ARCHI)/libraylib.a)

ifeq ($(ARCHI),Emscripten)
# We tell the linker that the game/library uses GLFW3
# library internally, it must be linked automatically
# (emscripten provides the implementation)
LINKER_FLAGS += -s USE_GLFW=3
# Add this flag ONLY in case we are using ASYNCIFY code
LINKER_FLAGS += -s ASYNCIFY
# For linking glfwGetProcAddress().
LINKER_FLAGS += -s GL_ENABLE_GET_PROC_ADDRESS
# All webs need a "shell" structure to load and run the game,
# by default emscripten has a `shell.html` but we can provide
# our own.
LINKER_FLAGS += --shell-file $(THIRDPART)/raylib/src/shell.html
endif
endif

###################################################
# Dear ImGui backends: Raylib
#
ifeq ($(DEAR_IMGUI_BACKEND),RayLib)
VPATH += $(THIRDPART)/rlImGui
VPATH += $(P)/src/Editor/DearImGui/Backends/RayLib
INCLUDES += -I$(THIRDPART)/rlImGui
INCLUDES += -I$(P)/src/Editor/DearImGui/Backends/RayLib
DEARIMGUI_DEAR_IMGUI_BACKEND_OBJS += rlImGui.o
endif

###################################################
# Dear ImGui backends: OpenGL/GLFW3
#
ifeq ($(DEAR_IMGUI_BACKEND),GLFW3)
VPATH += $(P)/src/Editor/DearImGui/Backends/GLFW3
INCLUDES += -I$(P)/src/Editor/DearImGui/Backends/GLFW3
DEARIMGUI_DEAR_IMGUI_BACKEND_OBJS += imgui_impl_glfw.o imgui_impl_opengl3.o
endif

###################################################
# Set thirdpart Dear ImGui
#
INCLUDES += -I$(THIRDPART)/imgui -I$(THIRDPART)/imgui/backends -I$(THIRDPART)/imgui/misc/cpp
VPATH += $(THIRDPART)/imgui $(THIRDPART)/imgui/backends $(THIRDPART)/imgui/misc/cpp
DEARIMGUI_OBJS += imgui_widgets.o imgui_draw.o imgui_tables.o imgui.o imgui_stdlib.o
# DEARIMGUI_OBJS += imgui_demo.o

###################################################
# Set thirdpart Dear ImGui Plot
#
VPATH += $(THIRDPART)/implot
INCLUDES += -I$(THIRDPART)/implot
DEARIMGUI_OBJS += implot_items.o implot.o

###################################################
# Set thirdpart file dialog
VPATH += $(THIRDPART)/ImGuiFileDialog
INCLUDES += -I$(THIRDPART)/ImGuiFileDialog
DEARIMGUI_OBJS += ImGuiFileDialog.o

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
# OpenGL: glfw and glew libraries
#
ifeq ($(ARCHI),Darwin)
INCLUDES += -I/usr/local/include -I/opt/local/include
LINKER_FLAGS += -framework OpenGL -framework Cocoa
LINKER_FLAGS += -framework IOKit -framework CoreVideo
LINKER_FLAGS += -L/usr/local/lib -L/opt/local/lib
LINKER_FLAGS += -lGLEW -lglfw
else ifeq ($(ARCHI),Linux)
LINKER_FLAGS += -lGL
PKG_LIBS += --static glfw3
else ifneq ($(ARCHI),Emscripten)
$(error Unknown architecture $(ARCHI) for OpenGL)
endif

###################################################
# Check if Dear im gui backend has been set
#
ifeq ($(DEARIMGUI_DEAR_IMGUI_BACKEND_OBJS),)
$(error "Define DEAR_IMGUI_BACKEND either as RayLib or GLFW3")
endif

###################################################
# Embed assets for web version. Assets shall be
# present inside $(BUILD) folder.
#
ifeq ($(ARCHI),Emscripten)
LINKER_FLAGS += --preload-file examples
LINKER_FLAGS += --preload-file data
LINKER_FLAGS += -s FORCE_FILESYSTEM=1
endif

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
# Make the list of compiled files for the application
#
LIB_OBJS += Path.o Howard.o Utils.o TimedTokens.o Receptivities.o
LIB_OBJS += PetriNet.o Algorithms.o Simulation.o History.o
LIB_OBJS += ExportJSON.o ExportSymfony.o ExportPnEditor.o
LIB_OBJS += ExportPetriLaTeX.o ExportJulia.o ExportGraphviz.o ExportDrawIO.o
LIB_OBJS += ExportGrafcetCpp.o ImportPNML.o ExportPNML.o Exports.o
LIB_OBJS += ImportJSON.o Imports.o
OBJS += $(DEARIMGUI_DEAR_IMGUI_BACKEND_OBJS) $(DEARIMGUI_OBJS)
OBJS += $(LIB_OBJS)
OBJS += DearUtils.o Drawable.o Application.o Editor.o main.o

###################################################
# Compile the project, the static and shared libraries
.PHONY: all
all: copy-assets $(STATIC_LIB_TARGET) $(SHARED_LIB_TARGET) $(PKG_FILE) $(TARGET)

###################################################
# Copy data inside BUILD to allow emscripten to embedded them
.PHONY: copy-assets
copy-assets: | $(BUILD)
ifeq ($(ARCHI),Emscripten)
	@$(call print-to,"Copying assets","$(TARGET)","$(BUILD)","")
	@mkdir -p $(BUILD)/data
	@mkdir -p $(BUILD)/examples
	@cp $(P)/data/examples/*.json $(BUILD)/examples
	@cp $(P)/data/font.ttf $(BUILD)/data
	@cp $(P)/data/imgui.ini $(BUILD)/data
endif

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
install: $(TARGET) $(STATIC_LIB_TARGET) $(SHARED_LIB_TARGET) $(PKG_FILE)
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
