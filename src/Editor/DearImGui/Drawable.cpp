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

#define FILL_COLOR(a) IM_COL32(255, 165, 0, (a))
#define OUTLINE_COLOR IM_COL32(165, 42, 42, 255) // Arcs, Places, Transitions
#define CRITICAL_COLOR IM_COL32(255, 0, 0, 255)

//------------------------------------------------------------------------------
static void drawArrow(ImDrawList* draw_list, ImVec2 const& A, ImVec2 const& B,
                      const ImU32 color)
{
    // Orientation
    const float teta = (B.y - A.y) / (B.x - A.x);
    const float arrowAngle = std::atan(teta);
    const float cos_a = std::cos(arrowAngle);
    const float sin_a = std::sin(arrowAngle);

    // Arc magnitude
    const float arrowLength = norm(A, B);

    // Reduce the arrow magnitude to avoid entering in the place and having
    // a mush of pixels when multiple arrows are pointing on the same
    // position. To get full scaled arrow comment this block of code and
    // uncomment A.x, B.x, A.y, B.y and tailSize.
    const float r = arrowLength - PLACE_RADIUS;
    const float dx = ((B.x - A.x) * r) / arrowLength;
    const float dy = ((B.y - A.y) * r) / arrowLength;
    const float a1 = B.x - dx;
    const float b1 = B.y - dy;
    const float a2 = A.x + dx;
    const float b2 = A.y + dy;

    // Head of the arrow
    const ImVec2 arrowHeadSize(14.0f, 14.0f);
    std::vector<ImVec2> points = {
        ImVec2(a2, b2 /*B.x, B.y*/) + rotate(ImVec2(0.0f, 0.0f), cos_a, sin_a),
        ImVec2(a2, b2 /*B.x, B.y*/) + rotate(ImVec2(arrowHeadSize.x, arrowHeadSize.y / 2.0f), cos_a, sin_a),
        ImVec2(a2, b2 /*B.x, B.y*/) + rotate(ImVec2(0.0f, arrowHeadSize.y), cos_a, sin_a)
    };
    draw_list->AddConvexPolyFilled(points.data(), points.size(), color);

    // Tail of the arrow.
    //const sf::Vector2f tailSize{ arrowLength - arrowHeadSize.x, 2.f };
    const ImVec2 tailSize(r - arrowHeadSize.x - 15.0f, 2.0f);
    draw_list->AddLine(A, B, color, 2.0f);
}

//------------------------------------------------------------------------------
void drawArc(ImDrawList* draw_list, Arc const& arc, TypeOfNet const type, ImVec2 const& origin, uint8_t const alpha)
{
    ImU32 color;

    if (alpha >= 0u)
    {
        color = FILL_COLOR(alpha);
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
        draw_list->AddText(ImVec2(x, y), IM_COL32(0, 0, 0, 255), stream.str().c_str());
    }
    else
    {
        // Transition -> Place
        drawArrow(draw_list, origin + ImVec2(arc.from.x, arc.from.y),
                  origin + ImVec2(arc.to.x, arc.to.y), color);

        if ((arc.from.type == Node::Type::Transition) && (type == TypeOfNet::TimedPetriNet))
        {
            // Print the timing for timed petri net
            float x = origin.x + arc.from.x + (arc.to.x - arc.from.x) / 2.0f;
            float y = origin.y + arc.from.y + (arc.to.y - arc.from.y) / 2.0f - 15.0f;
            std::stringstream stream;
            stream << std::fixed << std::setprecision(2) << arc.duration;
            draw_list->AddText(ImVec2(x, y), IM_COL32(0, 0, 0, 255), stream.str().c_str());
        }
    }
}

//------------------------------------------------------------------------------
void drawToken(ImDrawList* draw_list, float const x, float const y)
{
    draw_list->AddCircleFilled(ImVec2(x, y), TOKEN_RADIUS, IM_COL32(0, 0, 0, 255));
}

// TODO a virer en utilisant un wrapper RenderablePlace avec fading (alpha)
//------------------------------------------------------------------------------
void drawPlace(ImDrawList* draw_list, Place const& place, TypeOfNet const type, ImVec2 const& origin, bool const show_caption, uint8_t const alpha)
{
    // In graph event we "compress" the graph by not displaying places.
    if (type == TypeOfNet::TimedEventGraph)
        return ;

    //const uint8_t alpha = 255; // TODO m_fading[place.key]
    const ImVec2 p = origin + ImVec2(place.x, place.y);

    // Draw the place
    draw_list->AddCircleFilled(p, PLACE_RADIUS, FILL_COLOR(alpha), 64);
    draw_list->AddCircle(p, PLACE_RADIUS, OUTLINE_COLOR, 64);

    // Draw the caption
    ImVec2 dim = ImGui::CalcTextSize(place.key.c_str());
    ImVec2 ptext = p - ImVec2(dim.x / 2.0f, PLACE_RADIUS + dim.y);
    draw_list->AddText(ptext, IM_COL32(0, 0, 0, 255),
                       show_caption ? place.caption.c_str() : place.key.c_str());

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
        std::string tokens = std::to_string(place.tokens);
        draw_list->AddText(ImVec2(p.x, p.y), IM_COL32(0, 0, 0, 255), tokens.c_str());
    }
}

//------------------------------------------------------------------------------
void drawTransition(ImDrawList* draw_list, Transition const& transition, TypeOfNet const type, ImVec2 const& origin, bool const show_caption, uint8_t const alpha)
{
    //const uint8_t alpha = 255; // TODO m_fading[place.key]
    const ImVec2 p = origin + ImVec2(transition.x, transition.y);

    // Color of the transition: green if validated else yellow if enabled
    // else color is fadding value.
    ImU32 color;
    if ((type == TypeOfNet::PetriNet) && (transition.isValidated()))
    {
        color = IM_COL32(255, 0, 0, 255);
    }
    else
    {
        color = FILL_COLOR(transition.isEnabled() ? 0 : alpha);
    }

    // Draw the transition
    const ImVec2 pmin(p.x - TRANS_WIDTH / 2.0f, p.y - TRANS_HEIGHT / 2.0f);
    const ImVec2 pmax(p.x + TRANS_WIDTH / 2.0f, p.y + TRANS_HEIGHT / 2.0f);
    draw_list->AddRectFilled(pmin, pmax, color);
    draw_list->AddRect(pmin, pmax, OUTLINE_COLOR, 1.0f, ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersBottomRight, 1.0f);

    // Draw the caption
    ImVec2 dim = ImGui::CalcTextSize(transition.key.c_str());
    ImVec2 ptext = p - ImVec2(dim.x / 2.0f, TRANS_HEIGHT + dim.y);
    draw_list->AddText(ptext, IM_COL32(0, 0, 0, 255),
                       show_caption ? transition.caption.c_str() : transition.key.c_str());
}

} // namespace tpne