// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

// ImVec2 operators (+, -, *): also defined when the host does not.
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#  define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "Editor/Drawable.hpp"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <limits>
#include <cstdio>
#include <string>

namespace tpne {

// ============================================================================
// Small geometry and text helpers
// ============================================================================

//! \brief Length of a vector.
static float length(ImVec2 const& v)
{
    return std::sqrt(v.x * v.x + v.y * v.y);
}

//! \brief Unit vector, or (0, 1) for a null vector.
static ImVec2 unit(ImVec2 const& v)
{
    float const l = length(v);
    return (l < 1e-4f) ? ImVec2(0.0f, 1.0f) : ImVec2(v.x / l, v.y / l);
}

//! \brief Left normal of a direction.
static ImVec2 normal(ImVec2 const& v)
{
    ImVec2 const u = unit(v);
    return ImVec2(-u.y, u.x);
}

//! \brief Screen position of a net point.
static ImVec2 screen(ImVec2 const& origin, float zoom, ImVec2 const& p)
{
    return ImVec2(origin.x + p.x * zoom, origin.y + p.y * zoom);
}

//! \brief Line thickness in pixels, never thinner than one pixel.
static float stroke(float width, float zoom)
{
    return std::max(1.0f, width * zoom);
}

//------------------------------------------------------------------------------
float fontSizeAt(float zoom)
{
    // Whole pixels: Dear ImGui bakes the glyphs of each font size once, a
    // continuous zoom would bake hundreds of sizes.
    float const size = std::round(ImGui::GetFontSize() * std::min(zoom, 1.25f));
    return (size < 6.0f) ? 0.0f : size;
}

//! \brief Text with its top left corner at p; nothing when too small.
static void text(ImDrawList* draw_list, ImVec2 const& p, ImU32 color, char const* str,
                 float size)
{
    if (size > 0.0f)
        draw_list->AddText(ImGui::GetFont(), size, p, color, str);
}

//! \brief Size of a text at the given font size.
static ImVec2 textSize(char const* str, float size)
{
    if (size <= 0.0f)
        return ImVec2(0.0f, 0.0f);
    return ImGui::GetFont()->CalcTextSizeA(size, FLT_MAX, 0.0f, str);
}

//! \brief Text centered on c.
static void textCentered(ImDrawList* draw_list, ImVec2 const& c, ImU32 color,
                         char const* str, float size)
{
    ImVec2 const dim = textSize(str, size);
    text(draw_list, ImVec2(c.x - dim.x * 0.5f, c.y - dim.y * 0.5f), color, str, size);
}

//! \brief Label in a rounded box, centered on c, so that it stays readable
//! over arcs.
static void pill(ImDrawList* draw_list, ImVec2 const& c, ImU32 color, char const* str,
                 float size)
{
    if (size <= 0.0f)
        return;
    ImVec2 const dim = textSize(str, size);
    ImVec2 const pad(3.0f, 1.0f);
    ImVec2 const a(c.x - dim.x * 0.5f - pad.x, c.y - dim.y * 0.5f - pad.y);
    ImVec2 const b(c.x + dim.x * 0.5f + pad.x, c.y + dim.y * 0.5f + pad.y);
    draw_list->AddRectFilled(a, b, palette().label_bg, 3.0f);
    text(draw_list, ImVec2(a.x + pad.x, a.y + pad.y), color, str, size);
}

//! \brief Caption under a node: centered on x, top at y, on the color of the
//! canvas so that the arcs passing under it do not hide it.
static void caption(ImDrawList* draw_list, float x, float y, ImU32 color, char const* str,
                    float size)
{
    if ((size <= 0.0f) || (str[0] == '\0'))
        return;
    ImVec2 const dim = textSize(str, size);
    ImVec2 const a(x - dim.x * 0.5f - 2.0f, y);
    draw_list->AddRectFilled(a, a + dim + ImVec2(4.0f, 0.0f), palette().label_bg, 2.0f);
    text(draw_list, ImVec2(x - dim.x * 0.5f, y), color, str, size);
}

//! \brief Number with at most 2 decimals and no useless zeros ("2", "1.5").
static std::string number(float value)
{
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.2f", double(value));
    std::string s(buffer);
    while (!s.empty() && (s.back() == '0'))
        s.pop_back();
    if (!s.empty() && (s.back() == '.'))
        s.pop_back();
    return s;
}

//! \brief Mix two colors (t = 0: a, t = 1: b).
static ImU32 mix(ImU32 a, ImU32 b, float t)
{
    ImVec4 const ca = ImGui::ColorConvertU32ToFloat4(a);
    ImVec4 const cb = ImGui::ColorConvertU32ToFloat4(b);
    return ImGui::ColorConvertFloat4ToU32(ImVec4(ca.x + (cb.x - ca.x) * t, ca.y + (cb.y - ca.y) * t,
                                                 ca.z + (cb.z - ca.z) * t, ca.w + (cb.w - ca.w) * t));
}

// ============================================================================
// Shapes of nodes
// ============================================================================

//------------------------------------------------------------------------------
ImVec2 nodeHalfSize(Node const& node, TypeOfNet type)
{
    if (node.type == Node::Type::Place)
    {
        if (type == TypeOfNet::GRAFCET)
            return ImVec2(STEP_SIZE * 0.5f, STEP_SIZE * 0.5f);
        return ImVec2(PLACE_RADIUS, PLACE_RADIUS);
    }
    if (type == TypeOfNet::GRAFCET)
        return ImVec2(GRAFCET_BAR_WIDTH * 0.5f, GRAFCET_BAR_HEIGHT * 0.5f);
    return ImVec2(TRANS_WIDTH * 0.5f, TRANS_HEIGHT * 0.5f);
}

//! \brief Position of the node, from the layout of the net.
static ImVec2 at(Net const& net, Node const& node)
{
    Point const& p = net.position(node);
    return ImVec2(p.x, p.y);
}

//! \brief Petri places are circles, the other nodes rectangles.
static bool isCircle(Node const& node, TypeOfNet type)
{
    return (node.type == Node::Type::Place) && (type != TypeOfNet::GRAFCET);
}

//! \brief Point of the border of a node in the direction of a point.
static ImVec2 border(Net const& net, Node const& node, ImVec2 const& toward, float gap = 0.0f)
{
    TypeOfNet const type = net.type();
    ImVec2 const c = at(net, node);
    ImVec2 const d = unit(toward - c);
    ImVec2 const half = nodeHalfSize(node, type);
    if (isCircle(node, type))
        return c + d * (half.x + gap);
    float const tx = (std::fabs(d.x) < 1e-4f) ? FLT_MAX : (half.x + gap) / std::fabs(d.x);
    float const ty = (std::fabs(d.y) < 1e-4f) ? FLT_MAX : (half.y + gap) / std::fabs(d.y);
    return c + d * std::min(tx, ty);
}

// ============================================================================
// Arcs
// ============================================================================

//! \brief Second transition of a timed event graph arc T -> P -> T2, or null.
static Node const* tegTarget(Net const& net, Arc const& arc)
{
    if (arc.from.type != Node::Type::Transition)
        return nullptr;
    auto const next = net.arcsOut(net.node(arc.to));
    if (next.empty())
        return nullptr;
    return &net.node(next.front().to);
}

//! \brief True if an arc goes back from b to a (drawn bent, not on top).
static bool hasReverse(Net const& net, Node const& a, Node const& b)
{
    if (net.layout().compact)
    {
        // a and b are transitions: look for b -> P -> a.
        for (Arc const& out : net.arcsOut(b))
        {
            auto const next = net.arcsOut(net.node(out.to));
            if (!next.empty() && (next.front().to == a.nodeId()))
                return true;
        }
        return false;
    }
    for (Arc const& out : net.arcsOut(b))
    {
        if (out.to == a.nodeId())
            return true;
    }
    return false;
}

//! \brief Straight or bent (when a reverse arc exists) line between two nodes.
static void petriPath(Net const& net, Node const& a, Node const& b, ArcPath& path)
{
    ImVec2 const pa = at(net, a);
    ImVec2 const pb = at(net, b);
    if (!hasReverse(net, a, b))
    {
        path.points = { border(net, a, pb, 1.0f), border(net, b, pa, ARROW_SPACING) };
        return;
    }
    // Bent to the left of the direction a -> b: the reverse arc bends to the
    // other side.
    float const d = length(pb - pa);
    ImVec2 const control = (pa + pb) * 0.5f + normal(pb - pa) * std::clamp(d * 0.2f, 14.0f, 40.0f);
    ImVec2 const p0 = border(net, a, control, 1.0f);
    ImVec2 const p2 = border(net, b, control, ARROW_SPACING);
    constexpr int SEGMENTS = 16;
    path.points.clear();
    for (int i = 0; i <= SEGMENTS; ++i)
    {
        float const t = float(i) / float(SEGMENTS);
        float const u = 1.0f - t;
        path.points.push_back(p0 * (u * u) + control * (2.0f * u * t) + p2 * (t * t));
    }
}

//! \brief Bottom and top anchors of GRAFCET nodes.
static float bottomOf(Net const& net, Node const& n) { return at(net, n).y + nodeHalfSize(n, net.type()).y; }
static float topOf(Net const& net, Node const& n) { return at(net, n).y - nodeHalfSize(n, net.type()).y; }

//! \brief GRAFCET link: vertical and horizontal lines, IEC 60848.
static void grafcetPath(Net const& net, Arc const& arc, ArcPath& path)
{
    Node const& s = net.node(arc.from);
    Node const& t = net.node(arc.to);
    ImVec2 const ps = at(net, s);
    ImVec2 const pt = at(net, t);
    path.arrow_end = false;

    // Start and end points, with the AND (double) and OR (single) lines.
    ImVec2 start(ps.x, bottomOf(net, s));
    ImVec2 end(pt.x, topOf(net, t));
    float bus_start = NAN; // y of a line the link starts from
    float bus_end = NAN;   // y of a line the link ends on
    if (s.type == Node::Type::Transition)
    {
        if (net.arcsOut(s).size() >= 2u) // AND divergence: from the double line
        {
            start = ImVec2(pt.x, ps.y + GRAFCET_GAP);
        }
        else if (net.arcsIn(t).size() >= 2u) // OR convergence: to the line above the step
        {
            bus_end = topOf(net, t) - GRAFCET_GAP;
        }
    }
    else
    {
        if (net.arcsIn(t).size() >= 2u) // AND convergence: to the double line
        {
            end = ImVec2(ps.x, pt.y - GRAFCET_GAP);
        }
        else if (net.arcsOut(s).size() >= 2u) // OR divergence: from the line below the step
        {
            bus_start = bottomOf(net, s) + GRAFCET_GAP;
        }
    }

    // Going up: around on the side of the source (right when it is on the
    // right of its target, else left), with an arrow on the way up.
    if (end.y < start.y - 1.0f)
    {
        float const left = (start.x > end.x + 1.0f) ? (std::max(start.x, end.x) + GRAFCET_LOOP)
                                                    : (std::min(start.x, end.x) - GRAFCET_LOOP);
        float const below = (std::isnan(bus_start) ? start.y : bus_start) + 12.0f;
        float const above = end.y - 12.0f;
        if (!std::isnan(bus_start))
            path.points.push_back(ImVec2(start.x, start.y));
        path.points.push_back(start);
        path.points.push_back(ImVec2(start.x, below));
        path.points.push_back(ImVec2(left, below));
        path.points.push_back(ImVec2(left, above));
        path.points.push_back(ImVec2(end.x, above));
        path.points.push_back(end);
        path.arrow_segment = int(path.points.size()) - 4; // the vertical going up
        return;
    }

    path.points.push_back(start);
    if (!std::isnan(bus_start))
    {
        path.points.push_back(ImVec2(start.x, bus_start));
        path.points.push_back(ImVec2(end.x, bus_start));
    }
    else if (!std::isnan(bus_end))
    {
        path.points.push_back(ImVec2(start.x, bus_end));
        path.points.push_back(ImVec2(end.x, bus_end));
    }
    else if (std::fabs(start.x - end.x) > 0.5f)
    {
        float const middle = (start.y + end.y) * 0.5f;
        path.points.push_back(ImVec2(start.x, middle));
        path.points.push_back(ImVec2(end.x, middle));
    }
    path.points.push_back(end);
}

//------------------------------------------------------------------------------
ArcPath arcPath(Net const& net, Arc const& arc)
{
    ArcPath path;
    if (net.type() == TypeOfNet::GRAFCET)
    {
        grafcetPath(net, arc, path);
    }
    else if (net.layout().compact)
    {
        Node const* next = tegTarget(net, arc);
        if (next != nullptr)
            petriPath(net, net.node(arc.from), *next, path);
    }
    else
    {
        petriPath(net, net.node(arc.from), net.node(arc.to), path);
        if (arc.kind == ArcKind::Inhibitor)
            path.head = ArcPath::Head::Circle;
        else if (arc.kind == ArcKind::Reset)
            path.head = ArcPath::Head::Double;
    }
    return path;
}

//! \brief Distance between a point and a segment.
static float segmentDistance(ImVec2 const& p, ImVec2 const& a, ImVec2 const& b)
{
    ImVec2 const ab = b - a;
    float const l2 = ab.x * ab.x + ab.y * ab.y;
    float t = (l2 < 1e-6f) ? 0.0f : ((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / l2;
    t = std::clamp(t, 0.0f, 1.0f);
    return length(p - (a + ab * t));
}

//------------------------------------------------------------------------------
float distanceToArc(Net const& net, Arc const& arc, ImVec2 const& point)
{
    ArcPath const path = arcPath(net, arc);
    float best = FLT_MAX;
    for (size_t i = 1u; i < path.points.size(); ++i)
        best = std::min(best, segmentDistance(point, path.points[i - 1u], path.points[i]));
    return best;
}

//! \brief Filled arrow head with its tip at `tip`, pointing along `direction`.
static void arrowHead(ImDrawList* draw_list, ImVec2 const& tip, ImVec2 const& direction,
                      ImU32 color, float zoom)
{
    ImVec2 const d = unit(direction);
    ImVec2 const n(-d.y, d.x);
    float const l = ARROW_WIDTH * zoom;
    float const w = l * 0.38f;
    ImVec2 const base = tip - d * l;
    draw_list->AddTriangleFilled(tip, base + n * w, base - n * w, color);
}

//! \brief Draw a path in screen space with its arrows.
static void drawPath(ImDrawList* draw_list, ArcPath const& path, ImVec2 const& origin,
                     float zoom, ImU32 color, float thickness)
{
    if (path.points.size() < 2u)
        return;
    std::vector<ImVec2> points;
    points.reserve(path.points.size());
    for (auto const& p : path.points)
        points.push_back(screen(origin, zoom, p));

    if (path.arrow_end && (path.head == ArcPath::Head::Circle))
    {
        // Inhibitor arc: the line ends on a small circle touching the node.
        ImVec2 const tip = points.back();
        ImVec2 const dir = unit(tip - points[points.size() - 2u]);
        float const r = ARROW_WIDTH * zoom * 0.4f;
        points.back() = tip - dir * (2.0f * r);
        draw_list->AddPolyline(points.data(), int(points.size()), color, ImDrawFlags_None, thickness);
        draw_list->AddCircle(tip - dir * r, r, color, 16, thickness);
    }
    else if (path.arrow_end)
    {
        // Stop the line under the head so that its tip stays sharp. A reset
        // arc has a second head behind the first one.
        ImVec2 const tip = points.back();
        ImVec2 const dir = tip - points[points.size() - 2u];
        points.back() = tip - unit(dir) * (ARROW_WIDTH * zoom * 0.8f);
        draw_list->AddPolyline(points.data(), int(points.size()), color, ImDrawFlags_None, thickness);
        arrowHead(draw_list, tip, dir, color, zoom);
        if (path.head == ArcPath::Head::Double)
            arrowHead(draw_list, tip - unit(dir) * (ARROW_WIDTH * zoom * 0.7f), dir, color, zoom);
    }
    else
    {
        draw_list->AddPolyline(points.data(), int(points.size()), color, ImDrawFlags_None, thickness);
    }
    if ((path.arrow_segment >= 0) && (size_t(path.arrow_segment) + 1u < points.size()))
    {
        ImVec2 const a = points[size_t(path.arrow_segment)];
        ImVec2 const b = points[size_t(path.arrow_segment) + 1u];
        arrowHead(draw_list, (a + b) * 0.5f, b - a, color, zoom);
    }
}

//! \brief Point at a fraction of the length of a polyline, and its direction.
static ImVec2 alongPath(std::vector<ImVec2> const& points, float fraction, ImVec2& direction)
{
    float total = 0.0f;
    for (size_t i = 1u; i < points.size(); ++i)
        total += length(points[i] - points[i - 1u]);
    float goal = total * fraction;
    for (size_t i = 1u; i < points.size(); ++i)
    {
        float const l = length(points[i] - points[i - 1u]);
        if ((goal <= l) || (i + 1u == points.size()))
        {
            direction = points[i] - points[i - 1u];
            float const t = (l < 1e-4f) ? 0.0f : std::min(goal / l, 1.0f);
            return points[i - 1u] + (points[i] - points[i - 1u]) * t;
        }
        goal -= l;
    }
    direction = ImVec2(1.0f, 0.0f);
    return points.empty() ? ImVec2() : points.front();
}

//------------------------------------------------------------------------------
void drawArc(ImDrawList* draw_list, Net const& net, Arc const& arc,
             ImVec2 const& origin, float const alpha, float zoom)
{
    TypeOfNet const type = net.type();
    ArcPath const path = arcPath(net, arc);
    if (path.points.size() < 2u)
        return;
    Palette const& pal = palette();
    bool const critical = (alpha < 0.0f);
    ImU32 const color = critical ? pal.critical : pal.outline;
    float const thickness = stroke(critical ? 3.0f : 1.6f, zoom);
    drawPath(draw_list, path, origin, zoom, color, thickness);

    float const size = fontSizeAt(zoom * 0.9f);
    if (net.layout().compact)
    {
        // Tokens near the start, duration in the middle.
        size_t const tokens = net.placeOf(arc).tokens;
        ImVec2 dir;
        if (tokens > 0u)
        {
            ImVec2 const p = screen(origin, zoom, alongPath(path.points, 0.22f, dir));
            if (tokens <= 3u)
            {
                ImVec2 const u = unit(dir);
                float const step = TOKEN_RADIUS * 2.6f * zoom;
                for (size_t k = 0u; k < tokens; ++k)
                    drawToken(draw_list, p.x + u.x * step * (float(k) - float(tokens - 1u) * 0.5f),
                              p.y + u.y * step * (float(k) - float(tokens - 1u) * 0.5f), zoom);
            }
            else
            {
                drawToken(draw_list, p.x, p.y, zoom);
                ImVec2 const n = normal(dir);
                pill(draw_list, p + n * (10.0f * zoom), pal.token, std::to_string(tokens).c_str(), size);
            }
        }
        if (arc.durationOrZero() != 0.0f)
        {
            ImVec2 const p = screen(origin, zoom, alongPath(path.points, 0.55f, dir));
            pill(draw_list, p + normal(dir) * (11.0f * zoom), pal.duration,
                 number(arc.durationOrZero()).c_str(), size);
        }
    }
    else if ((type == TypeOfNet::TimedPetriNet) && (arc.from.type == Node::Type::Transition) &&
             arc.duration.has_value())
    {
        ImVec2 dir;
        ImVec2 const p = screen(origin, zoom, alongPath(path.points, 0.5f, dir));
        pill(draw_list, p + normal(dir) * (11.0f * zoom), pal.duration,
             number(*arc.duration).c_str(), size);
    }

    // Weight, on the other side of the line than the duration.
    if (arc.weight != 1u)
    {
        ImVec2 dir;
        ImVec2 const p = screen(origin, zoom, alongPath(path.points, 0.5f, dir));
        pill(draw_list, p - normal(dir) * (11.0f * zoom), pal.outline,
             std::to_string(arc.weight).c_str(), size);
    }
}

//------------------------------------------------------------------------------
void drawArcHighlight(ImDrawList* draw_list, Net const& net, Arc const& arc,
                      ImVec2 const& origin, float zoom, ImU32 color, float thickness)
{
    ArcPath path = arcPath(net, arc);
    path.arrow_end = false;
    path.arrow_segment = -1;
    drawPath(draw_list, path, origin, zoom, color, thickness);
}

//------------------------------------------------------------------------------
void drawArc(ImDrawList* draw_list, ImVec2 const* from, ImVec2 const* to, ImVec2 const* click_position,
             ImVec2 const& origin, ImVec2 const& cursor, float zoom)
{
    ArcPath path;
    if (from != nullptr)
        path.points = { *from, cursor };
    else if (to != nullptr)
        path.points = { cursor, *to };
    else if (click_position != nullptr)
        path.points = { *click_position, cursor };
    else
        return;
    drawPath(draw_list, path, origin, zoom, palette().selection, stroke(2.0f, zoom));
}

// ============================================================================
// Tokens
// ============================================================================

//------------------------------------------------------------------------------
void drawToken(ImDrawList* draw_list, float const x, float const y, float zoom)
{
    draw_list->AddCircleFilled(ImVec2(x, y), std::max(1.5f, TOKEN_RADIUS * zoom), palette().token);
}

//------------------------------------------------------------------------------
void drawTimedToken(ImDrawList* draw_list, size_t tokens, float const x, float const y, float zoom)
{
    float const r = std::max(4.0f, 8.0f * zoom);
    draw_list->AddCircleFilled(ImVec2(x, y), r, palette().active);
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%zu", tokens);
    textCentered(draw_list, ImVec2(x, y), IM_COL32(0, 0, 0, 255), buffer, fontSizeAt(zoom * 0.85f));
}

//! \brief Tokens of a Petri place: dots up to 5, else a number.
static void drawPlaceTokens(ImDrawList* draw_list, size_t tokens, ImVec2 const& p, float zoom)
{
    float const d = 6.0f * zoom;
    switch (tokens)
    {
    case 0u: return;
    case 1u: drawToken(draw_list, p.x, p.y, zoom); return;
    case 2u: drawToken(draw_list, p.x - d, p.y, zoom); drawToken(draw_list, p.x + d, p.y, zoom); return;
    case 3u:
        drawToken(draw_list, p.x, p.y - d, zoom);
        drawToken(draw_list, p.x - d, p.y + d * 0.7f, zoom);
        drawToken(draw_list, p.x + d, p.y + d * 0.7f, zoom);
        return;
    case 4u: case 5u:
        drawToken(draw_list, p.x - d, p.y - d, zoom); drawToken(draw_list, p.x + d, p.y - d, zoom);
        drawToken(draw_list, p.x - d, p.y + d, zoom); drawToken(draw_list, p.x + d, p.y + d, zoom);
        if (tokens == 5u) drawToken(draw_list, p.x, p.y, zoom);
        return;
    default:
        textCentered(draw_list, p, palette().token, std::to_string(tokens).c_str(),
                     fontSizeAt(zoom * 1.1f));
        return;
    }
}

// ============================================================================
// Places and GRAFCET steps
// ============================================================================

//! \brief Identifier of a Petri node, small, above its right corner, when its
//! caption says something else.
static void identifier(ImDrawList* draw_list, Node const& node, ImVec2 const& corner, float zoom)
{
    if (node.caption.empty() || (node.caption == node.key))
        return;
    float const size = fontSizeAt(zoom * 0.75f);
    ImVec2 const dim = textSize(node.key.c_str(), size);
    text(draw_list, ImVec2(corner.x, corner.y - dim.y), palette().secondary, node.key.c_str(), size);
}

//! \brief Petri place: circle, tokens, caption below.
static void drawPetriPlace(ImDrawList* draw_list, Net const& net, Place const& place, ImVec2 const& origin,
                           bool show_caption, float zoom, bool show_identifier)
{
    Palette const& pal = palette();
    ImVec2 const p = screen(origin, zoom, at(net, place));
    float const r = PLACE_RADIUS * zoom;
    draw_list->AddCircleFilled(p + ImVec2(SHADOW_OFFSET, SHADOW_OFFSET) * zoom, r, pal.shadow, 48);
    draw_list->AddCircleFilled(p, r, pal.fill, 48);
    draw_list->AddCircle(p, r, pal.outline, 48, stroke(2.0f, zoom));
    drawPlaceTokens(draw_list, place.tokens, p, zoom);

    char const* label = show_caption ? place.caption.c_str() : place.key.c_str();
    caption(draw_list, p.x, p.y + r + 3.0f * zoom, show_caption ? pal.caption : pal.secondary,
            label, fontSizeAt(zoom));
    if (show_identifier && show_caption)
        identifier(draw_list, place, p + ImVec2(r * 0.7f, -r * 0.7f), zoom);

    // Capacity above the left corner: "≤ 4".
    if (place.capacity != std::numeric_limits<size_t>::max())
    {
        float const size = fontSizeAt(zoom * 0.75f);
        std::string const text = "\xe2\x89\xa4 " + std::to_string(place.capacity);
        ImVec2 const dim = textSize(text.c_str(), size);
        tpne::text(draw_list, ImVec2(p.x - r * 0.7f - dim.x, p.y - r * 0.7f - dim.y), pal.secondary,
                   text.c_str(), size);
    }
}

//! \brief Horizontal line from x0 to x1 (net units) at y, one or two strokes.
static void busLine(ImDrawList* draw_list, ImVec2 const& origin, float zoom,
                    float x0, float x1, float y, bool twice)
{
    Palette const& pal = palette();
    float const t = stroke(1.6f, zoom);
    float const margin = 10.0f;
    ImVec2 const a = screen(origin, zoom, ImVec2(std::min(x0, x1) - margin, y));
    ImVec2 const b = screen(origin, zoom, ImVec2(std::max(x0, x1) + margin, y));
    if (twice)
    {
        float const gap = std::max(2.0f, 2.5f * zoom);
        draw_list->AddLine(a - ImVec2(0.0f, gap), b - ImVec2(0.0f, gap), pal.outline, t);
        draw_list->AddLine(a + ImVec2(0.0f, gap), b + ImVec2(0.0f, gap), pal.outline, t);
    }
    else
    {
        draw_list->AddLine(a, b, pal.outline, t);
    }
}

//! \brief GRAFCET step: square with its number (double for the initial step),
//! a dot when active, caption on its left, OR lines of alternative sequences.
static void drawGrafcetStep(ImDrawList* draw_list, Net const& net, Place const& place, ImVec2 const& origin,
                            bool show_caption, float zoom, bool initial, bool show_identifier)
{
    Palette const& pal = palette();
    ImVec2 const c = at(net, place);
    ImVec2 const p = screen(origin, zoom, c);
    float const h = STEP_SIZE * 0.5f * zoom;
    bool const active = (place.tokens > 0u);
    ImU32 const fill = active ? mix(pal.fill, pal.active, 0.28f) : pal.fill;
    float const t = stroke(2.0f, zoom);

    ImVec2 const shadow = ImVec2(SHADOW_OFFSET, SHADOW_OFFSET) * zoom;
    draw_list->AddRectFilled(p - ImVec2(h, h) + shadow, p + ImVec2(h, h) + shadow, pal.shadow);
    draw_list->AddRectFilled(p - ImVec2(h, h), p + ImVec2(h, h), fill);
    draw_list->AddRect(p - ImVec2(h, h), p + ImVec2(h, h), active ? pal.active : pal.outline, 0.0f,
                       ImDrawFlags_None, t);
    if (initial)
    {
        float const g = 4.0f * zoom;
        draw_list->AddRect(p - ImVec2(h - g, h - g), p + ImVec2(h - g, h - g),
                           active ? pal.active : pal.outline, 0.0f, ImDrawFlags_None, stroke(1.2f, zoom));
    }

    // Step number inside; under it the caption, small and cut to the square
    // (the whole caption is in the tooltip); a dot in the corner when active.
    // Without the identifiers: the caption alone, larger.
    bool const caption = show_caption && !place.caption.empty();
    bool const number = show_identifier || !caption;
    float const size = fontSizeAt(zoom * 1.1f);
    float const small = fontSizeAt(zoom * (number ? 0.62f : 0.8f));
    if (number)
    {
        textCentered(draw_list, p - ImVec2(0.0f, (caption && (small > 0.0f)) ? 5.0f * zoom : 0.0f),
                     pal.caption, std::to_string(place.id).c_str(), size);
    }
    if (caption && (small > 0.0f))
    {
        // Cut on a character boundary (UTF-8: not inside a multi-byte one).
        std::string label = place.caption;
        float const room = 2.0f * h - 6.0f * zoom;
        auto const shorten = [&label]() {
            label.pop_back();
            while (!label.empty() && ((static_cast<unsigned char>(label.back()) & 0xC0u) == 0x80u))
                label.pop_back();
            if (!label.empty() && (static_cast<unsigned char>(label.back()) >= 0xC0u))
                label.pop_back();
        };
        while ((label.size() > 1u) && (textSize(label.c_str(), small).x > room))
            shorten();
        if (label.size() < place.caption.size())
        {
            if (label.size() > 1u)
                shorten();
            label += ".";
        }
        textCentered(draw_list, p + ImVec2(0.0f, number ? 9.0f * zoom : 0.0f),
                     number ? pal.secondary : pal.caption, label.c_str(), small);
    }
    if (active)
        drawToken(draw_list, p.x + h * 0.62f, p.y - h * 0.62f, zoom);

    // OR divergence (below) and OR convergence (above): single lines.
    if (net.arcsOut(place).size() >= 2u)
    {
        float x0 = c.x, x1 = c.x;
        for (Arc const& a : net.arcsOut(place)) { x0 = std::min(x0, net.position(a.to).x); x1 = std::max(x1, net.position(a.to).x); }
        float const y = c.y + STEP_SIZE * 0.5f + GRAFCET_GAP;
        busLine(draw_list, origin, zoom, x0, x1, y, false);
        draw_list->AddLine(screen(origin, zoom, ImVec2(c.x, c.y + STEP_SIZE * 0.5f)),
                           screen(origin, zoom, ImVec2(c.x, y)), pal.outline, stroke(1.6f, zoom));
    }
    if (net.arcsIn(place).size() >= 2u)
    {
        float x0 = c.x, x1 = c.x;
        for (Arc const& a : net.arcsIn(place)) { x0 = std::min(x0, net.position(a.from).x); x1 = std::max(x1, net.position(a.from).x); }
        float const y = c.y - STEP_SIZE * 0.5f - GRAFCET_GAP;
        busLine(draw_list, origin, zoom, x0, x1, y, false);
        draw_list->AddLine(screen(origin, zoom, ImVec2(c.x, y)),
                           screen(origin, zoom, ImVec2(c.x, c.y - STEP_SIZE * 0.5f)),
                           pal.outline, stroke(1.6f, zoom));
    }
}

//------------------------------------------------------------------------------
void drawPlace(ImDrawList* draw_list, Net const& net, Place const& place,
               ImVec2 const& origin, bool const show_caption, float const /*alpha*/,
               float zoom, bool isInitialStep, bool show_identifier)
{
    TypeOfNet const type = net.type();
    if (net.layout().compact)
        return; // places are drawn on their arcs
    if (type == TypeOfNet::GRAFCET)
        drawGrafcetStep(draw_list, net, place, origin, show_caption, zoom, isInitialStep, show_identifier);
    else
        drawPetriPlace(draw_list, net, place, origin, show_caption, zoom, show_identifier);
}

// ============================================================================
// Transitions
// ============================================================================

//------------------------------------------------------------------------------
void drawTransition(ImDrawList* draw_list, Net const& net, Transition const& transition,
                    ImVec2 const& origin, bool const show_caption, float const /*alpha*/,
                    float zoom, bool const show_identifier)
{
    TypeOfNet const type = net.type();
    Palette const& pal = palette();
    ImVec2 const c = at(net, transition);
    ImVec2 const p = screen(origin, zoom, c);
    ImVec2 const half = nodeHalfSize(transition, type) * zoom;

    if (type == TypeOfNet::GRAFCET)
    {
        // A thick line: green when it fires (enabled and true), orange when
        // only its receptivity is true.
        bool const enabled = net.isEnabled(transition);
        ImU32 color = pal.outline;
        if (transition.receptivity)
            color = enabled ? pal.active : pal.enabled;
        draw_list->AddRectFilled(p - half, p + half, color);

        // AND divergence (below) and AND convergence (above): double lines.
        if (net.arcsOut(transition).size() >= 2u)
        {
            float x0 = c.x, x1 = c.x;
            for (Arc const& a : net.arcsOut(transition)) { x0 = std::min(x0, net.position(a.to).x); x1 = std::max(x1, net.position(a.to).x); }
            float const y = c.y + GRAFCET_GAP;
            busLine(draw_list, origin, zoom, x0, x1, y, true);
            draw_list->AddLine(p, screen(origin, zoom, ImVec2(c.x, y)), pal.outline, stroke(1.6f, zoom));
        }
        if (net.arcsIn(transition).size() >= 2u)
        {
            float x0 = c.x, x1 = c.x;
            for (Arc const& a : net.arcsIn(transition)) { x0 = std::min(x0, net.position(a.from).x); x1 = std::max(x1, net.position(a.from).x); }
            float const y = c.y - GRAFCET_GAP;
            busLine(draw_list, origin, zoom, x0, x1, y, true);
            draw_list->AddLine(screen(origin, zoom, ImVec2(c.x, y)), p, pal.outline, stroke(1.6f, zoom));
        }

        // Receptivity on the right, number on the left.
        float const size = fontSizeAt(zoom);
        char const* label = show_caption ? transition.caption.c_str() : transition.key.c_str();
        ImVec2 const dim = textSize(label, size);
        text(draw_list, ImVec2(p.x + half.x + 6.0f * zoom, p.y - dim.y * 0.5f), pal.caption, label, size);
        if (transition.delay > 0.0f)
        {
            std::string const delay = "  (" + number(transition.delay) + " s)";
            text(draw_list, ImVec2(p.x + half.x + 6.0f * zoom + dim.x, p.y - dim.y * 0.5f),
                 pal.duration, delay.c_str(), size);
        }
        if (show_identifier)
        {
            float const small = fontSizeAt(zoom * 0.8f);
            std::string const id = "(" + std::to_string(transition.id) + ")";
            ImVec2 const idim = textSize(id.c_str(), small);
            text(draw_list, ImVec2(p.x - half.x - 4.0f * zoom - idim.x, p.y - idim.y * 0.5f),
                 pal.secondary, id.c_str(), small);
        }
        return;
    }

    // Petri nets: a rounded bar, colored by its state.
    ImU32 fill = pal.fill;
    if (net.canFire(transition))
        fill = pal.active;
    else if (net.isEnabled(transition))
        fill = (type == TypeOfNet::PetriNet) ? pal.active : pal.enabled;
    else if (transition.receptivity && (type == TypeOfNet::PetriNet))
        fill = pal.enabled;
    float const rounding = NODE_ROUNDING * zoom;
    ImVec2 const shadow = ImVec2(SHADOW_OFFSET, SHADOW_OFFSET) * zoom;
    draw_list->AddRectFilled(p - half + shadow, p + half + shadow, pal.shadow, rounding);
    draw_list->AddRectFilled(p - half, p + half, fill, rounding);
    draw_list->AddRect(p - half, p + half, pal.outline, rounding, ImDrawFlags_None, stroke(2.0f, zoom));

    char const* label = show_caption ? transition.caption.c_str() : transition.key.c_str();
    caption(draw_list, p.x, p.y + half.y + 3.0f * zoom, show_caption ? pal.caption : pal.secondary,
            label, fontSizeAt(zoom));
    if (show_identifier && show_caption)
        identifier(draw_list, transition, p + ImVec2(half.x + 2.0f * zoom, -half.y), zoom);
}

// ============================================================================
// Outlines (hover, selection)
// ============================================================================

//------------------------------------------------------------------------------
void drawNodeOutline(ImDrawList* draw_list, Net const& net, Node const& node,
                     ImVec2 const& origin, float zoom, ImU32 color, float margin,
                     float thickness)
{
    TypeOfNet const type = net.type();
    ImVec2 const p = screen(origin, zoom, at(net, node));
    ImVec2 const half = nodeHalfSize(node, type) * zoom + ImVec2(margin, margin);
    if (isCircle(node, type))
        draw_list->AddCircle(p, half.x, color, 48, thickness);
    else
        draw_list->AddRect(p - half, p + half, color, 4.0f, ImDrawFlags_None, thickness);
}

// ============================================================================
// GRAFCET actions
// ============================================================================

//------------------------------------------------------------------------------
void drawStepActions(ImDrawList* draw_list, Net const& net, Place const& place, ImVec2 const& origin,
                     float zoom, bool (*is_active)(size_t index, void* data), void* data,
                     bool compact)
{
    if (place.actions.empty())
        return;

    Palette const& pal = palette();
    ImVec2 const p = screen(origin, zoom, at(net, place));
    float const half = STEP_SIZE * 0.5f * zoom;
    float const t = stroke(1.4f, zoom);
    bool any_active = false;
    for (size_t i = 0u; i < place.actions.size(); ++i)
        any_active = any_active || ((is_active != nullptr) && is_active(i, data));

    // Compact: a small tag "3 ▸" on the right of the step.
    if (compact)
    {
        float const size = fontSizeAt(zoom * 0.75f);
        std::string const label = std::to_string(place.actions.size());
        ImVec2 const dim = textSize(label.c_str(), size);
        float const pad = 3.0f * zoom;
        ImVec2 const a(p.x + half + 6.0f * zoom, p.y - dim.y * 0.5f - pad);
        ImVec2 const b(a.x + dim.x + 2.0f * pad, p.y + dim.y * 0.5f + pad);
        draw_list->AddLine(ImVec2(p.x + half, p.y), ImVec2(a.x, p.y), pal.outline, t);
        draw_list->AddRectFilled(a, b, any_active ? mix(pal.fill, pal.active, 0.55f) : pal.fill, 2.0f * zoom);
        draw_list->AddRect(a, b, pal.outline, 2.0f * zoom, ImDrawFlags_None, t);
        text(draw_list, ImVec2(a.x + pad, a.y + pad), pal.caption, label.c_str(), size);
        return;
    }

    // One row per action, one row per forcing order (IEC 60848: in a double
    // frame, without qualifier).
    struct Row { size_t action; Forcing const* forcing; };
    std::vector<Row> rows;
    for (size_t i = 0u; i < place.actions.size(); ++i)
    {
        Action const& action = place.actions[i];
        if (!action.name.empty() || action.forcings.empty())
            rows.push_back({ i, nullptr });
        for (Forcing const& forcing : action.forcings)
            rows.push_back({ i, &forcing });
    }

    float const gap = 14.0f * zoom;
    float const size = fontSizeAt(zoom * 0.9f);
    float const padding = 5.0f * zoom;
    float const row = std::max(STEP_SIZE * zoom / float(std::max<size_t>(rows.size(), 1u)),
                               (size > 0.0f ? size : 6.0f) + 2.0f * padding * 0.6f);

    // Width of the qualifier column and of the text column.
    float qualifier_width = 0.0f;
    float name_width = 0.0f;
    for (Row const& r : rows)
    {
        Action const& action = place.actions[r.action];
        if (r.forcing != nullptr)
        {
            name_width = std::max(name_width, textSize(forcingLabel(*r.forcing).c_str(), size).x);
            continue;
        }
        qualifier_width = std::max(qualifier_width, textSize(qualifierToStr(action.qualifier), size).x);
        std::string label = action.name;
        if (!action.condition.empty())
            label = "if " + action.condition + ": " + label;
        if (action.duration > 0.0f)
            label += "  " + number(action.duration) + " s";
        name_width = std::max(name_width, textSize(label.c_str(), size).x);
    }
    qualifier_width += 2.0f * padding;
    name_width += 2.0f * padding;
    float const width = qualifier_width + name_width;

    // The boxes are as high as the step (IEC 60848), centered on it.
    float const total = row * float(rows.size());
    float const x0 = p.x + half + gap;
    float const y0 = p.y - total * 0.5f;
    draw_list->AddLine(ImVec2(p.x + half, p.y), ImVec2(x0, p.y), pal.outline, t);

    for (size_t i = 0u; i < rows.size(); ++i)
    {
        Action const& action = place.actions[rows[i].action];
        float const y = y0 + float(i) * row;
        ImVec2 const a(x0, y);
        ImVec2 const b(x0 + width, y + row);
        bool const active = (is_active != nullptr) && is_active(rows[i].action, data);
        draw_list->AddRectFilled(a, b, active ? mix(pal.fill, pal.active, 0.55f) : pal.fill);
        draw_list->AddRect(a, b, pal.outline, 0.0f, ImDrawFlags_None, t);
        float const ty = y + (row - size) * 0.5f;

        if (rows[i].forcing != nullptr)
        {
            float const g = std::max(2.0f, 2.5f * zoom);
            draw_list->AddRect(a + ImVec2(g, g), b - ImVec2(g, g), pal.outline, 0.0f, ImDrawFlags_None, t);
            std::string const label = forcingLabel(*rows[i].forcing);
            text(draw_list, ImVec2(x0 + (width - textSize(label.c_str(), size).x) * 0.5f, ty),
                 pal.caption, label.c_str(), size);
            continue;
        }

        draw_list->AddLine(ImVec2(x0 + qualifier_width, y), ImVec2(x0 + qualifier_width, y + row),
                           pal.outline, t);
        text(draw_list, ImVec2(x0 + padding, ty), pal.secondary, qualifierToStr(action.qualifier), size);
        // Conditional action: "if Dr: Motor" (the condition dimmed).
        float x = x0 + qualifier_width + padding;
        if (!action.condition.empty())
        {
            std::string const condition = "if " + action.condition + ": ";
            text(draw_list, ImVec2(x, ty), pal.secondary, condition.c_str(), size);
            x += textSize(condition.c_str(), size).x;
        }
        text(draw_list, ImVec2(x, ty), pal.caption, action.name.c_str(), size);
        x += textSize(action.name.c_str(), size).x;
        if (action.duration > 0.0f)
        {
            std::string const duration = "  " + number(action.duration) + " s";
            text(draw_list, ImVec2(x, ty), pal.duration, duration.c_str(), size);
        }
    }
}

// ============================================================================
// Picking
// ============================================================================

//! \brief First node of the container closer than the radius to the position.
template<class Nodes>
static auto closest(Net const& net, Nodes& nodes, ImVec2 const& position, float const radius)
    -> decltype(&nodes[0])
{
    float const r2 = radius * radius;
    for (auto& node : nodes)
    {
        ImVec2 const p = at(net, node);
        float const dx = p.x - position.x;
        float const dy = p.y - position.y;
        if (dx * dx + dy * dy < r2)
            return &node;
    }
    return nullptr;
}

//------------------------------------------------------------------------------
Place* placeAt(Net& net, ImVec2 const& position, float const radius)
{
    return closest(net, net.places(), position, radius);
}

//------------------------------------------------------------------------------
Transition* transitionAt(Net& net, ImVec2 const& position, float const radius)
{
    return closest(net, net.transitions(), position, radius);
}

//------------------------------------------------------------------------------
Node* nodeAt(Net& net, ImVec2 const& position, float const place_radius,
             float const transition_radius)
{
    Node* node = transitionAt(net, position, transition_radius);
    if ((node != nullptr) || net.layout().compact)
        return node;
    return placeAt(net, position, place_radius);
}

} // namespace tpne
