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

#ifndef DEAR_IMGUI_UTILS_HPP
#  define DEAR_IMGUI_UTILS_HPP

#  include <string>
#  include "imgui/imgui.h"

//------------------------------------------------------------------------------
static inline ImVec2& operator+=(ImVec2& lhs, const ImVec2& rhs)
{
    lhs.x += rhs.x;
    lhs.y += rhs.y;
    return lhs;
}

//------------------------------------------------------------------------------
static inline ImVec2 operator+(ImVec2 const& lhs, ImVec2 const& rhs)
{
    return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y);
}

//------------------------------------------------------------------------------
static inline ImVec2& operator-=(ImVec2& lhs, const ImVec2& rhs)
{
    lhs.x -= rhs.x;
    lhs.y -= rhs.y;
    return lhs;
}

//------------------------------------------------------------------------------
static inline ImVec2 operator-(ImVec2 const& lhs, ImVec2 const& rhs)
{
    return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y);
}

//------------------------------------------------------------------------------
static inline ImVec2 operator*(ImVec2 const& lhs, float const& rhs)
{
    return ImVec2(lhs.x * rhs, lhs.y * rhs);
}

//------------------------------------------------------------------------------
static inline ImVec2 operator/(ImVec2 const& lhs, float const& rhs)
{
    return ImVec2(lhs.x / rhs, lhs.y / rhs);
}

//------------------------------------------------------------------------------
void inputInteger(std::string const& title, size_t const maxTokens, size_t& tokens);

#endif