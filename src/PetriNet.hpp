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

#  include <SFML/System.hpp>
#  include <math.h>
#  include <atomic>
#  include <string>
#  include <deque>
#  include <vector>
#  include <cassert>

class Arc;
struct SparseMatrix;

// *****************************************************************************
//! \brief Since Petri nets are bipartite graph there are two kind of nodes:
//! place and transition. This struct allows to factorize the code of derived
//! struct Place and Transition and therefore shall not be used directly as
//! instance.
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
    //! \brief Constructor. No checks are made in this method.
    //! \param[in] type: Type of Petri node: Place or Transition.
    //! \param[in] id: unique identifier (0, 1, ...).
    //! \param[in] x: X-axis coordinate in the window needed for the display.
    //! \param[in] y: Y-axis coordinate in the window needed for the display.
    //--------------------------------------------------------------------------
    Node(Type const type_, size_t const id_, float const x_, float const y_)
        : id(id_), type(type_), x(x_), y(y_),
        // Unique string identifier: "P0", "P1" ... for places and "T0",
        // "T1" ... for transitions. Once created it is not supposed to be
        // changed.
        key((type == Node::Type::Place ? 'P' : 'T') + std::to_string(id))
    {
        // Caption is a text that the user can modify. Defaut value is the
        // string unique key.
        caption = key;

        fading.restart();
    }

    //--------------------------------------------------------------------------
    //! \brief Copy operator needed because this class has constants variable
    //! members.
    //--------------------------------------------------------------------------
    Node& operator=(Node const& other)
    {
        this->~Node(); // destroy
        new (this) Node(other); // copy construct in place
        return *this;
    }

    //--------------------------------------------------------------------------
    //! \brief Needed to remove compilation warnings
    //--------------------------------------------------------------------------
    Node(Node const& other)
        : Node(other.type, other.id, other.x, other.y)
    {}

    //--------------------------------------------------------------------------
    //! \brief Needed to remove compilation warnings
    //--------------------------------------------------------------------------
    Node(Node&& other)
        : Node(other.type, other.id, other.x, other.y)
    {}

    //--------------------------------------------------------------------------
    //! \brief Needed to remove compilation warnings
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
    inline bool operator!=(Node const &other) const
    {
        return !(*this == other);
    }

    //! \brief Unique identifier (auto-incremented from 0 by the derived class).
    size_t const id;
    //! \brief Petri Place or Petri Transition.
    Type const type;
    //! \brief Position in the window needed for the display.
    float x;
    //! \brief Position in the window needed for the display.
    float y;
    //! \brief Unique node identifier as string. It is formed by the 'P' char
    //! for place or by the 'T' char for transition followed by the unique
    //! identifier (i.e. "P0", "P1", "T0", "T1", ...)
    std::string const key;
    //! \brief Text displayed near the place (by default initialized with \c key
    //! string).
    std::string caption;
    //! \brief Timer for fading colors.
    sf::Clock fading;
    //! \brief Hold the incoming arcs.
    //! \note this vector is updated by the method
    //! PetriNet::generateArcsInArcsOut() and posible evolution could be to
    //! update dynamicaly this vector when editing the net through the GUI.
    std::vector<Arc*> arcsIn;
    //! \brief Hold the outcoming arcs.
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
    //! \brief to access m_backup_tokens
    friend class PetriEditor;

    //--------------------------------------------------------------------------
    //! \brief Constructor. To be used when loading a Petri net from JSON file.
    //! \param[in] id: unique node identifier. Shall be unique.
    //! \param[in] x: X-axis coordinate in the window needed for the display.
    //! \param[in] y: Y-axis coordinate in the window needed for the display.
    //! \param[in] tok: Initial number of tokens in the place.
    //--------------------------------------------------------------------------
    Place(size_t const id_, float const x_, float const y_, size_t const tok_)
        : Node(Node::Type::Place, id_, x_, y_), tokens(tok_), m_backup_tokens(tok_)
    {}

    //--------------------------------------------------------------------------
    //! \brief Stringify: return "P42" for example.
    //--------------------------------------------------------------------------
    static std::string to_str(size_t const id_)
    {
        std::string strid("P");
        strid += std::to_string(id_);
        return strid;
    }

    //! \brief the number of tokens hold by the Place. Public access is fine
    //! since this will facilitate its access during the simulation.
    size_t tokens;

