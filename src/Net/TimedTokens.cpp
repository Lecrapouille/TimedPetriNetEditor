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

#include "Net/TimedTokens.hpp"

namespace tpne {

//------------------------------------------------------------------------------
static inline float norm(const float xa, const float ya, const float xb, const float yb)
{
    return sqrtf((xb - xa) * (xb - xa) + (yb - ya) * (yb - ya));
}

//------------------------------------------------------------------------------
TimedToken::TimedToken(Arc& arc_,size_t const tokens_, TypeOfNet const type_)
    : arc(arc_), x(arc_.from.x), y(arc_.from.y), tokens(tokens_), type(type_)
{
    assert(arc.from.type == Node::Type::Transition);
    assert(arc.to.type == Node::Type::Place);

    // Note: we are supposing the norm and duration is never updated by
    // the user during the simulation.
    if (type != TypeOfNet::TimedEventGraph)
    {
        magnitude = norm(arc.from.x, arc.from.y, arc.to.x, arc.to.y);
    }
    else
    {
        // With graph event we have to skip implicit places.
        assert(arc.to.arcsOut.size() == 1u && "malformed graph event");
        Node& next = arc.to.arcsOut[0]->to;
        magnitude = norm(arc.from.x, arc.from.y, next.x, next.y);
    }

    // Set the token animation speed. Depending on the type of Petri net,
    // and for pure entertainment reason, override the arc duration to
    // avoid unpleasant instaneous transitions (teleportation effect).
    switch (type_)
    {
    case TypeOfNet::TimedPetriNet:
    case TypeOfNet::TimedEventGraph:
        speed = magnitude / std::max(0.000001f, arc.duration);
        break;
        // In theory duration is 0 but nicer for the user to see animation.
    case TypeOfNet::PetriNet:
        speed = magnitude / 0.2f;
        break;
        // In theory duration is 0 but nicer for the user to see animation.
    case TypeOfNet::GRAFCET:
        speed = magnitude / 1.5f;
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
    Node& next = (type != TypeOfNet::TimedEventGraph)
                ? arc.to : arc.to.arcsOut[0]->to;

    offset += dt * speed / magnitude;
    x = arc.from.x + (next.x - arc.from.x) * offset;
    y = arc.from.y + (next.y - arc.from.y) * offset;

    return (offset >= 1.0);
}

} // namespace tpne