// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Editor/ForceDirected.hpp"
#include "PetriNet/PetriNet.hpp"
#include <algorithm>

namespace tpne {

//------------------------------------------------------------------------------
void ForceDirected::reset(float width, float height, Net& net, bool randomize)
{
    m_net = &net;
    m_width = width;
    m_height = height;

    const size_t N = net.transitions().size() + net.places().size();
    if (N == 0)
    {
        m_temperature = 0.0f;
        return;
    }

    // Optimal distance between nodes
    m_K = sqrtf(m_width * m_height / float(N)) * 0.75f;
    m_temperature = std::min(m_width, m_height) / 2.0f;

    // Build vertex list and lookup table
    m_vertices.clear();
    m_vertices.reserve(N);
    m_node_to_index.clear();
    m_node_to_index.reserve(N);

    size_t index = 0;
    for (auto& t : net.transitions())
    {
        m_vertices.emplace_back(&t);
        m_node_to_index[&t] = index++;
    }
    for (auto& p : net.places())
    {
        m_vertices.emplace_back(&p);
        m_node_to_index[&p] = index++;
    }

    // Build neighbor lists (deduplicated using set)
    for (auto& v : m_vertices)
    {
        std::unordered_set<Node*> neighbor_set;

        for (auto const& arc : net.arcsIn(*v.node))
            neighbor_set.insert(&net.node(arc.from));
        for (auto const& arc : net.arcsOut(*v.node))
            neighbor_set.insert(&net.node(arc.to));

        v.neighbors.assign(neighbor_set.begin(), neighbor_set.end());
    }

    // Initialize positions for nodes without valid coordinates
    initializePositions(randomize);
}

//------------------------------------------------------------------------------
void ForceDirected::initializePositions(bool randomize)
{
    if (!randomize)
        return;

    const Vec2 c = center();
    const float radius = std::min(m_width, m_height) * 0.3f;
    std::uniform_real_distribution<float> dist(-radius, radius);

    // Detect nodes that are at origin or overlapping
    for (auto& v : m_vertices)
    {
        bool needs_init = (v.node->x == 0.0f && v.node->y == 0.0f);

        // Also check for nodes too close to each other
        if (!needs_init)
        {
            for (auto& u : m_vertices)
            {
                if (u.node == v.node)
                    continue;
                float dx = v.node->x - u.node->x;
                float dy = v.node->y - u.node->y;
                if (dx * dx + dy * dy < 1.0f)
                {
                    needs_init = true;
                    break;
                }
            }
        }

        if (needs_init)
        {
            v.node->x = c.x + dist(m_rng);
            v.node->y = c.y + dist(m_rng);
        }
    }
}

//------------------------------------------------------------------------------
void ForceDirected::update()
{
    if (!isRunning())
        return;

    // Multiple steps per frame for faster convergence
    for (int i = 0; i < params.steps_per_frame; ++i)
    {
        step();
        if (!isRunning())
            break;
    }
}

//------------------------------------------------------------------------------
void ForceDirected::step()
{
    if (m_net == nullptr)
        return;

    const Vec2 canvas_center = center();
    const size_t N = m_vertices.size();

    // Compute forces for each vertex
    for (size_t i = 0; i < N; ++i)
    {
        Vertex& v = m_vertices[i];
        const Vec2 v_pos(v.node->x, v.node->y);

        // Repulsive forces: all nodes repel each other (O(N²))
        for (size_t j = 0; j < N; ++j)
        {
            if (i == j)
                continue;

            Vertex& u = m_vertices[j];
            const Vec2 u_pos(u.node->x, u.node->y);
            const Vec2 delta = v_pos - u_pos;
            const float dist = length(delta);
            const float force = repulsive_force(dist);
            v.displacement += delta * (force / dist / float(N));
        }

        // Attractive forces: connected nodes attract
        for (Node* neighbor : v.neighbors)
        {
            const Vec2 n_pos(neighbor->x, neighbor->y);
            const Vec2 delta = v_pos - n_pos;
            const float dist = length(delta);
            const float force = attractive_force(dist);
            v.displacement -= delta * (force / dist / float(N));
        }

        // Gravity: pull toward center to keep graph compact
        if (params.gravity > 0.0f)
        {
            const Vec2 to_center = canvas_center - v_pos;
            v.displacement += to_center * params.gravity;
        }
    }

    // Update positions with temperature-limited displacement
    constexpr float BORDER = 50.0f;
    for (auto& v : m_vertices)
    {
        const float disp_len = length(v.displacement);
        Vec2 pos(v.node->x, v.node->y);

        // Limit displacement by temperature (simulated annealing)
        if (disp_len > m_temperature)
        {
            pos += v.displacement * (m_temperature / disp_len);
        }
        else
        {
            pos += v.displacement;
        }

        // Constrain to canvas bounds
        v.node->x = std::clamp(pos.x, BORDER, m_width - BORDER);
        v.node->y = std::clamp(pos.y, BORDER, m_height - BORDER);

        // Reset displacement for next iteration
        v.displacement = Vec2();
    }

    cooling();
}

} // namespace tpne