private:

    //! \brief Before starting the simulation of the Petri net, we have to
    //! save the number of tokens to allow restoring initial states when the
    //! simulation is finished.
    size_t m_backup_tokens;
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
    //--------------------------------------------------------------------------
    Transition(size_t const id_, float const x_, float const y_, int const angle_)
        : Node(Node::Type::Transition, id_, x_, y_), angle(angle_)
    {}

    //--------------------------------------------------------------------------
    //! \brief Stringify: return "T42" for example.
    //--------------------------------------------------------------------------
    static std::string to_str(size_t const id_)
    {
        std::string strid("T");
        strid += std::to_string(id_);
        return strid;
    }

    //--------------------------------------------------------------------------
    //! \brief Check if the transitivity is active and return the number of
    //! tokens burnt (return 0 if the transitiviy is disabled).
    //!
    //! \note The theory would return the maximum possibe of tokens that can be
    //! burnt but for the animation we fire one by one.
    //--------------------------------------------------------------------------
    size_t canFire();

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

    //! \brief Transitions are depicted by rectangles. We allow to rotate it
    //! to have horizontal, vertical or diagonal shape transitions.
    int angle = 0u;

    //! \brief Temporary matrix index used when building max-plus linear system.
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
    //! \param[in] from: Origin node (Place or Transition).
    //! \param[in] to: Destination node (Place or Transition).
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

    //--------------------------------------------------------------------------
    //! \brief Copy constructors.
    //--------------------------------------------------------------------------
    PetriNet& operator=(PetriNet const& other)
    {
        if (this != &other)
        {
            m_places = other.m_places;
            m_transitions = other.m_transitions;
            m_arcs.clear(); // We have to redo references
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

            m_next_place_id = other.m_next_place_id;
            m_next_transition_id = other.m_next_transition_id;
            generateArcsInArcsOut(/*arcs: true*/);
        }

        return *this;
    }

    //--------------------------------------------------------------------------
    //! \brief Default constructor
    //--------------------------------------------------------------------------
    PetriNet() = default;

    //--------------------------------------------------------------------------
    //! \brief Needed to remove compilation warnings
    //--------------------------------------------------------------------------
    PetriNet(PetriNet const& other)
    {
        if (this != &other)
        {
            m_places = other.m_places;
            m_transitions = other.m_transitions;
            m_arcs.clear(); // We have to redo references
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

            m_next_place_id = other.m_next_place_id;
            m_next_transition_id = other.m_next_transition_id;
            generateArcsInArcsOut(/*arcs: true*/);
        }
    }

    //--------------------------------------------------------------------------
    //! \brief Remove all nodes and arcs. Reset counters for unique identifiers.
    //--------------------------------------------------------------------------
    void reset()
    {
        m_places.clear();
        m_transitions.clear();
        m_arcs.clear();
        m_critical.clear();
        m_next_place_id = 0u;
        m_next_transition_id = 0u;
    }

    //--------------------------------------------------------------------------
    //! \brief Return true if the Petri nets has no nodes (no places and no
    //! transitions).
    //--------------------------------------------------------------------------
    bool isEmpty()
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
    Place& addPlace(float const x, float const y, size_t const tokens = 0u)
    {
        m_places.push_back(Place(m_next_place_id++, x, y, tokens));
        return m_places.back();
    }

    //--------------------------------------------------------------------------
    //! \brief Add a new Petri Place. To be used when loading a Petri net from
    //! JSON file.
    //! \param[in] id: unique node identifier.
    //! \param[in] x: X-axis coordinate in the window needed for the display.
    //! \param[in] y: Y-axis coordinate in the window needed for the display.
    //! \param[in] tokens: Initial number of tokens in the place.
    //! \return the reference of the inserted place.
    //--------------------------------------------------------------------------
    Place& addPlace(size_t const id, float const x, float const y, size_t const tokens)
    {
        m_places.push_back(Place(id, x, y, tokens));
        if (id + 1u > m_next_place_id)
            m_next_place_id = id + 1u;
        return m_places.back();
    }

    //--------------------------------------------------------------------------
    //! \brief Const getter. Return the reference to the container of Places.
    //--------------------------------------------------------------------------
    std::deque<Place> const& places() const
    {
        return m_places;
    }

    //--------------------------------------------------------------------------
    //! \brief Non const getter. TODO: to be removed (ideally).
    //--------------------------------------------------------------------------
    std::deque<Place>& places()
    {
        return m_places;
    }

    //--------------------------------------------------------------------------
    //! \brief Add a new Petri Transition. To be called when the user clicked on
    //! the GUI.
    //! \param[in] x: X-axis coordinate in the window needed for the display.
    //! \param[in] y: Y-axis coordinate in the window needed for the display.
    //! \return the reference of the inserted element.
    //--------------------------------------------------------------------------
    Transition& addTransition(float const x, float const y)
    {
        m_transitions.push_back(Transition(m_next_transition_id++, x, y, 0u));
        return m_transitions.back();
    }

    //--------------------------------------------------------------------------
    //! \brief Add a new Petri Transition. To be used when loading a Petri net
    //! from JSON file.
    //! \param[in] id: unique node identifier.
    //! \param[in] x: X-axis coordinate in the window needed for the display.
    //! \param[in] y: Y-axis coordinate in the window needed for the display.
    //! \param[in] angle: angle of rotation of the displayed rectangle.
    //! \return the reference of the inserted element.
    //--------------------------------------------------------------------------
    Transition& addTransition(size_t const id, float const x, float const y,
                              int const angle)
    {
        m_transitions.push_back(Transition(id, x, y, angle));
        if (id + 1u > m_next_transition_id)
            m_next_transition_id = id + 1u;
        return m_transitions.back();
    }

    //--------------------------------------------------------------------------
    //! \brief Const getter. Return the reference to the container of Transitions.
    //--------------------------------------------------------------------------
    std::deque<Transition> const& transitions() const
    {
        return m_transitions;
    }

    //--------------------------------------------------------------------------
    //! \brief Non const getter. TODO: to be removed (ideally).
    //--------------------------------------------------------------------------
    std::deque<Transition>& transitions()
    {
        return m_transitions;
    }

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
    //! \param[in] key for example "42" for the Transition 42.
    //--------------------------------------------------------------------------
    Transition* findTransition(size_t const id);

    //--------------------------------------------------------------------------
    //! \brief Search and return a Place by its unique identifier. Search
    //! is O(n) where n is the number of nodes. Return nullptr if not found.
    //! \param[in] key for example "42" for the Place 42.
    //--------------------------------------------------------------------------
    Place* findPlace(size_t const id);

    //--------------------------------------------------------------------------
    //! \brief Add a new arc between two Petri nodes (place or transition).
    //! \return true if the arc is valid and has been added, else return false
    //! if an arc is already present or nodes have the same type.
    //--------------------------------------------------------------------------
    bool addArc(Node& from, Node& to, float const duration = 0.0f);

    //--------------------------------------------------------------------------
    //! \brief Return if the arc linking the two given nodes is present in the
    //! net.
    //! \return the address of the arc if found, else return nullptr.
    //--------------------------------------------------------------------------
    Arc* findArc(Node const& from, Node const& to)
    {
        for (auto& it: m_arcs)
        {
            if ((it.from == from) && (it.to == to))
                return &it;
        }
        return nullptr;
    }

    //--------------------------------------------------------------------------
    //! \brief Const getter.
    //--------------------------------------------------------------------------
    std::deque<Arc> const& arcs() const
    {
        return m_arcs;
    }

    //--------------------------------------------------------------------------
    //! \brief Non const getter. TODO: to be removed (ideally).
    //--------------------------------------------------------------------------
    std::deque<Arc>& arcs()
    {
        return m_arcs;
    }

    //--------------------------------------------------------------------------
    //! \brief Save the Petri net in a JSON file.
    //! \param[in] filename: the file path in where to save the Petri net.
    //! Should have the .json extension.
    //! \return true if the Petri net has been saved with success. Return false
    //! in case of failure.
    //--------------------------------------------------------------------------
    bool save(std::string const& filename);

    //--------------------------------------------------------------------------
    //! \brief Load the Petri net from a JSON file.
    //! \param[in] filename: the file path in where a Petri net has been
    //! saved. Should have the .json extension.
    //! \return true if the Petri net has been loaded with success. Return false
    //! in case of failure.
    //--------------------------------------------------------------------------
    bool load(std::string const& filename);

    //--------------------------------------------------------------------------
    //! \brief Chech if the Petri net is a graph event meaning that eacg places
    //! have exactly one input arc and one output arc.
    //! \return true if the Petri net is a graph event.
    //! \note call generateArcsInArcsOut(/*arcs: true*/); before calling this
    //! method.
    //--------------------------------------------------------------------------
    bool isEventGraph();

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    bool showCriticalCycle();

    std::stringstream showCounterForm(std::string const& comment = "# ") const;
    std::stringstream showDaterForm(std::string const& comment = "# ") const;

    //--------------------------------------------------------------------------
    //! \brief Export the Petri net as Grafcet in C++ header file.
    //! \param[in] filename the path of h++ file. Should have the .hpp or .h
    //! extension (or any associated to header header files).
    //! \param[in] name the namespace.
    //! \return true if the Petri net has been exported with success. Return
    //! false in case of failure.
    //--------------------------------------------------------------------------
    bool exportToCpp(std::string const& filename,
                     std::string const& name);

    //--------------------------------------------------------------------------
    //! \brief Export the Petri net as Max-Plus linear system in Julia code.
    //! \param[in] filename the path of julia file. Should have the .jl
    //! extension.
    //! \return true if the Petri net has been exported with success. Return
    //! false in case of failure.
    //--------------------------------------------------------------------------
    bool exportToJulia(std::string const& filename); //TODO const;

    //--------------------------------------------------------------------------
    //! \brief Return the event graph as 2 adjacency matrices.
    //! \param[out] N the adjacency matrix of tokens.
    //! \param[out] Tthe adjacency matrix of durations.
    //! \note This will work only if isEventGraph() returned true.
    //! \return false if the Petri net is not an event graph.
    //--------------------------------------------------------------------------
    bool toAdjacencyMatrices(SparseMatrix& N, SparseMatrix&T); //TODO  const;

    //--------------------------------------------------------------------------
    //! \brief Return the event graph as implicit dynamic linear Max-Plus system.
    //! X(n) = D X(n) ⨁ A X(n-1) ⨁ B U(n)
    //! Y(n) = C X(n)
    //! \note This will work only if isEventGraph() returned true.
    //! TODO return false if the Petri net is not an event graph.
    //--------------------------------------------------------------------------
    bool toSysLin(SparseMatrix& D, SparseMatrix& A, SparseMatrix& B, SparseMatrix& C); //TODO const;

    //--------------------------------------------------------------------------
    //! \brief Transform the Event Graph to canonical form
    //! \param[out] pn: resulting Petri net in canonical mode
    //! \note This will work only if isEventGraph() returned true.
    //! TODO return false if the Petri net is not an event graph.
    //--------------------------------------------------------------------------
    void toCanonicalForm(PetriNet& pn); //TODO  const;

    //--------------------------------------------------------------------------
    //! \brief Remove an existing Place or a Transition (usually refered by the
    //! mouse cursor).
    //--------------------------------------------------------------------------
    void removeNode(Node& node);

    //--------------------------------------------------------------------------
    //! \brief Remove an existing arc.
    //--------------------------------------------------------------------------
    bool removeArc(Arc const& arc);
    bool removeArc(Node& from, Node& to);

    //--------------------------------------------------------------------------
    //! \brief Populate or update Node::arcsIn and Node::arcsOut for all
    //! transitions and places in the Petri net.
    //--------------------------------------------------------------------------
    void generateArcsInArcsOut();

private:

    void toSysLin(SparseMatrix& D, SparseMatrix& A, SparseMatrix& B, SparseMatrix& C,
                  size_t const nb_inputs, size_t const nb_states, size_t const nb_outputs);

private:

    //! \brief List of Places. We do not use std::vector to avoid invalidating
    //! node references for arcs after a possible resizing.
    std::deque<Place> m_places;
    //! \brief List of Transitions. We do not use std::vector to avoid
    //! invalidating node references for arcs after a possible resizing.
    std::deque<Transition> m_transitions;
    //! \brief List of Arcs.
    std::deque<Arc> m_arcs;
    //! \brief Auto increment unique identifier. Start from 0 (code placed in
    //! the cpp file).
    size_t m_next_place_id = 0u;
    //! \brief Auto increment unique identifier. Start from 0 (code placed in
    //! the cpp file). Note: their reset is possible through class friendship.
    size_t m_next_transition_id = 0u;

public: // FIXME
    //
    std::vector<Arc*> m_critical;
};

#endif
