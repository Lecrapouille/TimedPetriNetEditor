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
    //! \brief Take a screenshot of the game and save it as PNG to the given path.
    //--------------------------------------------------------------------------
    bool screenshot(std::string const& screenshot_path);

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
};

#endif
