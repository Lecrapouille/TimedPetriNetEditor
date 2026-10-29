// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Editor/Backends/GLFW3/Application.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <stdio.h>
#include <vector>

#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#  include <emscripten/html5.h>
#endif

// A third-party header: its warnings are not ours.
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations" // sprintf on macOS
#include "stb_image_write.h"
#pragma GCC diagnostic pop

//------------------------------------------------------------------------------
void reloadFonts()
{
    // Nothing to do: since Dear ImGui 1.92 the backend uploads the atlas of
    // the fonts each time it changes (ImGuiBackendFlags_RendererHasTextures).
}

//------------------------------------------------------------------------------
static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

#if defined(__EMSCRIPTEN__) && !defined(EXAEQUOS)
//------------------------------------------------------------------------------
//! \brief The window fills the page. Emscripten writes the size of the window
//! in the style of the canvas, so the canvas cannot follow the page by CSS.
static EM_BOOL resize_callback(int /*event_type*/, EmscriptenUiEvent const* event, void* window)
{
    glfwSetWindowSize(static_cast<GLFWwindow*>(window), event->windowInnerWidth,
                      event->windowInnerHeight);
    return EM_TRUE;
}
#endif

//------------------------------------------------------------------------------
Application::Application(size_t const width, size_t const height, std::string const& title)
    : m_clear_color(ImVec4(0.1058, 0.1137f, 0.1255f, 1.00f))
{
    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
        std::exit(1);

#ifdef __EMSCRIPTEN__
    // WebGL 2 (OpenGL ES 3), for the web version and ExaequOS.
    const char* glsl_version = "#version 300 es";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#  if defined(GLFW_SCALE_TO_MONITOR) && !defined(EXAEQUOS)
    // Draw at the density of the screen (devicePixelRatio), not blurred.
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
#  endif
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
    // The main loop paces the frames itself (Application::run). On Wayland a
    // swap with vsync waits for the compositor, which does not answer while
    // the window is hidden or being mapped: the application would freeze.
    int interval = 1;
#if (GLFW_VERSION_MAJOR > 3) || ((GLFW_VERSION_MAJOR == 3) && (GLFW_VERSION_MINOR >= 4))
    if (glfwGetPlatform() == GLFW_PLATFORM_WAYLAND)
        interval = 0;
#endif
    glfwSwapInterval(interval);

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
#if defined(__EMSCRIPTEN__) && !defined(EXAEQUOS)
    // Dear ImGui fixes the mouse wheel; the window follows the page.
    ImGui_ImplGlfw_InstallEmscriptenCallbacks(m_window, "#canvas");
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, m_window, false, resize_callback);
    int const page_width = EM_ASM_INT({ return window.innerWidth; });
    int const page_height = EM_ASM_INT({ return window.innerHeight; });
    glfwSetWindowSize(m_window, page_width, page_height);
#endif

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

    glfwMakeContextCurrent(nullptr);  // Release GL context before destroying window
    glfwDestroyWindow(m_window);
    glfwPollEvents();  // Workaround: flush pending Wayland events to avoid segfault
                       // (see https://github.com/glfw/glfw/issues/2744)
    glfwTerminate();
}

