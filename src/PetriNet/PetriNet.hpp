// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef PETRI_NET_HPP
#  define PETRI_NET_HPP

#  include "PetriNet/Grafcet.hpp"
#  include "PetriNet/SafeFloat.hpp"

#  include <cmath>
#  include <string>
#  include <atomic>
#  include <deque>
#  include <map>
#  include <vector>
#  include <cassert>
#  include <sstream>
#  include <iosfwd>
#  include <limits>

namespace tpne {

class Arc;
struct NodeId;

//------------------------------------------------------------------------------
//! \brief Enum determining the type of net (GRAFCET, timed Petri net ...) and
//! defining the type of simulation (for example, GRAFCET cannot have more than
//! one token in a step ...).
//! \note: We do not use inheritance, if-then-else on the type of net is used
//! to modify the behavior.
//------------------------------------------------------------------------------
enum class TypeOfNet
{
    //! \brief The user has to click on fireable transitions to consume tokens.
    //! Tokens in input places are consumed one by one.
    PetriNet,
    //! \brief Is a Petri with duration on arcs transition -> place.
    //! Guards are set to true. When transitions are enabled (when all
    //! input places have at least one token), the firing is
    //! automatic (The user cannot click on transitions to fire). The
    //! policy concerning divergence transitions: the maximum of tokens are
    //! consumed at once but tokens are shuffled along arcs.
    TimedPetriNet,
    //! \brief Is a timed Petri where all places have a single input arc and
    //! a single output arc.
    TimedEventGraph,
    //! \brief Is a Petri net used for making industrial automata (productive):
    //! Places are named Steps and do discrete actions. Transitions have boolean
    //! expression (named receptivities aka conditions) linked to sensors
    //! (i.e. door closed and alarm off). Steps have at maximum one token (aka
    //! 1-S net).
    GRAFCET
};

//------------------------------------------------------------------------------
//! \brief Kind of an arc. Inhibitor and reset arcs go from a place to a
//! transition:
//!   - Normal: the transition needs \c weight tokens in the place and takes
//!     them (from a transition to a place: gives \c weight tokens);
//!   - Inhibitor: the transition is enabled only while the place holds less
//!     than \c weight tokens (1: the place is empty); it takes none;
//!   - Reset: the transition empties the place; it does not need tokens.
//------------------------------------------------------------------------------
enum class ArcKind { Normal, Inhibitor, Reset };

//------------------------------------------------------------------------------
//! \brief Extensions of the formalism that a net uses, as bits: see
//! features() and allows(). TypeOfNet says how the net evolves; these say what
//! its arcs and places contain.
//------------------------------------------------------------------------------
enum Feature : unsigned
{
    //! \brief Arcs with a weight other than 1.
    Weights = 1u << 0,
    //! \brief Inhibitor arcs.
    Inhibitors = 1u << 1,
    //! \brief Reset arcs.
    Resets = 1u << 2,
    //! \brief Places with a capacity (other than the one of the type of net).
    Capacities = 1u << 3,
};

// *****************************************************************************
//! \brief Since Petri nets are bipartite graph there are two kind of nodes:
//! place and transition. This class shall not be used directly as instance, it
//! only allows to factorize the code of derived classes Place and Transition.
// *****************************************************************************
class Node
{
public:

    //--------------------------------------------------------------------------
    //! \brief Petri net is a bipartite graph. So we have to define the type of
    //! the node: Place or Transition.
    //--------------------------------------------------------------------------
    enum class Type { Place, Transition };

    //--------------------------------------------------------------------------
    //! \brief Constructor. No sanity checks are made in this method.
    //! \param[in] type_: Type of Petri node: Place or Transition.
    //! \param[in] id_: unique identifier (0u, 1u, ...).
    //! \param[in] caption_: Text to displaying identifying the node.
    //! \param[in] x_: X-axis coordinate in the window needed for the display.
    //! \param[in] y_: Y-axis coordinate in the window needed for the display.
    //--------------------------------------------------------------------------
    Node(Type const type_, size_t const id_, std::string const& caption_,
         float const x_, float const y_)
        : type(type_), id(id_),
          key((type == Node::Type::Place ? 'P' : 'T') + std::to_string(id)),
          x(x_), y(y_), caption(caption_.empty() ? key : caption_)
    {}

    //--------------------------------------------------------------------------
    //! \brief Reference to this node, as stored by arcs.
    //--------------------------------------------------------------------------
    NodeId nodeId() const;

public:

