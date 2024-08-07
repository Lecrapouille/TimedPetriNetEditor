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

#ifndef PETRI_NET_HPP
#  define PETRI_NET_HPP

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

namespace tpne {

class Arc;

//------------------------------------------------------------------------------
//! \brief Enum determining the type of net (GRAFCET, timed Petri net ...) and
//! defining the type of simulation (for example, GRAFCET cannot have more than
//! one token in a step ...).
//! \note: We do not use inheritance, if-then-else on the type of net is used
//! to modify the behavior.
//------------------------------------------------------------------------------
enum class TypeOfNet
{
    //! \brief The user has to click on fireable transitions to burn tokens.
    //! Tokens in incoming places are burnt one by one.
    PetriNet,
    //! \brief Is a Petri with duration on arcs transition -> place.
    //! Receptivities are set to true. When transitions are enabled (when all
    //! immediate incoming places have at least one token), the fire is
    //! automaticaly made (The user cannot click to transitions to fire). The
    //! policy concerning divergence transitions: the maximum of tokens are
    //! burnt in once but tokens are shuffled along arcs.
    TimedPetriNet,
    //! \brief Is a timed Petri where all places have a single input arc and
    //! a single output arc. TODO currently the net is displayed ugly!
    TimedEventGraph,
    //! \brief Is a Petri net used for making industrial automata (productive):
    //! Places are named Steps and do discrete actions. Transitions have boolean
    //! expression (named receptivities aka conditions) linked to sensors
    //! (i.e. door closed and alarm off). Steps have at maximum one token (aka
    //! 1-S net). TODO This mode is partially implemented: partial GRAFCET norm
    //! is respected, GUI does not allow to simulated actions inside the
    //! simulator.
    GRAFCET
    // TODO state machine
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
    enum Type { Place, Transition };

    //--------------------------------------------------------------------------
    //! \brief Constructor. No sanity checks are made in this method.
    //! \param[in] type_: Type of Petri node: Place or Transition.
    //! \param[in] id_: unique identifier (0u, 1u, ...).
    //! \param[in] caption_: Text to displaying indentifying the node.
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
    //! \brief Needed because this class has constant member variables.
    //--------------------------------------------------------------------------
    Node(Node const& other)
        : Node(other.type, other.id, other.caption, other.x, other.y)
    {}

    //--------------------------------------------------------------------------
    //! \brief Needed because this class has constant member variables.
    //--------------------------------------------------------------------------
    Node(Node&& other)
        : Node(other.type, other.id, other.caption, other.x, other.y)
    {}

    //--------------------------------------------------------------------------
    //! \brief Needed because this class has constant member variables.
    //--------------------------------------------------------------------------
    Node& operator=(Node&& other)
    {
        this->~Node(); // destroy
        new (this) Node(other); // copy construct in place
        return *this;
    }

public:

    //! \brief Type of nodes: Petri Place or Petri Transition. Once created, it
    //! is not supposed to be changed.
    Type const type;
    //! \brief Unique identifier (auto-incremented from 0 by the derived class).
    //! Once created, it is not supposed to be changed.
    size_t const id;
    //! \brief Unique node identifier as string. It is formed by the 'P' char
    //! for place or by the 'T' char for transition followed by the unique
    //! identifier (i.e. "P0", "P1", "T0", "T1", ...). Once created, it is not
    //! supposed to be changed.
    //! \fixme: TBD to be replaced by a method instead ?
    std::string const key;
    //! \brief Position inside the window needed for the display.
    //! \fixme: TBD to be moved inside the editor since we do not care of
    //! position.
    float x;
    //! \brief Position in the window needed for the display.
    //! \fixme: TBD to be moved inside the editor since we do not care of
    //! position.
    float y;
    //! \brief Text displayed near a node the user can modify. Defaut value is
    //! the tring unique \c key.
    std::string caption;
    //! \brief Hold the incoming arcs to access to previous nodes.
    //! \note this vector is not updated by this class but shall be made by
    //! the caller.
    std::vector<Arc*> arcsIn;
    //! \brief Hold the outcoming arcs to access to successor nodes.
    //! \note this vector is not updated by this class but shall be made by
    //! the caller.
    std::vector<Arc*> arcsOut;
};

// *****************************************************************************
//! \brief Petri Place node. Places represent system states. Places hold tokens
//! (resources). In GRAFCET, Places are nammed Steps and have at max one token.
//! When steps are activated actions are performed.
// *****************************************************************************
class Place : public Node
{
public:

    //! \brief to access s_next_id
    friend class Net;

