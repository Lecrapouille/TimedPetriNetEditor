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

#include "Application.hpp"

// -----------------------------------------------------------------------------
Application::GUI::GUI(Application& application, const char* name,
                      sf::Color const& color)
    : background_color(color), m_application(application),
      m_renderer(application.renderer()), m_name(name)
{}

// -----------------------------------------------------------------------------
Application::Application(uint32_t const width, uint32_t const height,
                         std::string const& title)
{
    m_renderer.create(sf::VideoMode(width, height), title);
    m_renderer.setFramerateLimit(120);
}

// -----------------------------------------------------------------------------
Application::~Application()
{
    // Clear the satck of GUIs
    std::stack<Application::GUI*>().swap(m_stack);
    // Stop the SFML renderer
    m_renderer.close();
}

// FIXME never call m_application.push(m_application.gui<GUIxx>("xxx"));
// from the deactivate() callback!!
// -----------------------------------------------------------------------------
void Application::push(Application::GUI& gui)
{
    GUI* g = peek();
    if (g != nullptr)
    {
        std::cout << "Deactivate GUI: " << g->name() << std::endl;
        g->deactivate();
    }
    m_stack.push(&gui);
    std::cout << "Create GUI: " << gui.name() << std::endl;
    gui.create();
    m_gui = &gui;
}

// -----------------------------------------------------------------------------
void Application::pop()
{
    m_gui = peek();
    if (m_gui != nullptr)
    {
        std::cout << "Release GUI: " << m_gui->name() << std::endl;
        m_stack.pop();
        m_gui->release();

        m_gui = peek();
        if (m_gui != nullptr)
        {
            std::cout << "Activate GUI: " << m_gui->name() << std::endl;
            m_gui->activate();
        }
    }
    else
    {
        std::cout << "Warning! Cannot pop GUI from empty stack" << std::endl;
        return ;
    }
}

// -----------------------------------------------------------------------------
void Application::loop()
{
    sf::Clock clock;

    while (m_renderer.isOpen())
    {
        float dt = clock.restart().asSeconds();

        m_gui = peek();
        if (m_gui != nullptr)
        {
            m_gui->handleInput();
            m_gui->update(dt);
            m_renderer.clear(m_gui->background_color);
            m_gui->draw();
        }
        m_renderer.display();
    }
}

// -----------------------------------------------------------------------------
void Application::loop(Application::GUI& starting_gui)
{
    push(starting_gui);
    loop();
    pop();
}
