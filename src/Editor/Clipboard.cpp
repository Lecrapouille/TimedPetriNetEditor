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

#include "Editor/Clipboard.hpp"

namespace tpne {

//------------------------------------------------------------------------------
void Clipboard::addPlace(Place const& place)
{
    m_places.push_back({place.id, place.caption, place.x, place.y, place.tokens});
}

//------------------------------------------------------------------------------
void Clipboard::addTransition(Transition const& transition)
{
    m_transitions.push_back({transition.id, transition.caption,
                             transition.x, transition.y, transition.angle});
}

//------------------------------------------------------------------------------
void Clipboard::addArc(Arc const& arc)
{
    m_arcs.push_back({
        arc.from.id,
        arc.from.type == Node::Type::Place,
        arc.to.id,
        arc.to.type == Node::Type::Place,
        arc.duration
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
        new_trans.angle = t.angle;
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
