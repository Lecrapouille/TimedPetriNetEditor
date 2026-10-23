// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "PetriNet/PetriNet.hpp"
#include "PetriNet/TimedTokens.hpp"

#include <algorithm>
#include <cmath>

namespace tpne {

//------------------------------------------------------------------------------
TimedToken::TimedToken(Net const& net, size_t const arc_, size_t const tokens_,
                       TypeOfNet const type_)
    : arc(arc_), tokens(tokens_), type(type_)
{
    Arc const& a = net.arcs()[arc];
    assert(a.from.type == Node::Type::Transition);
    assert(a.to.type == Node::Type::Place);
    place = a.to.id;

    // Note: we are supposing positions and duration are not updated by
    // the user during the simulation.
    Node const& from = net.node(a.from);
    Node const* to = &net.node(a.to);
    if (type == TypeOfNet::TimedEventGraph)
    {
        // With graph event we have to skip implicit places.
        assert(net.arcsOut(*to).size() == 1u && "malformed graph event");
        to = &net.node(net.arcsOut(*to)[0].to);
    }
    x0 = x = from.x;
    y0 = y = from.y;
    x1 = to->x;
    y1 = to->y;
    magnitude = std::sqrt((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0));

    // Set the token animation speed. Depending on the type of Petri net,
    // and for pure entertainment reason, override the arc duration to
    // avoid unpleasant instantaneous transitions (teleportation effect).
    switch (type_)
    {
    case TypeOfNet::TimedPetriNet:
    case TypeOfNet::TimedEventGraph:
        travel_time = std::max(0.000001f, a.duration);
        break;
        // In theory duration is 0 but nicer for the user to see animation.
    case TypeOfNet::PetriNet:
        travel_time = 0.2f;
        break;
        // In theory duration is 0 but nicer for the user to see animation.
        // Fast animation for GRAFCET (quasi-instantaneous token movement).
    case TypeOfNet::GRAFCET:
        travel_time = 0.15f;
        break;
    default:
        assert(false && "Unknown type of net");
        break;
    }
    speed = magnitude / travel_time;
}

//------------------------------------------------------------------------------
bool TimedToken::update(float const dt)
{
    offset += dt / travel_time;
    x = x0 + (x1 - x0) * offset;
    y = y0 + (y1 - y0) * offset;
    return offset >= 1.0f;
}

} // namespace tpne
