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

#include "Application/DearImGui/Backends/GLFW3/Application.hpp"

#include <cstdlib>
#include <functional>
#include <iostream>
#include <stdio.h>

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

Application::Application(size_t const width, size_t const height, std::string const& title)
    : m_clear_color(ImVec4(0.1058, 0.1137f, 0.1255f, 1.00f))
{
    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
        std::exit(1);

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

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

void Application::run()
{
    // Initialize the underlying app
    onStartUp();

    while (!glfwWindowShouldClose(m_window))
    {
        // Poll events like key presses, mouse movements etc.
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

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
    }
}
