###################################################
# Set ZeroMQ Library for remote control.
#
ifneq ($(OS),Emscripten)
    PKG_LIBS += libzmq
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
    INCLUDES += $(P)/src/Editor/DearImGui/Backends/GLFW3
    GUI_FILES += $(P)/src/Editor/DearImGui/Backends/GLFW3/Application.cpp
    INCLUDES += $(THIRD_PARTIES_DIR)/imgui/backends/
    GUI_FILES += $(THIRD_PARTIES_DIR)/imgui/backends/imgui_impl_glfw.cpp
    GUI_FILES += $(THIRD_PARTIES_DIR)/imgui/backends/imgui_impl_opengl3.cpp
    ifeq ($(OS),Linux)
        # OpenGL, GLEW, GLFW3
        LINKER_FLAGS += -lGL
        PKG_LIBS += --static glfw3
    else ifeq ($(OS),Darwin)
        # OpenGL, GLEW, GLFW3
        INCLUDES += -I/usr/local/include -I/opt/local/include
        LINKER_FLAGS += -framework OpenGL -framework Cocoa
        LINKER_FLAGS += -framework IOKit -framework CoreVideo
        LINKER_FLAGS += -L/usr/local/lib -L/opt/local/lib
        LINKER_FLAGS += -lGLEW -lglfw
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
    VPATH += $(P)/src/Editor/DearImGui
    VPATH += $(P)/src/Editor/DearImGui/Backends/RayLib
    INCLUDES += $(P)/src/Editor/DearImGui/Backends/RayLib
    GUI_FILES += $(P)/src/Editor/DearImGui/Backends/RayLib/Application.cpp
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
    USER_CXXFLAGS += -Wno-cast-qual -Wno-undef
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
USER_CXXFLAGS += -Wno-useless-cast

###################################################
# Dear ImGui plot
#
GUI_FILES += $(THIRD_PARTIES_DIR)/implot/implot_items.cpp
GUI_FILES += $(THIRD_PARTIES_DIR)/implot/implot.cpp

###################################################
# Dear ImGui file dialog
#
GUI_FILES += $(THIRD_PARTIES_DIR)/ImGuiFileDialog/ImGuiFileDialog.cpp
USER_CXXFLAGS += -Wno-unknown-pragmas -Wno-double-promotion

###################################################
# Petri Editor
#
INCLUDES += $(P)/src/Editor/DearImGui
INCLUDES += $(P)/src/Editor/DearImGui/Remote
INCLUDES += $(P)/src
GUI_FILES += $(call wildcard,$(P)/src/Editor/DearImGui/*.cpp)
GUI_FILES += $(call wildcard,$(P)/src/Editor/DearImGui/Remote/*.cpp)