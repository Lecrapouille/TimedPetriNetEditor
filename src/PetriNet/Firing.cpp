// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "PetriNet/Firing.hpp"
#include "PetriNet/PetriNet.hpp"

#include <algorithm>
#include <cassert>
#include <limits>

namespace tpne {

//------------------------------------------------------------------------------
//! \brief The enabling rule, whatever holds the tokens: tokens(p) gives the
//! tokens of the place p, incoming(p) the tokens on their way to it.
//------------------------------------------------------------------------------
template<class Tokens, class Incoming>
static bool enabled(Net const& net, Transition const& transition, Tokens const& tokens,
                    Incoming const& incoming)
{
    for (auto const& a: net.arcsIn(transition))
    {
        size_t const n = tokens(a.from.id);
        if ((a.kind == ArcKind::Normal) && (n < a.weight))
            return false;
        if ((a.kind == ArcKind::Inhibitor) && (n >= a.weight))
            return false;
    }

    for (auto const& a: net.arcsOut(transition))
    {
        size_t const capacity = net.places()[a.to.id].capacity;
        if (capacity == std::numeric_limits<size_t>::max())
            continue;
        size_t n = tokens(a.to.id);
        if (Arc const* in = net.findArc(net.places()[a.to.id], transition))
        {
            if (in->kind == ArcKind::Normal)
                n -= in->weight;
            else if (in->kind == ArcKind::Reset)
                n = 0u;
        }
        n += incoming(a.to.id);
        if (a.weight > capacity - std::min(capacity, n))
            return false;
    }
    return true;
}

//------------------------------------------------------------------------------
bool isEnabled(Net const& net, Marking const& marking, size_t const transition,
               Marking const* incoming)
{
    assert(marking.size() == net.places().size());
    assert(transition < net.transitions().size());
    return enabled(net, net.transitions()[transition],
                   [&marking](size_t p) { return marking[p]; },
                   [incoming](size_t p) { return (incoming == nullptr) ? 0u : (*incoming)[p]; });
}

//------------------------------------------------------------------------------
bool isEnabled(Net const& net, Transition const& transition)
{
    return enabled(net, transition,
                   [&net](size_t p) { return net.places()[p].tokens; },
                   [](size_t) { return size_t(0u); });
}

//------------------------------------------------------------------------------
std::vector<size_t> enabledTransitions(Net const& net, Marking const& marking)
{
    std::vector<size_t> result;
    for (size_t t = 0u; t < net.transitions().size(); ++t)
    {
        if (isEnabled(net, marking, t))
            result.push_back(t);
    }
    return result;
}

//------------------------------------------------------------------------------
void consume(Net const& net, Marking& marking, size_t const transition)
{
    for (auto const& a: net.arcsIn(net.transitions()[transition]))
    {
        size_t& n = marking[a.from.id];
        if (a.kind == ArcKind::Normal)
        {
            assert(n >= a.weight);
            n -= a.weight;
        }
        else if (a.kind == ArcKind::Reset)
        {
            n = 0u;
        }
    }
}

//------------------------------------------------------------------------------
void produce(Net const& net, Marking& marking, size_t const transition)
{
    for (auto const& a: net.arcsOut(net.transitions()[transition]))
    {
        size_t const capacity = net.places()[a.to.id].capacity;
        size_t& n = marking[a.to.id];
        n = (a.weight > capacity - std::min(capacity, n)) ? capacity : n + a.weight;
    }
}

//------------------------------------------------------------------------------
Marking fire(Net const& net, Marking const& marking, size_t const transition)
{
    assert(isEnabled(net, marking, transition));
    Marking next(marking);
    consume(net, next, transition);
    produce(net, next, transition);
    return next;
}

} // namespace tpne
