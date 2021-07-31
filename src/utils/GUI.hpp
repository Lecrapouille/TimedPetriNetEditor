//=====================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 Quentin Quadrat <lecrapouille@gmail.com>
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

#ifndef GUI_HPP
#  define GUI_HPP

// *****************************************************************************
//! \file This file gets its inspiration from the following document but I
//! adapted it for my own usage:
//! https://www.binpress.com/tutorial/creating-a-city-building-game-with-sfml/137
// *****************************************************************************

#  include <SFML/Graphics.hpp>
#  include <SFML/System.hpp>
#  include <stack>
#  include <cassert>

// *****************************************************************************
//! \brief Interface class for drawing a SFML window and handling mouse and
//! keyboard events.
// *****************************************************************************
class GUIStates
{
    friend class Application;

public:

    //! \brief Hold and manage an application. The application shall be destroy
    //! after this class.
    GUIStates(const char* name, sf::RenderWindow& render)
        : m_render(render), bgColor(0, 0, 100, 255), m_name(name)
    {}

    //! \brief Needed because of virtual methods.
    virtual ~GUIStates() = default;

    //! \brief Return the SFML window.
    inline sf::RenderWindow& renderer()
    {
        return m_render;
    }

    //! \brief Return then GUI name (debug purpose)
    inline std::string const& name() const
    {
        return m_name;
    }

private:

    //! \brief Private methods that derived classes have to implement: called
    //! when the GUI is pushed.
    virtual void activate() = 0;
    //! \brief Private methods that derived classes have to implement: called
    //! when the GUI is poped.
    virtual void deactivate() = 0;
    //! \brief Private methods that derived classes have to implement: return
    //! true if the thread of the GUI is running.
    virtual bool isRunning() = 0;
    //! \brief Private methods that derived classes have to implement: draw the
    //! GUI.
    virtual void draw(const float dt) = 0;
    //! \brief Private methods that derived classes have to implement: manage
    //! the logic of the GUI.
    virtual void update(const float dt) = 0;
    //! \brief Private methods that derived classes have to implement: manage IO
    //! events (mouse, keyboard).
    virtual void handleInput() = 0;

protected:

    sf::RenderWindow& m_render;

public:

    sf::Color bgColor;

private:

    std::string m_name;
};

// *****************************************************************************
//! \brief Manage a stack of GUI class. Since we desire to manage a very simple
//! application (chess board GUI and promotion GUI), GUIs are simply pushed and
//! poped on a stack and only the GUI of the top of the stack (TOS) is active
//! (drawn by SFML and reactives to IO events), others are inactive until they
//! reached the stack top position. Poped GUI are not released.
// *****************************************************************************
class Application
{
    friend class GUIStates;

public:

    //! \brief Create a SFML window with an empty stack.
    //! \param[in] width: width dimension of the window.
    //! \param[in] height: height dimension of the window.
    //! \param[in] title: window title.
    Application(uint32_t const width, uint32_t const height, std::string const& title)
    {
        m_renderer.create(sf::VideoMode(width, height), title);
        m_renderer.setFramerateLimit(60);
    }

    //! \brief Push a new GUI which will be draw by SFML.
    inline void push(GUIStates& gui)
    {
        m_guis.push(&gui);
        gui.activate();
    }

    //! \brief Drop the current GUI. The new GUI on the top of the stack will be
    //! active and draw by SFML.
    inline void pop()
    {
        assert(!m_guis.empty());
        m_guis.top()->deactivate();
        m_guis.pop();
    }

    //! \brief Get the GUI placed on the top of the stack.
    inline GUIStates& peek()
    {
        assert(!m_guis.empty());
        return *m_guis.top();
    }

    //! \brief Push a new GUI on the top of the stack and start a loop for
    //! managing its draw and IO events. When the GUI is closed it will be drop
    //! from the stack.
    void loop(GUIStates& gui)
    {
        sf::Clock clock;

        push(gui);
        while (gui.isRunning())
        {
            float dt = clock.restart().asSeconds();
            GUIStates& gui = peek();
            m_renderer.clear(gui.bgColor);
            gui.handleInput();
            gui.update(dt);
            gui.draw(dt);
            m_renderer.display();
        }

        pop();
    }

    //! \brief Pop the GUI on the top of the stack and start a loop for
    //! managing its draw and IO events. When the GUI is closed it will be drop
    //! from the stack.
    inline void loop()
    {
        loop(peek());
    }

    inline sf::RenderWindow& renderer()
    {
        return m_renderer;
    }

private:

    std::stack<GUIStates*> m_guis;
    //! \brief GUIs use th SMFL library for their rendering.
    sf::RenderWindow m_renderer;
};

#endif
