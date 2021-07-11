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
#  include <cassert>

class Arc;

// *****************************************************************************
//! \brief Since Petri nets are bipartite graph there is two kind of nodes:
//! place and transition. This struct allows to factorize the code of derived
//! struct Place and Transition and therefore shall not be used directly as
//! instance.
// *****************************************************************************
struct Node
{
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

        timer.restart();
    }

    //--------------------------------------------------------------------------
    //! \brief Copy constructor needed because this class has constants variable
    //! members.
    //--------------------------------------------------------------------------
    Node& operator=(const Node& obj)
    {
        this->~Node(); // destroy
        new (this) Node(obj); // copy construct in place
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

    //! \brief Unique identifier (auto-incremented from 0).
    size_t const id;
    //! \brief Petri Place or Petri Transition.
    Type const type;
    //! \brief Position in the window needed for the display.
    float x;
    //! \brief Position in the window needed for the display.
    float y;
    //! \brief Unique node identifier as string. It is formed by the 'P' char
    //! for place or by the 'T' char for transition followed by the unique
    //! identifier (i.e. P0, P1, T0, T1 ...)
    std::string const key;
    //! \brief text display near the place (by default initialized with key
    //! string).
    std::string caption;
    //! \brief Timer for fading colors.
    sf::Clock timer;
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
//! \brief Petri nodes of different types are directed by arcs. Therefore arcs
//! only make the link between Place and Transition.
// *****************************************************************************
struct Arc
{
    //--------------------------------------------------------------------------
    //! \brief Constructor. This method does not check if the two nodes have
    //! different types. This shall be made by the caller class.
    //! \param[in] from: Origin node (Place or Transition).
    //! \param[in] to: Destination node (Place or Transition).
    //! \note Nodes shall have different types. No check is made here.
    //--------------------------------------------------------------------------
    Arc(Node& from_, Node& to_, float duration_ = 0.0f)
        : from(from_), to(to_), duration(duration_)
    {
        assert(from.type != to.type);
        timer.restart();
    }

    //--------------------------------------------------------------------------
    //! \brief Hack needed because of references
    //--------------------------------------------------------------------------
    Arc& operator=(const Arc& obj)
    {
        this->~Arc(); // destroy
        new (this) Arc(obj); // copy construct in place
        return *this;
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
    sf::Clock timer;
};

// *****************************************************************************
//! \brief Petri Place node. Places represent system states. Places hold tokens
//! (resources). In Grafcet, Place has only one token and when they are
//! activated actions are performed. This class does not managed Grafcet.
// *****************************************************************************
struct Place : public Node
{
    //! \brief to access s_next_id
    friend class PetriNet;
    //! \brief to access m_backup_tokens
    friend class PetriGUI;

    //--------------------------------------------------------------------------
    //! \brief Constructor. To be used when the user clicked on the GUI.
    //! \param[in] x: X-axis coordinate in the window needed for the display.
    //! \param[in] y: Y-axis coordinate in the window needed for the display.
    //--------------------------------------------------------------------------
    Place(float const x, float const y)
        : Node(Node::Type::Place, s_next_id++, x, y),
          tokens(0u), m_backup_tokens(0u)
    {}

    //--------------------------------------------------------------------------
    //! \brief Constructor. To be used when loading a Petri net from JSON file.
    //! \param[in] id: unique node identifier.
    //! \param[in] x: X-axis coordinate in the window needed for the display.
    //! \param[in] y: Y-axis coordinate in the window needed for the display.
    //! \param[in] tok: Initial number of tokens in the place.
    //--------------------------------------------------------------------------
    Place(size_t const id, float const x, float const y, size_t const tok)
        : Node(Node::Type::Place, id, x, y),
          tokens(tok), m_backup_tokens(tok)
    {
        s_next_id = std::max(s_next_id.load(), id) + 1u;
    }

    //! \brief the number of tokens hold by the Place
    size_t tokens;

private:

    //! \brief Before starting the simulation on the Petri net we have to
    //! save the number of tokens to allow restoring initial states when the
    //! simulation is done.
    size_t m_backup_tokens;
    //! \brief Auto increment unique identifier. Start from 0 (code placed in
    //! the cpp file).
    static std::atomic<size_t> s_next_id;
};

// *****************************************************************************
//! \brief Petri Transition node. In Petri transitivities are always true and
//! when in each above Places, all of them have at least one token they let pass
//! tokens. In Grafcet, transitivities are dynamic and usually depends on
//! systems inputs (sensors). This class does not managed Grafcet.
// *****************************************************************************
struct Transition : public Node
{
    //! \brief to access s_next_id
    friend class PetriNet;

    //--------------------------------------------------------------------------
    //! \brief Constructor. To be used when the user clicked on the GUI.
    //! \param[in] x: X-axis coordinate in the window needed for the display.
    //! \param[in] y: Y-axis coordinate in the window needed for the display.
    //--------------------------------------------------------------------------
    Transition(float const x, float const y)
        : Node(Node::Type::Transition, s_next_id++, x, y)
    {}

    //--------------------------------------------------------------------------
    //! \brief Constructor. To be used when loading a Petri net from JSON file.
    //! \param[in] id: unique node identifier.
    //! \param[in] x: X-axis coordinate in the window needed for the display.
    //! \param[in] y: Y-axis coordinate in the window needed for the display.
    //! \param[in] angle_: angle in degree of rotation for the display.
    //--------------------------------------------------------------------------
    Transition(size_t const id, float const x, float const y, int const angle_)
        : Node(Node::Type::Transition, id, x, y),
          angle(angle_)
    {
        s_next_id = std::max(s_next_id.load(), id) + 1u;
    }

    //! \brief Transitions are rendered as rectangle. Allow to turn it when
    //! displayed this allows to display horizontal or vertical transition.
    int angle = 0u;

private:

    //! \brief Auto increment unique identifier. Start from 0 (code placed in
    //! the cpp file).
    static std::atomic<size_t> s_next_id;
};

// *****************************************************************************
//! \brief Tokens are systems resources. Places indicate how many tokens they
//! have but in this project, when simulation is run, we want to render them
//! moving along arcs Transitions -> Places (note there is no animation for arcs
//! Places -> Transitions: they are teleported). For the rendering, instead of
//! showing many tokens (dots) at the same position, we "group" them as a dot
//! with the number of tokens carried as caption. Since we are working on timed
//! petri nets arcs have a duration which is also constrain their velocity.
// *****************************************************************************
struct AnimatedToken
{
    AnimatedToken& operator=(const AnimatedToken& obj)
    {
        this->~AnimatedToken(); // destroy
        new (this) AnimatedToken(obj); // copy construct in place
        return *this;
    }

    //--------------------------------------------------------------------------
    //! \brief Constructor.
    //! \param[in] arc: to which arc token are moving along. Shall be an arc
    //! Transition -> Place. No check is performed here.
    //! \param[in] tokens: the number of tokens it shall carry.
    //--------------------------------------------------------------------------
    AnimatedToken(Arc& arc, size_t tokens);

    //! \brief Update position on the screen.
    //! \param[in] dt: the delta time (in seconds) from the previous call.
    //! \return true when arriving to the destination node (Place) else false.
    bool update(float const dt);

    //! \brief Return the reference of the destination node casted as a Place.
    inline Place& toPlace()
    {
        return *reinterpret_cast<Place*>(&(arc.to));
    }

    //! \brief X-axis coordinate in the window used for the display.
    float x;
    //! \brief Y-axis coordinate in the window used for the display.
    float y;
    //! \brief Number of carried tokens.
    size_t tokens;
    //! \brief In which arc the token is moving along.
    Arc& arc;
    //! \brief The length of the arc.
    float magnitude;
    //! \brief The speed of the token.
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

    //--------------------------------------------------------------------------
    //! \brief Remove all nodes and arcs. Reset counters for unique identifiers.
    //--------------------------------------------------------------------------
    void reset()
    {
        m_places.clear();
        m_transitions.clear();
        m_arcs.clear();
        Place::s_next_id = 0u;
        Transition::s_next_id = 0u;
    }

    //--------------------------------------------------------------------------
    //! \brief Add a new Petri Place. To be used when the user clicked on the
    //! GUI.
    //! \param[in] x: X-axis coordinate in the window needed for the display.
    //! \param[in] y: Y-axis coordinate in the window needed for the display.
    //! \return the reference of the inserted element.
    //--------------------------------------------------------------------------
    Place& addPlace(float const x, float const y)
    {
        m_places.push_back(Place(x, y));
        return m_places.back();
    }

    //--------------------------------------------------------------------------
    //! \brief Add a new Petri Place. To be used when loading a Petri net from
    //! JSON file.
    //! \param[in] id: unique node identifier.
    //! \param[in] x: X-axis coordinate in the window needed for the display.
    //! \param[in] y: Y-axis coordinate in the window needed for the display.
    //! \param[in] tokens: Initial number of tokens in the place.
    //! \return the reference of the inserted element.
    //--------------------------------------------------------------------------
    Place& addPlace(size_t const id, float const x, float const y, size_t const tokens)
    {
        m_places.push_back(Place(id, x, y, tokens));
        return m_places.back();
    }

    //--------------------------------------------------------------------------
    //! \brief Const getter.
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
    Transition& addTransition(size_t const id, float const x, float const y,
                              int const angle)
    {
        m_transitions.push_back(Transition(id, x, y, angle));
        return m_transitions.back();
    }

    //--------------------------------------------------------------------------
    //! \brief Const getter.
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
    //! identifier. Search is O(n) where n is the number of nodes.
    //--------------------------------------------------------------------------
    Node* findNode(std::string const& key);

    //--------------------------------------------------------------------------
    //! \brief Add a new arc between two Petri nodes (place or transition).
    //! \return true if the arc is valid and has been added, else return false
    //! if an arc is already present or nodes have the same type.
    //--------------------------------------------------------------------------
    bool addArc(Node& from, Node& to, float duration = 0.0f)
    {
        if (from.type == to.type)
            return false;

        if (hasArc(from, to))
            return false;

        m_arcs.push_back(Arc(from, to, duration));
        return true;
    }

    //--------------------------------------------------------------------------
    //! \brief Return if the arc linking the two given nodes is present in the
    //! net.
    //! \return the address of the arc if found, else return nullptr.
    //--------------------------------------------------------------------------
    Arc* hasArc(Node const& from, Node const& to)
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
    bool exportToJulia(std::string const& filename);

    //--------------------------------------------------------------------------
    //! \brief Remove an existing Place or a Transition (usually refered by the
    //! mouse cursor).
    //--------------------------------------------------------------------------
    void removeNode(Node& node);

    //--------------------------------------------------------------------------
    //! \brief Populate or update Node::arcsIn and Node::arcsOut for all
    //! transitions and places in the Petri net.
    //--------------------------------------------------------------------------
    void generateArcsInArcsOut();

private:

    //! \brief List of Places. We do not use std::vector to avoid invalidating
    //! node references for arcs after a possible resizing.
    std::deque<Place> m_places;
    //! \brief List of Transitions. We do not use std::vector to avoid
    //! invalidating node references for arcs after a possible resizing.
    std::deque<Transition> m_transitions;
    //! \brief List of Arcs.
    std::deque<Arc> m_arcs;
};

// *****************************************************************************
//! \brief A text inside a rectangle
// *****************************************************************************
struct MessageBar : public sf::Drawable
{
    MessageBar(sf::Font& font)
    {
        m_text.setPosition(0, 0);
        m_text.setFont(font);
        m_text.setCharacterSize(20);
        m_text.setFillColor(sf::Color::Black);

        m_shape.setFillColor(sf::Color(100,100,100));
        m_shape.setOutlineThickness(-1);
        m_shape.setOutlineColor(sf::Color::Black);

        m_timer.restart();
    }

    void setText(const std::string& message)
    {
        m_message = message;
        m_text.setString(m_message);
        m_timer.restart();
    }

    void setSize(sf::Vector2u const& dimensions)
    {
        m_shape.setSize(sf::Vector2f(dimensions.x, 25.0f));
    }

private:

    void draw(sf::RenderTarget& target, sf::RenderStates /*states*/) const override final
    {
        const float BLINK_PERIOD = 2.5f;

        float period = m_timer.getElapsedTime().asSeconds();
        if (period >= BLINK_PERIOD)
        {
            period = BLINK_PERIOD;
        }
        else
        {
            target.draw(m_shape);
            target.draw(m_text);
        }
    }

private:

    sf::Clock m_timer;

    //! \brief Text displayed on the entry
    sf::Text m_text;

    //! \brief Handles appearance of the entry
    sf::RectangleShape m_shape;

    //! \brief String returned when the entry is activated
    std::string m_message;
};

// *****************************************************************************
//! \brief Graphic representation of the Petri net using the SFML library.
// *****************************************************************************
class PetriGUI: public GUI
{
public:

    PetriGUI(Application& application);
    ~PetriGUI();

private: // Derived from GUI

    //--------------------------------------------------------------------------
    //! \brief Inherit from GUI class. Draw the chessboard and pieces.
    //--------------------------------------------------------------------------
    virtual void draw(const float dt) override;

    //--------------------------------------------------------------------------
    //! \brief Inherit from GUI class. Update GUI.
    //--------------------------------------------------------------------------
    virtual void update(const float dt) override;

    //--------------------------------------------------------------------------
    //! \brief Inherit from GUI class. Manage mouse and keyboard events.
    //--------------------------------------------------------------------------
    virtual void handleInput() override;

    //--------------------------------------------------------------------------
    //! \brief Inherit from GUI class. Return true if GUI is alive.
    //--------------------------------------------------------------------------
    virtual bool isRunning() override
    {
        return m_running;
    }

    //--------------------------------------------------------------------------
    //! \brief Called when the GUI has been enabled.
    //--------------------------------------------------------------------------
    virtual void activate() override
    {
        // Do nothing
    }

    //--------------------------------------------------------------------------
    //! \brief Called when the GUI has been disabled.
    //--------------------------------------------------------------------------
    virtual void deactivate() override
    {
        // Do nothing
    }

private:

    //--------------------------------------------------------------------------
    //! \brief Draw a Petri Place (as circle), its caption (text) and its tokens
    //! (as back dots or as a number).
    //--------------------------------------------------------------------------
    void draw(Place const& place, uint8_t alpha);

    //--------------------------------------------------------------------------
    //! \brief Draw a transition as rectangle and its caption.
    //--------------------------------------------------------------------------
    void draw(Transition const& transition, uint8_t alpha);

    //--------------------------------------------------------------------------
    //! \brief Draw a Petri arc as arrow and its duration (text).
    //--------------------------------------------------------------------------
    void draw(Arc const& arc, uint8_t alpha);

    //--------------------------------------------------------------------------
    //! \brief Draw a string centered on x, y coordiates.
    //--------------------------------------------------------------------------
    void draw(sf::Text&, std::string const& str, float const x, float const y);

    //--------------------------------------------------------------------------
    //! \brief Draw a unsigned integer centered on x, y coordiates.
    //--------------------------------------------------------------------------
    void draw(sf::Text&, size_t const number, float const x, float const y);

    //--------------------------------------------------------------------------
    //! \brief Draw a float value centered on x, y coordiates.
    //--------------------------------------------------------------------------
    void draw(sf::Text&, float const number, float const x, float const y);

    //--------------------------------------------------------------------------
    //! \brief Search and return if a place or a transition is present at the
    //! given coordinates.
    //! \param[in] x: X-axis coordinate of the mouse cursor.
    //! \param[in] y: Y-axis coordinate of the mouse cursor.
    //! \return the address of the place or the transition if present, else
    //! return nullptr.
    //--------------------------------------------------------------------------
    Node* getNode(float const x, float const y);

    //--------------------------------------------------------------------------
    //! \brief Handle the origin node of the arc when the user is clicking on
    //! the window.
    //--------------------------------------------------------------------------
    void handleArcOrigin();

    //--------------------------------------------------------------------------
    //! \brief Handle the destination node of the arc when the user is clicking
    //! on the window.
    //--------------------------------------------------------------------------
    void handleArcDestination();

    //--------------------------------------------------------------------------
    //! \brief Handle the keyboard press event.
    //--------------------------------------------------------------------------
    void handleKeyPressed(sf::Event const& event);

    //--------------------------------------------------------------------------
    //! \brief Handle the mouse click event.
    //--------------------------------------------------------------------------
    void handleMouseButton(sf::Event const& event);

private:

    //! \brief State machine for the Petri net simulation.
    enum States
    {
        STATE_IDLE, //! Waiting the user request to start the simulation.
        STATE_STARTING, //! Init states before the simulation.
        STATE_ENDING, //! Restore states after the simulation.
        STATE_ANIMATING //! Simulation on-going: animate tokens.
    };

    //! \brief Set true if the thread of the application shall stay alive.
    //! Set false to quit the application.
    std::atomic<bool> m_running{true};
    //! \brief Set true for starting the simulation the Petri net and maintain
    //! it alive. Set false to halt the simulation.
    std::atomic<bool> m_simulating{false};
    //! \brief State machine for the simulation.
    std::atomic<States> m_state{STATE_IDLE};
    //! \brief Set true when the user is pressing the Control key.
    std::atomic<bool> m_ctrl{false};
    //! \brief SFML circle shape needed to draw a Petri Place.
    sf::CircleShape m_figure_place;
    //! \brief SFML circle shape needed to draw a Petri Token.
    sf::CircleShape m_figure_token;
    //! \brief SFML rectangle shape needed to draw a Petri Transition.
    sf::RectangleShape m_figure_trans;
    //! \brief SFML loaded font from a TTF file.
    sf::Font m_font;
    //! \brief SFML structure for rendering node captions.
    sf::Text m_text_caption;
    //! \brief SFML structure for rendering the number of tokens in Places.
    sf::Text m_text_token;
    //!
    MessageBar m_message_bar;
    //! \brief Selected origin node (place or transition) by the user when
    //! adding an arc.
    Node* m_node_from = nullptr;
    //! \brief Selected destination node (place or transition) by the user when
    //! adding an arc.
    Node* m_node_to = nullptr;
    //! \brief The user has select a node to be displaced.
    std::vector<Node*> m_selected_modes;
    // Ugly stuffs needed when trying to determine which node the user wants to
    // create.
    float m_x = 0.0f; float m_y = 0.0f; bool m_arc_from_unknown_node = false;
    //! \brief Mouse cursor position.
    sf::Vector2f m_mouse;
    //! \brief The Petri net.
    PetriNet m_petri_net;
    //! \brief Animation of tokens when transitioning from Transitions to Places.
    std::vector<AnimatedToken> m_animations;
};

#endif
