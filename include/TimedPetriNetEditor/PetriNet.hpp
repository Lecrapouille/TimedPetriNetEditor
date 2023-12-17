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

//--------------------------------------------------------------------------
//! \brief Configure the net as GRAFCET or timed Petri net or Petri net.
//--------------------------------------------------------------------------
enum class TypeOfNet
{
    //! \brief The user has to click on transitions to fire.
    //! Tokens are burnt one by one.
    PetriNet,
    //! \brief Is a Petri with duration on arcs transition -> place. When
    //! transitions are enables, firing is automatic, on divergence
    //! transition, the firing is shuffle and the maximum of tokens are
    //! burnt in once. The user cannot click to transitions to fire.
    TimedPetriNet,
    //! \brief Is a timed Petri where all places have a single input arc and
    //! a single output arc. TODO This mode is not yet implemented: the
    //! editor shall not display places.
    TimedEventGraph,
    //! \brief Is a Petri net used for making automata: Places do actions
    //! and receptivities are linked to sensors. Steps (the name for Places)
    //! have at maximum one token (1-S net). TODO This mode is partially
    //! implemented: partial GRAFCET norm is respected, GUI does not allow
    //! to simulated actions and sensors.
    GRAFCET
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
    //! \brief Needed to remove compilation warnings with clang++ and MacOSx.
    //--------------------------------------------------------------------------
    Node(Node const& other)
        : Node(other.type, other.id, other.caption, other.x, other.y)
    {}

    //--------------------------------------------------------------------------
    //! \brief Needed to remove compilation warnings with clang++ and MacOSx.
    //--------------------------------------------------------------------------
    Node(Node&& other)
        : Node(other.type, other.id, other.caption, other.x, other.y)
    {}

    //--------------------------------------------------------------------------
    //! \brief Needed to remove compilation warnings with clang++ and MacOSx.
    //--------------------------------------------------------------------------
    Node& operator=(Node&& other)
    {
        this->~Node(); // destroy
        new (this) Node(other); // copy construct in place
        return *this;
    }

public:

    //! \brief Type of nodes: Petri Place or Petri Transition .Once created it is
    //! not supposed to be changed.
    Type const type;
    //! \brief Unique identifier (auto-incremented from 0 by the derived class).
    //! Once created it is not supposed to be changed.
    size_t const id;
    //! \brief Unique node identifier as string. It is formed by the 'P' char
    //! for place or by the 'T' char for transition followed by the unique
    //! identifier (i.e. "P0", "P1", "T0", "T1", ...). Once created it is not
    //! supposed to be changed.
    std::string const key;
    //! \brief Position inside the window needed for the display.
    float x;
    //! \brief Position in the window needed for the display.
    float y;
    //! \brief Text displayed near a node the user can modify. Defaut value is
    //! the tring unique \c key.
    std::string caption;
    //! \brief Hold the incoming arcs to access to previous nodes.
    //! \note this vector is updated by the method
    //! Net::generateArcsInArcsOut() and posible evolution could be to
    //! update dynamicaly this vector when editing the net through the GUI.
    std::vector<Arc*> arcsIn;
    //! \brief Hold the outcoming arcs to access to successor nodes.
    //! \note this vector is updated by the method
    //! Net::generateArcsInArcsOut() and posible evolution could be to
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
    friend class Net;

    //--------------------------------------------------------------------------
    //! \brief Constructor. To be used when loading a Petri net from JSON file.
    //! \param[in] id_: unique node identifier. Shall be unique (responsability
    //!   given to the caller class).
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
    //! \brief Increment the number of token constrained by the type of net.
    //--------------------------------------------------------------------------
    size_t increment(size_t const count = 1u);

    //--------------------------------------------------------------------------
    //! \brief Decrement the number of token constrained by the type of net.
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
    friend class Net;

    //--------------------------------------------------------------------------
    //! \brief Constructor. To be used when loading a Petri net from JSON file.
    //! \param[in] id_: unique node identifier. Shall be unique (responsability
    //!   given to the caller class).
    //! \param[in] caption_: Displayed text under the node.
    //! \param[in] x_: X-axis coordinate in the window needed for the display.
    //! \param[in] y_: Y-axis coordinate in the window needed for the display.
    //! \param[in] angle_: angle in degree of rotation for the display.
    //! \param[in] recep_: receptivity of the transition.
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
    //! \brief Check if the transition is validated (meaning if the receptivity
    //! is true) or not validated (meaning if the receptivity is false).
    //--------------------------------------------------------------------------
    inline bool isValidated() const { return receptivity; }

    //--------------------------------------------------------------------------
    //! \brief Check if all previous places have all at leat one token (meaning
    //! if all previous places (steps for GRAFCET) are activated.
    //--------------------------------------------------------------------------
    bool isEnabled() const;

    //--------------------------------------------------------------------------
    //! \brief Check if the transition is validated and all previous places have
    //! all at leat one token. In this case the transition can burn tokens in all
    //! previous places.
    //! \note The burning of tokens is made by the PetriEditor class during the
    //! animation.
    //! \return true if can fire else return false.
    //--------------------------------------------------------------------------
    bool canFire() const { return isValidated() && isEnabled(); }

    //--------------------------------------------------------------------------
    //! \brief Return the maximum possibe of tokens that can be burnt in
    //!   previous places iff canFire() is true.
    //! \note This method does not modify the number of tokens in previous places.
    //! \return the max number of tokens that be burnt or 0u if cannot fire. This
    //! number can be > 1 even for GRAFCET because the saturation shall be done
    //! after by the caller.
    //--------------------------------------------------------------------------
    size_t howManyTokensCanBurnt() const;

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

