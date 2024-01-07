//=============================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2023 Quentin Quadrat <lecrapouille@gmail.com>
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

#include "Application/DearImGui/Backends/RayLib/Application.hpp"
#include "Utils/Utils.hpp"

#include <cstdlib>
#include <cstdio>
#include <functional>
#include <chrono>
// #include <stdio.h>
#include <fstream>
#include <iostream>

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

	Texture2D* fontTexture = (Texture2D*)io.Fonts->TexID;
	if (fontTexture && fontTexture->id != 0)
	{
		UnloadTexture(*fontTexture);
		MemFree(fontTexture);
	}

	fontTexture = (Texture2D*)MemAlloc(sizeof(Texture2D));
	*fontTexture = LoadTextureFromImage(image);
	UnloadImage(image);
	io.Fonts->TexID = fontTexture;
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

static uint64_t timeSinceEpochMillisec()
{
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

//------------------------------------------------------------------------------
void Application::run()
{
    tpne::Timer timer;
    float timeSinceLastUpdate = 0.0f;

    const float time_per_frame = 1.0f / m_framerate;
    while (!m_exit_window)
    {
        //uint64_t b = timeSinceEpochMillisec();
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
        EndDrawing(); // Stop raylib content
        //uint64_t e = timeSinceEpochMillisec();
        //std::cout << (e - b) << " " << m_framerate << std::endl;
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
bool Application::screenshot(std::string const& path)
{
    TakeScreenshot(path.c_str());
    return FileExists(path.c_str());
}

//------------------------------------------------------------------------------
void Application::title(std::string const& title_)
{
    SetWindowTitle(title_.c_str());
}