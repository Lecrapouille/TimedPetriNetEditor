//=====================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2022 Quentin Quadrat <lecrapouille@gmail.com>
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
//=====================================================================

#ifndef PETRI_NET_HPP
#  define PETRI_NET_HPP

#  include <SFML/System/Clock.hpp> // For fading
#  include <math.h> // Nan
#  include <atomic>
#  include <string>
#  include <deque>
#  include <vector>
#  include <cassert>
#  include <algorithm> // std::random_shuffle
#  include <random>
#  include <iostream>
#  include <sstream>

class Arc;
struct SparseMatrix;

// *****************************************************************************
//! \brief Settings for defining the type of net (GRAFCET, Petri net, timed
//! petri net). This structure is global for the current net and is used by the
//! PetriNet class.
// *****************************************************************************
struct Settings
{
    //! \brief Max number of tokens in places. For GRAFCET: 1. For Petri nets:
    //! std::numeric_limits<size_t>::max().
    static size_t maxTokens;

    //! \brief The theory would burn the maximum possibe of tokens that we can
    //! in a single action (Fire::MaxPossible) but we can also try to burn
    //! tokens one by one and randomize the transitions (Fire::OneByOne).
    enum class Fire { OneByOne, MaxPossible };

    //! \brief Burn tokens one by one or as many as possible.
    static Fire firing;
};

// *****************************************************************************
//! \brief Since Petri nets are bipartite graph there are two kind of nodes:
//! place and transition. This class shall not be used directly as instance, it
//! only allows to factorize the code of derived class Place and Transition.
// *****************************************************************************
class Node
{
public:

    //--------------------------------------------------------------------------
    //! \brief Petri net is a bipartite graph. So we have to define the type of
    //! the node: Place or Transition.
    //--------------------------------------------------------------------------
    enum Type { Place, Transition };

    //--------------------------------------------------------------------------
    //! \brief Constructor. No sanity checks are made in this method.
    //! \param[in] type: Type of Petri node: Place or Transition.
    //! \param[in] id_: unique identifier (0u, 1u, ...).
    //! \param[in] caption_: Text to displaying indentifying the node.
    //! \param[in] x_: X-axis coordinate in the window needed for the display.
    //! \param[in] y_: Y-axis coordinate in the window needed for the display.
    //--------------------------------------------------------------------------
    Node(Type const type_, size_t const id_, std::string const& caption_,
         float const x_, float const y_)
        : type(type_), id(id_),
          key((type == Node::Type::Place ? 'P' : 'T') + std::to_string(id)),
          x(x_), y(y_),
          caption(caption_.empty() ? key : caption_)
    {
        fading.restart();
    }

    //--------------------------------------------------------------------------
    //! \brief Copy operator needed because this class has constant member
    //! variables.
    //--------------------------------------------------------------------------
    Node& operator=(Node const& other)
    {
        this->~Node(); // destroy
        new (this) Node(other); // copy construct in place
        return *this;
    }

    //--------------------------------------------------------------------------
    //! \brief Needed to remove compilation warnings.
    //--------------------------------------------------------------------------
    Node(Node const& other)
        : Node(other.type, other.id, other.caption, other.x, other.y)
    {}

    //--------------------------------------------------------------------------
    //! \brief Needed to remove compilation warnings.
    //--------------------------------------------------------------------------
    Node(Node&& other)
        : Node(other.type, other.id, other.caption, other.x, other.y)
    {}

    //--------------------------------------------------------------------------
    //! \brief Needed to remove compilation warnings.
    //--------------------------------------------------------------------------
    Node& operator=(Node&& other)
    {
        this->~Node(); // destroy
        new (this) Node(other); // copy construct in place
        return *this;
    }

    //--------------------------------------------------------------------------
    //! \brief Compare node with another node. Perform a check ont the type of
    //! node and on the unique identifier.
    //--------------------------------------------------------------------------
    inline bool operator==(Node const &other) const
    {
        return (type == other.type) && (id == other.id);
    }

    //--------------------------------------------------------------------------
    //! \brief Compare node with another node. Perform a check ont the type of
    //! node and on the unique identifier.
    //--------------------------------------------------------------------------
    inline bool operator!=(Node const &other) const { return !(*this == other); }

public:

