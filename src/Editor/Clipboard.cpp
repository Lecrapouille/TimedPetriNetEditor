// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Editor/Clipboard.hpp"

namespace tpne {

//------------------------------------------------------------------------------
void Clipboard::addPlace(Place const& place, Point const& position)
{
    m_places.push_back({place.id, place.caption, position.x, position.y, place.tokens});
}

//------------------------------------------------------------------------------
void Clipboard::addTransition(Transition const& transition, Point const& position)
{
    m_transitions.push_back({transition.id, transition.caption,
                             position.x, position.y});
}

//------------------------------------------------------------------------------
void Clipboard::addArc(Arc const& arc)
{
    m_arcs.push_back({
        arc.from.id,
        arc.from.type == Node::Type::Place,
        arc.to.id,
        arc.to.type == Node::Type::Place,
        arc.durationOrZero()
    });
}

//------------------------------------------------------------------------------
void Clipboard::paste(Net& target_net, float x, float y,
                      std::vector<Node*>& created_nodes)
{
    float offset_x = x - m_center_x;
    float offset_y = y - m_center_y;

    std::map<size_t, Place*> old_to_new_place;
    std::map<size_t, Transition*> old_to_new_trans;

    created_nodes.clear();

    // Paste places
    for (auto& p : m_places)
    {
        Place& new_place = target_net.addPlace(p.x + offset_x, p.y + offset_y);
        new_place.caption = p.caption;
        new_place.tokens = p.tokens;
        old_to_new_place[p.id] = &new_place;
        created_nodes.push_back(&new_place);
    }

    // Paste transitions
    for (auto& t : m_transitions)
    {
        Transition& new_trans = target_net.addTransition(t.x + offset_x, t.y + offset_y);
        new_trans.caption = t.caption;
        old_to_new_trans[t.id] = &new_trans;
        created_nodes.push_back(&new_trans);
    }

    // Paste arcs
    for (auto& a : m_arcs)
    {
        Node* from_node = nullptr;
        Node* to_node = nullptr;

        if (a.from_is_place)
        {
            auto it = old_to_new_place.find(a.from_id);
            if (it != old_to_new_place.end())
                from_node = it->second;
        }
        else
        {
            auto it = old_to_new_trans.find(a.from_id);
            if (it != old_to_new_trans.end())
                from_node = it->second;
        }

        if (a.to_is_place)
        {
            auto it = old_to_new_place.find(a.to_id);
            if (it != old_to_new_place.end())
                to_node = it->second;
        }
        else
        {
            auto it = old_to_new_trans.find(a.to_id);
            if (it != old_to_new_trans.end())
                to_node = it->second;
        }

        if (from_node != nullptr && to_node != nullptr)
        {
            target_net.addArc(*from_node, *to_node, a.duration);
        }
    }
}

//------------------------------------------------------------------------------
void Clipboard::clear()
{
    m_places.clear();
    m_transitions.clear();
    m_arcs.clear();
}

//------------------------------------------------------------------------------
bool Clipboard::empty() const
{
    return m_places.empty() && m_transitions.empty();
}

//------------------------------------------------------------------------------
void Clipboard::setCenter(float x, float y)
{
    m_center_x = x;
    m_center_y = y;
}

} // namespace tpne
