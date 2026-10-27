// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Editor/TokenAnimator.hpp"
#include "Editor/Drawable.hpp"
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
        // Along the line of the arc as drawn (bent when a reverse arc
        // exists). Timed event graph: up to where the tokens of the hidden
        // place wait.
        ArcPath const path = arcPath(net, net.arcs()[flight.arc]);
        if (path.points.empty())
            continue;
        double const length = flight.end - flight.start;
        float ratio = (length > 0.0) ? float(std::clamp((date - flight.start) / length, 0.0, 1.0))
                                     : 1.0f;
        if (net.layout().compact)
            ratio *= TEG_TOKENS_AT;
        ImVec2 direction;
        ImVec2 const at = alongPath(path.points, ratio, direction);
        tokens.push_back({ at.x, at.y, it.second.second, flight.color });
    }
    return tokens;
}

} // namespace tpne