    //! \brief Type of nodes: Petri Place or Petri Transition.
    Type const type;
    //! \brief Unique identifier (auto-incremented from 0 by the derived class).
    size_t const id;
    //! \brief Unique node identifier as string. It is formed by the 'P' char
    //! for place or by the 'T' char for transition followed by the unique
    //! identifier (i.e. "P0", "P1", "T0", "T1", ...). Once created it is not
    //! supposed to be changed.
    std::string const key;
    //! \brief Position in the window needed for the display.
    float x;
    //! \brief Position in the window needed for the display.
    float y;
    //! \brief Text displayed near a node the user can modify. Defaut value is
    //! the tring unique \c key.
    std::string caption;
    //! \brief Timer for fading colors.
    sf::Clock fading;
    //! \brief Hold the incoming arcs to access to predecessor nodes.
    //! \note this vector is updated by the method
    //! PetriNet::generateArcsInArcsOut() and posible evolution could be to
    //! update dynamicaly this vector when editing the net through the GUI.
    std::vector<Arc*> arcsIn;
    //! \brief Hold the outcoming arcs to access to successor nodes.
    //! \note this vector is updated by the method
    //! PetriNet::generateArcsInArcsOut() and posible evolution could be to
    //! update dynamicaly this vector when editing the net through the GUI.
    std::vector<Arc*> arcsOut;
};

// *****************************************************************************
//! \brief Petri Place node. Places represent system states. Places hold tokens
//! (resources). In Grafcet, Place has only one token and when they are
//! activated actions are performed. This class does not managed Grafcet.
// *****************************************************************************
class Place : public Node
{
public:

    //! \brief to access s_next_id
    friend class PetriNet;

    //--------------------------------------------------------------------------
    //! \brief Constructor. To be used when loading a Petri net from JSON file.
    //! \param[in] id_: unique node identifier. Shall be unique (responsability
    //!   given to the caller class.
    //! \param[in] caption_: Text to displaying indentifying the node.
    //! \param[in] x_: X-axis coordinate in the window needed for the display.
    //! \param[in] y_: Y-axis coordinate in the window needed for the display.
    //! \param[in] tokens_: Initial number of tokens in the place.
    //--------------------------------------------------------------------------
    Place(size_t const id_, std::string const& caption_, float const x_,
          float const y_, size_t const tokens_)
        : Node(Node::Type::Place, id_, caption_, x_, y_)
    {
        // Petri net: infinite number of tokens but in GRAFCET max is one.
        tokens = std::min(Settings::maxTokens, tokens_);
    }

    //--------------------------------------------------------------------------
    //! \brief Static method stringifing a place given an identifier.
    //! \return "P42" for example.
    //--------------------------------------------------------------------------
    inline static std::string to_str(size_t const id_)
    {
        return std::string("P" + std::to_string(id_));
    }

    //--------------------------------------------------------------------------
    //! \brief
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
};

// *****************************************************************************
//! \brief Petri Transition node. In Petri transitivities are always true and
//! when in each above Places, all of them have at least one token they let pass
//! tokens. In Grafcet, transitivities have guards (boolean expression, usually
//! depending on external systems events (inputs, sensors, alarm ...).
//! FIXME This class does not managed Grafcet.
// *****************************************************************************
class Transition : public Node
{
public:

    //! \brief to access s_next_id
    friend class PetriNet;

    //--------------------------------------------------------------------------
    //! \brief Constructor. To be used when loading a Petri net from JSON file.
    //! \param[in] id: unique node identifier.
    //! \param[in] x: X-axis coordinate in the window needed for the display.
    //! \param[in] y: Y-axis coordinate in the window needed for the display.
    //! \param[in] angle_: angle in degree of rotation for the display.
    //! \param[in] recep_: receptivity of the transition.
    //--------------------------------------------------------------------------
    Transition(size_t const id_, std::string const& caption_, float const x_,
               float const y_, int const angle_, bool const recep_)
        : Node(Node::Type::Transition, id_, caption_, x_, y_), angle(angle_),
          receptivity(recep_)
    {}

    //--------------------------------------------------------------------------
    //! \brief Static method stringifing a transition given an identifier.
    //! \return "T42" for example.
    //--------------------------------------------------------------------------
    inline static std::string to_str(size_t const id_)
    {
        return std::string("T" + std::to_string(id_));
    }

    //--------------------------------------------------------------------------
    //! \brief Check if the transition is validated (meaning if the receptivity
    //! is true) or not validated (meaning if the receptivity is false).
    //--------------------------------------------------------------------------
    inline bool isValidated() const { return receptivity; }

    //--------------------------------------------------------------------------
    //! \brief Check if all upstream places have all at leat one token (meaning
    //! if all upstream places (steps for GRAFCET) are activated.
    //--------------------------------------------------------------------------
    bool isEnabled() const;

