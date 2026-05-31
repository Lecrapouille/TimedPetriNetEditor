###################################################
# ZeroMQ remote control (see TPNE_ZEROMQ in Makefile.common).
#
ifeq ($(OS),Emscripten)
TPNE_ZEROMQ := 0
endif

# Disable ZeroMQ when libzmq is not available.
ifneq ($(TPNE_ZEROMQ),0)
ifneq ($(OS),Emscripten)
ifeq ($(OS),Darwin)
    # Homebrew: pkg-config may succeed without usable -I flags on macOS CI.
    ZMQ_BREW_PREFIX := $(shell brew --prefix zeromq 2>/dev/null)
    ifneq ($(ZMQ_BREW_PREFIX),)
        ifneq ($(wildcard $(ZMQ_BREW_PREFIX)/include/zmq.h),)
            ZMQ_FOUND := 1
        endif
    endif
else
    ZMQ_FOUND := $(shell pkg-config --exists libzmq 2>/dev/null && echo 1)
endif
ifeq ($(ZMQ_FOUND),)
$(warning libzmq not found; building with TPNE_ZEROMQ=0)
TPNE_ZEROMQ := 0
endif
endif
endif

ifeq ($(TPNE_ZEROMQ),1)
ifneq ($(OS),Emscripten)
    # Do not add libzmq to PKG_LIBS: "--static glfw3" would pull broken private deps.
    USER_CXXFLAGS += $(shell pkg-config --cflags libzmq 2>/dev/null)
ifeq ($(OS),Darwin)
    ifneq ($(ZMQ_BREW_PREFIX),)
        INCLUDES += -I$(ZMQ_BREW_PREFIX)/include
        LINKER_FLAGS += -L$(ZMQ_BREW_PREFIX)/lib
    endif
endif
    LINKER_FLAGS += $(shell pkg-config --libs-only-l libzmq 2>/dev/null || echo -lzmq)
endif
    DEFINES += -DTPNE_USE_ZEROMQ=1
    GUI_REMOTE_FILES := $(P)/src/Editor/Remote/ZeroMQRemote.cpp
else
    DEFINES += -DTPNE_USE_ZEROMQ=0
    GUI_REMOTE_FILES := $(P)/src/Editor/Remote/NullRemoteControl.cpp
endif

###################################################
# If compiling for HTML5 (Emscripten) then force
# using Raylib backend since GLFW3 is not compilable.
#
ifeq ($(OS),Emscripten)
    ifdef EXAEQUOS
        ifneq ($(DEAR_IMGUI_BACKEND),GLFW3)
            $(warning Force GLFW3 backend for compiling with ExaequOS)
            DEAR_IMGUI_BACKEND = GLFW3
        endif
    else
        ifneq ($(DEAR_IMGUI_BACKEND),RayLib)
            $(warning Force RayLib backend for compiling with Emscripten)
            DEAR_IMGUI_BACKEND = RayLib
        endif
    endif
endif

###################################################
# Dear ImGui backends: GLFW3
#
ifeq ($(DEAR_IMGUI_BACKEND),GLFW3)
    INCLUDES += $(P)/src/Editor/Backends/GLFW3
    GUI_FILES += $(P)/src/Editor/Backends/GLFW3/Application.cpp
    INCLUDES += $(THIRD_PARTIES_DIR)/imgui/
    INCLUDES += $(THIRD_PARTIES_DIR)/imgui/backends
    GUI_FILES += $(THIRD_PARTIES_DIR)/imgui/backends/imgui_impl_glfw.cpp
    GUI_FILES += $(THIRD_PARTIES_DIR)/imgui/backends/imgui_impl_opengl3.cpp
    ifeq ($(OS),Linux)
        # OpenGL, GLEW, GLFW3
        LINKER_FLAGS += -lGL
        PKG_LIBS += --static glfw3
    else ifeq ($(OS),Darwin)
        # GLFW/GLEW via pkg-config (Homebrew: /opt/homebrew on Apple Silicon).
        PKG_LIBS += glfw3 glew
        DEFINES += -DGL_SILENCE_DEPRECATION
        LINKER_FLAGS += -framework OpenGL -framework Cocoa
        LINKER_FLAGS += -framework IOKit -framework CoreVideo
    else ifeq ($(OS),Emscripten)
        ifdef EXAEQUOS
            # Wayland, OpenGL, GLEW, GLFW3 for ExaequOS
            LINKER_FLAGS += -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sFULL_ES3
            PKG_LIBS += exa-wayland --static glfw
        else
            # We tell the linker that the game/library uses GLFW3
            # library internally, it must be linked automatically
            # (emscripten provides the implementation)
            LINKER_FLAGS += -s USE_GLFW=3
        endif
    else
        $(error Unknown architecture $(OS) for OpenGL)
    endif
    USER_CXXFLAGS := -Wno-old-style-cast -Wno-sign-conversion -Wno-float-equal
    USER_CXXFLAGS += -Wno-cast-qual -Wno-conversion
