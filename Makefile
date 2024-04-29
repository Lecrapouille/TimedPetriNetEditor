###################################################
# Location of the project directory and Makefiles
#
P := .
M := $(P)/.makefile

###################################################
# Project definition
#
include $(P)/Makefile.common
TARGET = $(PROJECT)
DESCRIPTION = Timed Petri Net Editor

###################################################
# Other targets
#
LIB_TPNE_CORE = $(abspath $(P)/src/Net/$(BUILD)/libtimedpetrinetcore.a)
LIB_TPNE_GUI = $(abspath $(P)/src/Editor/$(BUILD)/libtimedpetrinetgui.a)
LIB_TPNE_JULIA = $(abspath $(P)/src/julia/$(BUILD)/libtimedpetrinetjulia.a)

###################################################
# Sharable informations between all Makefiles
#
include $(M)/Makefile.header

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
# Stand-alone application.
#
include $(P)/src/Editor/DearImGui/Makefile.imgui
THIRDPART_LIBS += $(LIB_TPNE_GUI) $(LIB_TPNE_CORE)
THIRDPART_LIBS += $(LIB_TPNE_JULIA)
LINKER_FLAGS += -ldl -lpthread
VPATH += $(P)/include $(P)/src $(P)/src/Utils $(P)/src/Net
VPATH += $(P)/src/Net/Imports VPATH += $(P)/src/Net/Exports
INCLUDES += -I$(P)/include -I$(P)/src -I$(P)/external
OBJS += main.o

###################################################
# Compile the stand-alone application
.PHONY: all
all: | $(LIB_TPNE_GUI) $(LIB_TPNE_CORE) $(LIB_TPNE_JULIA)
all: $(TARGET) $(LIB_TPNE_GUI) $(LIB_TPNE_CORE) $(LIB_TPNE_JULIA) copy-emscripten-assets

###################################################
# Compile The Petri net core lib
$(LIB_TPNE_CORE): src/Net/Makefile Makefile
	@$(call print-from,"Compiling Petri net core lib",$(PROJECT),core)
	@$(MAKE) -C src/Net all
	@cp $(LIB_TPNE_CORE) $(BUILD)

###################################################
# Compile The Petri net gui lib 
$(LIB_TPNE_GUI): | $(LIB_TPNE_CORE)
$(LIB_TPNE_GUI): src/Editor/Makefile Makefile
	@$(call print-from,"Compiling Petri net GUI lib",$(PROJECT),gui)
	@$(MAKE) -C src/Editor all
	@cp $(LIB_TPNE_GUI) $(BUILD)

###################################################
# Compile interface for Julia
$(LIB_TPNE_JULIA): | $(LIB_TPNE_GUI)
$(LIB_TPNE_JULIA): $(LIB_TPNE_GUI) $(LIB_TPNE_CORE) src/julia/Makefile Makefile
	@$(call print-from,"Compiling Interface for Julia-lang",$(PROJECT),Julia-lang)
	@$(MAKE) -C src/julia all
	@cp $(LIB_TPNE_JULIA) $(BUILD)

###################################################
# Copy data inside BUILD to allow emscripten to embedded them
.PHONY: copy-assets
copy-emscripten-assets: | $(BUILD)
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
install: $(TARGET)
	@$(MAKE) --no-print-directory -C src/Net install
	@$(MAKE) --no-print-directory -C src/Editor install
	@$(MAKE) --no-print-directory -C src/julia install
	@$(call INSTALL_BINARY)
	@$(call INSTALL_DOCUMENTATION)
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
#
include $(M)/Makefile.footer

###################################################
# Override clean the project
.PHONY: clean
clean::
	@$(MAKE) --no-print-directory -C src/Net clean
	@$(MAKE) --no-print-directory -C src/Editor clean
	@$(MAKE) --no-print-directory -C src/julia clean