    //--------------------------------------------------------------------------
    //! \brief Check if the transition is validated and all upstream places have
    //! all at leat one token. In this case the transition is passable and can
    //! burn tokens in upstream places.
    //! \note The firing is made by the PetriNet class.
    //! \return true if can fire else return false.
    //--------------------------------------------------------------------------
    bool canFire() const { return isValidated() && isEnabled(); }

    //--------------------------------------------------------------------------
    //! \brief Return the maximum possibe of tokens that can be burnt in
    //!   upstream places.
    //! \note This method does not modify the number of tokens in predecessor
    //!   places.
    //! \return the number of tokens that be burnt or 0u if cannot fire.
    //--------------------------------------------------------------------------
    size_t howManyTokensCanBurnt() const;

    //--------------------------------------------------------------------------
    //! \brief Return true if the transition comes from an input place.
    //--------------------------------------------------------------------------
    inline bool isInput() const
    {
        return (arcsIn.size() == 0u) && (arcsOut.size() > 0u);
    }

    //--------------------------------------------------------------------------
    //! \brief Return true if the transition goes to an output place.
    //--------------------------------------------------------------------------
    inline bool isOutput() const
    {
        return (arcsIn.size() > 0u) && (arcsOut.size() == 0u);
    }

    //--------------------------------------------------------------------------
    //! \brief Return true is the transition is not an input or an output for
    //! the system.
    //--------------------------------------------------------------------------
    inline bool isState() const
    {
        return (arcsIn.size() > 0u) && (arcsOut.size() > 0u);
    }

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    friend std::ostream& operator<<(std::ostream& os, Transition const& t)
    {
        os << t.key << " (\"" << t.caption << "\", " << t.receptivity
           << ", (" << t.x << ", " << t.y << "))";
        return os;
    }

public:

    //! \brief Transitions are depicted by rectangles. We allow to rotate it
    //! to have horizontal, vertical or diagonal shape transitions.
    int angle = 0;

    //! \brief In petri net mode, the user has to click to validate the
    //! receptivity of the transition. If upstream places have all at least one
    //! token, the transition is fired, burning tokens in upstream places and
    //! create tokens in the successor places.
    bool receptivity = false;

    //! \brief Temporary matrix index used when building (max,+) linear systems.
    size_t index = 0u;
};

// *****************************************************************************
//! \brief Two Petri nodes are directed by arcs. Origin and destination nodes
//! shall be of different types. Therefore arcs only make the link between Place
//! to Transition or Transition to Place.
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
    //! is a Place (else the duration is forced to 0).
    //! \note Nodes shall have different types. Assertion is made here.
    //--------------------------------------------------------------------------
    Arc(Node& from_, Node& to_, float duration_ = 0.0f)
        : from(from_), to(to_),
          duration(from_.type == Node::Type::Transition ? duration_ : NAN)
    {
        assert(from.type != to.type);
        fading.restart();
    }

    //--------------------------------------------------------------------------
    //! \brief Hack needed because of references
    //--------------------------------------------------------------------------
    Arc& operator=(Arc const& other)
    {
        this->~Arc(); // destroy
        new (this) Arc(other); // copy construct in place
        return *this;
    }

    //--------------------------------------------------------------------------
    //! \brief Needed to remove compilation warnings
    //--------------------------------------------------------------------------
    Arc(Arc const& other)
        : Arc(other.from, other.to, other.duration)
    {}

    //--------------------------------------------------------------------------
    //! \brief Needed to remove compilation warnings
    //--------------------------------------------------------------------------
    Arc(Arc&& other)
        : Arc(other.from, other.to, other.duration)
    {}

    //--------------------------------------------------------------------------
    //! \brief Needed to remove compilation warnings
    //--------------------------------------------------------------------------
    Arc& operator=(Arc&& other)
    {
        this->~Arc(); // destroy
        new (this) Arc(other); // copy construct in place
        return *this;
    }

    //--------------------------------------------------------------------------
    //! \brief Return the reference to the number of tokens from the origin
    //! Place.
    //--------------------------------------------------------------------------
    inline size_t& tokensIn()
    {
        assert(from.type == Node::Place);
        return reinterpret_cast<Place&>(from).tokens;
    }

    //--------------------------------------------------------------------------
    //! \brief Return the reference to the number of tokens from the destination
    //! Place.
    //--------------------------------------------------------------------------
    inline size_t& tokensOut()
    {
        assert(to.type == Node::Place);
        return reinterpret_cast<Place&>(to).tokens;
    }

    //--------------------------------------------------------------------------
    //! \brief Compare node with another node. Perform a check ont the type of
    //! node and on the unique identifier.
    //--------------------------------------------------------------------------
    inline bool operator==(Arc const &other) const
    {
        return (from == other.from) && (to == other.to);
    }

    //--------------------------------------------------------------------------
    //! \brief Compare node with another node. Perform a check ont the type of
    //! node and on the unique identifier.
    //--------------------------------------------------------------------------
    inline bool operator!=(Arc const &other) const
    {
        return !(*this == other);
    }

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    friend std::ostream& operator<<(std::ostream& os, Arc const& a)
    {
        os << a.from.key << " \"" << a.from.caption << "\" -> "
           << a.to.key << " \"" << a.to.caption << "\"";
        return os;
    }

    //! \brief Origin node (Place or Transition). Its type shall be different to
    //! the destination node.
    Node& from;
    //! \brief Destination node (Place or Transition). Its type shall be
    //! different to the origin node.
    Node& to;
    //! \brief Timed Petri. The time unit is context dependent.
    //! \note for the animation of tokens during the simulation, seconds are
    //! used.
    float duration;
    //! \brief Temporary memory used for counting tokens when they arrive to
    //! their destination node (place) and their conversion to an AnimatedToken
    //! instance. This variable is used for avoiding to display on the same
    //! coordinate several AnimatedToken with the same number of tokens as
    //! caption. Instead, a single AnimatedToken holding the sum of tokens is
    //! displayed. This sum is sensitive to the animation frame rate, a lower
    //! framerate is suggested to force bigger coordinate steps avoiding
    //! overlapping AnimatedToken.
    size_t count = 0u;
    //! \brief Timer for fading colors.
    sf::Clock fading;
};

