// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "PetriNet/PetriNet.hpp"
#include "PetriNet/TimedTokens.hpp"

#include <cmath>

namespace tpne {

//------------------------------------------------------------------------------
//! \brief Compute the Euclidean distance between two points with .x and .y members.
//------------------------------------------------------------------------------
template<typename T>
static inline float norm(T const& A, T const& B)
{
    return std::sqrt((B.x - A.x) * (B.x - A.x) + (B.y - A.y) * (B.y - A.y));
}

//------------------------------------------------------------------------------
TimedToken::TimedToken(Arc& arc_, size_t const tokens_, TypeOfNet const type_)
    : arc(&arc_), x(arc_.from.x), y(arc_.from.y), tokens(tokens_), type(type_)
{
    assert(arc->from.type == Node::Type::Transition);
    assert(arc->to.type == Node::Type::Place);

    // Note: we are supposing the norm and duration is never updated by
    // the user during the simulation.
    if (type != TypeOfNet::TimedEventGraph)
    {
        magnitude = norm(arc->from, arc->to);
    }
    else
    {
        // With graph event we have to skip implicit places.
        assert(arc->to.arcsOut.size() == 1u && "malformed graph event");
        Node const& next = arc->to.arcsOut[0]->to;
        magnitude = norm(arc->from, next);
    }

    // Set the token animation speed. Depending on the type of Petri net,
    // and for pure entertainment reason, override the arc duration to
    // avoid unpleasant instantaneous transitions (teleportation effect).
    switch (type_)
    {
    case TypeOfNet::TimedPetriNet:
    case TypeOfNet::TimedEventGraph:
        speed = magnitude / std::max(0.000001f, arc->duration);
        break;
        // In theory duration is 0 but nicer for the user to see animation.
    case TypeOfNet::PetriNet:
        speed = magnitude / 0.2f;
        break;
        // In theory duration is 0 but nicer for the user to see animation.
        // Fast animation for GRAFCET (quasi-instantaneous token movement).
    case TypeOfNet::GRAFCET:
        speed = magnitude / 0.15f;
        break;
    default:
        assert(false && "Unknown type of net");
        break;
    }
}

//------------------------------------------------------------------------------
bool TimedToken::update(float const dt)
{
    // With graph event we have to skip implicit places.
    Node const& next = (type != TypeOfNet::TimedEventGraph)
                ? arc->to : arc->to.arcsOut[0]->to;

    offset += dt * speed / magnitude;
    x = arc->from.x + (next.x - arc->from.x) * offset;
    y = arc->from.y + (next.y - arc->from.y) * offset;
    return offset >= 1.0f;
}

//------------------------------------------------------------------------------
Place& TimedToken::toPlace()
{
    return *reinterpret_cast<Place*>(&(arc->to));
}

} // namespace tpne
