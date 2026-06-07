//=============================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2026 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of TimedPetriNetEditor.
//
// TimedPetriNetEditor is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GNU Emacs.  If not, see <http://www.gnu.org/licenses/>.
//=============================================================================

#include "Editor/Backends/RayLib/Application.hpp"

#include <cstdlib>
#include <cstdio>
#include <functional>
#include <chrono>
#include <fstream>
#include <iostream>

class Timer
{
public:
    Timer() : m_begin(Clock::now()) {}

    float restart()
    {
        float res = elapsed();
        m_begin = Clock::now();
        return res;
    }

    float elapsed() const
    {
        return std::chrono::duration_cast<Second>(Clock::now() - m_begin).count();
    }

private:
    using Clock = std::chrono::steady_clock;
    using Second = std::chrono::duration<float, std::ratio<1>>;
    std::chrono::time_point<Clock> m_begin;
};

//------------------------------------------------------------------------------
void reloadFonts()
{
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels = nullptr;

    int width;
    int height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, nullptr);
    Image image = GenImageColor(width, height, BLANK);
    memcpy(image.data, pixels, width * height * 4);

#if IMGUI_VERSION_NUM >= 19200
    Texture2D* fontTexture =
        (Texture2D*)(intptr_t)io.Fonts->TexRef.GetTexID();
#else
    Texture2D* fontTexture = (Texture2D*)io.Fonts->TexID;
#endif
    if (fontTexture && fontTexture->id != 0)
    {
        UnloadTexture(*fontTexture);
        MemFree(fontTexture);
    }

    fontTexture = (Texture2D*)MemAlloc(sizeof(Texture2D));
    *fontTexture = LoadTextureFromImage(image);
    UnloadImage(image);
#if IMGUI_VERSION_NUM >= 19200
    io.Fonts->TexRef = ImTextureRef((ImTextureID)(intptr_t)fontTexture);
#else
    io.Fonts->TexID = (ImTextureID)fontTexture;
#endif
}

//------------------------------------------------------------------------------
Application::Application(size_t const width, size_t const height, std::string const &title)
{
    m_screen_resolution.x = 0.0f;
    m_screen_resolution.y = 0.0f;
    m_screen_resolution.width = int(width);
    m_screen_resolution.height = int(height);

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI);
    InitWindow(m_screen_resolution.width, m_screen_resolution.height, title.c_str());

    Vector2 windowPosition = {500, 200};
    SetWindowPosition(windowPosition.x, windowPosition.y);
    SetTargetFPS(m_framerate);

    // Setup Dear ImGui context
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();
    reloadFonts();

    // Setup Platform/Renderer backends
    ImGui_ImplRaylib_Init();
}

//------------------------------------------------------------------------------
Application::~Application()
{
    ImGui_ImplRaylib_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    CloseWindow(); // Stop raylib
}

//------------------------------------------------------------------------------
void Application::run()
{
    // Iterate update()
    Timer timer;
    float timeSinceLastUpdate = 0.0f;
    const float time_per_frame = 1.0f / float(m_framerate);

    while (!m_exit_window)
    {
        ImGui_ImplRaylib_ProcessEvents();

        // Start the Dear ImGui frame
        ImGui_ImplRaylib_NewFrame();
        ImGui::NewFrame();

        // Process events at fixed time steps
        timeSinceLastUpdate += timer.restart();
        while (timeSinceLastUpdate > time_per_frame)
        {
            timeSinceLastUpdate -= time_per_frame;
            onUpdate(time_per_frame);
        }

        // Main loop of the underlying app
        onDraw();

        // Rendering
        ImGui::Render();
        BeginDrawing();         // Start raylib content
        ClearBackground(WHITE); // gui->background_color);

        ImGui_ImplRaylib_RenderDrawData(ImGui::GetDrawData());
        onFrameEnd();
        EndDrawing(); // Stop raylib content
    }
}

//------------------------------------------------------------------------------
void Application::framerate(size_t const framerate)
{
    m_framerate = framerate;
    SetTargetFPS(m_framerate);
}

//------------------------------------------------------------------------------
// Siiiiiight! Poor RayLib API. Only manage file name not file path :(
// TakeScreenshot has been hot patched to use with absolute path.
bool Application::screenshot(std::string const& path, int const region_x,
                             int const region_y, int const region_w,
                             int const region_h)
{
    if (path.empty())
        return false;

    if (region_w <= 0 || region_h <= 0)
    {
        TakeScreenshot(path.c_str());
        return FileExists(path.c_str());
    }

    std::string const temp = path + ".tmp.full.png";
    TakeScreenshot(temp.c_str());
    if (!FileExists(temp.c_str()))
        return false;

    Image img = LoadImage(temp.c_str());
    RemoveFile(temp.c_str());
    if (img.data == nullptr)
        return false;

    Image const crop = ImageFromImage(
        img, (Rectangle){ float(region_x), float(region_y),
                          float(region_w), float(region_h) });
    UnloadImage(img);
    if (crop.data == nullptr)
        return false;

    bool const ok = ExportImage(crop, path.c_str());
    UnloadImage(crop);
    return ok && FileExists(path.c_str());
}

//------------------------------------------------------------------------------
void Application::title(std::string const& title_)
{
    SetWindowTitle(title_.c_str());
}

//------------------------------------------------------------------------------
bool Application::windowShouldClose()
{
    return WindowShouldClose();
}
