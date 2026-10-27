// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "PetriNet/Firing.hpp"
#include "PetriNet/ColoredMarking.hpp"
#include "PetriNet/PetriNet.hpp"

#include <algorithm>
#include <cassert>
#include <limits>

namespace tpne {

//------------------------------------------------------------------------------
//! \brief The enabling rule, whatever holds the tokens: tokens(p, color) gives
//! the tokens of the color in the place p (AnyColor: all), incoming(p) the
//! tokens on their way to it.
//------------------------------------------------------------------------------
template<class Tokens, class Incoming>
static bool enabled(Net const& net, Transition const& transition, Tokens const& tokens,
                    Incoming const& incoming)
{
    for (auto const& a: net.arcsIn(transition))
    {
        size_t const n = tokens(a.from.id, a.color);
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
        size_t n = tokens(a.to.id, AnyColor);
        if (Arc const* in = net.findArc(net.places()[a.to.id], transition))
        {
            if (in->kind == ArcKind::Normal)
                n -= std::min(n, in->weight);
            else if (in->kind == ArcKind::Reset)
                n -= std::min(n, tokens(a.to.id, in->color));
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
                   [&marking](size_t p, Color) { return marking[p]; },
                   [incoming](size_t p) { return (incoming == nullptr) ? 0u : (*incoming)[p]; });
}

//------------------------------------------------------------------------------
size_t placeTokens(Place const& place, Color const color)
{
    if (color == AnyColor)
        return place.tokens;
    size_t left = place.tokens;
    size_t result = 0u;
    for (auto const& [c, n] : place.colors)
    {
        size_t const k = std::min(n, left);
        left -= k;
        if (c == color)
            result += k;
    }
    return (color == Success) ? result + left : result;
}

//------------------------------------------------------------------------------
std::vector<std::pair<Color, size_t>> placeColors(Place const& place)
{
    std::vector<std::pair<Color, size_t>> result;
    auto const add = [&result](Color const color, size_t const n) {
        if (n == 0u)
            return;
        auto it = std::find_if(result.begin(), result.end(), [color](auto const& c) { return c.first == color; });
        if (it == result.end())
            result.push_back({ color, n });
        else
            it->second += n;
    };
    size_t left = place.tokens;
    for (auto const& [color, n] : place.colors)
    {
        size_t const k = std::min(n, left);
        left -= k;
        add((color == AnyColor) ? Success : color, k);
    }
    add(Success, left);
    std::sort(result.begin(), result.end());
    return result;
}

//------------------------------------------------------------------------------
bool isEnabled(Net const& net, Transition const& transition)
{
    return enabled(net, transition,
                   [&net](size_t p, Color c) { return placeTokens(net.places()[p], c); },
                   [](size_t) { return size_t(0u); });
}

//------------------------------------------------------------------------------
bool isEnabled(Net const& net, ColoredMarking const& marking, size_t const transition,
               Marking const* incoming)
{
    assert(marking.places() == net.places().size());
    assert(transition < net.transitions().size());
    return enabled(net, net.transitions()[transition],
                   [&marking](size_t p, Color c) { return marking.count(p, c); },
                   [incoming](size_t p) { return (incoming == nullptr) ? 0u : (*incoming)[p]; });
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
Color tokenColor(Arc const& arc, Color const result)
{
    return (arc.color != AnyColor) ? arc.color : result;
}

//------------------------------------------------------------------------------
std::vector<Color> results(Net const& net, size_t const transition)
{
    std::vector<Color> asked;
    for (auto const& out: net.arcsOut(net.transitions()[transition]))
    {
        if (out.color != AnyColor)
            continue;
        for (auto const& next: net.arcsOut(net.places()[out.to.id]))
        {
            if ((next.color != AnyColor) && (std::find(asked.begin(), asked.end(), next.color) == asked.end()))
                asked.push_back(next.color);
        }
    }
    if (asked.empty())
        return { Success };
    std::sort(asked.begin(), asked.end());
    return asked;
}

//------------------------------------------------------------------------------
void consume(Net const& net, ColoredMarking& marking, size_t const transition)
{
    for (auto const& a: net.arcsIn(net.transitions()[transition]))
    {
        if (a.kind == ArcKind::Normal)
            marking.take(a.from.id, a.color, a.weight);
        else if (a.kind == ArcKind::Reset)
            marking.clear(a.from.id, a.color);
    }
}

//------------------------------------------------------------------------------
void produce(Net const& net, ColoredMarking& marking, size_t const transition, Color const result)
{
    for (auto const& a: net.arcsOut(net.transitions()[transition]))
        marking.add(a.to.id, tokenColor(a, result), a.weight);
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
