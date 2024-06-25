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

#include "TimedPetriNetEditor/PetriNet.hpp"
#include "TimedPetriNetEditor/Algorithms.hpp"
#include "Net/Imports/Imports.hpp"
#include "Net/Exports/Exports.hpp"
#include "Utils/Utils.hpp"

#include <iomanip>
#include <iostream>
#include <fstream>
#include <cstring>
#include <ctype.h>
#include <limits>

namespace tpne {

//------------------------------------------------------------------------------
// Default net configuration: timed petri net. To change the type of nets, call
// Net::changeTypeOfNet(Net::TypeOfNet const)
size_t Net::Settings::maxTokens = std::numeric_limits<size_t>::max();
Net::Settings::Fire Net::Settings::firing = Net::Settings::Fire::OneByOne;

//------------------------------------------------------------------------------
static void applyNewNetSettings(TypeOfNet const type)
{
    switch (type)
    {
    case TypeOfNet::GRAFCET:
        Net::Settings::maxTokens = 1u;
        Net::Settings::firing = Net::Settings::Fire::OneByOne;
        break;
    case TypeOfNet::PetriNet:
        Net::Settings::maxTokens = std::numeric_limits<size_t>::max();
        Net::Settings::firing = Net::Settings::Fire::OneByOne;
        break;
    case TypeOfNet::TimedPetriNet:
        Net::Settings::maxTokens = std::numeric_limits<size_t>::max();
        Net::Settings::firing = Net::Settings::Fire::OneByOne;
        break;
    case TypeOfNet::TimedEventGraph:
        Net::Settings::maxTokens = std::numeric_limits<size_t>::max();
        Net::Settings::firing = Net::Settings::Fire::OneByOne;
        break;
    default:
        assert(false && "Undefined Petri behavior");
        break;
    }
}

//------------------------------------------------------------------------------
std::string to_str(TypeOfNet const type)
{
    switch (type)
    {
    case TypeOfNet::GRAFCET:
        return "GRAFCET";
    case TypeOfNet::PetriNet:
        return "Petri net";
    case TypeOfNet::TimedPetriNet:
        return "Timed Petri net";
    case TypeOfNet::TimedEventGraph:
        return "Timed event graph";
    default:
        return "Undefined type of net";
    }
}

//------------------------------------------------------------------------------
Place::Place(size_t const id_, std::string const& caption_, float const x_,
             float const y_, size_t const tokens_)
    : Node(Node::Type::Place, id_, caption_, x_, y_)
{
    // Petri net: infinite number of tokens but in GRAFCET max is one.
    tokens = std::min(Net::Settings::maxTokens, tokens_);
}

//------------------------------------------------------------------------------
size_t Place::increment(size_t const count)
{
    tokens = std::min(Net::Settings::maxTokens, tokens + count);
    return tokens;
}

//------------------------------------------------------------------------------
size_t Place::decrement(size_t const count)
{
    tokens = std::max(tokens, count) - count;
    return tokens;
}

//------------------------------------------------------------------------------
bool Transition::isValidated() const
{
    // Transition source will always produce tokens.
    if (arcsIn.size() == 0u)
        return true;

    // To enabled this current transition, all its previous Places shall have at
    // least one token.
    for (auto& a: arcsIn)
    {
        if (a->tokensIn() == 0u)
            return false;
    }

    return true;
}

//------------------------------------------------------------------------------
size_t Transition::countBurnableTokens() const
{
    // Transition source will fire one token iff the animated token transitioning
    // along the arcs has reached the Place (in this receptivity becomes true ...
    // FIXME this will conflict if we add code to the receptivity add && animation_done)
    if (arcsIn.size() == 0u)
        return size_t(receptivity != false);

    // The transition is false => it does not let burn tokens.
    if (receptivity == false)
        return 0u;

    // Iterate on all previous places to know how many tokens can be burned.
    size_t burnt = static_cast<size_t>(-1);
    for (auto& a: arcsIn)
    {
        const size_t tokens = a->tokensIn();
        if (tokens == 0u)
            return 0u;

        if (tokens < burnt)
            burnt = tokens;
    }
    return (Net::Settings::firing == Net::Settings::Fire::OneByOne) ? 1u : burnt;
}

//------------------------------------------------------------------------------
Net::Net(TypeOfNet const type)
    : m_type(type), name(to_str(type))
{
   applyNewNetSettings(type);
}

//------------------------------------------------------------------------------
Net::Net(Net const& other)
{
    if (this == &other)
        return ;

    m_type = other.m_type;
    applyNewNetSettings(m_type);
    m_places = other.m_places;
    m_transitions = other.m_transitions;

    // For arcs: we have to redo references to nodes
    m_arcs.clear();
    for (auto const& it: other.m_arcs)
    {
        Node& from = (it.from.type == Node::Type::Place)
                     ? reinterpret_cast<Node&>(m_places[it.from.id])
                     : reinterpret_cast<Node&>(m_transitions[it.from.id]);
        Node& to = (it.to.type == Node::Type::Place)
                   ? reinterpret_cast<Node&>(m_places[it.to.id])
                   : reinterpret_cast<Node&>(m_transitions[it.to.id]);
        m_arcs.push_back(Arc(from, to, it.duration));
    }
    generateArcsInArcsOut();

    m_next_place_id = other.m_next_place_id;
    m_next_transition_id = other.m_next_transition_id;
    name = other.name;
    m_message.str(std::string());
    modified = false;
}

//------------------------------------------------------------------------------
Net& Net::operator=(Net const& other)
{
    if (this == &other)
        return *this;

    this->~Net(); // destroy
    new (this) Net(other); // copy construct in place
    return *this;
}

//------------------------------------------------------------------------------
void Net::reset(TypeOfNet const type)
{
    m_type = type;
    applyNewNetSettings(type);
    clear();
    name = to_str(type);
}

//------------------------------------------------------------------------------
void Net::clear()
{
    m_places.clear();
    m_transitions.clear();
    m_arcs.clear();
    m_next_place_id = 0u;
    m_next_transition_id = 0u;
    modified = true;
    m_message.str("");
}

//------------------------------------------------------------------------------
std::vector<size_t> Net::tokens() const
{
    std::vector<size_t> tokens_;
    tokens_.resize(m_places.size());
    for (auto& place: m_places)
    {
        tokens_[place.id] = place.tokens;
    }
    return tokens_;
}

//------------------------------------------------------------------------------
bool Net::tokens(std::vector<size_t> const& tokens_)
{
    m_message.str("");
    if (m_places.size() != tokens_.size())
    {
        m_message << "The container dimension holding tokens does not match the number of places"
                  << std::endl;
        return false;
    }

    size_t i = tokens_.size();
    while (i--)
    {
        m_places[i].tokens = std::min(Net::Settings::maxTokens, tokens_[i]);
    }

    return true;
}

//------------------------------------------------------------------------------
Place& Net::addPlace(float const x, float const y, size_t const tokens)
{
    modified = true;
    m_places.push_back(Place(m_next_place_id++, "", x, y, tokens));
    return m_places.back();
}

//------------------------------------------------------------------------------
Place& Net::addPlace(size_t const id, std::string const& caption, float const x,
                     float const y, size_t const tokens)
{
    modified = true;
    m_places.push_back(Place(id, caption, x, y, tokens));
    if (id + 1u > m_next_place_id)
        m_next_place_id = id + 1u;
    return m_places.back();
}

//------------------------------------------------------------------------------
Transition& Net::addTransition(float const x, float const y)
{
    modified = true;
    m_transitions.push_back(
        Transition(m_next_transition_id++, "", x, y, 0u,
                   (m_type == TypeOfNet::TimedPetriNet) ? true : false));
    return m_transitions.back();
}

//------------------------------------------------------------------------------
Transition& Net::addTransition(size_t const id, std::string const& caption,
                               float const x, float const y, int const angle)
{
    modified = true;
    m_transitions.push_back(
        Transition(id, caption, x, y, angle,
                   (m_type == TypeOfNet::TimedPetriNet) ? true : false));
    if (id + 1u > m_next_transition_id)
        m_next_transition_id = id + 1u;
    return m_transitions.back();
}

//------------------------------------------------------------------------------
bool Net::sanityArc(Node const& from, Node const& to, bool const strict) const
{
    // Arc already existing ?
    if (findArc(from, to) != nullptr)
    {
        m_message.str("");
        m_message << "Failed adding arc " << from.key
                  << " --> " << to.key
                  << ": Arc already exist"
                  << std::endl;
        return false;
    }

    // Key if the origin node exists (TBD: is this really
    // necessary since findArc would have returned false ?)
    if (findNode(from.key) == nullptr)
    {
        m_message << "Failed adding arc " << from.key
                  << " --> " << to.key
                  << ": The node " << from.key
                  << " does not exist"
                  << std::endl;
        return false;
    }

    // Key if the destination node exists (TBD: is this really
    // necessary since findArc would have returned false ?)
    if (findNode(to.key) == nullptr)
    {
        m_message << "Failed adding arc " << from.key
                  << " --> " << to.key
                  << ": The node " << to.key
                  << " does not exist"
                  << std::endl;
        return false;
    }

    // The user tried to link two nodes of the same type: this is
    // not possible in Petri nets, GRAFCET ... We offer two options:
    if ((from.type == to.type) && (strict))
    {
        // Option 1: we simply fail (for example when loading file)
        m_message.str("");
        m_message << "Failed adding arc " << from.key
                  << " --> " << to.key
                  << ": nodes type shall not be the same"
                  << std::endl;
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
// FIXME: faire l'equivalent de generateArcsInArcsOut
bool Net::addArc(Transition& from, Transition& to, size_t const tokens, float const duration)
{
    // Create the intermediate node
    float x = from.x + (to.x - from.x) / 2.0f;
    float y = from.y + (to.y - from.y) / 2.0f;
    Place& n = addPlace(x, y, tokens);

    // Frist arc
    m_arcs.push_back(Arc(from, n, duration));
    from.arcsOut.push_back(&m_arcs.back());
    n.arcsIn.push_back(&m_arcs.back());

    // Second arc
    m_arcs.push_back(Arc(n, to, duration));
    n.arcsOut.push_back(&m_arcs.back());
    to.arcsIn.push_back(&m_arcs.back());

    generateArcsInArcsOut(); // FIXME a optimiser !!!
    modified = true;
    return true;
}

//------------------------------------------------------------------------------
// FIXME: faire l'equivalent de generateArcsInArcsOut
bool Net::addArc(Node& from, Node& to, float const duration)
{
    if (!sanityArc(from, to, false))
        return false;

    modified = true;

    // Create an arc "Place -> Transition" or "Transition -> Place" 
    if (from.type != to.type)
    {
        m_arcs.push_back(Arc(from, to, duration));
        from.arcsOut.push_back(&m_arcs.back());
        to.arcsIn.push_back(&m_arcs.back());
    }
    else // Manage the case "Place -> Place" or "Transition -> Transition"
    {
        // Create the intermediate node
        float x = from.x + (to.x - from.x) / 2.0f;
        float y = from.y + (to.y - from.y) / 2.0f;
        Node& n = addOppositeNode(to.type, x, y);

        // Frist arc
        m_arcs.push_back(Arc(from, n, duration));
        from.arcsOut.push_back(&m_arcs.back());
        n.arcsIn.push_back(&m_arcs.back());

        // Second arc
        m_arcs.push_back(Arc(n, to, duration));
        n.arcsOut.push_back(&m_arcs.back());
        to.arcsIn.push_back(&m_arcs.back());
    }

    generateArcsInArcsOut(); // FIXME a optimiser !!!
    return true;
}

//------------------------------------------------------------------------------
Arc* Net::findArc(Node const& from, Node const& to)
{
    for (auto& it: m_arcs)
    {
        if ((it.from.key == from.key) && (it.to.key == to.key))
            return &it;
    }
    return nullptr;
}

//------------------------------------------------------------------------------
Arc const* Net::findArc(Node const& from, Node const& to) const
{
    for (auto& it: m_arcs)
    {
        if ((it.from.key == from.key) && (it.to.key == to.key))
            return &it;
    }
    return nullptr;
}

//------------------------------------------------------------------------------
void Net::generateArcsInArcsOut()
{
    for (auto& trans: m_transitions)
    {
        trans.arcsIn.clear();
        trans.arcsOut.clear();

        for (auto& a: m_arcs)
        {
            if ((a.from.type == Node::Type::Place) && (a.to.id == trans.id))
                trans.arcsIn.push_back(&a);
            else if ((a.to.type == Node::Type::Place) && (a.from.id == trans.id))
                trans.arcsOut.push_back(&a);
        }
    }

    // if (true)
    for (auto& p: m_places)
    {
        p.arcsIn.clear();
        p.arcsOut.clear();

        for (auto& a: m_arcs)
        {
            if ((a.from.type == Node::Type::Transition) && (a.to.id == p.id))
                p.arcsIn.push_back(&a);
            else if ((a.to.type == Node::Type::Transition) && (a.from.id == p.id))
                p.arcsOut.push_back(&a);
        }
    }
}

//------------------------------------------------------------------------------
Node* Net::findNode(std::string const& key)
{
    if (key[0] == 'P')
    {
        for (auto& p: m_places)
        {
            if (p.key == key)
                return &p;
        }
        return nullptr;
    }

    if (key[0] == 'T')
    {
        for (auto& t: m_transitions)
        {
            if (t.key == key)
                return &t;
        }
        return nullptr;
    }

    return nullptr;
}

//------------------------------------------------------------------------------
Node const* Net::findNode(std::string const& key) const
{
    if (key[0] == 'P')
    {
        for (auto& p: m_places)
        {
            if (p.key == key)
                return &p;
        }
        return nullptr;
    }
    else if (key[0] == 'T')
    {
        for (auto& t: m_transitions)
        {
            if (t.key == key)
                return &t;
        }
        return nullptr;
    }

    return nullptr;
}

//------------------------------------------------------------------------------
Transition* Net::findTransition(size_t const id)
{
    for (auto& t: m_transitions)
    {
        if (t.id == id)
            return &t;
    }
    return nullptr;
}

//------------------------------------------------------------------------------
Place* Net::findPlace(size_t const id)
{
    for (auto& p: m_places)
    {
        if (p.id == id)
            return &p;
    }
    return nullptr;
}

//------------------------------------------------------------------------------
bool Net::removeArc(Arc const& a)
{
    return removeArc(a.from, a.to);
}

//------------------------------------------------------------------------------
bool Net::removeArc(Node const& from, Node const& to)
{
// Supprimer les arcs qui touchent la place supprimee

    size_t i = m_arcs.size();
    while (i--)
    {
        if ((m_arcs[i].from.key == from.key) && (m_arcs[i].to.key == to.key))
        {
            // Found the undesired arc: make the latest element take its
            // location in the container.
            m_arcs[i] = m_arcs[m_arcs.size() - 1u];
            m_arcs.pop_back();

            generateArcsInArcsOut();
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
void Net::helperRemovePlace(Node& node)
{
    size_t i = m_places.size();
    while (i--)
    {
        // Found the undesired node: make the latest element take its
        // location in the container. But before doing this we have to
        // restore references on impacted arcs.
        if (m_places[i].id == node.id)
        {
            // Swap element but keep the ID of the removed element
            Place& pi = m_places[i];
            Place& pe = m_places[m_places.size() - 1u];
            if (pe.caption == pe.key)
            {
                m_places[i] = Place(pi.id, pi.key, pe.x, pe.y, pe.tokens);
            }
            else
            {
                m_places[i] = Place(pi.id, pe.caption, pe.x, pe.y, pe.tokens);
            }
            assert(m_next_place_id >= 1u);
            m_next_place_id -= 1u;

            // Update the references to nodes of the arc
            for (auto& a: m_arcs) // TODO optim: use in/out arcs but they may not be generated
            {
                if (a.to.key == pe.key)
                    a = Arc(a.from, m_places[i], a.duration);
                if (a.from.key == pe.key)
                    a = Arc(m_places[i], a.to, a.duration);
            }

            m_places.pop_back();
        }
    }
}

//------------------------------------------------------------------------------
void Net::helperRemoveTransition(Node& node)
{
    size_t i = m_transitions.size();
    while (i--)
    {
        // Found the undesired node: make the latest element take its
        // location in the container. But before doing this we have to
        // restore references on impacted arcs.
        if (m_transitions[i].id == node.id)
        {
            Transition& ti = m_transitions[i];
            Transition& te = m_transitions[m_transitions.size() - 1u];
            if (te.caption == te.key)
            {
                m_transitions[i] = Transition(ti.id, ti.key, te.x, te.y, te.angle,
                                                (m_type == TypeOfNet::TimedPetriNet)
                                                ? true : false);
            }
            else
            {
                m_transitions[i] = Transition(ti.id, te.caption, te.x, te.y, te.angle,
                                                (m_type == TypeOfNet::TimedPetriNet)
                                                ? true : false);
            }
            assert(m_next_transition_id >= 1u);
            m_next_transition_id -= 1u;

            // Update the references to nodes of the arc
            for (auto& a: m_arcs) // TODO optim: use in/out arcs but they may not be generated
            {
                if (a.to.key == te.key)
                    a = Arc(a.from, m_transitions[i], a.duration);
                if (a.from.key == te.key)
                    a = Arc(m_transitions[i], a.to, a.duration);
            }

            m_transitions.pop_back();
        }
    }
}

//------------------------------------------------------------------------------
void Net::helperRemoveArcFromNode(Node& node)
{
    size_t s = m_arcs.size();
    size_t i = s;
    while (i--)
    {
        if ((m_arcs[i].to.key == node.key) || (m_arcs[i].from.key == node.key))
        {
            m_arcs[i] = m_arcs[m_arcs.size() - 1u];
            m_arcs.pop_back();
        }
    }
}

//------------------------------------------------------------------------------
void Net::removeNode(Node& node)
{
    std::vector<Node*> nodes_to_remove;
    if (m_type != TypeOfNet::TimedEventGraph)
    {
        helperRemoveArcFromNode(node);
    }
    else
    {
        for (auto& a: m_transitions[node.id].arcsIn)
        {
            nodes_to_remove.push_back(&a->from);
        }
        for (auto& a: m_transitions[node.id].arcsOut)
        {
            nodes_to_remove.push_back(&a->to);
        }

        // Since we swap nodes inside the array before removing
        // then, we have to start from greater id to lowest id. 
        std::sort(nodes_to_remove.begin(), nodes_to_remove.end(),
            [](Node* a, Node* b) { return a->id > b-> id; });

        for (auto it: nodes_to_remove)
        {
            helperRemoveArcFromNode(*it);
        }
    }

    // Search and remove the node.
    // Note: For fastest deletion, we simply swap the undesired node with the
    // latest node in the container. To do that, we have to iterate from the end
    // of the container.
    if (node.type == Node::Type::Place)
    {
        helperRemovePlace(node);
    }
    else
    {
        helperRemoveTransition(node);
        for (auto it: nodes_to_remove)
        {
            helperRemovePlace(*it);
        }
    }

    // Restore in arcs and out arcs for each node
    generateArcsInArcsOut();
    modified = true;
}

//------------------------------------------------------------------------------
void Net::resetReceptivies()
{
    // For PetriNet set receptivities to false since we want the user to click
    // on desired transitions for firing. For GRAFCET receptivities are initially
    // set to false since the syntax of the boolean expression is checked only when
    // the user starts the simulation and their results are computed at each cycle of
    // the simulation.
    if ((m_type == TypeOfNet::PetriNet) || (m_type == TypeOfNet::GRAFCET))
    {
        for (auto& transition: m_transitions)
        {
            transition.receptivity = false;
        }
    }
    // For other type of nets (timed Petri net, graph events) the
    // receptivities are always true.
    else
    {
        for (auto& transition: m_transitions)
        {
            transition.receptivity = true;
        }
    }
}

//------------------------------------------------------------------------------
std::string saveToFile(Net const& net, std::string const& filepath)
{
    Exporter const* exporter = getExporter(extension(filepath));
    if (exporter == nullptr)
    {
        return "Cannot export " + filepath + ". Reason: 'unknown file extension'\n";
    }
    return exporter->exportFct(net, filepath);
}

//------------------------------------------------------------------------------
std::string loadFromFile(Net& net, std::string const& filepath, bool& springify)
{
    // Search the importer
    Importer const* importer = getImporter(extension(filepath));
    if (importer == nullptr)
    {
        return "Cannot import " + filepath + ". Reason: 'unknown file extension'\n";
    }

    // Load the file
    net.clear();
    std::string error = importer->importFct(net, filepath);
    if (!error.empty())
    {
        net.reset(net.type());
    }
    else
    {
        springify = importer->springify;
    }

    // Get a name to the net
    if (net.name == "")
    {
        size_t lastindex = filepath.find_last_of(".");
        std::string _name = filepath.substr(0, lastindex);
        lastindex = _name.find_last_of("/");
        net.name = _name.substr(lastindex + 1u);
    }

    net.modified = false;

    return error;
}

//------------------------------------------------------------------------------
bool convertTo(Net& net, TypeOfNet const type, std::string& error, std::vector<Arc*>& erroneous_arcs)
{
    if (net.type() == type)
        return true;

    // Check conditions of a well formed event graph
    if (type == TypeOfNet::TimedEventGraph)
    {
        if ((!net.isEmpty()) && (!isEventGraph(net, error, erroneous_arcs)))
            return false;
    }
    else if (type == TypeOfNet::GRAFCET)
    {
        // Nothing to do! We do not check here validity of the boolean
        // expression for receptivities (transitions). We will check it
        // when the user wants to start its simulation.
    }

    applyNewNetSettings(type);

    net.m_type = type;
    net.resetReceptivies();

    if (type == TypeOfNet::GRAFCET)
    {
        // Constrain the number of tokens.
        // TBD: not constraining the number of tokens allow us to save the
        // net (json format) without loosing number of tokens for other nets.
        for (auto& place: net.places())
        {
            place.tokens = std::min(Net::Settings::maxTokens, place.tokens);
        }
    }

    return true;
}

} // namespace tpne