    //! \brief Type of nodes: Petri Place or Petri Transition. Once created, it
    //! is not supposed to be changed.
    Type type;
    //! \brief Unique identifier (auto-incremented from 0 by the derived class).
    //! It is also the index of the node in Net::places() or Net::transitions().
    //! Only the net changes it (when a node is removed).
    size_t id;
    //! \brief Unique node identifier as string. It is formed by the 'P' char
    //! for place or by the 'T' char for transition followed by the unique
    //! identifier (i.e. "P0", "P1", "T0", "T1", ...). Follows the id.
    std::string key;
    //! \brief Position inside the window needed for the display.
    float x;
    //! \brief Position in the window needed for the display.
    float y;
    //! \brief Text displayed near a node the user can modify. Default value is
    //! the tring unique \c key.
    std::string caption;
};

// *****************************************************************************
//! \brief Reference to a place or a transition: its type and its identifier,
//! which is its index in Net::places() or Net::transitions(). Arcs store their
//! ends this way: a copied net does not point to the nodes of the original.
// *****************************************************************************
struct NodeId
{
    Node::Type type;
    size_t id;

    bool isPlace() const { return type == Node::Type::Place; }
    bool isTransition() const { return type == Node::Type::Transition; }

    //! \brief "P42" or "T42", as Node::key.
    std::string key() const
    {
        return (isPlace() ? 'P' : 'T') + std::to_string(id);
    }

    bool operator==(NodeId const& other) const
    {
        return (type == other.type) && (id == other.id);
    }

    bool operator!=(NodeId const& other) const { return !(*this == other); }
};

inline NodeId Node::nodeId() const { return NodeId{ type, id }; }

// *****************************************************************************
//! \brief Petri Place node. Places represent system states. Places hold tokens
//! (resources). In GRAFCET, Places are named Steps and have at max one token.
//! When steps are activated actions are performed.
// *****************************************************************************
class Place : public Node
{
public:

    //! \brief to access s_next_id
    friend class Net;

    //--------------------------------------------------------------------------
    //! \brief Constructor. To be used when loading a Petri net from a file.
    //! \param[in] id_: unique node identifier. Shall be unique (responsibility
    //! given to the caller class).
    //! \param[in] caption_: Displayed text under the node.
    //! \param[in] x_: X-axis coordinate in the window needed for the display.
    //! \param[in] y_: Y-axis coordinate in the window needed for the display.
    //! \param[in] tokens_: Initial number of tokens in the place >= 0.
    //--------------------------------------------------------------------------
    Place(size_t const id_, std::string const& caption_, float const x_,
          float const y_, size_t const tokens_);

    //--------------------------------------------------------------------------
    //! \brief Static method stringify a place given an identifier.
    //! \return "P42" for example.
    //--------------------------------------------------------------------------
    inline static std::string to_str(size_t const id_)
    {
        return std::string("P") + std::to_string(id_);
    }

    //--------------------------------------------------------------------------
    //! \brief Increment the number of token but constrained by the capacity.
    //--------------------------------------------------------------------------
    size_t increment(size_t const count = 1u);

    //--------------------------------------------------------------------------
    //! \brief Decrement the number of token but constrained to 0.
    //--------------------------------------------------------------------------
    size_t decrement(size_t const count = 1u);

    //--------------------------------------------------------------------------
    //! \brief For debug purpose only.
    //--------------------------------------------------------------------------
    inline friend std::ostream& operator<<(std::ostream& os, Place const& p)
    {
        os << p.key << " (\"" << p.caption << "\", " << p.tokens
           << ", (" << p.x << ", " << p.y << "))";
        return os;
    }

public:

    //! \brief the number of tokens hold by the Place. Public access is fine
    //! since this will facilitate its access during the simulation.
    size_t tokens;
    //! \brief Max number of tokens the place can hold: 1 for GRAFCET steps,
    //! unbounded by default for other nets. A transition whose firing would
    //! exceed it is not enabled.
    size_t capacity = std::numeric_limits<size_t>::max();
    //! \brief GRAFCET actions associated with this step. Only used in GRAFCET mode.
    std::vector<Action> actions;
};

// *****************************************************************************
//! \brief Petri Transition node. A boolean condition (named guard or receptivity
//! in GRAFCET) is set but differs with the type of net (Petri: when the user
//! clicks on the transition, timed Petri net: always set to true, GRAFCET depends
//! on boolean expressions with sensors). When the guard is true and the transition
//! is enabled (meaning that all input places have at least one token each) the
//! transition fires and tokens in input places are consumed and placed in output
//! places.
//! \note There is currently no method for consuming tokens because this is made
//! by the simulator with animation.
// *****************************************************************************
class Transition : public Node
{
public:

    //! \brief to access s_next_id
    friend class Net;

