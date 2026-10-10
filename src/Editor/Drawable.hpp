// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef PETRI_DRAWABLE_HPP
#  define PETRI_DRAWABLE_HPP

#  include "Editor/Theme.hpp"
#  include "PetriNet/PetriNet.hpp"
#  include "imgui/imgui.h"

namespace tpne {

void drawArc(ImDrawList* draw_list, Node const* from, Node const* to, ImVec2 const* click_position,
             ImVec2 const& origin, ImVec2 const& cursor, float zoom = 1.0f);
void drawArc(ImDrawList* draw_list, Arc const& arc, TypeOfNet const type,
             ImVec2 const& origin, float const alpha, float zoom = 1.0f);
void drawToken(ImDrawList* draw_list, float const x, float const y, float zoom = 1.0f);
void drawTimedToken(ImDrawList* draw_list, size_t tokens, float const x, float const y, float zoom = 1.0f);
void drawPlace(ImDrawList* draw_list, Place const& place, TypeOfNet const type,
               ImVec2 const& origin, bool const show_caption, float const alpha,
               float zoom = 1.0f, bool isInitialStep = false);
void drawTransition(ImDrawList* draw_list, Transition const& transition, TypeOfNet const type,
                    ImVec2 const& origin, bool const show_caption, float const alpha, float zoom = 1.0f);
void drawPlot(const char* title, const char* label, std::vector<float> const& x, std::vector<float> const& y);

} // namespace tpne

#endif