    //--------------------------------------------------------------------------
    //! \brief Constructor. To be used when loading a Petri net from a file.
    //! \param[in] id_: unique node identifier. Shall be unique (responsability
    //! given to the caller class).
    //! \param[in] caption_: Displayed text under the node.
    //! \param[in] x_: X-axis coordinate in the window needed for the display.
    //! \param[in] y_: Y-axis coordinate in the window needed for the display.
    //! \param[in] tokens_: Initial number of tokens in the place >= 0.
    //--------------------------------------------------------------------------
    Place(size_t const id_, std::string const& caption_, float const x_,
          float const y_, size_t const tokens_);

    //--------------------------------------------------------------------------
    //! \brief Static method stringifying a place given an identifier.
    //! \return "P42" for example.
    //--------------------------------------------------------------------------
    inline static std::string to_str(size_t const id_)
    {
        return std::string("P" + std::to_string(id_));
    }

    //--------------------------------------------------------------------------
    //! \brief Increment the number of token but constrained by the type of net.
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
};

// *****************************************************************************
//! \brief Petri Transition node. A boolean condition (named receptivity) is set
//! but differ with the type of net (Petri: when the user click on the
//! transition, timed Petri net: always set to true, GRAFCER depend on boolean
//! expression with sensors). When the receptivity is true and the transition is
//! enabled (meaning that all incoming places have at least one token each) the
//! transition is fired and tokens in incoming places burnt and placed in
//! outcoming places.
//! \note There is currently no method for burning tokens because this is made
//! by the simulator with animation.
// *****************************************************************************
class Transition : public Node
{
public:

    //! \brief to access s_next_id
    friend class Net;

    //--------------------------------------------------------------------------
    //! \brief Constructor. To be used when loading a Petri net from a file.
    //! \param[in] id_: unique node identifier. Shall be unique (responsability
    //! given to the caller class).
    //! \param[in] caption_: Displayed text under the node.
    //! \param[in] x_: X-axis coordinate in the window needed for the display.
    //! \param[in] y_: Y-axis coordinate in the window needed for the display.
    //! \param[in] angle_: angle in degree of rotation for the display.
    //! \param[in] recep_: initial receptivity value.
    //--------------------------------------------------------------------------
    Transition(size_t const id_, std::string const& caption_, float const x_,
               float const y_, int const angle_, bool const recep_)
        : Node(Node::Type::Transition, id_, caption_, x_, y_), angle(angle_),
          receptivity(recep_)
    {}

    //--------------------------------------------------------------------------
    //! \brief Static method stringifying a transition given an identifier.
    //! \return "T42" for example.
    //--------------------------------------------------------------------------
    inline static std::string to_str(size_t const id_)
    {
        return std::string("T" + std::to_string(id_));
    }

    //--------------------------------------------------------------------------
    //! \brief Check if all immediatly incoming places have at least one token
    //! on each of them.
    //--------------------------------------------------------------------------
    bool isValidated() const;

    //--------------------------------------------------------------------------
    //! \brief Check if the transition has all its immediatly incoming places
    //! with at leat one token and if the transitivity (bool expression) is
    //! true.
    //! \note The burning of tokens is made by the PetriEditor class during the
    //! animation.
    //! \return true if can fire else return false.
    //--------------------------------------------------------------------------
    bool isFireable() const { return receptivity && isValidated(); }

    //--------------------------------------------------------------------------
    //! \brief Return the maximum number of tokens that can be burn in
    //! immediatly incoming places if and only if isFireable() is true.
    //! \note This method does not modify the number of tokens in previous
    //! places.
    //! \return the max number of tokens that be burnt or 0u if cannot fire.
    //--------------------------------------------------------------------------
    size_t countBurnableTokens() const;

    //--------------------------------------------------------------------------
    //! \brief Return true if the transition comes from an input place.
    //! This method is useful for converting event graph to (max,+) dynamic linear
    //! systems:
    //! X(n) = D X(n) ⨁ A X(n-1) ⨁ B U(n)
    //! Y(n) = C X(n)
    //! System inputs: B U(n) with U the column vector of system inputs.
    //--------------------------------------------------------------------------
    inline bool isInput() const
    {
        return (arcsIn.size() == 0u) && (arcsOut.size() > 0u);
    }

    //--------------------------------------------------------------------------
    //! \brief Return true if the transition goes to an output place.
    //! This method is useful for converting event graph to (max,+) dynamic linear
    //! systems:
    //! X(n) = D X(n) ⨁ A X(n-1) ⨁ B U(n)
    //! Y(n) = C X(n)
    //! System outputs: Y(n) = C X(n)
    //--------------------------------------------------------------------------
    inline bool isOutput() const
    {
        return (arcsIn.size() > 0u) && (arcsOut.size() == 0u);
    }

