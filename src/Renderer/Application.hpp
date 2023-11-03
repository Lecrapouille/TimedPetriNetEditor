#ifndef APPLICATION_HPP
#  define APPLICATION_HPP

#include "raylib.h"
#include "imgui_impl_raylib.h"
#include "imgui/misc/cpp/imgui_stdlib.h"
#include "imgui.h"
#include "implot.h"
#include "ImGuiFileDialog.h"

#include <string>

class Application
{
public:

    Application(size_t const width, size_t const height, std::string const& title);
    virtual ~Application();
    void run();

private:

    virtual void onStartUp() = 0;
    virtual void onDraw() = 0;

private:

    Rectangle m_screen_resolution;
    Vector2 m_mouse_position = { 0, 0 };
    bool m_exit_window = false;
    size_t m_framerate = 60u;
};

#endif
