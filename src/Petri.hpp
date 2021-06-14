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
//! \brief
// *****************************************************************************
struct Token
{
    Token(size_t const count)
        : m_count(count)
    {}

    // prefix increment
    Token& operator++()
    {
        ++m_count;
        return *this; // return new value by reference
    }

    // postfix increment
    Token operator++(int)
    {
        Token old = *this; // copy old value
        operator++();  // prefix increment
        return old;    // return old value
    }

    // prefix decrement
    Token& operator--()
    {
        if (m_count == 0u)
            throw std::range_error("No token to decrement");
        --m_count;
        return *this; // return new value by reference
    }

    // postfix decrement
    Token operator--(int)
    {
        Token old = *this; // copy old value
        operator--();  // prefix decrement
        return old;    // return old value
    }

    operator size_t const& () const
    {
        return m_count;
    }

    bool operator==(Token const &other) const
    {
        return m_count == other.m_count;
    }

    bool operator<(Token const &other) const
    {
        return m_count < other.m_count;
    }

private:

    size_t m_count;
};

// *****************************************************************************
//! \brief
// *****************************************************************************
struct Node
{
    enum Type { Place, Transition };

    Node(Type const type_, size_t const id_, float const x_, float const y_)
        : id(id_),
          type(type_),
          x(x_),
          y(y_)
    {
        m_key = (type == Node::Type::Place) ? 'P' : 'T';
        m_key += std::to_string(id);
        caption = m_key;
    }

    std::string const& key() const
    {
        return m_key;
    }

    bool operator==(Node const &other) const
    {
        return (type == other.type) && (id == other.id);
    }

    size_t const id;
    Type const type;
    float x;
    float y;
    std::string caption;

private:

    std::string m_key;
};

// *****************************************************************************
//! \brief
// *****************************************************************************
struct Place : public Node
{
    Place(float const x, float const y, size_t const tok = 0u)
        : Node(Node::Type::Place, s_count++, x, y),
          tokens(tok)
    {}

    Token tokens;

private:

    static std::atomic<size_t> s_count;
};

// *****************************************************************************
//! \brief
// *****************************************************************************
struct Transition : public Node
{
    Transition(float const x, float const y)
        : Node(Node::Type::Transition, s_count++, x, y)
    {}

private:

    static std::atomic<size_t> s_count;
};

// *****************************************************************************
//! \brief
// *****************************************************************************
struct Arc
{
    Arc(Node const& from_, Node const& to_)
        : from(from_), to(to_)
    {}

    Node const& from;
    Node const& to;
};

// *****************************************************************************
//! \brief
// *****************************************************************************
class PetriNet
{
public:

    PetriNet()
    {
        m_places.reserve(128u);
        m_transitions.reserve(128u);
        m_arcs.reserve(128u);
        reset();
    }

    void reset()
    {
        m_places.clear();
        m_transitions.clear();
        m_arcs.clear();
    }

    void simulate(float const /*dt*/)
    {
        if (!run)
            return ;
    }

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

    bool addArc(Node const& from, Node const& to)
    {
        if (from.type == to.type)
            return false;

        if (hasArc(from, to))
            return false;

        m_arcs.push_back(Arc(from, to));
        return true;
    }

    bool hasArc(Node const& from, Node const& to)
    {
        for (auto const& it: m_arcs)
        {
            if ((it.from == from) && (it.to == to))
                return true;
        }
        return false;
    }

    std::vector<Arc> const& arcs() const
    {
        return m_arcs;
    }

public:

    bool run = false;

private:

    std::vector<Place> m_places;
    std::vector<Transition> m_transitions;
    std::vector<Arc> m_arcs;
};

// *****************************************************************************
//! \brief
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

    Node* getNode(float const x, float const y);

private:

    std::atomic<bool> m_running{true};

    sf::CircleShape m_figure_place;
    sf::CircleShape m_figure_token;
    sf::RectangleShape m_figure_trans;
    sf::Font m_font;
    sf::Text m_text;

    // Selected node (place or transition)
    Node* m_node_from = nullptr;
    Node* m_node_to = nullptr;

    sf::Vector2f m_mouse;

    PetriNet m_petri_net;
};

#endif
