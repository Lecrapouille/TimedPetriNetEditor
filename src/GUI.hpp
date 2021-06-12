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

#ifndef GUI_HPP
#  define GUI_HPP

#  include <SFML/Graphics.hpp>
#  include <SFML/System.hpp>
#  include <stack>
#  include <cassert>

// *****************************************************************************
//! \file This file gets its inspiration from the following document but I
//! adapted it for my own usage:
//! https://www.binpress.com/tutorial/creating-a-city-building-game-with-sfml/137
// *****************************************************************************

class GUI;

// *****************************************************************************
//! \brief Manage a stack of GUI class. Since we desire to manage a very simple
//! application (chess board GUI and promotion GUI), GUIs are simply pushed and
//! poped on a stack and only the GUI of the top of the stack (TOS) is active
//! (drawn by SFML and reactives to IO events), others are inactive until they
//! reached the stack top position. Poped GUI are not released.
// *****************************************************************************
class Application
{
    friend class GUI;

public:

    //! \brief Create a SFML window with an empty stack.
    Application();
    //! \brief Release the whole stack (GUIs are released).
    ~Application();
    //! \brief Push a new GUI which will be draw by SFML.
    void push(GUI& gui);
    //! \brief Drop the current GUI. The new GUI on the top of the stack will be
    //! active and draw by SFML.
    void pop();
    //! \brief Get the GUI placed on the top of the stack.
    GUI* peek();
    //! \brief Push a new GUI on the top of the stack and start a loop for
    //! managing its draw and IO events. When the GUI is closed it will be drop
    //! from the stack.
    void loop(GUI& gui);

private:

    std::stack<GUI*> m_guis;
    //! \brief GUIs use th SMFL library for their rendering.
    sf::RenderWindow m_window;
};

// *****************************************************************************
//! \brief Interface class for drawing a SFML window and handling mouse and
//! keyboard events.
// *****************************************************************************
class GUI
{
    friend class Application;

public:

    //! \brief Hold and manage an application. The application shall be destroy
    //! after this class.
    GUI(const char* name, Application& application)
        : m_application(application),
          m_name(name)
    {}

    //! \brief Needed because of virtual methods.
    virtual ~GUI() = default;

    //! \brief Return the SFML window.
    sf::RenderWindow& window()
    {
        return m_application.m_window;
    }

    //! \brief Return then GUI name (debug purpose)
    std::string const& name() const { return m_name; }

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

public:

    Application& m_application;
    std::string m_name;
};

#endif
