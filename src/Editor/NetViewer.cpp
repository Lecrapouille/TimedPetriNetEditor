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
#include "Editor/NetViewer.hpp"
#include "Editor/Drawable.hpp"
#include "Editor/Theme.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace tpne {

//! \brief Zoom limits of the viewer.
static constexpr float ZOOM_MIN = 0.1f;
static constexpr float ZOOM_MAX = 5.0f;
//! \brief Room kept on the right of a GRAFCET step for its actions (net units).
static constexpr float ACTIONS_WIDTH = 140.0f;

//! \brief Data given to drawStepActions() to ask the host about one step.
struct StepActions
{
    NetViewer::Options const* options;
    size_t place_id;
};

//------------------------------------------------------------------------------
//! \brief Adapter from drawStepActions() to Options::is_action_active.
static bool isStepActionActive(size_t const index, void* data)
{
    StepActions const& step = *static_cast<StepActions const*>(data);
    return step.options->is_action_active(step.place_id, index, step.options->data);
}

//------------------------------------------------------------------------------
void NetViewer::fitNow(Net const& net, ImVec2 const& size)
{
    if (net.places().empty() && net.transitions().empty())
        return;

    float min_x = std::numeric_limits<float>::max();
    float min_y = std::numeric_limits<float>::max();
    float max_x = std::numeric_limits<float>::lowest();
    float max_y = std::numeric_limits<float>::lowest();
    auto const extend = [&](float const x, float const y)
    {
        min_x = std::min(min_x, x); max_x = std::max(max_x, x);
        min_y = std::min(min_y, y); max_y = std::max(max_y, y);
    };
    bool const grafcet = (net.type() == TypeOfNet::GRAFCET);
    for (auto const& place : net.places())
    {
        extend(place.x, place.y);
        if (grafcet && !place.actions.empty())
            extend(place.x + ACTIONS_WIDTH, place.y);
    }
    for (auto const& transition : net.transitions())
        extend(transition.x, transition.y);

    float const margin = 50.0f;
    min_x -= margin; min_y -= margin;
    max_x += margin; max_y += margin;

    m_zoom = std::clamp(std::min(size.x / (max_x - min_x), size.y / (max_y - min_y)),
                        ZOOM_MIN, ZOOM_MAX);
    m_scrolling.x = size.x / 2.0f - (min_x + max_x) / 2.0f * m_zoom;
    m_scrolling.y = size.y / 2.0f - (min_y + max_y) / 2.0f * m_zoom;
}

//------------------------------------------------------------------------------
void NetViewer::draw(char const* id, Net const& net, Options const& options)
{
    ImVec2 const corner = ImGui::GetCursorScreenPos();
    ImVec2 size = ImGui::GetContentRegionAvail();
    size.x = std::max(size.x, 50.0f);
    size.y = std::max(size.y, 50.0f);

    if (m_net != &net)
    {
        m_net = &net;
        fit();
    }
    if (m_fit_frames > 0)
    {
        fitNow(net, size);
        --m_fit_frames;
    }

    // The canvas catches the mouse: pan, zoom, fit.
    ImGui::InvisibleButton(id, size, ImGuiButtonFlags_MouseButtonLeft |
                           ImGuiButtonFlags_MouseButtonRight |
                           ImGuiButtonFlags_MouseButtonMiddle);
    bool const hovered = ImGui::IsItemHovered();
    ImGuiIO const& io = ImGui::GetIO();
    if (ImGui::IsItemActive() && (std::fabs(io.MouseDelta.x) + std::fabs(io.MouseDelta.y) > 0.0f))
    {
        m_scrolling.x += io.MouseDelta.x;
        m_scrolling.y += io.MouseDelta.y;
        m_fit_frames = 0;
    }
    if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
    {
        fit();
    }
    if (hovered && (io.MouseWheel != 0.0f))
    {
        // Zoom around the mouse cursor.
        ImVec2 const mouse(io.MousePos.x - corner.x, io.MousePos.y - corner.y);
        float const old_zoom = m_zoom;
        m_zoom = std::clamp(m_zoom * std::pow(1.1f, io.MouseWheel), ZOOM_MIN, ZOOM_MAX);
        m_scrolling.x = mouse.x - (mouse.x - m_scrolling.x) * m_zoom / old_zoom;
        m_scrolling.y = mouse.y - (mouse.y - m_scrolling.y) * m_zoom / old_zoom;
        m_fit_frames = 0;
    }

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 const corner_max(corner.x + size.x, corner.y + size.y);
    ImVec2 const origin(corner.x + m_scrolling.x, corner.y + m_scrolling.y);
    draw_list->PushClipRect(corner, corner_max, true);
    draw_list->AddRectFilled(corner, corner_max, themePetriViewColor());

    TypeOfNet const type = net.type();
    for (auto const& arc : net.arcs())
    {
        drawArc(draw_list, arc, type, origin, 1.0f, m_zoom);
    }
    if (options.highlighted_arcs != nullptr)
    {
        for (Arc const* arc : *options.highlighted_arcs)
            drawArc(draw_list, *arc, type, origin, -1.0f, m_zoom);
    }

    auto const& places = net.places();
    for (size_t i = 0u; i < places.size(); ++i)
    {
        Place const& place = places[i];
        bool const initial = (options.initial_marking != nullptr) &&
            (i < options.initial_marking->size()) && ((*options.initial_marking)[i] > 0u);
        drawPlace(draw_list, place, type, origin, options.captions, 1.0f, m_zoom, initial);

        if ((type == TypeOfNet::GRAFCET) && options.actions)
        {
            StepActions step { &options, place.id };
            if (options.is_action_active != nullptr)
                drawStepActions(draw_list, place, origin, m_zoom, isStepActionActive, &step);
            else
                drawStepActions(draw_list, place, origin, m_zoom);
        }
    }
    for (auto const& transition : net.transitions())
    {
        drawTransition(draw_list, transition, type, origin, options.captions, 1.0f, m_zoom);
    }

    // Tooltip of the node under the mouse.
    m_hovered = nullptr;
    if (hovered)
    {
        float const x = (io.MousePos.x - origin.x) / m_zoom;
        float const y = (io.MousePos.y - origin.y) / m_zoom;
        m_hovered = const_cast<Net&>(net).findNodeAt(x, y, PLACE_RADIUS, TRANS_WIDTH / 2.0f);
        if (m_hovered != nullptr)
        {
            ImGui::BeginTooltip();
            ImGui::Text("%s  %s", m_hovered->key.c_str(), m_hovered->caption.c_str());
            if (m_hovered->type == Node::Type::Place)
            {
                Place const& place = *static_cast<Place const*>(m_hovered);
                ImGui::Text("tokens: %zu", place.tokens);
                for (auto const& action : place.actions)
                    ImGui::Text("%s %s", qualifierToStr(action.qualifier), action.name.c_str());
            }
            ImGui::EndTooltip();
        }
    }

    draw_list->AddRect(corner, corner_max, ImGui::GetColorU32(ImGuiCol_Border));
    draw_list->PopClipRect();
}

} // namespace tpne
