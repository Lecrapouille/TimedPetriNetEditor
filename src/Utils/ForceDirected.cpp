/* *****************************************************************************
** MIT License
**
** Copyright (c) 2022 Quentin Quadrat
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all
** copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
** SOFTWARE.
********************************************************************************
*/

#include "Utils/ForceDirected.hpp"

namespace tpne {

//------------------------------------------------------------------------------
void ForceDirected::reset(float width, float height, Net& net)
{
    std::cout << "ForceDirected::reset " << width << " x " << height << "\n";
    m_net = &net;
    m_width = width;
    m_height = height;

    N = net.transitions().size() + net.places().size();
    K = sqrtf(m_width * m_height / 2.0f / float(N));
    m_temperature = m_width + m_height;
    m_vertices.clear();
    m_vertices.reserve(N);

    // Copy Petri nodes to Graph vertices
    for (auto& it: net.transitions())
    {
        m_vertices.emplace_back(it);
    }
    for (auto& it: net.places())
    {
        m_vertices.emplace_back(it);
    }

    // Lookup table: Node ID to index on the vector
    std::map<std::string, size_t> lookup;
    for (size_t n = 0u; n < N; ++n)
    {
        lookup[m_vertices[n].node->key] = n;
    }

    // Add edges "source node" -> "destination node".
    // Since, we need undirected graph so add edges "destination node" -> "source node"
    for (size_t n = 0u; n < N; ++n)
    {
        Vertex& v = m_vertices[n];
        v.neighbors.reserve(2u * (v.node->arcsIn.size() + v.node->arcsOut.size()));

        for (auto& it: v.node->arcsIn)
        {
            v.neighbors.emplace_back(m_vertices[lookup[it->from.key]].node);
            v.neighbors.emplace_back(m_vertices[lookup[it->to.key]].node);
        }

        for (auto& it: v.node->arcsOut)
        {
            v.neighbors.emplace_back(m_vertices[lookup[it->from.key]].node);
            v.neighbors.emplace_back(m_vertices[lookup[it->to.key]].node);
        }
    }
}

//------------------------------------------------------------------------------
void ForceDirected::update()
{
    if (m_net == nullptr)
        return ;

    if (m_temperature < 0.1f)
        return ;

    step();
}

//------------------------------------------------------------------------------
void ForceDirected::step()
{
    if (m_net == nullptr)
        return ;

    for (auto& v: m_vertices)
    {
        const ImVec2 v_position(v.node->x, v.node->y);

        // Repulsive forces: nodes -- nodes
        for (auto& u: m_vertices)
        {
            if (u.node->key == v.node->key)
                continue ;

            const ImVec2 u_position(u.node->x, u.node->y);
            const ImVec2 direction(v_position - u_position);
            const float dist = distance(direction);
            const float rf = repulsive_force(dist);
            v.displacement += direction * rf / dist;
        }

        // Attractive forces: edges
        for (auto& u: v.neighbors)
        {
            if (u->key == v.node->key)
                continue ;

            const ImVec2 u_position(u->x, u->y);
            const ImVec2 direction(v_position - u_position);
            const float dist = distance(direction);
            const float af = attractive_force(dist);
            v.displacement -= direction * af / dist;
        }
    }

    // Update position and constrain position to the window bounds
    for (auto& v: m_vertices)
    {
        constexpr ImVec2 LAYOUT_BORDER(50.0f, 50.0f);
        const float dist = distance(v.displacement);
        ImVec2 v_position(v.node->x, v.node->y);
        v_position += (dist > m_temperature)
                      ? v.displacement * m_temperature / dist
                      : v.displacement;
        v.node->x = std::min(m_width - LAYOUT_BORDER.x,
                             std::max(LAYOUT_BORDER.x, v_position.x));
        v.node->y = std::min(m_height - LAYOUT_BORDER.y,
                             std::max(LAYOUT_BORDER.y, v_position.y));
        v.displacement = { 0.0f, 0.0f };
    }

    cooling();
}

} // namespace tpne