// *****************************************************************************
//! \brief Container class holding and managing Places, Transitions and
//! Arcs. This class does not manage simulation. FIXME to be defined: since
//! currently the PetriEditor is doing the simulation which is mainly animations
//! with some basic features such as burning tokens ...
// *****************************************************************************
class PetriNet
{
public:

    using Places = std::deque<Place>;
    using Transitions = std::deque<Transition>;
    using Arcs = std::deque<Arc>;

    //--------------------------------------------------------------------------
    //! \brief Configure the net as GRAFCET or timed Petri net or Petri net.
    //! TODO GraphEvent displaying graph like doc/Graph01.png
    //! TODO StateMachine
    //--------------------------------------------------------------------------
    enum class Type
    {
        //! \brief The user has to click on transitions to fire.
        //! Tokens are burnt one by one.
        Petri,
        //! \brief Is a Petri with duration on arcs transition -> palce. When
        //! transitions are enables, firing is automatic, on divergence
        //! transition, the firing is shuffle and the maximum of tokens are
        //! burnt in once. The user cannot click to transitions to fire.
        TimedPetri,
        //! \brief Is a timed Petri where all places have a single input arc and
        //! a single output arc. TODO This mode is not yet implemented: the
        //! editor shall not display places.
        TimedGraphEvent,
        //! \brief Is a Petri net used for making automata: Places do actions
        //! and receptivities are linked to sensors. Steps (the name for Places)
        //! have at maximum one token (1-S net). TODO This mode is partially
        //! implemented: partial GRAFCET norm is respected, GUI does not allow
        //! to simulated actions and sensors.
        GRAFCET,
        //! \brief TODO Is a 1-S Petri net This mode is not managed
        StateMachine
    };

    //--------------------------------------------------------------------------
    //! \brief Default constructor.
    //! \param[in] mode: select the type of net: GRAFCET or timed Petri net
    //! or Petri net.
    //--------------------------------------------------------------------------
    PetriNet(PetriNet::Type const mode) { this->changeTypeOfNet(mode); }

    //--------------------------------------------------------------------------
    //! \brief Copy constructor. Needed to remove compilation warnings.
    //--------------------------------------------------------------------------
    PetriNet(PetriNet const& other) { *this = other; }

    //--------------------------------------------------------------------------
    //! \brief Copy operator. Needed to remove compilation warnings.
    //--------------------------------------------------------------------------
    PetriNet& operator=(PetriNet const& other);

    //--------------------------------------------------------------------------
    //! \brief Remove all nodes and arcs. Reset counters for unique identifiers.
    //! \note the type of net (timed, classic ... ) stay the same.
    //--------------------------------------------------------------------------
    void clear();