    //--------------------------------------------------------------------------
    //! \brief Constructor. To be used when loading a Petri net from a file.
    //! \param[in] id_: unique node identifier. Shall be unique (responsibility
    //! given to the caller class).
    //! \param[in] caption_: Displayed text under the node.
    //! \param[in] x_: X-axis coordinate in the window needed for the display.
    //! \param[in] y_: Y-axis coordinate in the window needed for the display.
    //! \param[in] recep_: initial receptivity value.
    //--------------------------------------------------------------------------
    Transition(size_t const id_, std::string const& caption_, float const x_,
               float const y_, bool const recep_)
        : Node(Node::Type::Transition, id_, caption_, x_, y_), receptivity(recep_)
    {}

    //--------------------------------------------------------------------------
    //! \brief Static method stringify a transition given an identifier.
    //! \return "T42" for example.
    //--------------------------------------------------------------------------
    inline static std::string to_str(size_t const id_)
    {
        return std::string("T") + std::to_string(id_);
    }

    //--------------------------------------------------------------------------
    //! \brief For debug purpose only.
    //--------------------------------------------------------------------------
    friend std::ostream& operator<<(std::ostream& os, Transition const& t)
    {
        os << t.key << " (\"" << t.caption << "\", " << t.receptivity
           << ", (" << t.x << ", " << t.y << "))";
        return os;
    }

public:

    //! \brief Store the result of the transition condition (boolean expression
    //! of sensors for GRAFCET; always true for timed Petri net; false by
    //! default except if the user clicks on it for Petri).
    //! \note This structure does not store the boolean expression directly but
    //! the result. The simulation will hold boolean expressions. This allows
    //! to separate things.
    bool receptivity = false;

    //! \brief Delay before the transition can fire (GRAFCET temporization).
    //! The transition must remain enabled for this duration before firing.
    //! 0 means no delay (immediate firing when enabled).
    float delay = 0.0f;
};

// *****************************************************************************
//! \brief Two Petri nodes are directed by arcs. Origin and destination nodes
//! shall be of different types. Therefore arcs only make the link between Place
//! to Transition or Transition to Place. Ends are stored as NodeId: use
//! Net::node(), Net::placeOf() or Net::transitionOf() to reach the nodes.
//! \note With timed event graph, we "compress graphically" the net by not
//! drawing places and merging the incoming arc with the outcoming arc).
// *****************************************************************************
class Arc
{
public:

    //--------------------------------------------------------------------------
    //! \brief Constructor. This method expects the two nodes have different
    //! types. The check shall be made by the caller class.
    //! \param[in] from_: Origin node (Place or Transition).
    //! \param[in] to_: Destination node (Place or Transition).
    //! \param[in] duration_: Duration of the process (in unit of time) if \c to_
    //! is a Place (else the duration is forced to NaN).
    //! \note Nodes shall have different types. Assertion is made here.
    //--------------------------------------------------------------------------
    Arc(NodeId const from_, NodeId const to_, float duration_ = 0.0f,
        ArcKind const kind_ = ArcKind::Normal, size_t const weight_ = 1u)
        : from(from_), to(to_),
          duration(from_.isTransition() ? duration_ : safeNaNF()),
          kind(kind_), weight(weight_)
    {
        assert(from.type != to.type);
        assert((kind == ArcKind::Normal) || from.isPlace());
    }

    //--------------------------------------------------------------------------
    //! \brief Constructor from the nodes.
    //--------------------------------------------------------------------------
    Arc(Node const& from_, Node const& to_, float duration_ = 0.0f,
        ArcKind const kind_ = ArcKind::Normal, size_t const weight_ = 1u)
        : Arc(from_.nodeId(), to_.nodeId(), duration_, kind_, weight_)
    {}

    //--------------------------------------------------------------------------
    //! \brief Features of the formalism this arc uses (weight, inhibitor ...).
    //--------------------------------------------------------------------------
    unsigned features() const
    {
        unsigned f = (weight != 1u) ? Feature::Weights : 0u;
        if (kind == ArcKind::Inhibitor)
            f |= Feature::Inhibitors;
        else if (kind == ArcKind::Reset)
            f |= Feature::Resets;
        return f;
    }

    //--------------------------------------------------------------------------
    //! \brief The end of the arc that is a place.
    //--------------------------------------------------------------------------
    NodeId place() const { return from.isPlace() ? from : to; }

    //--------------------------------------------------------------------------
    //! \brief The end of the arc that is a transition.
    //--------------------------------------------------------------------------
    NodeId transition() const { return from.isTransition() ? from : to; }

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    friend std::ostream& operator<<(std::ostream& os, Arc const& a)
    {
        os << a.from.key() << " -> " << a.to.key();
        return os;
    }

