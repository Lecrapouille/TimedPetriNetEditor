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
# The application is using Dear ImGui for the human
# interface.
#
include $(P)/src/Editor/DearImGui/Makefile.imgui

###################################################
# Check if objects have been set for the GUI.
#
ifeq ($(GUI_OBJS),)
$(error "No .o files have been defined for the graphical interface")
endif

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
# Embed assets for web version. Assets shall be
# present inside $(BUILD) folder.
#
ifeq ($(ARCHI),Emscripten)
LINKER_FLAGS += --preload-file examples
LINKER_FLAGS += --preload-file data
LINKER_FLAGS += -s FORCE_FILESYSTEM=1
endif

###################################################
# Create a MacOS X bundle application.
#
ifeq ($(ARCHI),Darwin)
BUILD_MACOS_APP_BUNDLE = 1
APPLE_IDENTIFIER = lecrapouille
MACOS_BUNDLE_ICON = data/TimedPetriNetEditor.icns
LINKER_FLAGS += -framework CoreFoundation
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
# Make the list of compiled files for the library
#
IMPORT_FORMATS += ImportJSON.o ImportPNML.o
EXPORT_FORMATS += ExportJSON.o ExportPNML.o ExportSymfony.o ExportPnEditor.o
EXPORT_FORMATS += ExportPetriLaTeX.o ExportJulia.o ExportGraphviz.o ExportDrawIO.o
EXPORT_FORMATS += ExportGrafcetCpp.o
LIB_OBJS += Path.o Howard.o Utils.o TimedTokens.o Receptivities.o
LIB_OBJS += PetriNet.o Algorithms.o Simulation.o History.o
LIB_OBJS += $(IMPORT_FORMATS) Imports.o $(EXPORT_FORMATS) Exports.o

###################################################
# Make the list of compiled files for the application
#
OBJS += $(LIB_OBJS) $(GUI_OBJS) Drawable.o Application.o Editor.o main.o

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