    //--------------------------------------------------------------------------
    //! \brief Return true is the transition is not an input or an output for
    //! the system.
    //! This method is useful for converting event graph to (max,+) dynamic linear
    //! systems:
    //! X(n) = D X(n) ⨁ A X(n-1) ⨁ B U(n)
    //! Y(n) = C X(n)
    //! Systems states: X(n) = D X(n) (+) A X(n-1) with A the state matrix and
    //! D the implicit matrix.
    //--------------------------------------------------------------------------
    inline bool isState() const
    {
        return (arcsIn.size() > 0u) && (arcsOut.size() > 0u);
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

    //! \brief Transitions are depicted by rectangles. We allow to rotate it to
    //! have horizontal, vertical or diagonal shape transitions when rendering
    //! transitions.
    //! \fixme: TBD to be moved inside the editor since we do not care of
    //! displayed here.
    int angle = 0;

    //! \brief Store the result of the transition condition (boolean expression
    //! of sensors for GRAFCET; always true for timed Petri net; false by
    //! default execept if the user clicks on it for Petri).
    //! \note This structure does not store the boolean expression directly but
    //! the result. The simulation will hold boolean expressions. This allows
    //! to separate things.
    bool receptivity = false;
};

// *****************************************************************************
//! \brief Two Petri nodes are directed by arcs. Origin and destination nodes
//! shall be of different types. Therefore arcs only make the link between Place
//! to Transition or Transition to Place.
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
    Arc(Node& from_, Node& to_, float duration_ = 0.0f)
        : from(from_), to(to_),
          duration(from_.type == Node::Type::Transition ? duration_ : NAN)
    {
        assert(from.type != to.type);
    }

    //--------------------------------------------------------------------------
    //! \brief Needed because of usage of references.
    //--------------------------------------------------------------------------
    Arc& operator=(Arc const& other)
    {
        this->~Arc(); // destroy
        new (this) Arc(other); // copy construct in place
        return *this;
    }

    //--------------------------------------------------------------------------
    //! \brief Needed because of usage of references.
    //--------------------------------------------------------------------------
    Arc(Arc const& other)
        : Arc(other.from, other.to, other.duration)
    {}

    //--------------------------------------------------------------------------
    //! \brief Needed because of usage of references.
    //--------------------------------------------------------------------------
    Arc(Arc&& other)
        : Arc(other.from, other.to, other.duration)
    {}

    //--------------------------------------------------------------------------
    //! \brief Needed because of usage of references.
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
    size_t count = 0u; // FIXME to be moved to the Editor class
};

// *****************************************************************************
//! \brief Class storing and managing Places, Transitions and Arcs.
//! This class does not offer method for the simulation but has to be seen as a
//! a container with helper methods for implementing algorithms.
// *****************************************************************************
class Net
{
    friend bool convertTo(Net& net, TypeOfNet const type, std::string& error,
                          std::vector<Arc*>& erroneous_arcs);

public:

    using Places = std::deque<Place>;
    using Transitions = std::deque<Transition>;
    using Arcs = std::deque<Arc>;

    // *************************************************************************
    //! \brief Settings for defining the type of net (GRAFCET, Petri net, timed
    //! petri net, timed graph event ...). This structure is global for the
    //! current net and is used by the Net class.
    // *************************************************************************
    struct Settings
    {
        //! \brief Max number of tokens in places. For GRAFCET: 1. For other
        //! nets: +infinity (aka std::numeric_limits<size_t>::max()).
        static size_t maxTokens;

        //! \brief The theory would burn the maximum possibe of tokens that we
        //! can within a single action (Fire::MaxPossible) but we can also try
        //! to burn tokens one by one and randomize the transitions
        //! (Fire::OneByOne). This will favor dispatching tokens along arcs.
        enum class Fire { OneByOne, MaxPossible };

        //! \brief Burn tokens one by one or as many as possible.
        static Fire firing;
    };

    //--------------------------------------------------------------------------
    //! \brief Default constructor.
    //! \param[in] mode: select the type of net: GRAFCET or timed Petri net
    //! or Petri net.
    //--------------------------------------------------------------------------
    explicit Net(TypeOfNet const type = TypeOfNet::TimedPetriNet);

    //--------------------------------------------------------------------------
    //! \brief Copy constructor. Needed to remove compilation warnings.
    //--------------------------------------------------------------------------
    Net(Net const& other);