    //! \brief Origin node (Place or Transition). Its type shall be different to
    //! the destination node.
    NodeId from;
    //! \brief Destination node (Place or Transition). Its type shall be
    //! different to the origin node.
    NodeId to;
    //! \brief Timed Petri. The time unit is context dependent.
    //! \note for the animation of tokens during the simulation, seconds are
    //! used.
    float duration;
    //! \brief Normal, inhibitor or reset arc.
    ArcKind kind = ArcKind::Normal;
    //! \brief Tokens taken or given (normal arc), threshold (inhibitor arc).
    //! At least 1.
    size_t weight = 1u;
};

// *****************************************************************************
//! \brief The arcs entering or leaving a node, seen as a container of Arc&.
//! Given by Net::arcsIn() and Net::arcsOut(); valid until the structure of
//! the net changes.
// *****************************************************************************
template<class ArcT, class Container>
class ArcRange
{
public:

    class iterator
    {
    public:

        iterator(Container& arcs, std::vector<size_t>::const_iterator it)
            : m_arcs(&arcs), m_it(it)
        {}

        ArcT& operator*() const { return (*m_arcs)[*m_it]; }
        ArcT* operator->() const { return &(*m_arcs)[*m_it]; }
        iterator& operator++() { ++m_it; return *this; }
        bool operator==(iterator const& other) const { return m_it == other.m_it; }
        bool operator!=(iterator const& other) const { return m_it != other.m_it; }

    private:

        Container* m_arcs;
        std::vector<size_t>::const_iterator m_it;
    };

    ArcRange(Container& arcs, std::vector<size_t> const& indices)
        : m_arcs(arcs), m_indices(indices)
    {}

    iterator begin() const { return iterator(m_arcs, m_indices.begin()); }
    iterator end() const { return iterator(m_arcs, m_indices.end()); }
    size_t size() const { return m_indices.size(); }
    bool empty() const { return m_indices.empty(); }
    ArcT& operator[](size_t const i) const { return m_arcs[m_indices[i]]; }
    ArcT& front() const { return m_arcs[m_indices.front()]; }
    //! \brief Indices of the arcs in Net::arcs().
    std::vector<size_t> const& indices() const { return m_indices; }

private:

    Container& m_arcs;
    std::vector<size_t> const& m_indices;
};

// *****************************************************************************
//! \brief Dirty flag of a net that also counts the changes. Editing code
//! writes `net.modified = true`; saving writes `net.modified = false`. Each
//! `= true` increases revision(), so a cache made for an older revision is
//! known to be stale.
// *****************************************************************************
class ModifiedFlag
{
public:

    ModifiedFlag() : m_revision(nextRevision()) {}
    ModifiedFlag(ModifiedFlag const&) : m_revision(nextRevision()) {}

    ModifiedFlag& operator=(ModifiedFlag const& other)
    {
        m_value = other.m_value;
        m_revision = nextRevision();
        return *this;
    }

    ModifiedFlag& operator=(bool const value)
    {
        m_value = value;
        if (value)
            m_revision = nextRevision();
        return *this;
    }

    operator bool() const { return m_value; }

    //! \brief Changes when the net changes. Unique among all nets, so a net
    //! rebuilt at the same address does not reuse an old revision.
    size_t revision() const { return m_revision; }

private:

    static size_t nextRevision()
    {
        static std::atomic<size_t> counter{0u};
        return ++counter;
    }

    bool m_value = false;
    size_t m_revision;
};

// *****************************************************************************
//! \brief Class storing and managing Places, Transitions and Arcs.
//! This class does not offer method for the simulation but has to be seen as a
//! a container with helper methods for implementing algorithms.
// *****************************************************************************
class Net
{
    friend bool convertTo(Net& net, TypeOfNet const type, std::string& error,
                          std::vector<size_t>& erroneous_arcs);

public:

    using Places = std::deque<Place>;
    using Transitions = std::deque<Transition>;
    using Arcs = std::deque<Arc>;
    using ArcsOf = ArcRange<Arc, Arcs>;
    using ConstArcsOf = ArcRange<Arc const, Arcs const>;

    //--------------------------------------------------------------------------
    //! \brief Default constructor.
    //! \param[in] mode: select the type of net: GRAFCET or timed Petri net
    //! or Petri net.
    //--------------------------------------------------------------------------
    explicit Net(TypeOfNet const type = TypeOfNet::TimedPetriNet);

    //--------------------------------------------------------------------------
    //! \brief Copy. Arcs refer to nodes by identifiers, so the copy refers to
    //! its own nodes. The copy is not modified.
    //--------------------------------------------------------------------------
    Net(Net const& other);
    Net& operator=(Net const& other);
    Net(Net&&) = default;
    Net& operator=(Net&&) = default;

    //--------------------------------------------------------------------------
    //! \brief Remove all nodes and arcs. Reset counters for unique identifiers.
    //! Change the type of net for the new one. Reset the name of the net (give
    //! the name of type of net).
    //--------------------------------------------------------------------------
    void reset(TypeOfNet const type);

