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

#include "raylib.h"
#include "imgui_impl_raylib.h"
#include "imgui/misc/cpp/imgui_stdlib.h"
#include "imgui.h"
#include "implot.h"
#include "ImGuiFileDialog.h"

#include "Editor/Path.hpp" // FIXME

#include <string>

// *****************************************************************************
//! \brief
// *****************************************************************************
class Application
{
public:

    Application(size_t const width, size_t const height, std::string const& title);
    virtual ~Application();
    void run();
    void setFramerate(size_t const framerate);

private:

    virtual void onStartUp() = 0;
    virtual void onDraw() = 0;

protected:

    Path m_path;

private:

    Rectangle m_screen_resolution;
    Vector2 m_mouse_position = { 0, 0 };
    bool m_exit_window = false;
    size_t m_framerate = 60u;
};

#endif
