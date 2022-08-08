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

#ifndef APPLICATION_HPP
#  define APPLICATION_HPP

// *****************************************************************************
//! \file This file is an adaptation of the following code source:
//! https://www.binpress.com/creating-city-building-game-with-sfml-state-manager
//! The adaptation is made for the usage of this particular project.
// *****************************************************************************

#  include <SFML/Graphics.hpp>
#  include <SFML/System.hpp>
#  include "utils/Utils.hpp"
#  include <stack>
#  include <cassert>
#  include <iostream>
#  include <memory>

// *****************************************************************************

//! \brief Manage a stack of GUI instances (GUI which shall derive from the
//! interface Application::GUI class). Since we desire to manage a very simple
//! application, GUIs are simply pushed and poped in a stack (\c m_stack) and
//! only the GUI on the top of the stack is active, reacts to IO events (mouse,
//! keyboard ...)  and are rendered by the SFML library. Others stacked GUIs are
//! inactive until they reached the top position in the stack where they become
//! active.  Therefore a GUI pushing a child GUI in the stack will be in pause
//! until the child GUI is poped off the stack.

//! Finally, note when a GUI is puash it does not have not their memory created
//! (and when poped they do not have their memory released). Indeed, the stack
//! holds and refers GUIs fromraw pointers. GUI are in fact created can be
//! stored into a separate internal container (\c m_guis created through the \c
//! create() method and get from the \c gui() method).

// *****************************************************************************
class Application
{
public:

    // *************************************************************************
    //! \brief Interface class for drawing a GUI and handling mouse and keyboard
    //! events. This instance knows the Application instance owning it.
    //! \pre This class is dependent from the SFML library.
    // *************************************************************************
    class GUI
    {
        //! \brief Access to private methods
        friend class Application;

    public:

        //----------------------------------------------------------------------
        //! \brief Default constructor. No actions are made except initializing
        //! internal states with values passed as parameters.
        //! \param[in] application: the instance owning GUIs.
        //! \param[in] name: Name of the GUI. Used as key for Application to
        //!   find it back.
        //! \param[in] color: Background color.
        //----------------------------------------------------------------------
        GUI(Application& application, const char* name, sf::Color const& color
            = sf::Color::White);

        //----------------------------------------------------------------------
        //! \brief Needed because of virtual methods.
        //----------------------------------------------------------------------
        virtual ~GUI() = default;

        //----------------------------------------------------------------------
        //! \brief Return the SFML renderer.
        //----------------------------------------------------------------------
        inline sf::RenderWindow& renderer()
        {
            return m_renderer;
        }

        //----------------------------------------------------------------------
        //! \brief Return then GUI name (as search key or for debug purpose).
        //----------------------------------------------------------------------
        inline std::string const& name() const
        {
            return m_name;
        }

        //----------------------------------------------------------------------
        //! \brief Change the windows title.
        //----------------------------------------------------------------------
        inline void title(std::string const& name)
        {
            return m_renderer.setTitle(name);
        }

    private:

        //----------------------------------------------------------------------
        //! \brief Internal methods called by the Application class when the GUI
        //! is pushed in the stack.
        //----------------------------------------------------------------------
        virtual void create() = 0;

        //----------------------------------------------------------------------
        //! \brief Internal methods called by the Application class when the GUI
        //! is poped from the stack.
        //----------------------------------------------------------------------
        virtual void release() = 0;

        //----------------------------------------------------------------------
        //! \brief Internal methods called by the Application class when the GUI
        //! is no longer on the top of the stack.
        //----------------------------------------------------------------------
        virtual void deactivate() = 0;

        //----------------------------------------------------------------------
        //! \brief Internal methods called by the Application class when the GUI
        //! has returned to the top of the stack.
        //----------------------------------------------------------------------
        virtual void activate() = 0;

        //----------------------------------------------------------------------
        //! \brief Internal methods called by the Application class to know if
        //! the GUI has finished and shall be poped.
        //----------------------------------------------------------------------
        virtual bool running() const = 0;

        //----------------------------------------------------------------------
        //! \brief Internal methods called by the Application class when the GUI
        //! has to be rendered.
        //----------------------------------------------------------------------
        virtual void draw() = 0;

        //----------------------------------------------------------------------
        //! \brief Internal methods called by the Application class when the GUI
        //! has to be updated.
        //! \param[in] dt the number of seconds spent this the previous call
        //! (delta time).
        //----------------------------------------------------------------------
        virtual void update(const float dt) = 0;