    //--------------------------------------------------------------------------
    //! \brief Remove all nodes and arcs. Reset counters for unique identifiers.
    //--------------------------------------------------------------------------
    void clear();

    //--------------------------------------------------------------------------
    //! \brief Return the type of net: GRAFCET, Petri, Timed Petri ...
    //--------------------------------------------------------------------------
    inline TypeOfNet type() const { return m_type; }

    //--------------------------------------------------------------------------
    //! \brief Capacity given to the places of a net of the given type: 1 for
    //! GRAFCET (1-S net), else unbounded.
    //--------------------------------------------------------------------------
    static size_t defaultCapacity(TypeOfNet const type)
    {
        return (type == TypeOfNet::GRAFCET) ? 1u : std::numeric_limits<size_t>::max();
    }

    //--------------------------------------------------------------------------
    //! \brief Return true if the Petri nets has no nodes (no places and no
    //! transitions).
    //--------------------------------------------------------------------------
    inline bool isEmpty() const
    {
        return (m_places.size() == 0u) && (m_transitions.size() == 0u);
    }

    //--------------------------------------------------------------------------
    //! \brief Helper function to create either a place or a transition and
    //! and returning the reference to the base class.
    //--------------------------------------------------------------------------
    Node& addOppositeNode(Node::Type const type, float const x, float const y, size_t const tokens = 0u)
    {
        if (type == Node::Type::Transition)
            return addPlace(x, y, tokens);
        return addTransition(x, y);
    }

    //--------------------------------------------------------------------------
    //! \brief Add a new Petri Place. To be used when the user clicked on the
    //! GUI.
    //! \param[in] x: X-axis coordinate in the window needed for the display.
    //! \param[in] y: Y-axis coordinate in the window needed for the display.
    //! \param[in] tokens: Initial number of tokens in the place.
    //! \return the reference of the created place.
    //--------------------------------------------------------------------------
    Place& addPlace(float const x, float const y, size_t const tokens = 0u);

    //--------------------------------------------------------------------------
    //! \brief Add a new Petri Place. To be used when loading a Petri net from
    //! a file.
    //! \param[in] id: unique node identifier. Identifiers are indices in
    //! places(): it shall be the number of places (importers sort nodes).
    //! \param[in] x: X-axis coordinate in the window needed for the display.
    //! \param[in] y: Y-axis coordinate in the window needed for the display.
    //! \param[in] tokens: Initial number of tokens in the place.
    //! \return the reference of the created place.
    //--------------------------------------------------------------------------
    Place& addPlace(size_t const id, std::string const& caption, float const x,
                    float const y, size_t const tokens);

    //--------------------------------------------------------------------------
    //! \brief Const getter. Return the reference to the container of Places.
    //--------------------------------------------------------------------------
    inline Places const& places() const { return m_places; }
    // FIXME: because of toCanonicalForm(), inspector
    inline Places& places() { return m_places; }

    //--------------------------------------------------------------------------
    //! \brief Set tokens in all places (aka markings). Can be used for GRAFCET
    //! forcage.
    //! \param[in] tokens_ the vector holding tokens for each places (P0 .. Pn).
    //! \return true if the length of the vector matchs the number of places,
    //! return false else and you shall call message() to get the error message.
    //--------------------------------------------------------------------------
    bool tokens(std::vector<size_t> const& tokens_);

    //--------------------------------------------------------------------------
    //! \brief Return marking (number tokens for each places).
    //! \return the vector holding tokens for each places (P0, P1 .. Pn).
    //--------------------------------------------------------------------------
    std::vector<size_t> tokens() const;

    //--------------------------------------------------------------------------
    //! \brief Add a new Petri Transition. To be called when the user clicked on
    //! the GUI.
    //! \param[in] x: X-axis coordinate in the window needed for the display.
    //! \param[in] y: Y-axis coordinate in the window needed for the display.
    //! \return the reference of the created element.
    //--------------------------------------------------------------------------
    Transition& addTransition(float const x, float const y);

    //--------------------------------------------------------------------------
    //! \brief Add a new Petri Transition. To be used when loading a Petri net
    //! from a file.
    //! \param[in] id: unique node identifier. Identifiers are indices in
    //! transitions(): it shall be the number of transitions.
    //! \param[in] caption: node description.
    //! \param[in] x: X-axis coordinate in the window needed for the display.
    //! \param[in] y: Y-axis coordinate in the window needed for the display.
    //! \return the reference of the created element.
    //--------------------------------------------------------------------------
    Transition& addTransition(size_t const id, std::string const& caption,
                              float const x, float const y);

    //--------------------------------------------------------------------------
    //! \brief Const getter. Return the reference to the container of Transitions.
    //--------------------------------------------------------------------------
    inline Transitions const& transitions() const { return m_transitions; }
    // FIXME because of inspector
    inline Transitions& transitions() { return m_transitions; }