    //--------------------------------------------------------------------------
    //! \brief Set the type of net: GRAFCET, Petri, Timed Petri ...
    //--------------------------------------------------------------------------
    void changeTypeOfNet(PetriNet::Type const type);

    //--------------------------------------------------------------------------
    //! \brief Get the type of net: GRAFCET, Petri, Timed Petri ...
    //--------------------------------------------------------------------------
    inline PetriNet::Type type() const { return m_type; }

    //--------------------------------------------------------------------------
    //! \brief Return true if the Petri nets has no nodes (no places and no
    //! transitions).
    //--------------------------------------------------------------------------
    inline bool isEmpty() const
    {
        return (m_places.size() == 0u) && (m_transitions.size() == 0u);
    }

    //--------------------------------------------------------------------------
    //! \brief Add a new Petri Place. To be used when the user clicked on the
    //! GUI.
    //! \param[in] x: X-axis coordinate in the window needed for the display.
    //! \param[in] y: Y-axis coordinate in the window needed for the display.
    //! \param[in] tokens: Initial number of tokens in the place.
    //! \return the reference of the inserted place.
    //--------------------------------------------------------------------------
    Place& addPlace(float const x, float const y, size_t const tokens = 0u);

    //--------------------------------------------------------------------------
    //! \brief Add a new Petri Place. To be used when loading a Petri net from
    //! JSON file.
    //! \param[in] id: unique node identifier.
    //! \param[in] x: X-axis coordinate in the window needed for the display.
    //! \param[in] y: Y-axis coordinate in the window needed for the display.
    //! \param[in] tokens: Initial number of tokens in the place.
    //! \return the reference of the inserted place.
    //--------------------------------------------------------------------------
    Place& addPlace(size_t const id, std::string const& caption, float const x,
                    float const y, size_t const tokens);

    //--------------------------------------------------------------------------
    //! \brief Const getter. Return the reference to the container of Places.
    //--------------------------------------------------------------------------
    inline Places const& places() const { return m_places; }

    //--------------------------------------------------------------------------
    //! \brief Non const getter. TODO: to be removed (ideally).
    //--------------------------------------------------------------------------
    inline Places& places() { return m_places; }

    //--------------------------------------------------------------------------
    //! \brief Add a new Petri Transition. To be called when the user clicked on
    //! the GUI.
    //! \param[in] x: X-axis coordinate in the window needed for the display.
    //! \param[in] y: Y-axis coordinate in the window needed for the display.
    //! \return the reference of the inserted element.
    //--------------------------------------------------------------------------
    Transition& addTransition(float const x, float const y);

    //--------------------------------------------------------------------------
    //! \brief Add a new Petri Transition. To be used when loading a Petri net
    //! from JSON file.
    //! \param[in] id: unique node identifier.
    //! \param[in] caption: node description.
    //! \param[in] x: X-axis coordinate in the window needed for the display.
    //! \param[in] y: Y-axis coordinate in the window needed for the display.
    //! \param[in] angle: angle of rotation of the displayed rectangle.
    //! \return the reference of the inserted element.
    //--------------------------------------------------------------------------
    Transition& addTransition(size_t const id, std::string const& caption,
                              float const x, float const y, int const angle);

    //--------------------------------------------------------------------------
    //! \brief Const getter. Return the container reference of the shuffled
    //! Transitions.
    //! \param[in] reset if set true, force rebuilding the vector from
    //! transitions before shuffling.
    //--------------------------------------------------------------------------
    std::vector<Transition*> const& shuffle_transitions(bool const reset = false);

    //--------------------------------------------------------------------------
    //! \brief Const getter. Return the reference to the container of Transitions.
    //--------------------------------------------------------------------------
    inline Transitions const& transitions() const { return m_transitions; }

    //--------------------------------------------------------------------------
    //! \brief Non const getter. TODO: to be removed (ideally).
    //--------------------------------------------------------------------------
    inline Transitions& transitions() { return m_transitions; }

    //--------------------------------------------------------------------------
    //! \brief Search and return a place or a transition by its unique
    //! identifier. Search is O(n) where n is the number of nodes. Return
    //! nullptr if not found.
    //! \param[in] key for example "P42" for the Place 42 or "T0" for the
    //! transition 0.
    //--------------------------------------------------------------------------
    Node* findNode(std::string const& key);

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
    //! \brief Add a new arc between two Petri nodes (place or transition) and
    //! a duration (only for Transition -> Place).
    //! \param[in] from: source node (Place or Transition).
    //! \param[in] to: destination node (Place or Transition but not of the same
    //! type than the destination node).
    //! \param[in] duration: duration of the process in unit of time. This
    //! information is only important for Transition -> Place arcs (else it is
    //! forced to 0).
    //! \return true if the arc is valid and has been added. Else return false
    //! if an arc is already present or nodes have the same type; call message()
    //! to get the exact reason.
    //--------------------------------------------------------------------------
    bool addArc(Node& from, Node& to, float const duration = 0.0f, bool const strict = true);

