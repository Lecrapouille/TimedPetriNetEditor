//=====================================================================
// PetriEditor: A petri net editor.
// Copyright 2021 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of PetriEditor.
//
// PetriEditor is free software: you can redistribute it and/or modify it
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

#ifndef PETRI_HPP
#  define PETRI_HPP

#  include "GUI.hpp"
#  include <atomic>
#  include <string>
#  include <deque>
#  include <vector>

class Arc;

// *****************************************************************************
//! \brief Base class for Petri Place class and Petri Transition. Allow to
//! factorize the code.
// *****************************************************************************
struct Node
{
    //! \brief Type of the node (Petri Place or Petri Transition)
    enum Type { Place, Transition };

    //! \brief Constructor. No check is made.
    //! \param[in] type: Petri Place or Petri Transition
    //! \param[in] id: unique identifier.
    //! \param[in] x: X-axis coordinate in the window needed for the display.
    //! \param[in] y: Y-axis coordinate in the window needed for the display.
    Node(Type const type_, size_t const id_, float const x_, float const y_)
        : id(id_), type(type_), x(x_), y(y_)
    {
        m_key = (type == Node::Type::Place) ? 'P' : 'T';
        m_key += std::to_string(id);
        caption = m_key;
    }

    //! \brief Necessary because of const var member. identifier and type does
    //! not chnage.
    Node& operator=(const Node& obj)
    {
        this->~Node(); // destroy
        new (this) Node(obj); // copy construct in place
        return *this;
    }

    //! \brief Return the unique identifier as a string. The first char is 'P'
    //! for place or 'T' for transiftion, next char is the unique identifier as
    //! integer.
    std::string const& key() const
    {
        return m_key;
    }

    //! \brief Compare node with another node
    bool operator==(Node const &other) const
    {
        return (type == other.type) && (id == other.id);
    }

    //! \brief Unique identifier (auto-incremented).
    size_t const id;
    //! \brief Petri Place or Petri Transition.
    Type const type;
    //! \brief Position in the window needed for the display.
    float x;
    //! \brief Position in the window needed for the display.
    float y;
    //! \brief text display near the place (by default == m_key).
    std::string caption;

    //! \brief Hold the incoming arcs.
    //! \note this vector is updated by the method PetriNet::generateArcsInArcsOut().
    //! Posible evolution: update dynamicaly this vector when editing the net
    //! through the GUI.
    std::vector<Arc*> arcsIn;

    //! \brief Hold the outcoming arcs.
    //! \note this vector is updated by the method PetriNet::generateArcsInArcsOut().
    //! Posible evolution: update dynamicaly this vector when editing the net
    //! through the GUI.
    std::vector<Arc*> arcsOut;

private:

    //! \brief Unique node identifier as string.
    std::string m_key;
};

// *****************************************************************************
//! \brief Petri arc. It make the link between two Petri nodes (Place ->
//! Transition or Transition -> Place). This class does not manage erroneous
//! case such as if the arc links two places or links two transitions. This
//! check shall be made by the caller class.
// *****************************************************************************
struct Arc
{
    //! \param[in] from: Origin node (Place or Transition).
    //! \param[in] to: Destination node (Place or Transition).
    //! \note Nodes shall have different types. No check is made here.
    Arc(Node& from_, Node& to_, float duration_ = 0.0f)
        : from(from_), to(to_), duration(duration_)
    {}

    //! \brief Hack needed because of references
    Arc& operator=(const Arc& obj)
    {
        this->~Arc(); // destroy
        new (this) Arc(obj); // copy construct in place
        return *this;
    }

    //! \brief Origin node (Place or Transition).
    Node& from;
    //! \brief Destination node (Place or Transition).
    Node& to;
    //! \brief Timed Petri.
    float duration; // TODO unit
    //! \brief Temporary token counter for creating animated tokens
    //! displaying the correct tokens number they are carrying.
    size_t count = 0u;
};

// *****************************************************************************
//! \brief Petri Place node. It hold tokens.
// *****************************************************************************
struct Place : public Node
{
    friend class PetriNet;

    //! \param[in] x: X-axis coordinate in the window needed for the display.
    //! \param[in] y: Y-axis coordinate in the window needed for the display.
    //! \param[in] tok: Initial number of tokens in the place.
    Place(float const x, float const y)
        : Node(Node::Type::Place, s_count++, x, y),
          tokens(0u), backup_tokens(0u)
    {}

    Place(size_t const id, float const x, float const y, size_t const tok)
        : Node(Node::Type::Place, id, x, y),
          tokens(tok), backup_tokens(tok)
    {
        s_count = std::max(s_count.load(), id) + 1u;
    }

    //! \brief the number of tokens hold by the Place
    size_t tokens;
    size_t backup_tokens; // TODO: to be private ?

private:

    //! \brief Auto increment unique identifier.
    static std::atomic<size_t> s_count;
};

// *****************************************************************************
//! \brief Petri Transition node.
// *****************************************************************************
struct Transition : public Node
{
    friend class PetriNet;

