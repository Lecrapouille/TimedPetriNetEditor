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

#ifndef PETRI_DRAWABLE_HPP
#  define PETRI_DRAWABLE_HPP

#  include "Editor/DearImGui/Theme.hpp"
#  include "TimedPetriNetEditor/PetriNet.hpp"
#  include "imgui.h"

namespace tpne {

void drawArc(ImDrawList* draw_list, Node* from, Node* to, ImVec2* click_position, ImVec2 const& origin, ImVec2 const& cursor);
void drawArc(ImDrawList* draw_list, Arc const& arc, TypeOfNet const type, ImVec2 const& origin, float const alpha);
void drawToken(ImDrawList* draw_list, float const x, float const y);
void drawTimedToken(ImDrawList* draw_list, size_t tokens, float const x, float const y);
void drawPlace(ImDrawList* draw_list, Place const& place, TypeOfNet const type, ImVec2 const& origin, bool const show_caption, float const alpha);
void drawTransition(ImDrawList* draw_list, Transition const& transition, TypeOfNet const type, ImVec2 const& origin, bool const show_caption, float const alpha);

} // namespace tpne

#endif