//------------------------------------------------------------------------------
void Application::run()
{
    // Frames drawn after the last event before sleeping: Dear ImGui needs a
    // couple of them to settle hover states, popups and window resizes.
    constexpr size_t SETTLE_FRAMES = 3u;
    // When idle, wake up anyway at this period (seconds) for tooltips delays
    // and blinking cursors.
    constexpr double IDLE_WAKEUP = 0.5;
    // Longest step of time given to the application (seconds): a frame
    // after a sleep, or a window moved, makes no jump in the simulation.
    constexpr double MAX_STEP = 0.1;

    m_lastUpdateTime = 0.0;
    m_lastFrameTime = 0.0;
    m_idle_frames = 0u;

    while (!m_exit_window)
    {
        const double time_per_frame = 1.0 / double(m_framerate);
        const bool animating = isAnimating();

#ifdef __EMSCRIPTEN__
        // The browser paces the frames.
        glfwPollEvents();
#else
        if (!animating && (m_idle_frames >= SETTLE_FRAMES))
        {
            // Nothing moves: sleep until an event (or the wake up period)
            // instead of spinning. This was the cause of a high CPU usage
            // when the net was not simulated (mainly visible on macOS where
            // the buffer swap does not always block on vsync).
            glfwWaitEventsTimeout(IDLE_WAKEUP);
            m_idle_frames = 0u;
        }
        else
        {
            // Sleep until the next frame, but wake up on events.
            const double remaining = time_per_frame - (glfwGetTime() - m_lastFrameTime);
            if (remaining > 0.0)
            {
                glfwWaitEventsTimeout(remaining);
                if ((glfwGetTime() - m_lastFrameTime) < time_per_frame)
                    continue;
            }
            else
            {
                glfwPollEvents();
            }
        }
#endif

        const double now = glfwGetTime();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // The time really gone since the previous frame, not the period of
        // the frames: a slower frame (vsync of a screen under 60 Hz, a busy
        // compositor, a big net) shall not slow the simulation down.
        const double elapsed = (m_lastUpdateTime > 0.0)
                             ? std::clamp(now - m_lastUpdateTime, 0.0, MAX_STEP)
                             : time_per_frame;
        onUpdate(float(elapsed));

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

        onFrameEnd();

        glfwSwapBuffers(m_window);
        m_lastFrameTime = now;
        m_lastUpdateTime = now;
        m_idle_frames = animating ? 0u : m_idle_frames + 1u;

#ifdef __EMSCRIPTEN__
        // Give the hand back to the browser until the next frame (needs
        // -s ASYNCIFY): it draws the canvas and gives the events.
        double const left = time_per_frame - (glfwGetTime() - now);
        emscripten_sleep((left > 0.0) ? unsigned(left * 1000.0) : 0u);
#endif
    }
}

void Application::wakeUp()
{
    m_idle_frames = 0u;
    glfwPostEmptyEvent();
}

void Application::framerate(size_t const framerate)
{
    m_framerate = framerate;
    m_lastUpdateTime = 0.0;
    m_lastFrameTime = 0.0;
}

//------------------------------------------------------------------------------
bool Application::screenshot(std::string const& path, int const region_x,
                             int const region_y, int const region_w,
                             int const region_h)
{
    if (path.empty() || m_window == nullptr)
        return false;

    int fb_width = 0;
    int fb_height = 0;
    glfwGetFramebufferSize(m_window, &fb_width, &fb_height);
    if (fb_width <= 0 || fb_height <= 0)
        return false;

    int const width = (region_w > 0) ? region_w : fb_width;
    int const height = (region_h > 0) ? region_h : fb_height;
    int const x = (region_w > 0) ? region_x : 0;
    int const y_top = (region_h > 0) ? region_y : 0;
    int const y = (region_h > 0) ? (fb_height - y_top - height) : 0;

    if (x < 0 || y < 0 || x + width > fb_width || y + height > fb_height)
        return false;

    std::vector<unsigned char> pixels(size_t(width) * size_t(height) * 4u);
    glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    std::vector<unsigned char> flipped(pixels.size());
    size_t const row_bytes = size_t(width) * 4u;
    for (int row = 0; row < height; ++row)
    {
        std::memcpy(flipped.data() + size_t(row) * row_bytes,
                    pixels.data() + size_t(height - 1 - row) * row_bytes,
                    row_bytes);
    }

    return stbi_write_png(path.c_str(), width, height, 4, flipped.data(),
                          int(row_bytes)) != 0;
}

//------------------------------------------------------------------------------
void Application::title(std::string const& title_)
{
    // Setting the title is an OS call (costly on macOS): only on change.
    if (title_ == m_title)
        return;
    m_title = title_;
    glfwSetWindowTitle(m_window, m_title.c_str());
}

//------------------------------------------------------------------------------
bool Application::windowShouldClose()
{
    return glfwWindowShouldClose(m_window);
}