    //! \param[in] x: X-axis coordinate in the window needed for the display.
    //! \param[in] y: Y-axis coordinate in the window needed for the display.
    Transition(float const x, float const y)
        : Node(Node::Type::Transition, s_count++, x, y)
    {}

    Transition(size_t const id, float const x, float const y, int const angle_)
        : Node(Node::Type::Transition, id, x, y),
          angle(angle_)
    {
        s_count = std::max(s_count.load(), id) + 1u;
    }

private:

    //! \brief Auto increment unique identifier.
    static std::atomic<size_t> s_count;

public:

    //! \brief Apply a rotation angle in degrees when displayed
    int angle = 0u;
};

// *****************************************************************************
//! \brief Animated Tokens when transitions are fired.
// *****************************************************************************
struct AnimatedToken
{
    AnimatedToken(Arc& arc, size_t tok, bool PT);

    bool update(float const dt);

    Place& toPlace()
    {
        return *reinterpret_cast<Place*>(&currentArc->to);
    }

    //! \brief Unique identifier (useless but can help for debugging).
    size_t id;
    //! \brief X-axis coordinate in the window used for the display.
    float x;
    //! \brief Y-axis coordinate in the window used for the display.
    float y;
    //! \brief Number of tokens carrying
    size_t tokens = 1u;
    //! \brief In which arc the token is transitioning.
    Arc* currentArc;
    //! \brief Cache the magnitude of the arc.
    float magnitude;
    //! \brief The speed of the token animated.
    float speed;
    //! \brief What ratio the token has transitioned over the arc (0%: origin
    //! position, 100%: destination position).
    float offset = 0.0f;
};

// *****************************************************************************
//! \brief Container class holding and managing Places, Transitions and
//! Arcs. This class does not manage simulation. FIXME: to be defined since
//! currently the PetriGUI is doing the simulation which is mainly animations
//! with some basic features such as burning tokens ...
// *****************************************************************************
class PetriNet
{
public:

    //! \brief Remove all nodes and arcs.
    void reset()
    {
        m_places.clear();
        m_transitions.clear();
        m_arcs.clear();
        Place::s_count = 0u;
        Transition::s_count = 0u;
    }

    //! \brief Add a new Petri Place.
    //! \param[in] x: X-axis coordinate in the window needed for the display.
    //! \param[in] y: Y-axis coordinate in the window needed for the display.
    //! \param[in] tokens: Initial number of tokens in the place.
    //! \return the reference of the inserted element.
    Place& addPlace(float const x, float const y)
    {
        m_places.push_back(Place(x, y));
        return m_places.back();
    }

    //! \brief From JSON file
    Place& addPlace(size_t const id, float const x, float const y, size_t const tokens)
    {
        m_places.push_back(Place(id, x, y, tokens));
        return m_places.back();
    }

    std::deque<Place> const& places() const
    {
        return m_places;
    }

    std::deque<Place>& places()
    {
        return m_places;
    }

    //! \brief Add a new Petri Transition.
    //! \param[in] x: X-axis coordinate in the window needed for the display.
    //! \param[in] y: Y-axis coordinate in the window needed for the display.
    //! \return the reference of the inserted element.
    Transition& addTransition(float const x, float const y)
    {
        m_transitions.push_back(Transition(x, y));
        return m_transitions.back();
    }

    //! \brief From JSON file
    Transition& addTransition(size_t const id, float const x,
                              float const y, int const angle)
    {
        m_transitions.push_back(Transition(id, x, y, angle));
        return m_transitions.back();
    }

    std::deque<Transition> const& transitions() const
    {
        return m_transitions;
    }

    std::deque<Transition>& transitions()
    {
        return m_transitions;
    }

    //! \brief Search and return a place or a transition by its unique
    //! identifier. Search is O(n) where n is the number of nodes.
    Node* findNode(std::string const& key)
    {
        if (key[0] == 'P')
        {
            for (auto& p: m_places)
            {
                if (p.key() == key)
                    return &p;
            }
            return nullptr;
        }

        if (key[0] == 'T')
        {
            for (auto& t: m_transitions)
            {
                if (t.key() == key)
                    return &t;
            }
            return nullptr;
        }

        return nullptr;
    }

    //! \brief Add a new arc between two Petri nodes (place or transition).
    //! \return true if the arc is valid and has been added, else return false
    //! if an arc is already present or nodes have the same type.
    bool addArc(Node& from, Node& to, float duration = 0.0f)
    {
        if (from.type == to.type)
            return false;

        if (hasArc(from, to))
            return false;

        m_arcs.push_back(Arc(from, to, duration));
        return true;
    }

    //! \brief Return if the arc linking the two given nodes is present in the
    //! net.
    //! \return the address of the arc if found, else return nullptr.
    Arc* hasArc(Node const& from, Node const& to)
    {
        for (auto& it: m_arcs)
        {
            if ((it.from == from) && (it.to == to))
                return &it;
        }
        return nullptr;
    }