    //--------------------------------------------------------------------------
    //! \brief Return the address of the arc linking the two given nodes.
    //! \param[in] from: source node (Place or Transition).
    //! \param[in] to: destination node (Place or Transition but not of the same
    //! type than the destination node).
    //! \return the address of the arc if found, else return nullptr.
    //--------------------------------------------------------------------------
    Arc* findArc(Node const& from, Node const& to);

    //--------------------------------------------------------------------------
    //! \brief Const getter of all arcs.
    //--------------------------------------------------------------------------
    inline Arcs const& arcs() const { return m_arcs; }

    //--------------------------------------------------------------------------
    //! \brief Non const getter. TODO: to be removed (ideally).
    //--------------------------------------------------------------------------
    inline Arcs& arcs() { return m_arcs; }

    //--------------------------------------------------------------------------
    //! \brief Set tokens in all places.
    //! \param[in] marks the vector holding tokens for each places (P0, P1 .. Pn).
    //! \return true if the length of the vector matchs the number of places,
    //! return false else and you shall call message() to get the error message.
    //--------------------------------------------------------------------------
    bool setTokens(std::vector<size_t> const& marks);

    //--------------------------------------------------------------------------
    //! \brief Get tokens from all places.
    //! \param[out] marks the vector holding tokens for each places (P0, P1 .. Pn).
    //--------------------------------------------------------------------------
    void getTokens(std::vector<size_t>& marks) const;

    //--------------------------------------------------------------------------
    //! \brief Chech if the Petri net is a graph event meaning that each places
    //! have exactly one input arc and one output arc.
    //! \param[inout] erroneous_arcs store detected erroneous arcs.
    //!
    //! \return true if the Petri net is a graph event and \c erroneous_arcs is
    //! empty. Return false if the Petri net is not a graph event and \c
    //! erroneous_arcs will contain defectuous arcs information and call message()
    //! to get the real reason.
    //!
    //! \note call generateArcsInArcsOut(/*arcs: true*/); before calling this
    //! method.
    //--------------------------------------------------------------------------
    bool isEventGraph(std::vector<Arc*>& erroneous_arcs) const;

    //--------------------------------------------------------------------------
    //! \brief Show to the critical circuit of the net (where the cycle takes
    //! the most of time).
    //! \param[inout] result container of arcs storing the cycle.
    //! The container is cleared before reserving its memory.
    //! \return true if the Petri net was an graph event and \c result will
    //! contain the arcs of the cycle. Return false if the Petri net was not an
    //! graph event and \c result will contain erroneous arcs.
    //--------------------------------------------------------------------------
    bool findCriticalCycle(std::vector<Arc*>& result);

    //--------------------------------------------------------------------------
    //! \brief Return the timed event graph as (min,+) system. For example
    //! T0(t) = min(2 + T2(t - 5)); where t - 5 is delay implied by duration on
    //! arcs and min(2 + implied by tokens from incoming places.
    //!
    //! \return the string depicting the timed Petri net as (min,+) system if
    //! this net is a timed graph event. Else return empty string.
    //--------------------------------------------------------------------------
    std::stringstream showCounterForm(std::string const& comment = "# ") const;

    //--------------------------------------------------------------------------
    //! \brief Return the timed event graph as (max,+) system. For example
    //! T0(n) = max(5 + T2(n - 2)); where n - 2 is delay implied by tokens from
    //! incoming places and max(5 + implied by duration from the incoming arc.
    //!
    //! \return the string depicting the timed Petri net as (max,+) system if
    //! this net is a timed graph event. Else return empty string.
    //--------------------------------------------------------------------------
    std::stringstream showDaterForm(std::string const& comment = "# ") const;

    //--------------------------------------------------------------------------
    //! \brief Load the Petri net from a JSON file. The current net is cleared
    //! before the loading. If the loading failed (missing file or invalid
    //! syntax) the net is set dummy.
    //! \param[in] filename: the file path in where a Petri net has been
    //! saved. Should have the .json extension.
    //! \return true if the Petri net has been loaded with success. Return false
    //! in case of failure.
    //--------------------------------------------------------------------------
    inline bool load(std::string const& filename) { return importFromJSON(filename); }

