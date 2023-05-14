//=============================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2022 Quentin Quadrat <lecrapouille@gmail.com>
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

#ifndef DEARIMGUI_HPP
#  define DEARIMGUI_HPP

// ****************************************************************************
//! \file GLImGUI.hpp wraps function calls of the ImGUI project.
// ****************************************************************************

#  include "imgui/imgui.h"
#  include "imgui/misc/cpp/imgui_stdlib.h"
#  include "imgui-sfml/imgui-SFML.h"

// *****************************************************************************
//! \brief Class wrapper for the dear imgui library: an imediate
//! mode (im) graphical user interface (GUI) for OpenGL.
//! https://github.com/ocornut/imgui
// *****************************************************************************
class DearImGui
{
public:

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    enum class Theme { Classic, Dark };

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    DearImGui(sf::RenderWindow& renderer, DearImGui::Theme const theme);

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    ~DearImGui();

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    void setTheme(Theme const style);

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    void update(sf::Time const dt);

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    void display();

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    void begin();

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    void end();

private:

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    void configurate();

private:

    sf::RenderWindow& m_renderer;
    bool m_opt_dockspace = true;
    bool m_opt_padding = false;
    bool m_opt_fullscreen = true;
    ImGuiWindowFlags m_window_flags;
    ImGuiDockNodeFlags m_dockspace_flags;
};

#endif // OPENGLCPPWRAPPER_UI_DEARIMGUI_HPP
