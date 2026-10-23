// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "PetriNet/PetriNet.hpp"

#include "PetriNet/Algorithms.hpp"
#include "PetriNet/Imports/Imports.hpp"
#include "PetriNet/Exports/Exports.hpp"

#include <algorithm>
#include <iostream>
#include <cstring>
#include <functional>
#include <limits>

namespace tpne {

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
std::string to_str(ArcKind const kind)
{
    switch (kind)
    {
    case ArcKind::Normal:
        return "normal";
    case ArcKind::Inhibitor:
        return "inhibitor";
    case ArcKind::Reset:
        return "reset";
    default:
        return "unknown";
    }
}

//------------------------------------------------------------------------------
std::string featuresToStr(unsigned const features)
{
    static std::pair<Feature, char const*> const NAMES[] = {
        { Feature::Weights, "weights" },
        { Feature::Inhibitors, "inhibitor arcs" },
        { Feature::Resets, "reset arcs" },
        { Feature::Capacities, "place capacities" },
    };
    std::string text;
    for (auto const& it: NAMES)
    {
        if ((features & it.first) != 0u)
            text += (text.empty() ? "" : ", ") + std::string(it.second);
    }
    return text;
}

//------------------------------------------------------------------------------
unsigned features(Net const& net)
{
    unsigned f = 0u;
    for (auto const& arc: net.arcs())
        f |= arc.features();
    size_t const capacity = Net::defaultCapacity(net.type());
    for (auto const& place: net.places())
    {
        if (place.capacity != capacity)
            f |= Feature::Capacities;
    }
    return f;
}

//------------------------------------------------------------------------------
unsigned allowedFeatures(TypeOfNet const type)
{
    switch (type)
    {
    case TypeOfNet::PetriNet:
    case TypeOfNet::TimedPetriNet:
        return Feature::Weights | Feature::Inhibitors | Feature::Resets |
               Feature::Capacities;
    case TypeOfNet::TimedEventGraph:
    case TypeOfNet::GRAFCET:
    default:
        return 0u;
    }
}

//------------------------------------------------------------------------------
Place::Place(size_t const id_, std::string const& caption_, float const x_,
             float const y_, size_t const tokens_)
    : Node(Node::Type::Place, id_, caption_, x_, y_), tokens(tokens_)
{}

//------------------------------------------------------------------------------
size_t Place::increment(size_t const count)
{
    tokens = (count > capacity - std::min(capacity, tokens)) ? capacity : tokens + count;
    return tokens;
}

//------------------------------------------------------------------------------
size_t Place::decrement(size_t const count)
{
    tokens = std::max(tokens, count) - count;
    return tokens;
}

//------------------------------------------------------------------------------
bool Net::isEnabled(Transition const& transition) const
{
    // Normal arcs: the input place shall have enough tokens. Inhibitor arcs:
    // fewer tokens than the weight. Reset arcs: no condition. Input
    // transition (source) is always enabled.
    for (auto const& a: arcsIn(transition))
    {
        size_t const tokens = placeOf(a).tokens;
        if ((a.kind == ArcKind::Normal) && (tokens < a.weight))
            return false;
        if ((a.kind == ArcKind::Inhibitor) && (tokens >= a.weight))
            return false;
    }

    // Output places shall not exceed their capacity after the firing.
    for (auto const& a: arcsOut(transition))
    {
        Place const& place = placeOf(a);
        if (place.capacity == std::numeric_limits<size_t>::max())
            continue;
        size_t tokens = place.tokens;
        if (Arc const* in = findArc(place, transition))
        {
            if (in->kind == ArcKind::Normal)
                tokens -= in->weight;
            else if (in->kind == ArcKind::Reset)
                tokens = 0u;
        }
        if (a.weight > place.capacity - std::min(place.capacity, tokens))
            return false;
    }
    return true;
}

//------------------------------------------------------------------------------
size_t Net::maxTokensToConsume(Transition const& transition) const
{
    // Input transition (source) will fire one token iff the animated token
    // transitioning along the arcs has reached the Place (then receptivity
    // becomes true).
    // FIXME this will conflict if we add code to the receptivity: && animation_done
    if (arcsIn(transition).empty())
        return size_t(transition.receptivity != false);

    // The guard is false => cannot consume tokens. Tokens are consumed one by
    // one: this favors dispatching tokens along arcs when a place feeds
    // several transitions.
    return canFire(transition) ? 1u : 0u;
}

//------------------------------------------------------------------------------
Net::Net(TypeOfNet const type)
    : name(to_str(type)), m_type(type)
{}

//------------------------------------------------------------------------------
Net::Net(Net const& other)
    : name(other.name), description(other.description), inputs(other.inputs),
      m_type(other.m_type), m_places(other.m_places),
      m_transitions(other.m_transitions), m_arcs(other.m_arcs),
      m_place_arcs(other.m_place_arcs), m_transition_arcs(other.m_transition_arcs),
      m_next_place_id(other.m_next_place_id),
      m_next_transition_id(other.m_next_transition_id)
{
    modified = false;
}

//------------------------------------------------------------------------------
Net& Net::operator=(Net const& other)
{
    if (this != &other)
    {
        Net copy(other);
        *this = std::move(copy);
    }
    return *this;
}

//------------------------------------------------------------------------------
void Net::reset(TypeOfNet const type)
{
    m_type = type;
    clear();
    name = to_str(type);
}

//------------------------------------------------------------------------------
void Net::clear()
{
    m_places.clear();
    m_transitions.clear();
    m_arcs.clear();
    m_place_arcs.clear();
    m_transition_arcs.clear();
    m_next_place_id = 0u;
    m_next_transition_id = 0u;
    m_error.clear();
    modified = true;
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
    if (m_places.size() != tokens_.size())
    {
        m_error = "The container dimension holding tokens does not match the number of places\n";
        return false;
    }

    size_t i = tokens_.size();
    while (i--)
    {
        m_places[i].tokens = std::min(m_places[i].capacity, tokens_[i]);
    }

    m_error.clear();
    return true;
}

//------------------------------------------------------------------------------
Place& Net::applyCapacity(Place& place) const
{
    place.capacity = defaultCapacity(m_type);
    place.tokens = std::min(place.capacity, place.tokens);
    return place;
}

//------------------------------------------------------------------------------
Place& Net::addPlace(float const x, float const y, size_t const tokens)
{
    modified = true;
    m_places.emplace_back(m_next_place_id++, "", x, y, tokens);
    generateArcsInArcsOut();
    return applyCapacity(m_places.back());
}

//------------------------------------------------------------------------------
Place& Net::addPlace(size_t const id, std::string const& caption, float const x,
                     float const y, size_t const tokens)
{
    assert((id == m_places.size()) && "Identifiers of places shall be their indices");
    modified = true;
    m_places.emplace_back(id, caption, x, y, tokens);
    if (id + 1u > m_next_place_id)
        m_next_place_id = id + 1u;
    generateArcsInArcsOut();
    return applyCapacity(m_places.back());
}

//------------------------------------------------------------------------------
Transition& Net::addTransition(float const x, float const y)
{
    modified = true;
    m_transitions.push_back(Transition(m_next_transition_id++, "", x, y,
                   (m_type == TypeOfNet::TimedPetriNet) ? true : false));
    generateArcsInArcsOut();
    return m_transitions.back();
}

//------------------------------------------------------------------------------
Transition& Net::addTransition(size_t const id, std::string const& caption,
                               float const x, float const y)
{
    assert((id == m_transitions.size()) && "Identifiers of transitions shall be their indices");
    modified = true;
    m_transitions.emplace_back(id, caption, x, y,
                   (m_type == TypeOfNet::TimedPetriNet) ? true : false);
    if (id + 1u > m_next_transition_id)
        m_next_transition_id = id + 1u;
    generateArcsInArcsOut();
    return m_transitions.back();
}

//------------------------------------------------------------------------------
bool Net::sanityArc(Node const& from, Node const& to, bool const strict, std::stringstream& message) const
{
    // Arc already existing ?
    if (findArc(from, to) != nullptr)
    {
        message << "Failed adding arc " << from.key
                << " --> " << to.key
                << ": Arc already exist"
                << std::endl;
        return false;
    }

    // Key if the origin node exists (TBD: is this really
    // necessary since findArc would have returned false ?)
    if (findNode(from.key) == nullptr)
    {
        message << "Failed adding arc " << from.key
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
        message << "Failed adding arc " << from.key
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
        message << "Failed adding arc " << from.key
                << " --> " << to.key
                << ": nodes type shall not be the same"
                << std::endl;
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
bool Net::addArc(Transition& from, Transition& to, size_t const tokens, float const duration)
{
    // Create the intermediate node
    float x = from.x + (to.x - from.x) / 2.0f;
    float y = from.y + (to.y - from.y) / 2.0f;
    Place& n = addPlace(x, y, tokens);

    m_arcs.emplace_back(from, n, duration);
    m_arcs.emplace_back(n, to, duration);

    generateArcsInArcsOut();
    modified = true;
    return true;
}

//------------------------------------------------------------------------------
std::string Net::addArc(Node& from, Node& to, float const duration)
{
    std::stringstream message;
    if (!sanityArc(from, to, false, message))
        return message.str();

    modified = true;

    // Create an arc "Place -> Transition" or "Transition -> Place"
    if (from.type != to.type)
    {
        m_arcs.emplace_back(from, to, duration);
    }
    else // Manage the case "Place -> Place" or "Transition -> Transition"
    {
        // Create the intermediate node. Note: from and to stay valid since
        // nodes are stored in a std::deque.
        float x = from.x + (to.x - from.x) / 2.0f;
        float y = from.y + (to.y - from.y) / 2.0f;
        Node& n = addOppositeNode(to.type, x, y);

        m_arcs.emplace_back(from, n, duration);
        m_arcs.emplace_back(n, to, duration);
    }

    generateArcsInArcsOut();
    return message.str();
}

//------------------------------------------------------------------------------
Arc* Net::findArc(Node const& from, Node const& to)
{
    NodeId const f = from.nodeId();
    NodeId const t = to.nodeId();
    for (auto& it: m_arcs)
    {
        if ((it.from == f) && (it.to == t))
            return &it;
    }
    return nullptr;
}

//------------------------------------------------------------------------------
Arc const* Net::findArc(Node const& from, Node const& to) const
{
    return const_cast<Net*>(this)->findArc(from, to);
}

//------------------------------------------------------------------------------
size_t Net::indexOf(Arc const& arc) const
{
    for (size_t i = 0u; i < m_arcs.size(); ++i)
    {
        if (&m_arcs[i] == &arc)
            return i;
    }
    assert(false && "Arc not owned by this net");
    return m_arcs.size();
}

//------------------------------------------------------------------------------
Node& Net::node(NodeId const id)
{
    if (id.isPlace())
    {
        assert(id.id < m_places.size());
        return m_places[id.id];
    }
    assert(id.id < m_transitions.size());
    return m_transitions[id.id];
}

//------------------------------------------------------------------------------
Node const& Net::node(NodeId const id) const
{
    return const_cast<Net*>(this)->node(id);
}

//------------------------------------------------------------------------------
Net::Adjacency const& Net::adjacency(Node const& node) const
{
    static Adjacency const none;
    std::vector<Adjacency> const& adjacencies =
        (node.type == Node::Type::Place) ? m_place_arcs : m_transition_arcs;
    return (node.id < adjacencies.size()) ? adjacencies[node.id] : none;
}

//------------------------------------------------------------------------------
void Net::generateArcsInArcsOut()
{
    m_place_arcs.assign(m_places.size(), Adjacency{});
    m_transition_arcs.assign(m_transitions.size(), Adjacency{});

    for (size_t i = 0u; i < m_arcs.size(); ++i)
    {
        Arc const& a = m_arcs[i];
        std::vector<Adjacency>& from = a.from.isPlace() ? m_place_arcs : m_transition_arcs;
        std::vector<Adjacency>& to = a.to.isPlace() ? m_place_arcs : m_transition_arcs;
        assert(a.from.id < from.size());
        assert(a.to.id < to.size());
        from[a.from.id].out.push_back(i);
        to[a.to.id].in.push_back(i);
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
    // Copy: a may be an element of m_arcs.
    NodeId const from = a.from;
    NodeId const to = a.to;
    return removeArc(node(from), node(to));
}

//------------------------------------------------------------------------------
bool Net::removeArc(Node const& from, Node const& to)
{
    NodeId const f = from.nodeId();
    NodeId const t = to.nodeId();
    size_t i = m_arcs.size();
    while (i--)
    {
        if ((m_arcs[i].from == f) && (m_arcs[i].to == t))
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
void Net::renameStep(size_t const from, size_t const to)
{
    // In this graph: "X3" and "<name>:X3".
    for (auto& transition : m_transitions)
    {
        transition.caption = renameStepReference(transition.caption, "", from, to);
        transition.caption = renameStepReference(transition.caption, name, from, to);
    }
    for (auto& place : m_places)
    {
        for (auto& action : place.actions)
        {
            action.condition = renameStepReference(action.condition, "", from, to);
            action.condition = renameStepReference(action.condition, name, from, to);
            for (auto& forcing : action.forcings)
                renameForcedStep(forcing, name, from, to);
        }
    }
}

//------------------------------------------------------------------------------
void Net::renameStepOf(std::string const& graph, size_t const from, size_t const to)
{
    // In another graph: only "<graph>:X3" and the forcing orders of <graph>.
    for (auto& transition : m_transitions)
        transition.caption = renameStepReference(transition.caption, graph, from, to);
    for (auto& place : m_places)
    {
        for (auto& action : place.actions)
        {
            action.condition = renameStepReference(action.condition, graph, from, to);
            for (auto& forcing : action.forcings)
                renameForcedStep(forcing, graph, from, to);
        }
    }
}

//------------------------------------------------------------------------------
void Net::renameArcEnds(NodeId const from, NodeId const to)
{
    for (auto& a: m_arcs)
    {
        if (a.from == from)
            a.from = to;
        if (a.to == from)
            a.to = to;
    }
}

//------------------------------------------------------------------------------
template<class N>
static void moveNode(std::deque<N>& nodes, size_t const last, size_t const id,
                     std::string const& key)
{
    // A caption left to its default value follows the key.
    N& moved = nodes[last];
    if (moved.caption == moved.key)
        moved.caption = key;
    moved.id = id;
    moved.key = key;
    nodes[id] = std::move(moved);
}

//------------------------------------------------------------------------------
void Net::removePlaceAt(size_t const id)
{
    assert(id < m_places.size());
    size_t const last = m_places.size() - 1u;

    // What read the removed step now reads a step that does not exist (an
    // error when the GRAFCET starts), not the moved one.
    if (m_type == TypeOfNet::GRAFCET)
        renameStep(id, m_places.size());

    if (id != last)
    {
        moveNode(m_places, last, id, Place::to_str(id));
        renameArcEnds(NodeId{ Node::Type::Place, last }, NodeId{ Node::Type::Place, id });
        // What reads the step by its number (X3) follows its new number.
        if (m_type == TypeOfNet::GRAFCET)
            renameStep(last, id);
    }
    m_places.pop_back();
    m_next_place_id = m_places.size();
}

//------------------------------------------------------------------------------
void Net::removeTransitionAt(size_t const id)
{
    assert(id < m_transitions.size());
    size_t const last = m_transitions.size() - 1u;
    if (id != last)
    {
        moveNode(m_transitions, last, id, Transition::to_str(id));
        renameArcEnds(NodeId{ Node::Type::Transition, last },
                      NodeId{ Node::Type::Transition, id });
    }
    m_transitions.pop_back();
    m_next_transition_id = m_transitions.size();
}

//------------------------------------------------------------------------------
void Net::removeNode(Node& node)
{
    // Nodes to remove: the node and, for timed event graphs, the implicit
    // places of the arcs of the transition.
    std::vector<size_t> places;
    std::vector<size_t> transitions;
    if (node.type == Node::Type::Place)
    {
        places.push_back(node.id);
    }
    else
    {
        transitions.push_back(node.id);
        if (m_type == TypeOfNet::TimedEventGraph)
        {
            for (auto const& a: arcsIn(node))
                places.push_back(a.from.id);
            for (auto const& a: arcsOut(node))
                places.push_back(a.to.id);
        }
    }

    // Remove the arcs touching these nodes.
    auto const removed = [&](NodeId const n) {
        std::vector<size_t> const& ids = n.isPlace() ? places : transitions;
        return std::find(ids.begin(), ids.end(), n.id) != ids.end();
    };
    m_arcs.erase(std::remove_if(m_arcs.begin(), m_arcs.end(), [&](Arc const& a) {
        return removed(a.from) || removed(a.to);
    }), m_arcs.end());

    // Remove the nodes: the last node of the container takes the place of the
    // removed one, so identifiers stay 0 .. n-1 (and are indices). Removing
    // from the greatest identifier keeps the identifiers still to remove valid.
    std::sort(places.begin(), places.end(), std::greater<size_t>());
    places.erase(std::unique(places.begin(), places.end()), places.end());
    for (size_t const id: places)
        removePlaceAt(id);
    for (size_t const id: transitions)
        removeTransitionAt(id);

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
// TBD: duplicated with Path::extension() but I do not want to have Path.hpp
// as dependency here.
static std::string fileExtension(std::string const& path)
{
    std::string::size_type pos = path.find_last_of(".");
    if (pos != std::string::npos)
    {
        std::string ext = path.substr(pos, std::string::npos);

        // Ignore trailing ~ (backup files like foo.txt~)
        if (!ext.empty() && ext.back() == '~')
            ext.pop_back();

        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext;
    }
    return "";
}

//------------------------------------------------------------------------------
std::string saveToFile(Net const& net, std::string const& filepath)
{
    Exporter const* exporter = getExporter(fileExtension(filepath));
    if (exporter == nullptr)
    {
        return "Cannot export '" + filepath + "'. Reason: 'unknown file extension'\n";
    }
    return exporter->exportFct(net, filepath);
}

//------------------------------------------------------------------------------
std::string loadFromFile(Net& net, std::string const& filepath, bool& springify)
{
    // Search the importer
    Importer const* importer = getImporter(fileExtension(filepath));
    if (importer == nullptr)
    {
        return "Cannot import '" + filepath + "'. Reason: 'unknown file extension'\n";
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
bool convertTo(Net& net, TypeOfNet const type, std::string& error, std::vector<size_t>& erroneous_arcs)
{
    if (net.type() == type)
        return true;

    // The new type shall accept the extensions (weights, inhibitor arcs ...)
    // the net uses.
    unsigned const used = features(net);
    if (!allows(type, used))
    {
        unsigned const refused = used & ~allowedFeatures(type);
        error = "A " + to_str(type) + " cannot have " + featuresToStr(refused);
        erroneous_arcs.clear();
        for (size_t i = 0u; i < net.arcs().size(); ++i)
        {
            if ((net.arcs()[i].features() & refused) != 0u)
                erroneous_arcs.push_back(i);
        }
        return false;
    }

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

    bool const grafcet = (type == TypeOfNet::GRAFCET) || (net.type() == TypeOfNet::GRAFCET);
    net.m_type = type;
    net.resetReceptivies();

    // GRAFCET constrains the number of tokens. Other nets keep the capacities
    // given by the user.
    if (grafcet)
    {
        for (auto& place: net.places())
        {
            net.applyCapacity(place);
        }
    }

    return true;
}

} // namespace tpne
