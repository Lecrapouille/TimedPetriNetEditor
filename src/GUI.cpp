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

#include "GUI.hpp"

Application::Application()
{
    m_window.create(sf::VideoMode(800, 600), "Petri Net Editor");
    //m_window.setFramerateLimit(10);
}

Application::~Application()
{
    while (!m_guis.empty())
    {
        pop();
    }
}

void Application::push(GUI& gui)
{
    m_guis.push(&gui);
    gui.activate();
}

void Application::pop()
{
    assert(!m_guis.empty());
    m_guis.top()->deactivate();
    m_guis.pop();
}

GUI* Application::peek()
{
    if (m_guis.empty())
        return nullptr;
    return m_guis.top();
}

void Application::loop(GUI& gui)
{
    // Push
    m_guis.push(&gui);
    gui.activate();

    // Infinite loop
    sf::Clock clock;
    while (gui.isRunning())
    {
        float dt = clock.restart().asSeconds();
        GUI* gui = peek();
        assert(gui != nullptr);
        gui->handleInput();
        gui->update(dt);
        m_window.clear();
        gui->draw(dt);
    }

    // Pop
    m_guis.top()->deactivate();
    m_guis.pop();
}
