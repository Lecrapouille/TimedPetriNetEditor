// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "MaxPlus/Flowshop.hpp"
#include "PetriNet/PetriNet.hpp"
#include "PetriNet/SafeFloat.hpp"

#include <algorithm>
#include <map>

namespace tpne {

//------------------------------------------------------------------------------
void operationOf(std::string const& name, std::string& machine, std::string& conveyor)
{
    size_t const underscore = name.find('_');
    bool const operation = (underscore != std::string::npos) && (underscore > 0u) &&
                           (underscore + 1u < name.size()) && (name.rfind("out_", 0) != 0);
    machine = operation ? name.substr(0, underscore) : name;
    conveyor = operation ? name.substr(underscore + 1u) : name;
}

//------------------------------------------------------------------------------
std::string earliestDates(Net const& net, size_t const cycles,
                          std::vector<std::vector<double>>& dates)
{
    auto const& transitions = net.transitions();

    // Input places of each transition: upstream transition, tokens, duration
    // (written on the arc transition -> place).
    struct Input { size_t from; size_t tokens; double duration; };
    std::vector<std::vector<Input>> inputs(transitions.size());
    for (auto const& arc : net.arcs())
    {
        if (arc.from.type != Node::Type::Place)
            continue;
        Place const& place = net.placeOf(arc);
        auto const upstreams = net.arcsIn(place);
        if (upstreams.size() > 1u)
            return "Place " + place.key + " has several inputs: not an event graph";
        if (upstreams.empty())
            return "Place " + place.key + " has no input: not an event graph";
        Arc const& upstream = upstreams.front();
        double const duration = double(upstream.durationOrZero());
        inputs[arc.to.id].push_back({ upstream.from.id, place.tokens, duration });
    }

    // Order of computation inside a cycle: the places without token must
    // not form a circuit (it would never fire).
    std::vector<size_t> waiting(transitions.size(), 0u);
    std::vector<std::vector<size_t>> next(transitions.size());
    for (size_t t = 0u; t < transitions.size(); ++t)
    {
        for (Input const& input : inputs[t])
        {
            if (input.tokens == 0u)
            {
                ++waiting[t];
                next[input.from].push_back(t);
            }
        }
    }
    std::vector<size_t> order;
    for (size_t t = 0u; t < transitions.size(); ++t)
    {
        if (waiting[t] == 0u)
            order.push_back(t);
    }
    for (size_t i = 0u; i < order.size(); ++i)
    {
        for (size_t t : next[order[i]])
        {
            if (--waiting[t] == 0u)
                order.push_back(t);
        }
    }
    if (order.size() != transitions.size())
        return "A circuit has no token: the event graph never fires";

    dates.assign(transitions.size(), std::vector<double>(cycles, 0.0));
    for (size_t k = 0u; k < cycles; ++k)
    {
        for (size_t t : order)
        {
            double date = 0.0;
            for (Input const& input : inputs[t])
            {
                if (k >= input.tokens)
                    date = std::max(date, dates[input.from][k - input.tokens] + input.duration);
            }
            dates[t][k] = date;
        }
    }
    return {};
}

} // namespace tpne