    //--------------------------------------------------------------------------
    //! \brief Copy operator. Needed to remove compilation warnings.
    //--------------------------------------------------------------------------
    Net& operator=(Net const& other);

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
    //! \param[in] id: unique node identifier.
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
    //! \param[in] id: unique node identifier.
    //! \param[in] caption: node description.
    //! \param[in] x: X-axis coordinate in the window needed for the display.
    //! \param[in] y: Y-axis coordinate in the window needed for the display.
    //! \param[in] angle: angle of rotation of the displayed rectangle.
    //! \return the reference of the created element.
    //--------------------------------------------------------------------------
    Transition& addTransition(size_t const id, std::string const& caption,
                              float const x, float const y, int const angle);

    //--------------------------------------------------------------------------
    //! \brief Const getter. Return the reference to the container of Transitions.
    //--------------------------------------------------------------------------
    inline Transitions const& transitions() const { return m_transitions; }
    // FIXME because of inspector
    inline Transitions& transitions() { return m_transitions; }

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
    //--------------------------------------------------------------------------
    bool addArc(Node& from, Node& to, float const duration = 0.0f);

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
    bool removeArc(Node const& from, Node const& to);

    //--------------------------------------------------------------------------
    //! \brief Return a error or information message.
    //--------------------------------------------------------------------------
    inline std::string error() const { return m_message.str(); }

    //--------------------------------------------------------------------------
    //! \brief Populate or update Node::arcsIn and Node::arcsOut for all
    //! transitions and places in the Petri net.
    //--------------------------------------------------------------------------
    void generateArcsInArcsOut(); // FIXME a placer dana protected

    //--------------------------------------------------------------------------
    //! \brief Set to false the receptivity for all transitions.
    //--------------------------------------------------------------------------
    void resetReceptivies(); // FIXME a placer dans protected

protected:

    //--------------------------------------------------------------------------
    //! \brief Helper function checking arguments.
    //! \param[in] strict: if set true then types of nodes for \from and \to
    //! shall differ (one shall be Transition, the other shall be Place). If set
    //! to false then nodes can be of the same type.
    //--------------------------------------------------------------------------
    bool sanityArc(Node const& from, Node const& to, bool const strict) const;

    //--------------------------------------------------------------------------
    //! \brief Helper method removing a transition. This function does not care
    //! of upstream/downstream arcs: they shall be removed.
    //! For fastest deletion, we simply swap the undesired node with the
    //! latest node in the container. To do that, we have to iterate from the end
    //! of the container.
    //--------------------------------------------------------------------------
    void helperRemoveTransition(Node& node);

    //--------------------------------------------------------------------------
    //! \brief Helper method removing a place. This function does not care
    //! of upstream/downstream arcs: they shall be removed.
    //! For fastest deletion, we simply swap the undesired node with the
    //! latest node in the container. To do that, we have to iterate from the end
    //! of the container.
    //--------------------------------------------------------------------------
    void helperRemovePlace(Node& node);

    //--------------------------------------------------------------------------
    //! \brief Helper method removing all arcs linked to the given node.
    //! Note: For fastest deletion, we simply swap the undesired arc with the
    //! latest arc in the container. To do that, we have to iterate from the end
    //! of the container.
    //--------------------------------------------------------------------------
    void helperRemoveArcFromNode(Node& node);

private:

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
    //! \brief Auto increment unique identifier. Start from 0 (code placed in
    //! the cpp file).
    size_t m_next_place_id = 0u;
    //! \brief Auto increment unique identifier. Start from 0 (code placed in
    //! the cpp file). Note: their reset is possible through class friendship.
    size_t m_next_transition_id = 0u;
    //! \brief Store current info/error message to the Petri net editor.
    mutable std::stringstream m_message;

public:

    //! \brief Name of Petri net given by its filename once load() has been called.
    std::string name;
    //! \brief Editor has changed content and save is needed.
    bool modified = false;
};

//-----------------------------------------------------------------------------
//! \brief Return the string of the type of Petri net.
//-----------------------------------------------------------------------------
std::string to_str(TypeOfNet const type);

//-----------------------------------------------------------------------------
//! \brief Convert to the desired type of net: GRAFCET, Petri, timed Petri,
//! timed graph even, etc.
//! \return false if the net cannot be changed (i.e. to graph event).
//-----------------------------------------------------------------------------
bool convertTo(Net& net, TypeOfNet const type, std::string& error, std::vector<Arc*>& erroneous_arcs);

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
