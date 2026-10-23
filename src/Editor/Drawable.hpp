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

#  include <vector>

// *****************************************************************************
//! \file Drawing of the nets with Dear ImGui draw lists, shared by the editor
//! and by NetViewer. Positions of nodes are in net units; a view gives its
//! origin (screen position of the net point 0, 0) and its zoom.
//!
//! GRAFCET follows IEC 60848: a step is a square with its number, the initial
//! step a double square, a transition a short thick line with its receptivity
//! on its right; links are vertical and horizontal lines, with an arrow only
//! when they go up; a double line opens and closes parallel sequences (AND),
//! a single line the alternative ones (OR).
// *****************************************************************************

namespace tpne {

// ****************************************************************************
//! \brief Line of an arc in net units, as drawn.
// ****************************************************************************
struct ArcPath
{
    //! \brief Polyline from the source to the destination.
    std::vector<ImVec2> points;
    //! \brief Arrow head at the last point.
    bool arrow_end = true;
    //! \brief Arrow in the middle of this segment (GRAFCET link going up),
    //! or -1.
    int arrow_segment = -1;
};

//------------------------------------------------------------------------------
//! \brief Line of the arc. Empty for arcs that are not drawn (the hidden
//! Place -> Transition half of a timed event graph arc).
//------------------------------------------------------------------------------
ArcPath arcPath(Net const& net, Arc const& arc);

//------------------------------------------------------------------------------
//! \brief Distance (net units) between a point and the line of the arc, as
//! drawn. A large value for arcs that are not drawn.
//------------------------------------------------------------------------------
float distanceToArc(Net const& net, Arc const& arc, ImVec2 const& point);

//------------------------------------------------------------------------------
//! \brief Font size for a zoom; 0 when the text is too small to be read.
//------------------------------------------------------------------------------
float fontSizeAt(float zoom);

//------------------------------------------------------------------------------
//! \brief Draw an arc. A negative alpha draws it as a critical circuit arc.
//------------------------------------------------------------------------------
void drawArc(ImDrawList* draw_list, Net const& net, Arc const& arc,
             ImVec2 const& origin, float const alpha, float zoom = 1.0f);

//------------------------------------------------------------------------------
//! \brief Draw the line of an arc over it (hover, selection).
//------------------------------------------------------------------------------
void drawArcHighlight(ImDrawList* draw_list, Net const& net, Arc const& arc,
                      ImVec2 const& origin, float zoom, ImU32 color, float thickness);

//------------------------------------------------------------------------------
//! \brief Draw the arc being created from a node (or a point) to the cursor.
//------------------------------------------------------------------------------
void drawArc(ImDrawList* draw_list, Node const* from, Node const* to, ImVec2 const* click_position,
             ImVec2 const& origin, ImVec2 const& cursor, float zoom = 1.0f);

void drawToken(ImDrawList* draw_list, float const x, float const y, float zoom = 1.0f);
void drawTimedToken(ImDrawList* draw_list, size_t tokens, float const x, float const y, float zoom = 1.0f);

//------------------------------------------------------------------------------
//! \brief Draw a place (a step for GRAFCET) and its caption.
//! \param[in] show_caption the caption (true) or the identifier (false) as
//! label.
//! \param[in] show_identifier also the identifier: small, over a Petri place
//! whose caption is not its identifier; the number of a GRAFCET step (else
//! only its caption).
//------------------------------------------------------------------------------
void drawPlace(ImDrawList* draw_list, Net const& net, Place const& place,
               ImVec2 const& origin, bool const show_caption, float const alpha,
               float zoom = 1.0f, bool isInitialStep = false, bool show_identifier = true);

//------------------------------------------------------------------------------
//! \brief Draw a transition and its caption (receptivity for GRAFCET).
//! \param[in] show_identifier also the identifier: small, over a Petri
//! transition whose caption is not its identifier; "(3)" on the left of a
//! GRAFCET transition.
//------------------------------------------------------------------------------
void drawTransition(ImDrawList* draw_list, Net const& net, Transition const& transition,
                    ImVec2 const& origin, bool const show_caption, float const alpha, float zoom = 1.0f,
                    bool show_identifier = true);

//------------------------------------------------------------------------------
//! \brief Draw the outline of a node grown by margin pixels (hover, selection).
//------------------------------------------------------------------------------
void drawNodeOutline(ImDrawList* draw_list, Node const& node, TypeOfNet type,
                     ImVec2 const& origin, float zoom, ImU32 color, float margin,
                     float thickness);

//------------------------------------------------------------------------------
//! \brief Place, transition or node whose center is closer than the radius to
//! the given position (net units), or null. For timed event graphs, places
//! are not drawn so nodeAt() only finds transitions.
//------------------------------------------------------------------------------
Place* placeAt(Net& net, ImVec2 const& position, float radius);
Transition* transitionAt(Net& net, ImVec2 const& position, float radius);
Node* nodeAt(Net& net, ImVec2 const& position, float place_radius, float transition_radius);

//------------------------------------------------------------------------------
//! \brief Half size of a node in net units (radius for a Petri place).
//------------------------------------------------------------------------------
ImVec2 nodeHalfSize(Node const& node, TypeOfNet type);

//------------------------------------------------------------------------------
//! \brief Draw the actions of a GRAFCET step as IEC 60848 boxes on its right:
//! one line per action with its qualifier and its name, one line in a double
//! frame per forcing order ("Production{init}").
//! \param[in] is_active tells if the action of the given index is running
//! (drawn highlighted). Can be null: then no action is highlighted.
//! \param[in] compact only a small tag with the number of actions (green
//! when one runs), to keep the net compact.
//------------------------------------------------------------------------------
void drawStepActions(ImDrawList* draw_list, Place const& place, ImVec2 const& origin,
                     float zoom, bool (*is_active)(size_t index, void* data) = nullptr,
                     void* data = nullptr, bool compact = false);

} // namespace tpne

#endif
