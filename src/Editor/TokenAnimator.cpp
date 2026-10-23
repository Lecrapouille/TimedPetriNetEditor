// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Editor/TokenAnimator.hpp"
#include "PetriNet/PetriNet.hpp"

#include <algorithm>
#include <map>

namespace tpne {

//------------------------------------------------------------------------------
std::vector<AnimatedToken> animateTokens(Net const& net, std::vector<Engine::Flight> const& flights,
                                         double const date)
{
    // Tokens of one firing on one arc are drawn as one dot.
    std::map<std::pair<size_t, double>, std::pair<Engine::Flight, size_t>> groups;
    for (auto const& flight : flights)
    {
        auto& group = groups[{ flight.arc, flight.start }];
        group.first = flight;
        group.second += flight.tokens;
    }

    std::vector<AnimatedToken> tokens;
    tokens.reserve(groups.size());
    for (auto const& it : groups)
    {
        Engine::Flight const& flight = it.second.first;
        if (flight.arc >= net.arcs().size())
            continue;
        Arc const& arc = net.arcs()[flight.arc];
        Point const& from = net.position(arc.from);
        Point const* to = &net.position(arc.to);
        if (net.type() == TypeOfNet::TimedEventGraph)
        {
            auto const next = net.arcsOut(net.node(arc.to));
            if (!next.empty())
                to = &net.position(next.front().to);
        }

        double const length = flight.end - flight.start;
        float const ratio = (length > 0.0) ? float(std::clamp((date - flight.start) / length, 0.0, 1.0))
                                           : 1.0f;
        tokens.push_back({ from.x + (to->x - from.x) * ratio, from.y + (to->y - from.y) * ratio,
                           it.second.second });
    }
    return tokens;
}

} // namespace tpne
