// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Editor/TopDownLayout.hpp"
#include "PetriNet/PetriNet.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <vector>

namespace tpne {

//------------------------------------------------------------------------------
//! \brief Place the GRAFCET from top to bottom as the standard draws it: the
//! layer of a node is its distance from the initial steps (links going back
//! up are ignored), each node under the mean of its parents, nodes of a layer
//! kept apart. Places are steps, transitions sit between two steps.
//------------------------------------------------------------------------------
void arrangeTopDown(Net& net)
{
    std::map<Node*, int> layer;
    std::map<Node*, float> xpos;
    std::vector<Node*> queue;
    float x0 = 100.0f;
    // Roots: initial steps, else places without input, else the first place.
    for (auto& place : net.places())
    {
        if ((place.tokens > 0u) || net.arcsIn(place).empty())
        {
            layer[&place] = 0;
            xpos[&place] = x0;
            x0 += 260.0f;
            queue.push_back(&place);
        }
    }
    if (queue.empty() && !net.places().empty())
    {
        layer[&net.places()[0]] = 0;
        xpos[&net.places()[0]] = x0;
        queue.push_back(&net.places()[0]);
    }
    for (size_t i = 0u; i < queue.size(); ++i)
    {
        Node* node = queue[i];
        size_t index = 0u;
        auto const arcs = net.arcsOut(*node);
        size_t const count = arcs.size();
        for (Arc const& arc : arcs)
        {
            Node* next = &net.node(arc.to);
            if (layer.count(next) != 0u)
                continue;
            layer[next] = layer[node] + 1;
            // Siblings spread around their parent.
            float const spread = 160.0f;
            xpos[next] = xpos[node] + (float(index) - float(count - 1u) * 0.5f) * spread;
            ++index;
            queue.push_back(next);
        }
    }
    // Nodes not reached (other components): below, in a row.
    int max_layer = 0;
    for (auto const& it : layer)
        max_layer = std::max(max_layer, it.second);
    float extra_x = 100.0f;
    auto place_rest = [&](Node& n) {
        if (layer.count(&n) == 0u)
        {
            layer[&n] = max_layer + 2;
            xpos[&n] = extra_x;
            extra_x += 120.0f;
        }
    };
    for (auto& p : net.places()) place_rest(p);
    for (auto& t : net.transitions()) place_rest(t);

    // Keep the nodes of a layer at least 150 apart.
    std::map<int, std::vector<Node*>> layers;
    for (auto const& it : layer)
        layers[it.second].push_back(it.first);
    for (auto& it : layers)
    {
        auto& nodes = it.second;
        std::sort(nodes.begin(), nodes.end(), [&](Node* a, Node* b) { return xpos[a] < xpos[b]; });
        for (size_t i = 1u; i < nodes.size(); ++i)
            xpos[nodes[i]] = std::max(xpos[nodes[i]], xpos[nodes[i - 1u]] + 150.0f);
        for (Node* n : nodes)
        {
            Point& p = net.position(*n);
            p.x = std::round(xpos[n] / 10.0f) * 10.0f;
            p.y = 60.0f + float(it.first) * 70.0f;
        }
    }
    net.modified = true;
}

} // namespace tpne
