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

#include "Petri.hpp"

//------------------------------------------------------------------------------
Petri::Petri(Application &application)
    : GUI("Petri", application)
{
    int r = 50;
    int x1 = 100;
    int y1 = 100;

    int x2 = 150;
    int y2 = 150;

    m_arm = new sf::RectangleShape(sf::Vector2f(5, r));
    m_arm->setOrigin(m_arm->getSize().x / 2, 0);
    m_arm->setPosition(sf::Vector2f(x1, y1));
    m_arm->setFillColor(sf::Color(100, 100, 66));
    //m_arm->setRotation(a * 180 / PI);

    m_body = new sf::CircleShape(25);
    m_body->setOrigin(sf::Vector2f(m_body->getRadius(), m_body->getRadius()));
    m_body->setPosition(sf::Vector2f(x2, y2));
    m_body->setFillColor(sf::Color(244, 125, 66));
}

//------------------------------------------------------------------------------
Petri::~Petri()
{
    delete m_arm;
    delete m_body;
    window().close();
}

//------------------------------------------------------------------------------
void Petri::draw(float const /*dt*/)
{
    window().draw(sf::RectangleShape(*m_arm));
    window().draw(sf::CircleShape(*m_body));

    // Swap buffer
    window().display();
}

//------------------------------------------------------------------------------
void Petri::update(float const /*dt*/)
{
}

//------------------------------------------------------------------------------
bool Petri::isRunning()
{
    return m_running;
}

//------------------------------------------------------------------------------
void Petri::handleInput()
{
    sf::Event event;

    while (window().pollEvent(event))
    {
        switch (event.type)
        {
        case sf::Event::Closed:
            m_running = false;
            break;

        case sf::Event::MouseButtonPressed:
            break;

        case sf::Event::KeyPressed:
            if (event.key.code == sf::Keyboard::Escape)
            {
                m_running = false;
            }
            break;

        default:
            break;
        }
    }
}
