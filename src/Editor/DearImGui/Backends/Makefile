###################################################
# Set MQTT Library.
#
ifneq ($(OS),Emscripten)
    INCLUDES += $(THIRDPART_DIR)/MQTT/include
    DEFINES += -DMQTT_BROKER_ADDR=\"localhost\"
    DEFINES += -DMQTT_BROKER_PORT=1883
    DEFINES += -DWITH_MQTT
    PKG_LIBS += libmosquitto
    GUI_FILES += $(THIRDPART_DIR)/MQTT/src/MQTT.cpp
else
    DEFINES += -UWITH_MQTT
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
    INCLUDES += $(THIRDPART_DIR)/imgui/backends/
    GUI_FILES += $(THIRDPART_DIR)/imgui/backends/imgui_impl_glfw.cpp
    GUI_FILES += $(THIRDPART_DIR)/imgui/backends/imgui_impl_opengl3.cpp
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
    VPATH += $(THIRDPART_DIR)/rlImGui
    INCLUDES += $(THIRDPART_DIR)/rlImGui
    GUI_FILES += $(THIRDPART_DIR)/rlImGui/rlImGui.cpp
    INCLUDES += $(THIRDPART_DIR)/raylib/src
    THIRDPART_LIBS += $(THIRDPART_DIR)/raylib/src/$(OS)/libraylib.a
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
            LINKER_FLAGS += --shell-file $(THIRDPART_DIR)/raylib/src/minshell.html
        endif
    endif

    USER_CXXFLAGS := -Wno-conversion -Wno-sign-conversion -Wno-float-equal -Wno-old-style-cast
    USER_CXXFLAGS += -Wno-cast-qual -Wno-undef
endif

# Dear ImGui
INCLUDES += $(THIRDPART_DIR)
INCLUDES += $(THIRDPART_DIR)/imgui
GUI_FILES += $(THIRDPART_DIR)/imgui/imgui_widgets.cpp
GUI_FILES += $(THIRDPART_DIR)/imgui/imgui_draw.cpp
GUI_FILES += $(THIRDPART_DIR)/imgui/imgui_tables.cpp
GUI_FILES += $(THIRDPART_DIR)/imgui/imgui.cpp
GUI_FILES += $(THIRDPART_DIR)/imgui/misc/cpp/imgui_stdlib.cpp

# Dear ImGui plot
GUI_FILES += $(THIRDPART_DIR)/implot/implot_items.cpp
GUI_FILES += $(THIRDPART_DIR)/implot/implot.cpp

# Dear ImGui file dialog
GUI_FILES += $(THIRDPART_DIR)/ImGuiFileDialog/ImGuiFileDialog.cpp
USER_CXXFLAGS += -Wno-unknown-pragmas

# Petri Editor
GUI_FILES += $(call wildcard,$(P)/src/Editor/DearImGui/*.cpp)