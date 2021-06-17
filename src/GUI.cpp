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

Application::Application(uint32_t const width, uint32_t const height,
                         std::string const& title)
{
    m_window.create(sf::VideoMode(width, height), title);
    //m_window.setFramerateLimit(10);
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

GUI& Application::peek()
{
    assert(!m_guis.empty());
    return *m_guis.top();
}

void Application::loop(GUI& gui)
{
    sf::Clock clock;

    push(gui);
    while (gui.isRunning())
    {
        float dt = clock.restart().asSeconds();
        GUI& gui = peek();
        gui.handleInput();
        gui.update(dt);
        m_window.clear(gui.bgColor);
        gui.draw(dt);
        m_window.display();
    }

    pop();
}

void Application::loop()
{
    loop(peek());
}
