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

#include "Editor/DearImGui/Backends/GLFW3/Application.hpp"

#include <cstdlib>
#include <functional>
#include <iostream>
#include <stdio.h>

//------------------------------------------------------------------------------
void reloadFonts()
{
    std::cerr << "reloadFonts: Not implemented yet" << std::endl;
}

//------------------------------------------------------------------------------
static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

//------------------------------------------------------------------------------
Application::Application(size_t const width, size_t const height, std::string const& title)
    : m_clear_color(ImVec4(0.1058, 0.1137f, 0.1255f, 1.00f))
{
    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
        std::exit(1);

#ifdef EXAEQUOS
    const char* glsl_version = "#version 300 es";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#else
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required on Mac: 3.2+ only
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

    // Create window with graphics context
    m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (m_window == NULL)
        std::exit(1);

    glfwSetWindowSize(m_window, width, height);
    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);

    // Add window based callbacks to the underlying app
    //glfwSetMouseButtonCallback(m_window, &Derived::MouseButtonCallback);
    //glfwSetCursorPosCallback(m_window, &Derived::CursorPosCallback);
    //glfwSetKeyCallback(m_window, &Derived::KeyCallback);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Enable controls
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;       // Enable Docking
}

//------------------------------------------------------------------------------
Application::~Application()
{
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(m_window);
    glfwTerminate();
}

//------------------------------------------------------------------------------
void Application::run()
{
    m_lastUpdateTime = 0.0;
    m_lastFrameTime = 0.0;
    const double time_per_frame = 1.0 / double(m_framerate);

    while (!m_exit_window)//!glfwWindowShouldClose(m_window))
    {
        // For the display framerate
        double now = glfwGetTime();
        //double deltaTime = now - m_lastUpdateTime;

        // Poll events like key presses, mouse movements etc.
        glfwPollEvents();

        if ((now - m_lastFrameTime) >= time_per_frame)
        {
            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            onUpdate(time_per_frame);

            // Main loop of the underlying app
            onDraw();

            // Rendering
            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(m_window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(m_clear_color.x * m_clear_color.w,
                        m_clear_color.y * m_clear_color.w,
                        m_clear_color.z * m_clear_color.w,
                        m_clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(m_window);
            m_lastFrameTime = now;
        }

        m_lastUpdateTime = now;
    }
}

//------------------------------------------------------------------------------
void Application::framerate(size_t const framerate)
{
    m_framerate = framerate;
    m_lastUpdateTime = 0.0;
    m_lastFrameTime = 0.0;
}

//------------------------------------------------------------------------------
bool Application::screenshot(std::string const& path)
{
    (void) path;
    std::cerr << "Application::screenshot: Not implemented yet" << std::endl;
    return false;
}

//------------------------------------------------------------------------------
void Application::title(std::string const& title_)
{
    glfwSetWindowTitle(m_window, title_.c_str());
}

//------------------------------------------------------------------------------
bool Application::windowShouldClose()
{
    return glfwWindowShouldClose(m_window);
}
