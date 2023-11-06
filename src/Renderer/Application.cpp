#include "Application.hpp"

#include <cstdlib>
#include <functional>
#include <iostream>
#include <stdio.h>

Application::Application(size_t const width, size_t const height, std::string const& title)
{
    m_screen_resolution.x = 0.0f;
    m_screen_resolution.y = 0.0f;
    m_screen_resolution.width = int(width);
    m_screen_resolution.height = int(height);

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI);
    InitWindow(m_screen_resolution.width, m_screen_resolution.height, title.c_str());

    Vector2 windowPosition = { 500, 200 };
    SetWindowPosition(windowPosition.x, windowPosition.y);
    SetTargetFPS(60);

    // Setup Dear ImGui context
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;    // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;       // Enable Docking

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplRaylib_Init();
}

Application::~Application()
{
    ImGui_ImplRaylib_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    CloseWindow(); // Stop raylib
}

void Application::run()
{
    // Initialize the underlying app
    onStartUp();

    while (!m_exit_window && !WindowShouldClose())
    {
        ImGui_ImplRaylib_ProcessEvents();

        // Start the Dear ImGui frame
        ImGui_ImplRaylib_NewFrame();
        ImGui::NewFrame();

        // Main loop of the underlying app
        onDraw();

        // Rendering
        ImGui::Render();
        BeginDrawing(); // Start raylib content
        ClearBackground(WHITE);//gui->background_color);


        ImGui_ImplRaylib_RenderDrawData(ImGui::GetDrawData());
        EndDrawing(); // Stop raylib content
    }
}
