// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef APPLICATION_HPP
#  define APPLICATION_HPP

#include "raylib.h"
#include "imgui_impl_raylib.h"
#include "imgui/misc/cpp/imgui_stdlib.h"
#include "imgui/imgui.h"
#include "implot/implot.h"
#include "ImGuiFileDialog/ImGuiFileDialog.h"
#include "rlImGui/rlImGui.h"

#include <string>
#include <functional>
#include <chrono>

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
    //! \brief Called once per frame after GPU rendering.
    //--------------------------------------------------------------------------
    virtual void onFrameEnd() {}

    //--------------------------------------------------------------------------
    //! \brief Return true while something moves without user input. When
    //! false, the main loop waits for the next event instead of drawing.
    //--------------------------------------------------------------------------
    virtual bool isAnimating() const { return true; }

    //--------------------------------------------------------------------------
    //! \brief Draw the next frames even if nothing animates.
    //--------------------------------------------------------------------------
    void wakeUp() { m_idle_frames = 0u; }

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    bool windowShouldClose(); // FIXME: doublon with halt()

private:

    virtual void onDraw() = 0;
    virtual void onUpdate(float const dt) = 0;

private:

    Rectangle m_screen_resolution;
    bool m_exit_window = false;
    size_t m_framerate = 60u;
    //! \brief Frames drawn since the last event while nothing animates.
    size_t m_idle_frames = 0u;
    //! \brief Last title given to the window.
    std::string m_title;
};

#endif