    //--------------------------------------------------------------------------
    //! \brief The node referred by an arc end. The node shall exist.
    //--------------------------------------------------------------------------
    Node& node(NodeId const id);
    Node const& node(NodeId const id) const;

    //--------------------------------------------------------------------------
    //! \brief The place, or the transition, an arc links.
    //--------------------------------------------------------------------------
    Place& placeOf(Arc const& arc) { return m_places[arc.place().id]; }
    Place const& placeOf(Arc const& arc) const { return m_places[arc.place().id]; }
    Transition& transitionOf(Arc const& arc) { return m_transitions[arc.transition().id]; }
    Transition const& transitionOf(Arc const& arc) const { return m_transitions[arc.transition().id]; }

    //--------------------------------------------------------------------------
    //! \brief Arcs entering or leaving the node.
    //--------------------------------------------------------------------------
    ArcsOf arcsIn(Node const& node) { return ArcsOf(m_arcs, adjacency(node).in); }
    ConstArcsOf arcsIn(Node const& node) const { return ConstArcsOf(m_arcs, adjacency(node).in); }
    ArcsOf arcsOut(Node const& node) { return ArcsOf(m_arcs, adjacency(node).out); }
    ConstArcsOf arcsOut(Node const& node) const { return ConstArcsOf(m_arcs, adjacency(node).out); }

    //--------------------------------------------------------------------------
    //! \brief Index of the arc in arcs().
    //--------------------------------------------------------------------------
    size_t indexOf(Arc const& arc) const;

    //--------------------------------------------------------------------------
    //! \brief Check if the transition is enabled by the tokens of the places:
    //! see tpne::isEnabled() in Firing.hpp for the rule (weights, inhibitor
    //! and reset arcs, capacities). A transition without input place is
    //! enabled.
    //--------------------------------------------------------------------------
    bool isEnabled(Transition const& transition) const;

    //--------------------------------------------------------------------------
    //! \brief Check if the transition can fire: it must be enabled (all input
    //! places have tokens) and its guard condition (receptivity) must be true.
    //--------------------------------------------------------------------------
    bool canFire(Transition const& transition) const
    {
        return transition.receptivity && isEnabled(transition);
    }

    //--------------------------------------------------------------------------
    //! \brief Return the number of tokens the transition consumes from each
    //! input place when it fires: tokens are consumed one by one, so 1 if it
    //! can fire else 0. A transition without input place fires when its
    //! receptivity is true.
    //--------------------------------------------------------------------------
    size_t maxTokensToConsume(Transition const& transition) const;

    //--------------------------------------------------------------------------
    //! \brief Return true if the transition has no input arc but output arcs.
    //! This method is useful for converting event graph to (max,+) dynamic
    //! linear systems:
    //! X(n) = D X(n) ⨁ A X(n-1) ⨁ B U(n)
    //! Y(n) = C X(n)
    //! System inputs: B U(n) with U the column vector of system inputs.
    //--------------------------------------------------------------------------
    bool isInput(Transition const& transition) const
    {
        return arcsIn(transition).empty() && !arcsOut(transition).empty();
    }

    //--------------------------------------------------------------------------
    //! \brief Return true if the transition has input arcs but no output arc.
    //! System outputs: Y(n) = C X(n)
    //--------------------------------------------------------------------------
    bool isOutput(Transition const& transition) const
    {
        return !arcsIn(transition).empty() && arcsOut(transition).empty();
    }

    //--------------------------------------------------------------------------
    //! \brief Return true is the transition is not an input or an output for
    //! the system. Systems states: X(n) = D X(n) (+) A X(n-1) with A the state
    //! matrix and D the implicit matrix.
    //--------------------------------------------------------------------------
    bool isState(Transition const& transition) const
    {
        return !arcsIn(transition).empty() && !arcsOut(transition).empty();
    }

    //--------------------------------------------------------------------------
    //! \brief Search and return a place or a transition by its unique
    //! identifier. Search is O(n) where n is the number of nodes. Return
    //! nullptr if not found.
    //! \param[in] key for example "P42" for the Place 42 or "T0" for the
    //! transition 0.
    //--------------------------------------------------------------------------
    Node* findNode(std::string const& key);
    Node const* findNode(std::string const& key) const;

    //--------------------------------------------------------------------------
    //! \brief Search and return a Transition by its unique identifier. Search
    //! is O(n) where n is the number of nodes. Return nullptr if not found.
    //! \param[in] id for example 42 for the Transition 42.
    //--------------------------------------------------------------------------
    Transition* findTransition(size_t const id);

    //--------------------------------------------------------------------------
    //! \brief Search and return a Place by its unique identifier. Search
    //! is O(n) where n is the number of nodes. Return nullptr if not found.
    //! \param[in] id for example 42 for the Place 42.
    //--------------------------------------------------------------------------
    Place* findPlace(size_t const id);