    //--------------------------------------------------------------------------
    //! \brief Save the Petri net in a JSON file.
    //! \param[in] filename: the file path in where to save the Petri net.
    //! Should have the .json extension.
    //! \return true if the Petri net has been saved with success. Return false
    //! in case of failure.
    //--------------------------------------------------------------------------
    inline bool save(std::string const& filename) const { return exportToJSON(filename); }

    //--------------------------------------------------------------------------
    //! \brief Export the Petri net as Symfony workflow code as yaml file.
    //! \param[in] filename the path of yaml file.
    //! \return true if the Petri net has been exported with success. Return
    //! false in case of failure.
    //--------------------------------------------------------------------------
    bool importFlowshop(std::string const& filename);

    //--------------------------------------------------------------------------
    //! \brief Import the Petri net from the given JSON file.  The current net
    //! is cleared before the loading. If the loading failed (missing file or
    //! invalid syntax) the net is set dummy.
    //! \param[in] filename: the file path in where a Petri net has been
    //! saved. Should have the .json extension.
    //! \return true if the Petri net has been loaded with success. Return false
    //! in case of failure.
    //--------------------------------------------------------------------------
    bool importFromJSON(std::string const& filename);

    //--------------------------------------------------------------------------
    //! \brief Export the Petri net as JSON file.
    //! \param[in] filename: the file path in where to save the Petri net.
    //! Should have the .json extension.
    //! \return true if the Petri net has been saved with success. Return false
    //! in case of failure.
    //--------------------------------------------------------------------------
    bool exportToJSON(std::string const& filename) const;

    //--------------------------------------------------------------------------
    //! \brief Export the Grafcet net as LaTeX code as tex file.
    //! \param[in] filename the path of tex file.
    //! \return true if the Grafcet net has been exported with success. Return
    //! false in case of failure.
    //--------------------------------------------------------------------------
    bool exportToGrafcetLaTeX(std::string const& filename) const;

    //--------------------------------------------------------------------------
    //! \brief Export the Petri net as LaTeX code as tex file.
    //! \param[in] filename the path of tex file.
    //! \param[in] scale X and Y scaling.
    //! \return true if the Petri net has been exported with success. Return
    //! false in case of failure.
    //--------------------------------------------------------------------------
    bool exportToPetriLaTeX(std::string const& filename, float const sx, float const sy) const;

    //--------------------------------------------------------------------------
    //! \brief Export the Petri net as Draw.io code as xml file.
    //! \param[in] filename the path of dot file.
    //! \return true if the Petri net has been exported with success. Return
    //! false in case of failure.
    //--------------------------------------------------------------------------
    bool exportToDrawIO(std::string const& filename) const;

    //--------------------------------------------------------------------------
    //! \brief Export the Petri net as Graphviz code as dot file.
    //! \param[in] filename the path of dot file.
    //! \return true if the Petri net has been exported with success. Return
    //! false in case of failure.
    //--------------------------------------------------------------------------
    bool exportToGraphviz(std::string const& filename) const;

    //--------------------------------------------------------------------------
    //! \brief Export the Petri net as Symfony workflow as yaml file.
    //! \param[in] filename the path of dot file.
    //! \return true if the Petri net has been exported with success. Return
    //! false in case of failure.
    //--------------------------------------------------------------------------
    bool exportToSymfony(std::string const& filename) const;

    //--------------------------------------------------------------------------
    //! \brief Export the Petri net to https://gitlab.com/porky11/pn-editor.
    //! \param[in] filename the path of dot file.
    //! \return true if the Petri net has been exported with success. Return
    //! false in case of failure.
    //--------------------------------------------------------------------------
    bool exportToPNEditor(std::string const& filename) const;

    //--------------------------------------------------------------------------
    //! \brief Export the Petri net as GRAFCET code as C++ header file.
    //! \param[in] filename the path of h++ file. Should have the .hpp or .h
    //! extension (or any associated to header header files).
    //--------------------------------------------------------------------------
    bool exportToCpp(std::string const& filename) const;

    //--------------------------------------------------------------------------
    //! \brief Export the Petri net as Max-Plus linear system in Julia code.
    //! \param[in] filename the path of julia file. Should have the .jl
    //! extension.
    //! \return true if the Petri net has been exported with success. Return
    //! false in case of failure and erroneous arcs can be get by \c markedArcs().
    //--------------------------------------------------------------------------
    bool exportToJulia(std::string const& filename) const;

