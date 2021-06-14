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
#include <math.h>

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
class Arrow : public sf::Drawable
{
public:

    Arrow(const float xa, const float ya, const float xb, const float yb)
    {
        const float arrowLength = sqrtf((xb - xa) * (xb - xa) +
                                        (yb - ya) * (yb - ya));

        const float teta = (yb - ya) / (xb - xa);
        float arrowAngle = std::atan(teta) * 180.0f / 3.1415f; // rad -> deg
        if (xb < xa)
            arrowAngle += 180.f;
        else if (yb < ya)
            arrowAngle += 360.f;

        const sf::Vector2f arrowHeadSize{ 14.f, 14.f };
        m_arrowHead = sf::ConvexShape{ 3 };
        m_arrowHead.setPoint(0, { 0.f, 0.f });
        m_arrowHead.setPoint(1, { arrowHeadSize.x, arrowHeadSize.y / 2.f });
        m_arrowHead.setPoint(2, { 0.f, arrowHeadSize.y });
        m_arrowHead.setOrigin(arrowHeadSize.x, arrowHeadSize.y / 2.f);
        m_arrowHead.setPosition(sf::Vector2f(xb, yb));
        m_arrowHead.setRotation(arrowAngle);
        m_arrowHead.setFillColor(sf::Color(244, 125, 66));

        const sf::Vector2f tailSize{ arrowLength - arrowHeadSize.x, 2.f };
        m_tail = sf::RectangleShape{ tailSize };
        m_tail.setOrigin(0.f, tailSize.y / 2.f);
        m_tail.setPosition(sf::Vector2f(xa, ya));
        m_tail.setRotation(arrowAngle);
        m_tail.setFillColor(sf::Color(244, 125, 66));
    }

    Arrow(sf::Vector2f const& startPoint, sf::Vector2f const& endPoint)
        : Arrow(startPoint.x, startPoint.y, endPoint.x, endPoint.y)
    {}

private:

    void draw(sf::RenderTarget& target, sf::RenderStates /*states*/) const override final
    {
        target.draw(m_tail);
        target.draw(m_arrowHead);
    }

private:

    sf::RectangleShape m_tail;
    sf::ConvexShape m_arrowHead;
};

//------------------------------------------------------------------------------
PetriGUI::PetriGUI(Application &application)
    : GUI("Petri Net Editor", application),
      m_figure_place(PL_RADIUS),
      m_figure_token(TN_RADIUS),
      m_figure_trans(sf::Vector2f(TR_HEIGHT, TR_WIDTH))
{
    m_mouse = sf::Vector2f(sf::Mouse::getPosition(window()));

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
        window().draw(m_figure_token);
    }
    else if (tokens == 2u)
    {
        m_figure_token.setPosition(sf::Vector2f(x - d, y));
        window().draw(m_figure_token);

        m_figure_token.setPosition(sf::Vector2f(x + d, y));
        window().draw(m_figure_token);
    }
    else if (tokens == 3u)
    {
        m_figure_token.setPosition(sf::Vector2f(x, y - r));
        window().draw(m_figure_token);

        m_figure_token.setPosition(sf::Vector2f(x - d, y + d));
        window().draw(m_figure_token);

        m_figure_token.setPosition(sf::Vector2f(x + d, y + d));
        window().draw(m_figure_token);
    }
    else if (tokens == 4u)
    {
        m_figure_token.setPosition(sf::Vector2f(x - d, y - d));
        window().draw(m_figure_token);

        m_figure_token.setPosition(sf::Vector2f(x + d, y - d));
        window().draw(m_figure_token);

        m_figure_token.setPosition(sf::Vector2f(x - d, y + d));
        window().draw(m_figure_token);

        m_figure_token.setPosition(sf::Vector2f(x + d, y + d));
        window().draw(m_figure_token);
    }
}

//------------------------------------------------------------------------------
void PetriGUI::draw(Place const& place)
{
    // Draw the place
    m_figure_place.setPosition(sf::Vector2f(place.x, place.y));
    window().draw(m_figure_place);

    // Draw tokens
    draw(place.tokens, place.x, place.y);

    // Draw the caption
    m_text.setString(place.caption);
    m_text.setPosition(sf::Vector2f(place.x - PL_RADIUS / 2,
                                    place.y - 2 * PL_RADIUS - 5));
    window().draw(m_text);
}

//------------------------------------------------------------------------------
void PetriGUI::draw(Transition const& transition)
{
    // Draw the transition
    m_figure_trans.setPosition(sf::Vector2f(transition.x, transition.y));
    window().draw(m_figure_trans);

    // Draw the caption
    m_text.setString(transition.caption);
    m_text.setPosition(sf::Vector2f(transition.x - 16,
                                    transition.y - TR_HEIGHT - FONT_SIZE));
    window().draw(m_text);
}

//------------------------------------------------------------------------------
void PetriGUI::draw(Arc const& arc)
{
    if (arc.from.type == Node::Type::Place)
    {
        // Place -> Transition
        Arrow arrow(arc.from.x, arc.from.y, arc.to.x, arc.to.y + TR_WIDTH / 2.0);
        window().draw(arrow);
    }
    else
    {
        // Transition -> Place
        Arrow arrow(arc.from.x, arc.from.y + TR_WIDTH / 2.0, arc.to.x, arc.to.y);
        window().draw(arrow);
    }
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

    for (auto const& a: m_petri_net.arcs())
    {
        draw(a);
    }

    if (m_node_from != nullptr)
    {
        Arrow arrow(m_node_from->x, m_node_from->y, m_mouse.x, m_mouse.y);
        window().draw(arrow);
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
Node* PetriGUI::getNode(float const x, float const y)
{
    for (auto& p: m_petri_net.places())
    {
        float d2 = (p.x - x) * (p.x - x) + (p.y - y) * (p.y - y);
        if (d2 < PL_RADIUS * PL_RADIUS)
        {
            return &p;
        }
    }

    for (auto& t: m_petri_net.transitions())
    {
        if ((x >= t.x) && (x <= t.x + TR_HEIGHT) &&
            (y >= t.y) && (y <= t.y + TR_WIDTH))
        {
            return &t;
        }
    }

    return nullptr;
}

//------------------------------------------------------------------------------
void PetriGUI::handleInput()
{
    sf::Event event;
    m_mouse = sf::Vector2f(sf::Mouse::getPosition(window()));

    while (window().pollEvent(event))
    {
        switch (event.type)
        {
        case sf::Event::Closed:
            m_running = false;
            return;

        case sf::Event::KeyPressed:
            m_running = (event.key.code != sf::Keyboard::Escape);
            if (!m_running) return ;
            break;

        case sf::Event::MouseButtonPressed:
            m_node_from = m_node_to = nullptr;
            if (event.mouseButton.button == sf::Mouse::Left)
            {
                m_petri_net.addPlace(m_mouse.x, m_mouse.y, 0u);
            }
            else if (event.mouseButton.button == sf::Mouse::Right)
            {
                m_petri_net.addTransition(m_mouse.x, m_mouse.y);
            }
            else
            {
                m_node_from = getNode(m_mouse.x, m_mouse.y);
            }
            break;

        case sf::Event::MouseButtonReleased:
            m_node_to = getNode(m_mouse.x, m_mouse.y);
            if ((m_node_from != nullptr) && (m_node_to != nullptr))
            {
                m_petri_net.addArc(*m_node_from, *m_node_to);
            }
            m_node_from = m_node_to = nullptr;
            break;

        default:
            break;
        }
    }
}
