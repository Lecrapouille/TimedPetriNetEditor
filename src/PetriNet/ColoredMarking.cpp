// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "PetriNet/ColoredMarking.hpp"
#include "PetriNet/PetriNet.hpp"

#include <algorithm>

namespace tpne {

//==============================================================================
// ColoredMarking
//==============================================================================

//------------------------------------------------------------------------------
size_t ColoredMarking::count(size_t const place, Color const color) const
{
    std::vector<size_t> const& colors = m_counts[place];
    if (color != AnyColor)
        return (color < colors.size()) ? colors[color] : 0u;
    size_t total = 0u;
    for (size_t const n : colors)
        total += n;
    return total;
}

//------------------------------------------------------------------------------
void ColoredMarking::add(size_t const place, Color const color, size_t const tokens)
{
    std::vector<size_t>& colors = m_counts[place];
    if (color >= colors.size())
        colors.resize(size_t(color) + 1u, 0u);
    colors[color] += tokens;
    touch(place);
}

//------------------------------------------------------------------------------
bool ColoredMarking::take(size_t const place, Color const color, size_t const tokens)
{
    if (count(place, color) < tokens)
        return false;
    touch(place);
    std::vector<size_t>& colors = m_counts[place];
    if (color != AnyColor)
    {
        colors[color] -= tokens;
        return true;
    }
    size_t left = tokens;
    for (size_t& n : colors)
    {
        size_t const taken = std::min(n, left);
        n -= taken;
        left -= taken;
    }
    return true;
}

//------------------------------------------------------------------------------
void ColoredMarking::clear(size_t const place, Color const color)
{
    touch(place);
    std::vector<size_t>& colors = m_counts[place];
    if (color == AnyColor)
        std::fill(colors.begin(), colors.end(), 0u);
    else if (color < colors.size())
        colors[color] = 0u;
}

//------------------------------------------------------------------------------
std::vector<std::pair<Color, size_t>> ColoredMarking::tokens(size_t const place) const
{
    std::vector<std::pair<Color, size_t>> result;
    std::vector<size_t> const& colors = m_counts[place];
    for (size_t c = 0u; c < colors.size(); ++c)
    {
        if (colors[c] > 0u)
            result.push_back({ Color(c), colors[c] });
    }
    return result;
}

//------------------------------------------------------------------------------
std::vector<size_t> ColoredMarking::key() const
{
    std::vector<size_t> result;
    for (size_t p = 0u; p < m_counts.size(); ++p)
    {
        size_t const at = result.size();
        result.push_back(0u);
        for (size_t c = 0u; c < m_counts[p].size(); ++c)
        {
            if (m_counts[p][c] == 0u)
                continue;
            result.push_back(c);
            result.push_back(m_counts[p][c]);
            ++result[at];
        }
    }
    return result;
}

//------------------------------------------------------------------------------
Marking ColoredMarking::counts() const
{
    Marking result(m_counts.size(), 0u);
    for (size_t p = 0u; p < m_counts.size(); ++p)
        result[p] = count(p);
    return result;
}

//------------------------------------------------------------------------------
ColoredMarking initialMarking(Net const& net)
{
    ColoredMarking marking(net.places().size());
    for (auto const& place : net.places())
    {
        size_t left = place.tokens;
        for (auto const& [color, tokens] : place.colors)
        {
            size_t const n = std::min(tokens, left);
            if (n > 0u)
                marking.add(place.id, color, n);
            left -= n;
        }
        if (left > 0u)
            marking.add(place.id, Success, left);
    }
    marking.takeTouched();
    return marking;
}

} // namespace tpne
