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
    Arc(Node& from_, Node& to_)
        : from(from_), to(to_)
    {}

    //! \brief Origin node (Place or Transition).
    Node& from;
    //! \brief Destination node (Place or Transition).
    Node& to;
};

// *****************************************************************************
//! \brief Petri Place node. It hold tokens.
// *****************************************************************************
struct Place : public Node
{
    //! \param[in] x: X-axis coordinate in the window needed for the display.
    //! \param[in] y: Y-axis coordinate in the window needed for the display.
    //! \param[in] tok: Initial number of tokens in the place.
    Place(float const x, float const y, size_t const tok = 0u)
        : Node(Node::Type::Place, s_count++, x, y), tokens(tok)
    {}

    //! \brief the number of tokens hold by the Place
    size_t tokens;

private:

    //! \brief Auto increment unique identifier.
    static std::atomic<size_t> s_count;
};

// *****************************************************************************
//! \brief Petri Transition node.
// *****************************************************************************
struct Transition : public Node
{
    //! \param[in] x: X-axis coordinate in the window needed for the display.
    //! \param[in] y: Y-axis coordinate in the window needed for the display.
    Transition(float const x, float const y)
        : Node(Node::Type::Transition, s_count++, x, y)
    {}

    //! \brief Hold the incoming arcs.
    //! \note this vector is updated by the method PetriNet::cacheArcs().
    //! Posible evolution: update dynamicaly this vector when editing the net
    //! through the GUI.
    std::vector<Arc*> arcsIn;

    //! \brief Hold the outcoming arcs.
    //! \note this vector is updated by the method PetriNet::cacheArcs().
    //! Posible evolution: update dynamicaly this vector when editing the net
    //! through the GUI.
    std::vector<Arc*> arcsOut;

private:

    //! \brief Auto increment unique identifier.
    static std::atomic<size_t> s_count;
};

// *****************************************************************************
//! \brief Animated Tokens when transitions are fired.
// *****************************************************************************
struct AnimatedToken
{
    AnimatedToken(Arc& arc, bool PT);

    bool update(float const dt);

    //! \brief Unique identifier (useless but can help for debugging).
    size_t id;
    //! \brief X-axis coordinate in the window used for the display.
    float x;
    //! \brief Y-axis coordinate in the window used for the display.
    float y;
    //! \brief In which arc the token is transitioning.
    Arc* currentArc;
    //! \brief Cache the magnitude of the arc.
    float magnitude;
    //! \brief What ratio the token has transitioned over the arc (0%: origin
    //! position, 100%: destination position).
    float offset = 0.0f;
};

// *****************************************************************************
//! \brief Class holding and managing Places, Transitions and Arcs.
// *****************************************************************************
class PetriNet
{
public:

    //! \brief Reserve memory for manipulating up to 128 nodes and arcs without
    //! needed doing intermediate allocations.
    PetriNet()
    {
        m_places.reserve(128u);
        m_transitions.reserve(128u);
        m_arcs.reserve(128u);
    }

    //! \brief Remove all nodes and arcs.
    void reset()
    {
        m_places.clear();
        m_transitions.clear();
        m_arcs.clear();
    }

    //! \brief Add a new Petri Place.
    void addPlace(float const x, float const y, size_t const tokens = 0u)
    {
        m_places.push_back(Place(x, y, tokens));
    }

    std::vector<Place> const& places() const
    {
        return m_places;
    }

    std::vector<Place>& places()
    {
        return m_places;
    }

    //! \brief Add a new Petri Transition.
    void addTransition(float const x, float const y)
    {
        m_transitions.push_back(Transition(x, y));
    }

    std::vector<Transition> const& transitions() const
    {
        return m_transitions;
    }

    std::vector<Transition>& transitions()
    {
        return m_transitions;
    }

    //! \brief Add a new arc between two Petri nodes (place or transition).
    //! \return true if the arc is valid and has been added, else return false
    //! if an arc is already present or nodes have the same type.
    bool addArc(Node& from, Node& to)
    {
        if (from.type == to.type)
            return false;

        if (hasArc(from, to))
            return false;

        m_arcs.push_back(Arc(from, to));
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

    std::vector<Arc> const& arcs() const
    {
        return m_arcs;
    }

    //! \brief Populate in and out arcs for all transitions.
    void cacheArcs();

private:

    //! \brief List of Places.
    std::vector<Place> m_places;
    //! \brief List of Transitions.
    std::vector<Transition> m_transitions;
    //! \brief List of Arcs.
    std::vector<Arc> m_arcs;
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
    virtual bool isRunning() override;

    //! \brief Called when the GUI has been enabled.
    virtual void activate() override {}

    //! \brief Called when the GUI has been disabled.
    virtual void deactivate() override {}

private:

    //! \brief Draw a place
    void draw(Place const& place);

    //! \brief Draw a transition
    void draw(Transition const& transition);

    //! \brief Draw tokens inside a place
    void draw(size_t const tokens, float const x, float const y);

    //! \brief Draw a Petri arc with arrow
    void draw(Arc const& arc);

    //! \brief Search and return if a place or a transition is present at the
    //! given coordinates.
    //! \param[in] x: X-axis coordinate of the mouse cursor.
    //! \param[in] y: Y-axis coordinate of the mouse cursor.
    //! \return the address of the place or the transition if present, else
    //! return nullptr.
    Node* getNode(float const x, float const y);

private:

    //! \brief Set true if the thread of the application shall stay alive.
    std::atomic<bool> m_running{true};
    //! \brief Set true when simulating the Petri net.
    std::atomic<bool> m_simulating{false};
    //! \brief SFML shape needed to draw a Petri Place.
    sf::CircleShape m_figure_place;
    //! \brief SFML shape needed to draw a Petri Token.
    sf::CircleShape m_figure_token;
    //! \brief SFML shape needed to draw a Petri Transition.
    sf::RectangleShape m_figure_trans;
    //! \brief SFML loaded font from a ttf file.
    sf::Font m_font;
    //! \brief SFML structure for rendering the caption on Places.
    sf::Text m_text_place;
    //! \brief SFML structure for rendering the number of tokens in Places.
    sf::Text m_text_token;
    //! \brief SFML structure for rendering the caption on Transitions.
    sf::Text m_text_trans;
    //! \brief Selected origin node (place or transition) by the user when
    //! adding an arc.
    Node* m_node_from = nullptr;
    //! \brief Selected destination node (place or transition) by the user when
    //! adding an arc.
    Node* m_node_to = nullptr;
    //! \brief Mouse cursor position.
    sf::Vector2f m_mouse;
    //! \brief The Petri net.
    PetriNet m_petri_net;
    //! \brief Animation of tokens when transitioning from Places to Transitions.
    std::vector<AnimatedToken> m_animation_PT;
    //! \brief Animation of tokens when transitioning from Transitions to Places.
    std::vector<AnimatedToken> m_animation_TP;
};

#endif