    //! \brief Transitions are depicted by rectangles. We allow to rotate it
    //! to have horizontal, vertical or diagonal shape transitions when rendering
    //! transitions.
    int angle = 0;

    //! \brief In petri net mode, the user has to click to validate the
    //! receptivity of the transition. If previous places have all at least one
    //! token, the transition is fired, burning tokens in previous places and
    //! create tokens in the successor places. In timed Petri net receptivities
    //! are always true. In GRAFCET receptivity depends on boolean logic on
    //! sensors (i.e. urgency button pressed).
    bool receptivity = false;
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
};

// *****************************************************************************
//! \brief Container class holding and managing Places, Transitions and
//! Arcs. This class does not manage simulation. FIXME to be defined: since
//! currently the PetriEditor is doing the simulation which is mainly animations
//! with some basic features such as burning tokens ...
// *****************************************************************************
class Net
{
public:

    using Places = std::deque<Place>;
    using Transitions = std::deque<Transition>;
    using Arcs = std::deque<Arc>;

    // *****************************************************************************
    //! \brief Settings for defining the type of net (GRAFCET, Petri net, timed
    //! petri net, timed graph event ...). This structure is global for the current
    //! net and is used by the Net class.
    // *****************************************************************************
    struct Settings
    {
        //! \brief Max number of tokens in places. For GRAFCET: 1. For other nets:
        //! std::numeric_limits<size_t>::max().
        static size_t maxTokens;

        //! \brief The theory would burn the maximum possibe of tokens that we can
        //! within a single action (Fire::MaxPossible) but we can also try to burn
        //! tokens one by one and randomize the transitions (Fire::OneByOne). This
        //! will favor dispatching tokens along arcs.
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
    //! \note the type of net (timed, classic ... ) stay the same.
    //--------------------------------------------------------------------------
    void clear(TypeOfNet const type);

    //--------------------------------------------------------------------------
    //! \brief Return the name of net.
    //--------------------------------------------------------------------------
    //std::string const& name() const { return m_name; }

    //--------------------------------------------------------------------------
    //! \brief Return the file of net to save.
    //--------------------------------------------------------------------------
    std::string const& filename() const { return m_filename; }

    //--------------------------------------------------------------------------
    //! \brief Get the type of net: GRAFCET, Petri, Timed Petri ...
    //--------------------------------------------------------------------------
    inline TypeOfNet type() const { return m_type; }

    //--------------------------------------------------------------------------
    //! \brief Convert to the desired type of net: GRAFCET, Petri, timed Petri,
    //! timed graph even, etc.
    //! \return false if the net cannot be changed (i.e. to graph event).
    //--------------------------------------------------------------------------
    bool convertTo(TypeOfNet const type, std::string& error, std::vector<Arc*>& erroneous_arcs);

    //--------------------------------------------------------------------------
    //! \brief Load the Petri net from a JSON file. The current net is cleared
    //! before the loading. If the loading failed (missing file or invalid
    //! syntax) the net is set dummy.
    //! \param[in] filename: the file path in where a Petri net has been
    //! saved. Should have the .json extension.
    //! \return true if the Petri net has been loaded with success. Return false
    //! in case of failure.
    //--------------------------------------------------------------------------
    bool load(std::string const& filename);

    //--------------------------------------------------------------------------
    //! \brief Save the Petri net in a JSON file.
    //! \param[in] filename: the file path in where to save the Petri net.
    //! Should have the .json extension.
    //! \return true if the Petri net has been saved with success. Return false
    //! in case of failure.
    //--------------------------------------------------------------------------
    bool saveAs(std::string const& filename) const;

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
    inline Places& places() { return m_places; } // FIXME: because of toCanonicalForm(), inspector

    //--------------------------------------------------------------------------
    //! \brief Set tokens in all places.
    //! \param[in] tokens_ the vector holding tokens for each places (P0, P1 .. Pn).
    //! \return true if the length of the vector matchs the number of places,
    //! return false else and you shall call message() to get the error message.
    //--------------------------------------------------------------------------
    bool tokens(std::vector<size_t> const& tokens_);

    //--------------------------------------------------------------------------
    //! \brief Get tokens from all places.
    //! \return the vector holding tokens for each places (P0, P1 .. Pn).
    //--------------------------------------------------------------------------
    std::vector<size_t> tokens() const;

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
    //! \brief Const getter. Return the reference to the container of Transitions.
    //--------------------------------------------------------------------------
    inline Transitions const& transitions() const { return m_transitions; }
    inline Transitions& transitions() { return m_transitions; } // FIXME because of inspector

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
    bool addArc(Node& from, Node& to, float const duration = 0.0f,
                bool const strict = true);

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
    //! \return false if in GRAFCET mode and if at least one transition has an
    //! invalid syntaxt in its recepetivity.
    //--------------------------------------------------------------------------
    bool resetReceptivies(); // FIXME a placer dana protected

protected:

    //--------------------------------------------------------------------------
    //! \brief Helper function checking arguments.
    //--------------------------------------------------------------------------
    bool sanityArc(Node const& from, Node const& to, bool const strict) const;

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
    //! \brief File used to load the net.
    std::string m_filename;

public:

    //! \brief Editor has changed content and save is needed.
    bool modified = false;
    //! \brief Name of Petri net given by its filename once load() has been called.
    std::string name;
};

//--------------------------------------------------------------------------
//! \brief Return the string of the type of Petri net.
//--------------------------------------------------------------------------
std::string to_str(TypeOfNet const type);

} // namespace tpne

#endif
