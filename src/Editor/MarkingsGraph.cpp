// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Editor/MarkingsGraph.hpp"
#include "Editor/Theme.hpp"
#include "PetriNet/PetriNet.hpp"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <deque>

namespace tpne {


static float length(ImVec2 a) { return std::sqrt(a.x * a.x + a.y * a.y); }

//------------------------------------------------------------------------------
void MarkingsGraph::set(Net const& net, std::vector<Marking> const& states,
                        std::vector<ReachabilityGraph::Edge> const& edges,
                        std::vector<ColoredMarking> const& colored, std::string const& title,
                        std::string dot)
{
    m_title = title;
    m_dot = std::move(dot);
    m_nodes.clear();
    m_links.clear();
    if (states.size() > MAX_DRAWN)
        return;

    std::vector<bool> leaves(states.size(), true);
    for (auto const& e : edges)
        leaves[e.from] = false;
    for (size_t s = 0u; s < states.size(); ++s)
    {
        Box box;
        box.label = "M" + std::to_string(s) + ": " +
                    markingLabel(net, states[s], (s < colored.size()) ? &colored[s] : nullptr);
        box.initial = (s == 0u);
        box.deadlock = leaves[s];
        m_nodes.push_back(box);
    }
    // Linked boxes wanted at about the width of the widest one.
    m_spring = 140.0f;
    for (auto const& b : m_nodes)
        m_spring = std::max(m_spring, ImGui::CalcTextSize(b.label.c_str()).x + 60.0f);
    for (auto const& e : edges)
    {
        Transition const& t = net.transitions()[e.transition];
        m_links.push_back({ e.from, e.to, t.caption.empty() ? t.key : t.caption });
    }
    relayout();
}

//------------------------------------------------------------------------------
void MarkingsGraph::relayout()
{
    // Start from the layers of a breadth first search: the initial marking at
    // the top, the markings found from it below.
    constexpr size_t NONE = size_t(-1);
    std::vector<size_t> depth(m_nodes.size(), NONE);
    std::vector<std::vector<size_t>> next(m_nodes.size());
    for (auto const& l : m_links)
        next[l.from].push_back(l.to);
    std::vector<size_t> layer_size;
    std::deque<size_t> queue;
    for (size_t root = 0u; root < m_nodes.size(); ++root)
    {
        if (depth[root] != NONE)
            continue;
        depth[root] = 0u;
        queue.push_back(root);
        while (!queue.empty())
        {
            size_t const s = queue.front();
            queue.pop_front();
            if (layer_size.size() <= depth[s])
                layer_size.resize(depth[s] + 1u, 0u);
            m_nodes[s].at = ImVec2(float(layer_size[depth[s]]++) * m_spring, float(depth[s]) * m_spring);
            for (size_t const n : next[s])
            {
                if (depth[n] == NONE)
                {
                    depth[n] = depth[s] + 1u;
                    queue.push_back(n);
                }
            }
        }
    }
    m_temperature = m_spring;
    m_fit = true;
}

//------------------------------------------------------------------------------
void MarkingsGraph::step()
{
    size_t const n = m_nodes.size();
    if ((n < 2u) || (m_temperature < 0.5f))
        return;
    std::vector<ImVec2> move(n, ImVec2(0.0f, 0.0f));

    // All the boxes push each other away, the linked ones pull each other.
    float const k2 = m_spring * m_spring;
    for (size_t i = 0u; i < n; ++i)
    {
        for (size_t j = i + 1u; j < n; ++j)
        {
            ImVec2 d = m_nodes[i].at - m_nodes[j].at;
            float dist = length(d);
            if (dist < 1.0f)
            {
                d = ImVec2(float(i % 7) - 3.0f, float(j % 5) - 2.0f);
                dist = std::max(1.0f, length(d));
            }
            ImVec2 const f = d * (k2 / (dist * dist));
            move[i] = move[i] + f;
            move[j] = move[j] - f;
        }
    }
    for (auto const& l : m_links)
    {
        if (l.from == l.to)
            continue;
        ImVec2 const d = m_nodes[l.from].at - m_nodes[l.to].at;
        float const dist = std::max(1.0f, length(d));
        ImVec2 const f = d * (dist / m_spring);
        move[l.from] = move[l.from] - f;
        move[l.to] = move[l.to] + f;
    }

    // Each box moves at most of the temperature (the dragged one not).
    for (size_t i = 0u; i < n; ++i)
    {
        if (int(i) == m_dragged)
            continue;
        float const d = length(move[i]);
        if (d > 0.0f)
            m_nodes[i].at = m_nodes[i].at + move[i] * (std::min(d, m_temperature) / d);
    }
    m_temperature *= 0.97f;
}

//------------------------------------------------------------------------------
//! \brief Where the segment from the center c of a box of half size h toward
//! p leaves the box.
static ImVec2 border(ImVec2 c, ImVec2 h, ImVec2 p)
{
    ImVec2 const d = p - c;
    float const tx = (std::fabs(d.x) > 1e-3f) ? h.x / std::fabs(d.x) : FLT_MAX;
    float const ty = (std::fabs(d.y) > 1e-3f) ? h.y / std::fabs(d.y) : FLT_MAX;
    float const t = std::min(1.0f, std::min(tx, ty));
    return c + d * t;
}

//------------------------------------------------------------------------------
void MarkingsGraph::draw()
{
    if (m_nodes.empty())
    {
        ImGui::TextWrapped("The graph has more than %zu markings: it is not drawn. "
                           "Export it to dot.", MAX_DRAWN);
        return;
    }
    for (int i = 0; i < 4; ++i)
        step();

    Palette const& pal = palette();
    ImVec2 const origin = ImGui::GetCursorScreenPos();
    ImVec2 const area = ImGui::GetContentRegionAvail();
    if ((area.x < 10.0f) || (area.y < 10.0f))
        return;
    ImGui::InvisibleButton("##markings", area, ImGuiButtonFlags_MouseButtonLeft);
    bool const hovered = ImGui::IsItemHovered();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->PushClipRect(origin, origin + area, true);
    draw->AddRectFilled(origin, origin + area, pal.canvas);

    // Sizes of the boxes on the screen (their text at its drawn size); fit
    // the view while the layout moves.
    float const font = ImGui::GetFontSize();
    float const size = font * std::clamp(m_zoom, 0.75f, 1.2f);
    ImVec2 lo(FLT_MAX, FLT_MAX), hi(-FLT_MAX, -FLT_MAX);
    ImVec2 widest(0.0f, 0.0f);
    for (auto& b : m_nodes)
    {
        b.size = ImGui::CalcTextSize(b.label.c_str()) * (size / font) + ImVec2(12.0f, 8.0f);
        widest = ImVec2(std::max(widest.x, b.size.x), std::max(widest.y, b.size.y));
        lo = ImVec2(std::min(lo.x, b.at.x), std::min(lo.y, b.at.y));
        hi = ImVec2(std::max(hi.x, b.at.x), std::max(hi.y, b.at.y));
    }
    if (m_fit)
    {
        ImVec2 const span = hi - lo + (widest + ImVec2(40.0f, 40.0f)) * (1.0f / m_zoom);
        m_zoom = std::clamp(std::min(area.x / span.x, area.y / span.y), 0.1f, 1.5f);
        m_offset = area * 0.5f - (lo + hi) * 0.5f * m_zoom;
        if (m_temperature < 0.5f)
            m_fit = false;
    }
    auto screen = [&](ImVec2 p) { return origin + m_offset + p * m_zoom; };

    // Mouse: drag a box or the view, wheel to zoom around the mouse.
    ImGuiIO const& io = ImGui::GetIO();
    ImVec2 const mouse = (io.MousePos - origin - m_offset) * (1.0f / m_zoom);
    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        m_dragged = -1;
        for (size_t i = 0u; i < m_nodes.size(); ++i)
        {
            ImVec2 const d = mouse - m_nodes[i].at;
            if ((std::fabs(d.x) * m_zoom <= m_nodes[i].size.x * 0.5f) &&
                (std::fabs(d.y) * m_zoom <= m_nodes[i].size.y * 0.5f))
                m_dragged = int(i);
        }
    }
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        m_fit = false;
        if (m_dragged >= 0)
            m_nodes[size_t(m_dragged)].at = mouse;
        else
            m_offset = m_offset + io.MouseDelta;
    }
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
        m_dragged = -1;
    if (hovered && (io.MouseWheel != 0.0f))
    {
        m_fit = false;
        float const zoom = std::clamp(m_zoom * ((io.MouseWheel > 0.0f) ? 1.1f : 1.0f / 1.1f), 0.05f, 4.0f);
        m_offset = io.MousePos - origin - mouse * zoom;
        m_zoom = zoom;
    }

    // Arrows, then boxes over them.
    for (auto const& l : m_links)
    {
        Box const& a = m_nodes[l.from];
        Box const& b = m_nodes[l.to];
        ImVec2 const ha = a.size * 0.5f, hb = b.size * 0.5f;
        ImVec2 const pa = screen(a.at), pb = screen(b.at);
        if (l.from == l.to)
        {
            // A loop above the box.
            ImVec2 const c = pa - ImVec2(0.0f, ha.y + 12.0f * m_zoom);
            draw->AddCircle(c, 12.0f * m_zoom, pal.outline, 24, 1.5f);
            draw->AddText(nullptr, size, c - ImVec2(0.0f, 14.0f * m_zoom + size), pal.caption, l.label.c_str());
            continue;
        }
        // Each arrow a little on its right: the two of a round trip apart.
        ImVec2 const c = pb - pa;
        ImVec2 const side = ImVec2(-c.y, c.x) * (5.0f / std::max(1.0f, length(c)));
        ImVec2 const from = border(pa, ha, pb) + side;
        ImVec2 const to = border(pb, hb, pa) + side;
        draw->AddLine(from, to, pal.outline, 1.5f);
        ImVec2 const d = to - from;
        float const len = std::max(1.0f, length(d));
        ImVec2 const u = d * (1.0f / len), n(-u.y, u.x);
        float const arrow = 9.0f;
        draw->AddTriangleFilled(to, to - u * arrow + n * (arrow * 0.5f), to - u * arrow - n * (arrow * 0.5f),
                                pal.outline);
        // The label on the same side, its corner toward the arrow.
        ImVec2 const t = ImGui::CalcTextSize(l.label.c_str()) * (size / font);
        ImVec2 at = from + d * 0.5f + n * 6.0f;
        if (n.x < 0.0f)
            at.x -= t.x;
        if (n.y < 0.0f)
            at.y -= t.y;
        draw->AddText(nullptr, size, at, pal.duration, l.label.c_str());
    }
    for (auto const& b : m_nodes)
    {
        ImVec2 const p = screen(b.at);
        ImVec2 const h = b.size * 0.5f;
        ImU32 const border_color = b.deadlock ? pal.critical : pal.outline;
        draw->AddRectFilled(p - h, p + h, pal.fill, 4.0f);
        draw->AddRect(p - h, p + h, border_color, 4.0f, ImDrawFlags_None, b.initial ? 3.0f : 1.5f);
        ImVec2 const t = ImGui::CalcTextSize(b.label.c_str()) * (size / font);
        draw->AddText(nullptr, size, p - t * 0.5f, b.deadlock ? pal.critical : pal.caption, b.label.c_str());
    }
    draw->PopClipRect();
}

} // namespace tpne