endif

###################################################
# Dear ImGui backends: Raylib
#
ifeq ($(DEAR_IMGUI_BACKEND),RayLib)
    VPATH += $(P)/src/Editor/Backends/RayLib
    INCLUDES += $(P)/src/Editor/Backends/RayLib
    GUI_FILES += $(P)/src/Editor/Backends/RayLib/Application.cpp
    VPATH += $(THIRD_PARTIES_DIR)/rlImGui
    INCLUDES += $(THIRD_PARTIES_DIR)/rlImGui
    GUI_FILES += $(THIRD_PARTIES_DIR)/rlImGui/rlImGui.cpp
    INCLUDES += $(THIRD_PARTIES_DIR)/raylib/src
    GUI_THIRDPART_LIBS += $(THIRD_PARTIES_DIR)/raylib/src/$(OS)/libraylib.a
    ifeq ($(OS),Emscripten)
        ifdef EXAEQUOS
            LINKER_FLAGS += -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sFULL_ES3
            PKG_LIBS += exa-wayland --static raylib
        else
            # We tell the linker that the game/library uses GLFW3
            # library internally, it must be linked automatically
            # (emscripten provides the implementation)
            LINKER_FLAGS += -s USE_GLFW=3
            # All webs need a "shell" structure to load and run the game,
            # by default emscripten has a `shell.html` but we can provide
            # our own.
            LINKER_FLAGS += --shell-file $(THIRD_PARTIES_DIR)/raylib/src/minshell.html
        endif
    endif

    USER_CXXFLAGS := -Wno-conversion -Wno-sign-conversion -Wno-float-equal -Wno-old-style-cast
    USER_CXXFLAGS += -Wno-cast-qual -Wno-undef -Wno-unused-variable -Wno-missing-field-initializers
    USER_CXXFLAGS += -Wno-sign-compare
endif

###################################################
# Dear ImGui
#
INCLUDES += $(THIRD_PARTIES_DIR)
INCLUDES += $(THIRD_PARTIES_DIR)/imgui
GUI_FILES += $(THIRD_PARTIES_DIR)/imgui/imgui_widgets.cpp
GUI_FILES += $(THIRD_PARTIES_DIR)/imgui/imgui_draw.cpp
GUI_FILES += $(THIRD_PARTIES_DIR)/imgui/imgui_tables.cpp
GUI_FILES += $(THIRD_PARTIES_DIR)/imgui/imgui.cpp
GUI_FILES += $(THIRD_PARTIES_DIR)/imgui/misc/cpp/imgui_stdlib.cpp
ifneq ($(OS),Emscripten)
USER_CXXFLAGS += -Wno-useless-cast
endif
USER_CXXFLAGS += -DIMGUI_DEFINE_MATH_OPERATORS
# Third-party Dear ImGui extras (implot, file dialog): not built with -ffast-math-safe code.
USER_CXXFLAGS += -Wno-nan-infinity-disabled -Wno-format-nonliteral

###################################################
# Dear ImGui plot
#
GUI_FILES += $(THIRD_PARTIES_DIR)/implot/implot_items.cpp
GUI_FILES += $(THIRD_PARTIES_DIR)/implot/implot.cpp
USER_CXXFLAGS += -Wno-deprecated-enum-enum-conversion

###################################################
# Dear ImGui file dialog
#
GUI_FILES += $(THIRD_PARTIES_DIR)/ImGuiFileDialog/ImGuiFileDialog.cpp
USER_CXXFLAGS += -Wno-unknown-pragmas -Wno-double-promotion
ifeq ($(OS),Emscripten)
USER_CXXFLAGS += -Wno-unknown-warning-option
endif

###################################################
# Petri Editor
#
INCLUDES += $(P)/src/Editor/Remote
INCLUDES += $(P)/src
GUI_FILES += $(wildcard $(P)/src/Editor/*.cpp)
GUI_FILES += $(GUI_REMOTE_FILES)