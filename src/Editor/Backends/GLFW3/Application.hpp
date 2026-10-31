// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef APPLICATION_HPP
#  define APPLICATION_HPP

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_stdlib.h"
#include "implot.h"
#include "ImGuiFileDialog.h"
#include <GLFW/glfw3.h>
#include <string>

//------------------------------------------------------------------------------
void reloadFonts();

// *****************************************************************************
//! \brief
// *****************************************************************************
class Application
{
public:

    Application(size_t const width, size_t const height, std::string const& title);
    virtual ~Application();

    //--------------------------------------------------------------------------
    //! \brief Start a blocking loop for managing its draw and IO events.
    //--------------------------------------------------------------------------
    void run();

    //--------------------------------------------------------------------------
    //! \brief Limit the framerate to a maximum fixed frequency.
    //--------------------------------------------------------------------------
    void framerate(size_t const framerate);

    //--------------------------------------------------------------------------
    //! \brief Take a screenshot and save it as PNG to the given path.
    //! \param[in] x Top-left X in framebuffer pixels (0 = full window).
    //! \param[in] y Top-left Y in framebuffer pixels (0 = full window).
    //! \param[in] width Region width; 0 captures the full window.
    //! \param[in] height Region height; 0 captures the full window.
    //--------------------------------------------------------------------------
    bool screenshot(std::string const& screenshot_path, int x = 0, int y = 0,
                    int width = 0, int height = 0);

    //--------------------------------------------------------------------------
    //! \brief Set title.
    //--------------------------------------------------------------------------
    void title(std::string const& title_);

    //--------------------------------------------------------------------------
    //! \brief Request halting.
    //--------------------------------------------------------------------------
    void halt() { m_exit_window = true; }

    //--------------------------------------------------------------------------
    //! \brief Called once per frame after GPU rendering (before buffer swap).
    //--------------------------------------------------------------------------
    virtual void onFrameEnd() {}

    //--------------------------------------------------------------------------
    //! \brief Return true while something moves without user input (running
    //! simulation, graph layout, remote control ...). When false, the main
    //! loop sleeps until the next keyboard or mouse event instead of drawing
    //! frames that are all the same.
    //--------------------------------------------------------------------------
    virtual bool isAnimating() const { return true; }

    //--------------------------------------------------------------------------
    //! \brief Wake up the main loop (thread safe). Call it when the state
    //! changes outside an input event, for example from a network thread.
    //--------------------------------------------------------------------------
    void wakeUp();

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    bool windowShouldClose(); // FIXME: doublon with halt()

private:

    virtual void onDraw() = 0;
    virtual void onUpdate(float const dt) = 0;

private:

    GLFWwindow* m_window = nullptr;
    ImVec4 m_clear_color;
    bool m_exit_window = false;
    size_t m_framerate = 60u;
    double m_lastUpdateTime = 0.0;  // number of seconds since the last loop
    double m_lastFrameTime = 0.0;   // number of seconds since the last frame
    //! \brief Frames drawn since the last event while nothing animates. Dear
    //! ImGui needs a few frames to settle after an input (hover, popups).
    size_t m_idle_frames = 0u;
    //! \brief Last title given to the window, to call the OS only on change.
    std::string m_title;
};

#endif
