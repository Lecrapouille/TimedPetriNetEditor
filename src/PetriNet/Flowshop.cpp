// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "PetriNet/Flowshop.hpp"
#include "PetriNet/PetriNet.hpp"
#include "PetriNet/SafeFloat.hpp"

#include <algorithm>
#include <map>

namespace tpne {

//------------------------------------------------------------------------------
std::string earliestDates(Net const& net, size_t const cycles,
                          std::vector<std::vector<double>>& dates)
{
    auto const& transitions = net.transitions();
    std::map<Node const*, size_t> index;
    for (size_t t = 0u; t < transitions.size(); ++t)
        index[&transitions[t]] = t;

    // Input places of each transition: upstream transition, tokens, duration
    // (written on the arc transition -> place).
    struct Input { size_t from; size_t tokens; double duration; };
    std::vector<std::vector<Input>> inputs(transitions.size());
    for (auto const& arc : net.arcs())
    {
        if (arc.from.type != Node::Type::Place)
            continue;
        Place const& place = static_cast<Place const&>(arc.from);
        Arc const* upstream = nullptr;
        for (auto const& a : net.arcs())
        {
            if (&a.to == &place)
            {
                if (upstream != nullptr)
                    return "Place " + place.key + " has several inputs: not an event graph";
                upstream = &a;
            }
        }
        if (upstream == nullptr)
            return "Place " + place.key + " has no input: not an event graph";
        double const duration = safeIsNaN(upstream->duration) ? 0.0 : double(upstream->duration);
        inputs[index.at(&arc.to)].push_back({ index.at(&upstream->from), place.tokens, duration });
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