    //--------------------------------------------------------------------------
    //! \brief Return the event graph as 2 adjacency matrices.
    //! \param[out] N the adjacency matrix of tokens.
    //! \param[out] Tthe adjacency matrix of durations.
    //! \note This will work only if isEventGraph() returned true.
    //! \return false if the Petri net is not an event graph.
    //--------------------------------------------------------------------------
    bool toAdjacencyMatrices(SparseMatrix& N, SparseMatrix&T); //TODO const;

    //--------------------------------------------------------------------------
    //! \brief Transform the Event Graph to canonical form
    //! \param[out] pn: resulting Petri net in canonical mode
    //! \note This will work only if isEventGraph() has returned true so make
    //! the caller of this method be aware of what is he doing.
    //--------------------------------------------------------------------------
    void toCanonicalForm(PetriNet& pn) const;

    //--------------------------------------------------------------------------
    //! \brief Return the event graph as implicit dynamic linear Max-Plus system.
    //! X(n) = D X(n) ⨁ A X(n-1) ⨁ B U(n)
    //! Y(n) = C X(n)
    //! \note This will work only if isEventGraph() returned true.
    //! \return true if the Petri net was an graph event, else return false and
    //! erroneous arcs can be get by \c markedArcs().
    //--------------------------------------------------------------------------
    bool toSysLin(SparseMatrix& D, SparseMatrix& A, SparseMatrix& B, SparseMatrix& C); //TODO const;

    //--------------------------------------------------------------------------
    //! \brief Remove an existing Place or a Transition (usually refered by the
    //! mouse cursor).
    //--------------------------------------------------------------------------
    void removeNode(Node& node);

    //--------------------------------------------------------------------------
    //! \brief Remove an existing arc.
    //--------------------------------------------------------------------------
    bool removeArc(Arc const& arc);

    //--------------------------------------------------------------------------
    //! \brief Remove an existing arc.
    //--------------------------------------------------------------------------
    bool removeArc(Node& from, Node& to);

    //--------------------------------------------------------------------------
    //! \brief Populate or update Node::arcsIn and Node::arcsOut for all
    //! transitions and places in the Petri net.
    //--------------------------------------------------------------------------
    void generateArcsInArcsOut();

    //--------------------------------------------------------------------------
    //! \brief Set to false the receptivity for all transitions.
    //--------------------------------------------------------------------------
    void resetReceptivies();

    //--------------------------------------------------------------------------
    //! \brief Return the result of algorithm
    //--------------------------------------------------------------------------
    inline std::vector<Arc*> const& markedArcs() const { return m_result_arcs; }

    //--------------------------------------------------------------------------
    //! \brief Return a error or information message.
    //--------------------------------------------------------------------------
    inline std::string message() const { return m_message.str(); }

private:

    //--------------------------------------------------------------------------
    //! \brief Inner method for the public toSysLin() method.
    //--------------------------------------------------------------------------
    void toSysLin(SparseMatrix& D, SparseMatrix& A, SparseMatrix& B, SparseMatrix& C,
                  size_t const nb_inputs, size_t const nb_states, size_t const nb_outputs);

private:

    //! \brief Type of net GRAFCET, Petri, Timed Petri ...
    Type m_type;
    //! \brief List of Places. We do not use std::vector to avoid invalidating
    //! node references for arcs after a possible resizing.
    Places m_places;
    //! \brief List of Transitions. We do not use std::vector to avoid
    //! invalidating node references for arcs after a possible resizing.
    Transitions m_transitions;
    //! \brief List of shuffled Transitions.
    std::vector<Transition*> m_shuffled_transitions;
    //! \brief List of Arcs.
    Arcs m_arcs;
    //! \brief Store arcs after some algorithm to return either result i.e.
    //! findCriticalCycle() or to return the error (i.e. isEventGraph()).
    std::vector<Arc*> m_result_arcs;
    //! \brief Auto increment unique identifier. Start from 0 (code placed in
    //! the cpp file).
    size_t m_next_place_id = 0u;
    //! \brief Auto increment unique identifier. Start from 0 (code placed in
    //! the cpp file). Note: their reset is possible through class friendship.
    size_t m_next_transition_id = 0u;
    //! \brief Store current info/error message to the Petri net editor.
    mutable std::stringstream m_message;

public:

    //! \brief Petri net has been modified, change the title and ask for saving
    //! the net when leaving the application.
    bool modified = false;
};

#endif
