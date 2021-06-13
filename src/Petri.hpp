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
struct Place
{
    Place(float const x_, float const y_, size_t const tokens_ = 0u)
        : m_id(s_count),
          x(x_),
          y(y_),
          tokens(tokens_)
    {
        s_count += 1u;
        caption = "P" + std::to_string(m_id);
    }

    size_t id() const
    {
        return m_id;
    }

private:

    static std::atomic<size_t> s_count;
    size_t m_id;

public:

    float x;
    float y;
    size_t tokens;
    std::string caption;
};

// *****************************************************************************
//! \brief
// *****************************************************************************
struct Transition
{
    Transition(float const x_, float const y_)
        : m_id(s_count),
          x(x_),
          y(y_)
    {
        s_count += 1u;
        caption = "T" + std::to_string(m_id);
    }

    size_t id() const
    {
        return m_id;
    }

private:

    static std::atomic<size_t> s_count;
    size_t m_id;

public:

    float x;
    float y;
    std::string caption;
};

// *****************************************************************************
//! \brief
// *****************************************************************************
struct Arc
{/*
    Arc(size_t const from_, size_t const to_)
        : from(_from), to(_to)
    {
        keys[0] = from.key;
        keys[1] = to.key;
    }

    size_t from;
    size_t to;
    size_t keys[2];*/
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
        reset();
    }

    void reset()
    {
        m_places.clear();
        m_transitions.clear();
        //m_arcs.clear();
    }

    void addPlace(float const x, float const y, size_t const tokens = 0u)
    {
        m_places.push_back(Place(x, y, tokens));
    }

    std::vector<Place> const& places() const
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

private:

    std::vector<Place> m_places;
    std::vector<Transition> m_transitions;
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

    //! \brief Draw tokans inside a place
    void draw(size_t const tokens, float const x, float const y);

private:

    std::atomic<bool> m_running{true};

    sf::CircleShape m_figure_place;
    sf::CircleShape m_figure_token;
    sf::RectangleShape m_figure_trans;
    sf::Font m_font;
    sf::Text m_text;

    PetriNet m_petri_net;
};

#endif
