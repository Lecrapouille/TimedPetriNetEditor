//=============================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2023 Quentin Quadrat <lecrapouille@gmail.com>
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

#include "Editor/DearImGui/Drawable.hpp"
#include "Editor/DearImGui/DearUtils.hpp"
#include "Utils/Utils.hpp"
#include <iomanip> // std::setprecision

namespace tpne {

#define FILL_COLOR(alpha) ImGui::GetColorU32(ImGuiCol_FrameBg, alpha)
#define OUTLINE_COLOR     ImGui::GetColorU32(ImGuiCol_FrameBgActive)
#define CAPTION_COLOR     ImGui::GetColorU32(ImGuiCol_Text)
#define DURATION_COLOR    ImGui::GetColorU32(ImGuiCol_FrameBgActive)
#define TOKEN_COLOR       ImGui::GetColorU32(ImGuiCol_Text)
#define CRITICAL_COLOR    ImGui::GetColorU32(ImGuiCol_PlotLinesHovered)
#define TRANS_FIREABLE_COLOR    IM_COL32(0, 255, 0, 255)
#define TRANS_VALIDATED_COLOR   IM_COL32(205, 205, 60, 255)
#define TRANS_ENABLED_COLOR     IM_COL32(205, 205, 60, 255)

//------------------------------------------------------------------------------
static void drawArrow(ImDrawList* draw_list, ImVec2 const& A, ImVec2 const& B,
                      const ImU32 color)
{
    constexpr float pi = 3.14159265358979323846f;

    // Orientation
    const float arrowAngle = std::atan((B.y - A.y) / (B.x - A.x))
        + ((B.x < A.x) ? pi : ((B.y < A.y) ? (2.0f * pi) : 0.0f));
    const float cos_a = std::cos(arrowAngle);
    const float sin_a = std::sin(arrowAngle);

    // Tail of the arrow.
    // Reduce the arrow magnitude to avoid entering in the place and having
    // a mush of pixels when multiple arrows are pointing on the same
    // position. To get full scaled arrow comment this block of code and
    // uncomment A.x, B.x, A.y, B.y and tailSize.
    const float length = norm(A, B);
    float r = length - PLACE_RADIUS - ARROW_SPACING;
    float dx = ((B.x - A.x) * r) / length;
    float dy = ((B.y - A.y) * r) / length;
    const ImVec2 from(B.x - dx, B.y - dy);
    const ImVec2 to(A.x + dx, A.y + dy);

    // Reduce the head size to avoid overlapping the line and head of the
    // arrow. With the transparency this is noticable.
    r = length - PLACE_RADIUS - ARROW_WIDTH - ARROW_SPACING;
    dx = ((B.x - A.x) * r) / length;
    dy = ((B.y - A.y) * r) / length;
    const ImVec2 to2(A.x + dx, A.y + dy);
    draw_list->AddLine(from, to2, color, 2.0f);

    // Head of the arrow
    const ImVec2 head(-ARROW_WIDTH, -ARROW_WIDTH / 2.0f);
    std::vector<ImVec2> points = {
        to + rotate(head + ImVec2(0.0f, 0.0f), cos_a, sin_a),
        to + rotate(head + ImVec2(ARROW_WIDTH, ARROW_WIDTH / 2.0f), cos_a, sin_a),
        to + rotate(head + ImVec2(0.0f, ARROW_WIDTH), cos_a, sin_a)
    };
    draw_list->AddConvexPolyFilled(points.data(), points.size(), color);
}

//------------------------------------------------------------------------------
void drawArc(ImDrawList* draw_list, Node* from, Node* to, ImVec2* click_position, ImVec2 const& origin, ImVec2 const& cursor)
{
    if (from != nullptr)
    {
        drawArrow(draw_list,
                  origin + ImVec2(from->x, from->y),
                  origin + ImVec2(cursor.x, cursor.y),
                  OUTLINE_COLOR);
    }
    else if (to != nullptr)
    {
        drawArrow(draw_list,
                  origin + ImVec2(cursor.x, cursor.y),
                  origin + ImVec2(to->x, to->y),
                  OUTLINE_COLOR);
    }
    else if (click_position != nullptr)
    {
        drawArrow(draw_list,
                  origin + ImVec2(click_position->x, click_position->y),
                  origin + ImVec2(cursor.x, cursor.y),
                  OUTLINE_COLOR);
    }
}

//------------------------------------------------------------------------------
void drawArc(ImDrawList* draw_list, Arc const& arc, TypeOfNet const type, ImVec2 const& origin, float const alpha)
{
    ImU32 color;

    if (alpha >= 0.0f)
    {
        color = OUTLINE_COLOR; // FIXME FILL_COLOR(alpha);
    }
    else
    {
        // hack to show critical cycles
        color = CRITICAL_COLOR;
    }

    if (type == TypeOfNet::TimedEventGraph)
    {
        // In graph event we "compress" the graph by not displaying places.
        if (arc.from.type == Node::Type::Place)
            return ;

        // We draw arrows between Transition to Transition using the
        // property of graph event: there is only one place between them.
        assert((arc.to.arcsOut.size() == 1u) && "malformed graph event");
        Node& next = arc.to.arcsOut[0]->to;
        drawArrow(draw_list, origin + ImVec2(arc.from.x, arc.from.y),
                  origin + ImVec2(next.x, next.y), color);

        // Print the timing / tokens
        float x = origin.x + arc.from.x + (next.x - arc.from.x) / 2.0f;
        float y = origin.y + arc.from.y + (next.y - arc.from.y) / 2.0f;
        std::stringstream stream;
        stream << std::fixed << std::setprecision(2) << arc.duration << ", "
               << arc.to.key << "(" << reinterpret_cast<Place&>(arc.to).tokens << ")";
        draw_list->AddText(ImVec2(x, y), DURATION_COLOR, stream.str().c_str());
    }
    else
    {
        // Transition -> Place
        drawArrow(draw_list, origin + ImVec2(arc.from.x, arc.from.y),
                  origin + ImVec2(arc.to.x, arc.to.y), color);

        // Print the duration for timed petri net
        if ((arc.from.type == Node::Type::Transition) && (type == TypeOfNet::TimedPetriNet))
        {
            float x = origin.x + arc.from.x + (arc.to.x - arc.from.x) / 2.0f;
            float y = origin.y + arc.from.y + (arc.to.y - arc.from.y) / 2.0f - 15.0f;
            std::stringstream stream;
            stream << std::fixed << std::setprecision(1) << arc.duration;
            draw_list->AddText(ImVec2(x, y), DURATION_COLOR, stream.str().c_str());
        }
    }
}

//------------------------------------------------------------------------------
void drawToken(ImDrawList* draw_list, float const x, float const y)
{
    draw_list->AddCircleFilled(ImVec2(x, y), TOKEN_RADIUS, TOKEN_COLOR);
}

//------------------------------------------------------------------------------
void drawTimedToken(ImDrawList* draw_list, size_t tokens, float const x, float const y)
{
    draw_list->AddCircleFilled(ImVec2(x, y), TOKEN_RADIUS, TOKEN_COLOR);
    draw_list->AddText(ImVec2(x, y), CAPTION_COLOR, std::to_string(tokens).c_str());
}

//------------------------------------------------------------------------------
void drawPlace(ImDrawList* draw_list, Place const& place, TypeOfNet const type, ImVec2 const& origin, bool const show_caption, float const alpha)
{
    // In graph event we "compress" the graph by not displaying places.
    if (type == TypeOfNet::TimedEventGraph)
        return ;

    //const uint8_t alpha = 255; // TODO m_fading[place.key]
    const ImVec2 p = origin + ImVec2(place.x, place.y);

    if (type != TypeOfNet::GRAFCET)
    {
        // Draw the place as circle
        draw_list->AddCircleFilled(p, PLACE_RADIUS, FILL_COLOR(alpha), 64);
        draw_list->AddCircle(p, PLACE_RADIUS, OUTLINE_COLOR, 64, 2.5f);
    }
    else
    {
        // Draw the transition
        const ImVec2 pmin(p.x - TRANS_WIDTH / 2.0f, p.y - TRANS_WIDTH / 2.0f);
        const ImVec2 pmax(p.x + TRANS_WIDTH / 2.0f, p.y + TRANS_WIDTH / 2.0f);
        draw_list->AddRectFilled(pmin, pmax, FILL_COLOR(alpha));
        draw_list->AddRect(pmin, pmax, OUTLINE_COLOR, 0.0f, ImDrawFlags_None, 2.5f);
    }

    // Draw the caption
    const char* text = show_caption ? place.caption.c_str() : place.key.c_str();
    ImVec2 dim = ImGui::CalcTextSize(text);
    ImVec2 ptext = p - ImVec2(dim.x / 2.0f, PLACE_RADIUS + dim.y);
    draw_list->AddText(ptext, CAPTION_COLOR, text);

    // Draw the number of tokens
    if (place.tokens == 0u)
        return ;

    float r = TOKEN_RADIUS;
    float d = TOKEN_RADIUS + 1.0f;

    if (place.tokens == 1u)
    {
        drawToken(draw_list, p.x, p.y);
    }
    else if (place.tokens == 2u)
    {
        drawToken(draw_list, p.x - d, p.y);
        drawToken(draw_list, p.x + d, p.y);
    }
    else if (place.tokens == 3u)
    {
        drawToken(draw_list, p.x, p.y - r);
        drawToken(draw_list, p.x - d, p.y + d);
        drawToken(draw_list, p.x + d, p.y + d);
    }
    else if ((place.tokens == 4u) || (place.tokens == 5u))
    {
        if (place.tokens == 5u)
        {
            d = r + 3.0f;
            drawToken(draw_list, p.x, p.y);
        }

        drawToken(draw_list, p.x - d, p.y - d);
        drawToken(draw_list, p.x + d, p.y - d);
        drawToken(draw_list, p.x - d, p.y + d);
        drawToken(draw_list, p.x + d, p.y + d);
    }
    else
    {
        drawToken(draw_list, p.x, p.y);
        std::string tokens = std::to_string(place.tokens);
        draw_list->AddText(ImVec2(p.x, p.y), CAPTION_COLOR, tokens.c_str());
    }
}

//------------------------------------------------------------------------------
void drawTransition(ImDrawList* draw_list, Transition const& transition,
                    TypeOfNet const type, ImVec2 const& origin,
                    bool const show_caption, float const alpha)
{
    //const uint8_t alpha = 255; // TODO m_fading[place.key]
    const ImVec2 p = origin + ImVec2(transition.x, transition.y);

    // Color of the transition: green if validated else yellow if enabled
    // else color is fadding value.
    ImU32 color;
    if (transition.isFireable())
    {
        color = TRANS_FIREABLE_COLOR;
    }
    else if (transition.isValidated())
    {
        color = (type == TypeOfNet::PetriNet)
              ? TRANS_VALIDATED_COLOR : FILL_COLOR(alpha);
    }
    else if (transition.receptivity)
    {
        color = (type == TypeOfNet::GRAFCET)
              ? TRANS_ENABLED_COLOR : TRANS_FIREABLE_COLOR;
    }
    else
    {
        color = FILL_COLOR(alpha);
    }

    // Draw the transition
    const ImVec2 pmin(p.x - TRANS_WIDTH / 2.0f, p.y - TRANS_HEIGHT / 2.0f);
    const ImVec2 pmax(p.x + TRANS_WIDTH / 2.0f, p.y + TRANS_HEIGHT / 2.0f);
    draw_list->AddRectFilled(pmin, pmax, color);
    draw_list->AddRect(pmin, pmax, OUTLINE_COLOR, 0.0f, ImDrawFlags_None, 2.5f);

    // Draw the caption
    const char* text = show_caption ? transition.caption.c_str() : transition.key.c_str();
    ImVec2 dim = ImGui::CalcTextSize(text);
    ImVec2 ptext = p - ImVec2(dim.x / 2.0f, TRANS_HEIGHT / 2.0f + dim.y);
    draw_list->AddText(ptext, CAPTION_COLOR, text);
}

} // namespace tpne