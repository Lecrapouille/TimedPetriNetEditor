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
#include <iostream>

std::atomic<size_t> Place::s_count{0u};
std::atomic<size_t> Transition::s_count{0u};

const float TR_WIDTH = 50.0f; // Transition
const float TR_HEIGHT = 5.0f; // Transition
const float PL_RADIUS = 25.0f; // Places
const float TN_RADIUS = 4.0f;  // Token in places
const float ARC_SHORT = PL_RADIUS + 2.0f;
const float ARC_TYPE = 2.0f;
const float DOUBLE_SHIFT = 10.0f;
const float FONT_SIZE = 24.0f;

//------------------------------------------------------------------------------
PetriGUI::PetriGUI(Application &application)
    : GUI("PetriGUI", application),
      m_figure_place(PL_RADIUS),
      m_figure_token(TN_RADIUS),
      m_figure_trans(sf::Vector2f(TR_HEIGHT, TR_WIDTH))
{
    // Precompute SFML struct for drawing places
    m_figure_place.setOrigin(sf::Vector2f(m_figure_place.getRadius(), m_figure_place.getRadius()));
    m_figure_place.setFillColor(sf::Color::White);
    m_figure_place.setOutlineThickness(2.0f);
    m_figure_place.setOutlineColor(sf::Color(244, 125, 66));

    // Precompute SFML struct for drawing tokens inside places
    m_figure_token.setOrigin(sf::Vector2f(m_figure_token.getRadius(), m_figure_token.getRadius()));
    m_figure_token.setFillColor(sf::Color::Black);

    // Precompute SFML struct for drawing transitions
    m_figure_trans.setOrigin(m_figure_trans.getSize().x / 2, 0);
    m_figure_trans.setFillColor(sf::Color(100, 100, 66));

    // Precompute SFML struct for drawing text (places and transitions)
    if (!m_font.loadFromFile("OpenSans-Regular.ttf"))
    {
        std::cerr << "Could not load font file ..." << std::endl;
        exit(1);
    }
    m_text.setFont(m_font);
    m_text.setCharacterSize(FONT_SIZE);
    m_text.setFillColor(sf::Color(244, 125, 66));
}

//------------------------------------------------------------------------------
PetriGUI::~PetriGUI()
{
    window().close();
}

//------------------------------------------------------------------------------
void PetriGUI::draw(size_t const tokens, float const x, float const y)
{
    const float r = TN_RADIUS;
    const float d = TN_RADIUS + 1;

    if (tokens == 0u)
        return ;

    if (tokens == 1u)
    {
        m_figure_token.setPosition(sf::Vector2f(x, y));
        window().draw(sf::CircleShape(m_figure_token));
    }
    else if (tokens == 2u)
    {
        m_figure_token.setPosition(sf::Vector2f(x - d, y));
        window().draw(sf::CircleShape(m_figure_token));

        m_figure_token.setPosition(sf::Vector2f(x + d, y));
        window().draw(sf::CircleShape(m_figure_token));
    }
    else if (tokens == 3u)
    {
        m_figure_token.setPosition(sf::Vector2f(x, y - r));
        window().draw(sf::CircleShape(m_figure_token));

        m_figure_token.setPosition(sf::Vector2f(x - d, y + d));
        window().draw(sf::CircleShape(m_figure_token));

        m_figure_token.setPosition(sf::Vector2f(x + d, y + d));
        window().draw(sf::CircleShape(m_figure_token));
    }
    else if (tokens == 4u)
    {
        m_figure_token.setPosition(sf::Vector2f(x - d, y - d));
        window().draw(sf::CircleShape(m_figure_token));

        m_figure_token.setPosition(sf::Vector2f(x + d, y - d));
        window().draw(sf::CircleShape(m_figure_token));

        m_figure_token.setPosition(sf::Vector2f(x - d, y + d));
        window().draw(sf::CircleShape(m_figure_token));

        m_figure_token.setPosition(sf::Vector2f(x + d, y + d));
        window().draw(sf::CircleShape(m_figure_token));
    }
}

//------------------------------------------------------------------------------
void PetriGUI::draw(Place const& place)
{
    m_figure_place.setPosition(sf::Vector2f(place.x, place.y));
    window().draw(sf::CircleShape(m_figure_place));

    draw(place.tokens, place.x, place.y);

    m_text.setString(place.caption);
    m_text.setPosition(sf::Vector2f(place.x - PL_RADIUS / 2,
                                    place.y - 2 * PL_RADIUS - 5));
    window().draw(m_text);
}

//------------------------------------------------------------------------------
void PetriGUI::draw(Transition const& transition)
{
    m_figure_trans.setPosition(sf::Vector2f(transition.x, transition.y));
    window().draw(sf::RectangleShape(m_figure_trans));

    m_text.setString(transition.caption);
    m_text.setPosition(sf::Vector2f(transition.x - 16,
                                    transition.y - TR_HEIGHT - FONT_SIZE));
    window().draw(m_text);
}

//------------------------------------------------------------------------------
void PetriGUI::draw(float const /*dt*/)
{
    window().clear(sf::Color(0u, 0u, 102u, 255u));

    for (auto const& p: m_petri_net.places())
    {
        draw(p);
    }

    for (auto const& t: m_petri_net.transitions())
    {
        draw(t);
    }

    // Swap buffer
    window().display();
}

//------------------------------------------------------------------------------
void PetriGUI::update(float const /*dt*/)
{
}

//------------------------------------------------------------------------------
bool PetriGUI::isRunning()
{
    return m_running;
}

//------------------------------------------------------------------------------
void PetriGUI::handleInput()
{
    sf::Event event;
    sf::Vector2f mouse = sf::Vector2f(sf::Mouse::getPosition(window()));

    while (window().pollEvent(event))
    {
        switch (event.type)
        {
        case sf::Event::Closed:
            m_running = false;
            break;

        case sf::Event::MouseButtonPressed:
            if (event.mouseButton.button == sf::Mouse::Left)
                m_petri_net.addPlace(mouse.x, mouse.y, 3u); // FIXME TEMPORAIRE
            else if (event.mouseButton.button == sf::Mouse::Right)
                m_petri_net.addTransition(mouse.x, mouse.y);
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