        //----------------------------------------------------------------------
        //! \brief Internal methods called by the Application class when the GUI
        //! has to manage IO events (mouse, keyboard, ...).
        //----------------------------------------------------------------------
        virtual void handleInput() = 0;

    public:

        //! \brief the background color
        sf::Color background_color;

    protected:

        //! \brief The application owning this GUI instance
        Application& m_application;
        //! \brief the FSML renderer for drawing things.
        sf::RenderWindow& m_renderer;
        //! \brief Title of the main application
        std::string m_title;

    private:

        //! \brief the GUI name (used as key search or for debug purpose).
        std::string m_name;
    };

public:

    //--------------------------------------------------------------------------
    //! \brief Create a SFML window with an empty stack of GUI.
    //! \param[in] width: the initial width dimension of the window [pixel].
    //! \param[in] height: the initial height dimension of the window [pixel].
    //! \param[in] title: window title.
    //--------------------------------------------------------------------------
    Application(uint32_t const width, uint32_t const height, std::string const& title);

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    ~Application();

    //--------------------------------------------------------------------------
    //! \brief Return the window width [pixel].
    //--------------------------------------------------------------------------
    inline uint32_t width() const
    {
        return m_renderer.getSize().x;
    }

    //--------------------------------------------------------------------------
    //! \brief Return the window height [pixel].
    //--------------------------------------------------------------------------
    inline uint32_t height() const
    {
        return m_renderer.getSize().y;
    }

    //----------------------------------------------------------------------
    //! \brief Create a new GUI whih is not stored in the GUI stack.
    //! \param[in] name he GUI name used as key search or for debug purpose.
    //! \return the reference to the created GUI and stored internally.
    //----------------------------------------------------------------------
    template<class GUI>
    GUI& create(const char* name)
    {
        m_guis.push_back(std::make_unique<GUI>(*this, name));
        return *reinterpret_cast<GUI*>(m_guis.back().get());
    }

    //----------------------------------------------------------------------
    //! \brief Search and return the given GUI name and type.
    //! \param[in] name he GUI name used as key search.
    //! \return the reference to the created GUI and stored internally.
    //! \note if the GUI is not found a new one is created.
    //----------------------------------------------------------------------
    template<class GUI>
    GUI& gui(const char* name)
    {
        for (auto& it: m_guis)
        {
            if (it->name() != name)
                continue;

            GUI* g = dynamic_cast<GUI*>(it.get());
            if (g != nullptr)
                return *g;
        }
        return create<GUI>(name);
    }

    //--------------------------------------------------------------------------
    //! \brief Push a new GUI which will be draw by SFML.
    //! \note the GUI will not be released when poped.
    //--------------------------------------------------------------------------
    void push(Application::GUI& gui);

    //--------------------------------------------------------------------------
    //! \brief Drop the current GUI. The new GUI on the top of the stack will be
    //! active and draw by SFML.
    //! \note the poped GUI is not released.
    //! \pre The stack shall have at least one GUI.
    //--------------------------------------------------------------------------
    void pop();

    //--------------------------------------------------------------------------
    //! \brief Get the GUI placed on the top of the stack.
    //! \return the address of the GUI (nullptr is the stack is empty).
    //--------------------------------------------------------------------------
    inline Application::GUI* peek()
    {
        return m_stack.empty() ? nullptr : m_stack.top();
    }

    //--------------------------------------------------------------------------
    //! \brief Push a new GUI on the top of the stack and start a loop for
    //! managing its draw and IO events. When the GUI is closed it will be drop
    //! from the stack.
    //!
    //! \note this method will call Application::GUI's private pure virtual
    //! methods.
    //--------------------------------------------------------------------------
    void loop(Application::GUI& starting_gui);

    //--------------------------------------------------------------------------
    //! \brief Return the SFML renderer needed to paint SFML shapes
    //! (rectangles, circles ...)
    //--------------------------------------------------------------------------
    inline sf::RenderWindow& renderer()
    {
        return m_renderer;
    }

    sf::Rect<float> bounds()
    {
        return { 0.0f, 0.0f, float(m_renderer.getSize().x), float(m_renderer.getSize().y) };
    }

private:

    void loop();

private:

    //! \brief Current activate gui
    Application::GUI* m_gui = nullptr;
    //! \brief List of GUIs.
    std::vector<std::unique_ptr<Application::GUI>> m_guis;
    //! \brief Stack of GUIs.
    std::stack<Application::GUI*> m_stack;
    //! \brief SFML renderer.
    sf::RenderWindow m_renderer;
};

#endif