    std::deque<Arc> const& arcs() const
    {
        return m_arcs;
    }

    std::deque<Arc>& arcs()
    {
        return m_arcs;
    }

    //! \brief Save the Petri net in a JSON file
    bool save(std::string const& filename);

    //! \brief Load the Petri net from a JSON file
    bool load(std::string const& filename);

    //! \brief Export the Petri net to C++ code as Grafcet.
    //! \param[in] filename the path of h++ file.
    //! \param[in] name the namespace.
    bool exportToCpp(std::string const& filename,
                     std::string const& name);

    bool exportToJulia(std::string const& filename);

    //! \brief Remove a Place or a Transition
    void removeNode(Node& node);

    //! \brief Populate or update Transition::arcsIn and Transition::arcsOut
    //! for all transitions in the net.
    void generateArcsInArcsOut();

private:

    //! \brief List of Places.
    std::deque<Place> m_places;
    //! \brief List of Transitions.
    std::deque<Transition> m_transitions;
    //! \brief List of Arcs.
    std::deque<Arc> m_arcs;
};

// *****************************************************************************
//! \brief Graphic representation of the Petri net
// *****************************************************************************
class PetriGUI: public GUI
{
public:

    PetriGUI(Application& application);
    ~PetriGUI();

private: // Derived from GUI

    //! \brief Inherit from GUI class. Draw the chessboard and pieces.
    virtual void draw(const float dt) override;

    //! \brief Inherit from GUI class. Update GUI.
    virtual void update(const float dt) override;

    //! \brief Inherit from GUI class. Manage mouse and keyboard events.
    virtual void handleInput() override;

    //! \brief Inherit from GUI class. Return true if GUI is alive.
    virtual bool isRunning() override
    {
        return m_running;
    }

    //! \brief Called when the GUI has been enabled.
    virtual void activate() override {}

    //! \brief Called when the GUI has been disabled.
    virtual void deactivate() override {}

private:

    //! \brief Draw a place and its tokens
    void draw(Place const& place);

    //! \brief Draw a transition
    void draw(Transition const& transition);

    //! \brief Draw a Petri arc with arrow
    void draw(Arc const& arc);

    //! \brief Draw a string centered on x, y coordiantes
    void draw(sf::Text&, std::string const& str, float const x, float const y);

    //! \brief Draw a integer centered on x, y coordiantes
    void draw(sf::Text&, size_t const number, float const x, float const y);

    //! \brief Draw a float centered on x, y coordiantes
    void draw(sf::Text&, float const number, float const x, float const y);

    //! \brief Search and return if a place or a transition is present at the
    //! given coordinates.
    //! \param[in] x: X-axis coordinate of the mouse cursor.
    //! \param[in] y: Y-axis coordinate of the mouse cursor.
    //! \return the address of the place or the transition if present, else
    //! return nullptr.
    Node* getNode(float const x, float const y);

    void handleArcOrigin();
    void handleArcDestination();
    void handleKeyPressed(sf::Event const& event);
    void handleMouseButton(sf::Event const& event);

private:

    //! \brief State machine for the animation.
    enum States
    {
        STATE_IDLE, STATE_STARTING, STATE_ENDING, STATE_ANIMATING
    };

    //! \brief Set true if the thread of the application shall stay alive.
    std::atomic<bool> m_running{true};
    std::atomic<bool> m_pause{false};
    //! \brief Set true when simulating the Petri net.
    std::atomic<bool> m_simulating{false};
    //! \brief State machine for the animation.
    std::atomic<States> m_state{STATE_IDLE};
    //! \brief Set true when the user is pressing the Control key.
    std::atomic<bool> m_ctrl{false};
    //! \brief SFML shape needed to draw a Petri Place.
    sf::CircleShape m_figure_place;
    //! \brief SFML shape needed to draw a Petri Token.
    sf::CircleShape m_figure_token;
    //! \brief SFML shape needed to draw a Petri Transition.
    sf::RectangleShape m_figure_trans;
    //! \brief SFML loaded font from a ttf file.
    sf::Font m_font;
    //! \brief SFML structure for rendering the caption on nodes.
    sf::Text m_text_caption;
    //! \brief SFML structure for rendering the number of tokens in Places.
    sf::Text m_text_token;
    //! \brief Selected origin node (place or transition) by the user when
    //! adding an arc.
    Node* m_node_from = nullptr;
    //! \brief Selected destination node (place or transition) by the user when
    //! adding an arc.
    Node* m_node_to = nullptr;
    //! \brief The user has select a node to be displaced.
    std::vector<Node*> m_selected_modes;
    float m_x = 0.0f; float m_y = 0.0f; bool m_arc_from_unknown_node = false;
    //! \brief Mouse cursor position.
    sf::Vector2f m_mouse;
    //! \brief The Petri net.
    PetriNet m_petri_net;
    //! \brief Animation of tokens when transitioning from Transitions to Places.
    std::vector<AnimatedToken> m_animation_TP;
};

#endif
