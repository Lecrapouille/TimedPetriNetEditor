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

#include "TimedPetriNetEditor/PetriNet.hpp"
#include "imgui.h"

namespace tpne {

static const float ARROW_WIDTH = 14.0f;
static const float ARROW_SPACING = 10.0f;
static const float TRANS_WIDTH = 36.0f;  // Rectangle width for rendering Transitions
static const float TRANS_HEIGHT = TRANS_WIDTH / 3.0f;  // Rectangle height for rendering Transitions
static const float PLACE_RADIUS = TRANS_WIDTH / 2.0f; // Circle radius for rendering Places
static const float TOKEN_RADIUS = 2.0f;  // Circle radius for rendering tokens


void drawArc(ImDrawList* draw_list, Arc const& arc, TypeOfNet const type, ImVec2 const& origin, float const alpha);
void drawToken(ImDrawList* draw_list, float const x, float const y);
void drawTimedToken(ImDrawList* draw_list, size_t tokens, float const x, float const y);
void drawPlace(ImDrawList* draw_list, Place const& place, TypeOfNet const type, ImVec2 const& origin, bool const show_caption, float const alpha);
void drawTransition(ImDrawList* draw_list, Transition const& transition, TypeOfNet const type, ImVec2 const& origin, bool const show_caption, float const alpha);

} // namespace tpne

#endif