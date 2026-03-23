//=============================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2026 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of TimedPetriNetEditor.
//
// TimedPetriNetEditor is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GNU Emacs.  If not, see <http://www.gnu.org/licenses/>.
//=============================================================================

#include "Editor/Drawable.hpp"

#include "implot/implot.h"

#include <iomanip> // std::setprecision

namespace tpne {

#define FILL_COLOR(alpha)      ((ThemeId::Light == theme()) ? LIGHT_THEME_FILL_COLOR(alpha) : DARK_THEME_FILL_COLOR(alpha))
#define OUTLINE_COLOR          ((ThemeId::Light == theme()) ? LIGHT_THEME_OUTLINE_COLOR : DARK_THEME_OUTLINE_COLOR)
#define CAPTION_COLOR          ((ThemeId::Light == theme()) ? LIGHT_THEME_CAPTION_COLOR : DARK_THEME_CAPTION_COLOR)
#define DURATION_COLOR         ((ThemeId::Light == theme()) ? LIGHT_THEME_DURATION_COLOR : DARK_THEME_DURATION_COLOR)
#define TOKEN_COLOR            ((ThemeId::Light == theme()) ? LIGHT_THEME_TOKEN_COLOR : DARK_THEME_TOKEN_COLOR)
#define CRITICAL_COLOR         ((ThemeId::Light == theme()) ? LIGHT_THEME_CRITICAL_COLOR : DARK_THEME_CRITICAL_COLOR)
#define TRANS_FIREABLE_COLOR   ((ThemeId::Light == theme()) ? LIGHT_THEME_TRANS_FIREABLE_COLOR : DARK_THEME_TRANS_FIREABLE_COLOR)
#define TRANS_VALIDATED_COLOR  ((ThemeId::Light == theme()) ? LIGHT_THEME_TRANS_VALIDATED_COLOR : DARK_THEME_TRANS_VALIDATED_COLOR)
#define TRANS_ENABLED_COLOR    ((ThemeId::Light == theme()) ? LIGHT_THEME_TRANS_ENABLED_COLOR : DARK_THEME_TRANS_ENABLED_COLOR)

//------------------------------------------------------------------------------
//! \brief Rotate a 2D vector by angle (given as cos/sin).
//------------------------------------------------------------------------------
template<typename T>
static T rotate(T const& v, float const cos_a, float const sin_a)
{
    return T(v.x * cos_a - v.y * sin_a, v.x * sin_a + v.y * cos_a);
}

//------------------------------------------------------------------------------
//! \brief Compute the Euclidean distance between two points with .x and .y members.
//------------------------------------------------------------------------------
template<typename T>
static inline float norm(T const& A, T const& B)
{
    return sqrtf((B.x - A.x) * (B.x - A.x) + (B.y - A.y) * (B.y - A.y));
}

//------------------------------------------------------------------------------
static void drawArrow(ImDrawList* draw_list, ImVec2 const& A, ImVec2 const& B,
                      const ImU32 color, float zoom = 1.0f)
{
    constexpr float pi = 3.14159265358979323846f;

    const float place_radius = PLACE_RADIUS * zoom;
    const float arrow_spacing = ARROW_SPACING * zoom;
    const float arrow_length = ARROW_WIDTH * zoom;
    const float arrow_half_width = arrow_length * 0.35f;
    const float line_thickness = 2.5f * zoom;

    // Orientation
    const float arrowAngle = std::atan((B.y - A.y) / (B.x - A.x))
        + ((B.x < A.x) ? pi : ((B.y < A.y) ? (2.0f * pi) : 0.0f));
    const float cos_a = std::cos(arrowAngle);
    const float sin_a = std::sin(arrowAngle);

    // Tail of the arrow.
    const float length = norm(A, B);
    if (length < 0.001f) return;

    float r = length - place_radius - arrow_spacing;
    float dx = ((B.x - A.x) * r) / length;
    float dy = ((B.y - A.y) * r) / length;
    const ImVec2 from(B.x - dx, B.y - dy);
    const ImVec2 to(A.x + dx, A.y + dy);

    // Reduce the head size to avoid overlapping the line and head of the arrow.
    r = length - place_radius - arrow_length - arrow_spacing;
    dx = ((B.x - A.x) * r) / length;
    dy = ((B.y - A.y) * r) / length;
    const ImVec2 to2(A.x + dx, A.y + dy);
    draw_list->AddLine(from, to2, color, line_thickness);

    // Head of the arrow - sleeker design
    const ImVec2 head(-arrow_length, -arrow_half_width);
    std::vector<ImVec2> points = {
        to + rotate(head + ImVec2(0.0f, 0.0f), cos_a, sin_a),
        to + rotate(head + ImVec2(arrow_length, arrow_half_width), cos_a, sin_a),
        to + rotate(head + ImVec2(0.0f, arrow_half_width * 2.0f), cos_a, sin_a)
    };
    draw_list->AddConvexPolyFilled(points.data(), points.size(), color);
}

//------------------------------------------------------------------------------
void drawArc(ImDrawList* draw_list, Node* from, Node* to, ImVec2* click_position,
             ImVec2 const& origin, ImVec2 const& cursor, float zoom)
{
    if (from != nullptr)
    {
        drawArrow(draw_list,
                  origin + ImVec2(from->x * zoom, from->y * zoom),
                  origin + ImVec2(cursor.x * zoom, cursor.y * zoom),
                  OUTLINE_COLOR, zoom);
    }
    else if (to != nullptr)
    {
        drawArrow(draw_list,
                  origin + ImVec2(cursor.x * zoom, cursor.y * zoom),
                  origin + ImVec2(to->x * zoom, to->y * zoom),
                  OUTLINE_COLOR, zoom);
    }
    else if (click_position != nullptr)
    {
        drawArrow(draw_list,
                  origin + ImVec2(click_position->x * zoom, click_position->y * zoom),
                  origin + ImVec2(cursor.x * zoom, cursor.y * zoom),
                  OUTLINE_COLOR, zoom);
    }
}

//------------------------------------------------------------------------------
void drawArc(ImDrawList* draw_list, Arc const& arc, TypeOfNet const type,
             ImVec2 const& origin, float const alpha, float zoom)
{
    ImU32 color;

    if (alpha >= 0.0f)
    {
        color = OUTLINE_COLOR; // FIXME FILL_COLOR(alpha);
    }
    else
    {
        color = CRITICAL_COLOR;
    }

    if (type == TypeOfNet::TimedEventGraph)
    {
        if (arc.from.type == Node::Type::Place)
            return ;

        assert((arc.to.arcsOut.size() == 1u) && "malformed graph event");
        Node& next = arc.to.arcsOut[0]->to;
        drawArrow(draw_list,
                  origin + ImVec2(arc.from.x * zoom, arc.from.y * zoom),
                  origin + ImVec2(next.x * zoom, next.y * zoom), color, zoom);

        float x = origin.x + (arc.from.x + (next.x - arc.from.x) / 2.0f) * zoom;
        float y = origin.y + (arc.from.y + (next.y - arc.from.y) / 2.0f) * zoom;
        std::stringstream stream;
        stream << std::fixed << std::setprecision(2) << arc.duration;
        draw_list->AddText(ImVec2(x, y + 15.0f * zoom), DURATION_COLOR, stream.str().c_str());
        drawTimedToken(draw_list, reinterpret_cast<Place&>(arc.to).tokens, x, y, zoom);
    }
    else
    {
        drawArrow(draw_list,
                  origin + ImVec2(arc.from.x * zoom, arc.from.y * zoom),
                  origin + ImVec2(arc.to.x * zoom, arc.to.y * zoom), color, zoom);

        if ((arc.from.type == Node::Type::Transition) && (type == TypeOfNet::TimedPetriNet))
        {
            float x = origin.x + (arc.from.x + (arc.to.x - arc.from.x) / 2.0f) * zoom;
            float y = origin.y + (arc.from.y + (arc.to.y - arc.from.y) / 2.0f - 15.0f) * zoom;
            std::stringstream stream;
            stream << std::fixed << std::setprecision(1) << arc.duration;
            draw_list->AddText(ImVec2(x, y), DURATION_COLOR, stream.str().c_str());
        }
    }
}

//------------------------------------------------------------------------------
void drawToken(ImDrawList* draw_list, float const x, float const y, float zoom)
{
    draw_list->AddCircleFilled(ImVec2(x, y), TOKEN_RADIUS * zoom, TOKEN_COLOR);
}

//------------------------------------------------------------------------------
void drawTimedToken(ImDrawList* draw_list, size_t tokens, float const x, float const y, float zoom)
{
    draw_list->AddCircleFilled(ImVec2(x, y), TOKEN_RADIUS * zoom, TOKEN_COLOR);
    draw_list->AddText(ImVec2(x, y), CAPTION_COLOR, std::to_string(tokens).c_str());
}

//------------------------------------------------------------------------------
static void drawPetriPlace(ImDrawList* draw_list, Place const& place, ImVec2 const& origin,
                           bool const show_caption, float const alpha, float zoom)
{
    const ImVec2 p = origin + ImVec2(place.x * zoom, place.y * zoom);
    const float place_radius = PLACE_RADIUS * zoom;
    const float token_radius = TOKEN_RADIUS * zoom;
    const float outline_thickness = 2.5f * zoom;
    const float shadow_offset = SHADOW_OFFSET * zoom;

    // Shadow
    ImVec2 shadow_p(p.x + shadow_offset, p.y + shadow_offset);
    draw_list->AddCircleFilled(shadow_p, place_radius, IM_COL32(0, 0, 0, 40), 64);

    // Draw the place as circle
    if (place.tokens == 0u)
        draw_list->AddCircleFilled(p, place_radius, FILL_COLOR(alpha), 64);
    else
        draw_list->AddCircleFilled(p, place_radius, FILL_COLOR(255), 64);
    draw_list->AddCircle(p, place_radius, OUTLINE_COLOR, 64, outline_thickness);

    // Draw the caption
    const char* text = show_caption ? place.caption.c_str() : place.key.c_str();
    ImVec2 dim = ImGui::CalcTextSize(text);
    ImVec2 ptext = p - ImVec2(dim.x / 2.0f, place_radius + dim.y);
    draw_list->AddText(ptext, CAPTION_COLOR, text);

    // Draw the number of tokens
    if (place.tokens == 0u)
        return ;

    float r = token_radius;
    float d = token_radius + 1.0f * zoom;

    if (place.tokens == 1u)
    {
        drawToken(draw_list, p.x, p.y, zoom);
    }
    else if (place.tokens == 2u)
    {
        drawToken(draw_list, p.x - d, p.y, zoom);
        drawToken(draw_list, p.x + d, p.y, zoom);
    }
    else if (place.tokens == 3u)
    {
        drawToken(draw_list, p.x, p.y - r, zoom);
        drawToken(draw_list, p.x - d, p.y + d, zoom);
        drawToken(draw_list, p.x + d, p.y + d, zoom);
    }
    else if ((place.tokens == 4u) || (place.tokens == 5u))
    {
        if (place.tokens == 5u)
        {
            d = r + 3.0f * zoom;
            drawToken(draw_list, p.x, p.y, zoom);
        }

        drawToken(draw_list, p.x - d, p.y - d, zoom);
        drawToken(draw_list, p.x + d, p.y - d, zoom);
        drawToken(draw_list, p.x - d, p.y + d, zoom);
        drawToken(draw_list, p.x + d, p.y + d, zoom);
    }
    else
    {
        drawToken(draw_list, p.x, p.y, zoom);
        std::string tokens = std::to_string(place.tokens);
        draw_list->AddText(ImVec2(p.x, p.y), CAPTION_COLOR, tokens.c_str());
    }
}

//------------------------------------------------------------------------------
static void drawGrafcetPlace(ImDrawList* draw_list, Place const& place, ImVec2 const& origin,
                              float const alpha, float zoom)
{
    const ImVec2 p = origin + ImVec2(place.x * zoom, place.y * zoom);
    const float trans_width = TRANS_WIDTH * zoom;
    const float trans_width2 = TRANS_WIDTH2 * zoom;
    const float outline_thickness = 2.5f * zoom;
    const float shadow_offset = SHADOW_OFFSET * zoom;

    // Draw the step (place) as square. Double square for initial steps.
    if (place.tokens != 0u)
    {
        // Shadow for outer square
        const ImVec2 shadow_min(p.x - trans_width2 / 2.0f + shadow_offset, p.y - trans_width2 / 2.0f + shadow_offset);
        const ImVec2 shadow_max(p.x + trans_width2 / 2.0f + shadow_offset, p.y + trans_width2 / 2.0f + shadow_offset);
        draw_list->AddRectFilled(shadow_min, shadow_max, IM_COL32(0, 0, 0, 40));

        // Outer square
        const ImVec2 pmin(p.x - trans_width2 / 2.0f, p.y - trans_width2 / 2.0f);
        const ImVec2 pmax(p.x + trans_width2 / 2.0f, p.y + trans_width2 / 2.0f);
        draw_list->AddRectFilled(pmin, pmax, FILL_COLOR(alpha));
        draw_list->AddRect(pmin, pmax, OUTLINE_COLOR, 0.0f, ImDrawFlags_None, outline_thickness);

        // Token
        drawToken(draw_list, p.x, p.y + trans_width * 1.0f / 3.0f, zoom);
    }
    else
    {
        // Shadow for inner square (only when not initial step)
        const ImVec2 shadow_min(p.x - trans_width / 2.0f + shadow_offset, p.y - trans_width / 2.0f + shadow_offset);
        const ImVec2 shadow_max(p.x + trans_width / 2.0f + shadow_offset, p.y + trans_width / 2.0f + shadow_offset);
        draw_list->AddRectFilled(shadow_min, shadow_max, IM_COL32(0, 0, 0, 40));
    }

    // Inner square
    const ImVec2 pmin(p.x - trans_width / 2.0f, p.y - trans_width / 2.0f);
    const ImVec2 pmax(p.x + trans_width / 2.0f, p.y + trans_width / 2.0f);
    draw_list->AddRectFilled(pmin, pmax, FILL_COLOR(alpha));
    draw_list->AddRect(pmin, pmax, OUTLINE_COLOR, 0.0f, ImDrawFlags_None, outline_thickness);

    // Draw the caption inside the square
    const char* text = place.caption.c_str();
    ImVec2 dim = ImGui::CalcTextSize(text) / 2.0f;
    ImVec2 ptext = p - dim + ImVec2(0.0f, -trans_width / 3.0f + 5.0f * zoom);
    draw_list->AddText(ptext, CAPTION_COLOR, text);
}

//------------------------------------------------------------------------------
void drawPlace(ImDrawList* draw_list, Place const& place, TypeOfNet const type,
               ImVec2 const& origin, bool const show_caption, float const alpha, float zoom)
{
    if (type == TypeOfNet::TimedEventGraph)
        return ;

    if (type == TypeOfNet::GRAFCET)
    {
       drawGrafcetPlace(draw_list, place, origin, alpha, zoom);
    }
    else
    {
       drawPetriPlace(draw_list, place, origin, show_caption, alpha, zoom);
    }
}

//------------------------------------------------------------------------------
void drawTransition(ImDrawList* draw_list, Transition const& transition,
                    TypeOfNet const type, ImVec2 const& origin,
                    bool const show_caption, float const alpha, float zoom)
{
    const ImVec2 p = origin + ImVec2(transition.x * zoom, transition.y * zoom);
    const float trans_width = TRANS_WIDTH * zoom;
    const float trans_height = TRANS_HEIGHT * zoom;
    const float outline_thickness = 2.5f * zoom;
    const float rounding = NODE_ROUNDING * zoom;
    const float shadow_offset = SHADOW_OFFSET * zoom;

    // Color of the transition: green if validated else yellow if enabled
    ImU32 color = FILL_COLOR(alpha);
    if (transition.receptivity)
    {
        if ((type != TypeOfNet::TimedPetriNet) && (type != TypeOfNet::TimedEventGraph))
            color = TRANS_ENABLED_COLOR;
    }
    if (transition.canFire())
    {
        color = TRANS_FIREABLE_COLOR;
    }
    else if (transition.isEnabled())
    {
        color = (type == TypeOfNet::PetriNet)
            ? TRANS_FIREABLE_COLOR : TRANS_ENABLED_COLOR;
    }

    // Draw the transition
    const ImVec2 pmin(p.x - trans_width / 2.0f, p.y - trans_height / 2.0f);
    const ImVec2 pmax(p.x + trans_width / 2.0f, p.y + trans_height / 2.0f);

    // Shadow
    const ImVec2 shadow_min(pmin.x + shadow_offset, pmin.y + shadow_offset);
    const ImVec2 shadow_max(pmax.x + shadow_offset, pmax.y + shadow_offset);
    draw_list->AddRectFilled(shadow_min, shadow_max, IM_COL32(0, 0, 0, 40), rounding);

    // Main rectangle with rounding
    draw_list->AddRectFilled(pmin, pmax, color, rounding);
    draw_list->AddRect(pmin, pmax, OUTLINE_COLOR, rounding, ImDrawFlags_None, outline_thickness);

    // Draw the caption
    if (type == TypeOfNet::GRAFCET)
    {
        const char* text = transition.caption.c_str();
        ImVec2 dim = ImGui::CalcTextSize(text) / 2.0f;
        draw_list->AddText(p + ImVec2(dim.x, -dim.y) + ImVec2(trans_width / 2.0f, 0.0f), CAPTION_COLOR, text);
    }
    else
    {
        const char* text = show_caption ? transition.caption.c_str() : transition.key.c_str();
        ImVec2 dim = ImGui::CalcTextSize(text);
        ImVec2 ptext = p - ImVec2(dim.x / 2.0f, trans_height / 2.0f + dim.y);
        draw_list->AddText(ptext, CAPTION_COLOR, text);
    }
}

//------------------------------------------------------------------------------
void drawPlot(const char* title, const char* label, std::vector<float> const& x,
    std::vector<float> const& y)
{
    if (ImPlot::BeginPlot(title))
    {
        ImPlot::PlotLine(label, x.data(), y.data(), x.size());
        ImPlot::EndPlot();
    }
}

} // namespace tpne
