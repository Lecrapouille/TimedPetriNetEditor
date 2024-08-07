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

#ifndef APPLICATION_HPP
#  define APPLICATION_HPP

#include "imgui/imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui/misc/cpp/imgui_stdlib.h"
#include "implot/implot.h"
#include "ImGuiFileDialog/ImGuiFileDialog.h"
#include <GLFW/glfw3.h>

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
};

#endif