    //--------------------------------------------------------------------------
    //! \brief Add a new arc between two Petri nodes (place or transition). The
    //! duration is only applied for Transition -> Place. If nodes \from and \to
    //! have the same type then an extra node and an extra arc is created.
    //! \param[in] from: source node (Place or Transition).
    //! \param[in] to: destination node (Place or Transition but not of the same
    //! type than the destination node).
    //! \param[in] duration: duration of the process in unit of time. This
    //! information is only important for Transition -> Place arcs (else it is
    //! forced to 0).
    //! \return true if the arc is valid and has been added. Else return false
    //! if an arc is already present or nodes have the same type; call message()
    //! to get the exact reason.
    //! \return error or information message.
    //--------------------------------------------------------------------------
    std::string addArc(Node& from, Node& to, float const duration = 0.0f);

    //--------------------------------------------------------------------------
    //! \brief Add an arc with an intermediate place between the two given
    //! transitions. This method is usefull for timed event graph.
    //--------------------------------------------------------------------------
    bool addArc(Transition& from, Transition& to, size_t const tokens, float const duration);

    //--------------------------------------------------------------------------
    //! \brief Return the address of the arc linking the two given nodes.
    //! \param[in] from: source node (Place or Transition).
    //! \param[in] to: destination node (Place or Transition but not of the same
    //! type than the destination node).
    //! \return the address of the arc if found, else return nullptr.
    //--------------------------------------------------------------------------
    Arc* findArc(Node const& from, Node const& to);
    Arc const* findArc(Node const& from, Node const& to) const;

    //--------------------------------------------------------------------------
    //! \brief Const getter of all arcs.
    //--------------------------------------------------------------------------
    inline Arcs const& arcs() const { return m_arcs; }
    inline Arcs& arcs() { return m_arcs; } // FIXME because of inspector

    //--------------------------------------------------------------------------
    //! \brief Remove an existing Place or a Transition (usually referred by the
    //! mouse cursor) and its arcs. For timed event graphs, removing a
    //! transition also removes the implicit places of its arcs.
    //! Identifiers stay 0 .. n-1: the last node of the same type takes the
    //! identifier of the removed one (with its key, and its caption when it
    //! was the default one). References to nodes and arcs are invalidated.
    //--------------------------------------------------------------------------
    void removeNode(Node& node);

    //--------------------------------------------------------------------------
    //! \brief Remove an existing arc.
    //--------------------------------------------------------------------------
    bool removeArc(Arc const& arc);

    //--------------------------------------------------------------------------
    //! \brief Remove an existing arc.
    //--------------------------------------------------------------------------
    bool removeArc(Node const& from, Node const& to);

    //--------------------------------------------------------------------------
    //! \brief Give transitions the receptivity they have when no simulation
    //! runs: false for Petri nets (the user clicks to fire) and GRAFCET (the
    //! engine computes them), true for timed nets.
    //--------------------------------------------------------------------------
    void resetReceptivies();

protected:

    //--------------------------------------------------------------------------
    //! \brief Helper function checking arguments.
    //! \param[in] strict: if set true then types of nodes for \from and \to
    //! shall differ (one shall be Transition, the other shall be Place). If set
    //! to false then nodes can be of the same type.
    //! \param[out] message: error or information message.
    //--------------------------------------------------------------------------
    bool sanityArc(Node const& from, Node const& to, bool const strict,
            std::stringstream& message) const;

    //--------------------------------------------------------------------------
    //! \brief Remove the place, or the transition, of the given identifier.
    //! Its arcs shall have been removed. The last node takes its place (and
    //! its identifier) so identifiers stay 0 .. n-1. Adjacency is not updated.
    //--------------------------------------------------------------------------
    void removePlaceAt(size_t const id);
    void removeTransitionAt(size_t const id);

    //--------------------------------------------------------------------------
    //! \brief Arcs whose end is \c from now have \c to as end.
    //--------------------------------------------------------------------------
    void renameArcEnds(NodeId const from, NodeId const to);

public:

    //--------------------------------------------------------------------------
    //! \brief GRAFCET: the step `from` is now numbered `to` (a removed step
    //! gives its number to the last one): the receptivities, the conditions
    //! and the forcing orders of this net that name it follow.
    //--------------------------------------------------------------------------
    void renameStep(size_t from, size_t to);

    //--------------------------------------------------------------------------
    //! \brief GRAFCET: the same in this net for the step of another graph
    //! ("Production:X3", forcing orders of Production).
    //--------------------------------------------------------------------------
    void renameStepOf(std::string const& graph, size_t from, size_t to);

private:

    //! \brief Arcs entering and leaving a node, by index in m_arcs.
    struct Adjacency
    {
        std::vector<size_t> in;
        std::vector<size_t> out;
    };

    //--------------------------------------------------------------------------
    //! \brief Adjacency of the node (empty for an unknown node).
    //--------------------------------------------------------------------------
    Adjacency const& adjacency(Node const& node) const;

    //--------------------------------------------------------------------------
    //! \brief Rebuild the arcs entering and leaving each node. Called by each
    //! method changing the structure of the net, and only by them.
    //--------------------------------------------------------------------------
    void generateArcsInArcsOut();

    //--------------------------------------------------------------------------
    //! \brief Give the place the capacity of this type of net and constrain
    //! its tokens.
    //--------------------------------------------------------------------------
    Place& applyCapacity(Place& place) const;

public:

    //! \brief Name of Petri net given by its filename once load() has been called.
    std::string name;
    //! \brief What the net does, for the reader (shown by the editor).
    std::string description;
    //! \brief GRAFCET only: inputs (sensors, buttons ...) declared with their
    //! initial value. Inputs read by receptivities but not declared here
    //! start at 0.
    std::map<std::string, int> inputs;
    //! \brief Editor has changed content and save is needed. Each time it is
    //! set to true, modified.revision() increases: caches of results computed
    //! from the net (analysis, compiled receptivities) compare it.
    ModifiedFlag modified;

    //! \brief Last error message from Net operations (e.g. tokens()).
    std::string const& error() const { return m_error; }

private:

    std::string m_error;

    //! \brief Type of net GRAFCET, Petri, Timed Petri ...
    TypeOfNet m_type;
    //! \brief List of Places. We do not use std::vector to avoid invalidating
    //! node references for arcs after a possible resizing.
    Places m_places;
    //! \brief List of Transitions. We do not use std::vector to avoid
    //! invalidating node references for arcs after a possible resizing.
    Transitions m_transitions;
    //! \brief List of Arcs.
    Arcs m_arcs;
    //! \brief Arcs of each place and of each transition, by node identifier.
    std::vector<Adjacency> m_place_arcs;
    std::vector<Adjacency> m_transition_arcs;
    //! \brief Auto increment unique identifier. Start from 0 (code placed in
    //! the cpp file).
    size_t m_next_place_id = 0u;
    //! \brief Auto increment unique identifier. Start from 0 (code placed in
    //! the cpp file). Note: their reset is possible through class friendship.
    size_t m_next_transition_id = 0u;
};

//-----------------------------------------------------------------------------
//! \brief Return the string of the type of Petri net.
//-----------------------------------------------------------------------------
std::string to_str(TypeOfNet const type);

//-----------------------------------------------------------------------------
//! \brief Names of the features ("weights, inhibitor arcs").
//-----------------------------------------------------------------------------
std::string to_str(ArcKind const kind);
std::string featuresToStr(unsigned const features);

//-----------------------------------------------------------------------------
//! \brief Features of the formalism the net uses (Feature bits).
//-----------------------------------------------------------------------------
unsigned features(Net const& net);

//-----------------------------------------------------------------------------
//! \brief Features a type of net accepts: all for Petri nets and timed Petri
//! nets, none for timed event graphs ((max,+) algebra) and GRAFCET.
//-----------------------------------------------------------------------------
unsigned allowedFeatures(TypeOfNet const type);
inline bool allows(TypeOfNet const type, unsigned const features)
{
    return (features & ~allowedFeatures(type)) == 0u;
}

//-----------------------------------------------------------------------------
//! \brief Convert to the desired type of net: GRAFCET, Petri, timed Petri,
//! timed graph even, etc.
//! \return false if the net cannot be changed (i.e. to graph event): the
//! indices of the arcs preventing the conversion are given.
//-----------------------------------------------------------------------------
bool convertTo(Net& net, TypeOfNet const type, std::string& error, std::vector<size_t>& erroneous_arcs);

//-----------------------------------------------------------------------------
//! \brief Load the Petri net from a a file. The current net is cleared
//! before the loading. If the loading failed (missing file or invalid
//! syntax) the net is set dummy.
//! \param[in] filename: the file path in where a Petri net has been
//! saved. Should have the .json extension.
//! \return error message in case of failure, else return dummy string.
//-----------------------------------------------------------------------------
// FIXME remplacer bool& springify par bool const springify
std::string loadFromFile(Net& net, std::string const& filepath, bool& springify);

//-----------------------------------------------------------------------------
//! \brief Save the Petri net in a a file.
//! \param[in] filename: the file path in where to save the Petri net.
//! Should have the .json extension.
//! \return error message in case of failure, else return dummy string.
//-----------------------------------------------------------------------------
std::string saveToFile(Net const& net, std::string const& filename);

} // namespace tpne

#endif
