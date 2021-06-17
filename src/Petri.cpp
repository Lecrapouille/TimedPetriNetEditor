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

//------------------------------------------------------------------------------
std::atomic<size_t> Place::s_count{0u};
std::atomic<size_t> Transition::s_count{0u};

//------------------------------------------------------------------------------
const float TR_WIDTH = 50.0f; // Transition
const float TR_HEIGHT = 5.0f; // Transition
const float PL_RADIUS = 25.0f; // Places
const float TN_RADIUS = 4.0f;  // Token in places
const float ARC_SHORT = PL_RADIUS + 2.0f;
const float ARC_TYPE = 2.0f;
const float DOUBLE_SHIFT = 10.0f;
const float FONT_SIZE = 24.0f;
const float ANIMATED_TOKEN_SPEED = 100.0f;
const float TICKS_PER_SECOND = 10.0f;


// *****************************************************************************
//! \brief Allow to draw an arrow needed for drawing Petri arcs.
// *****************************************************************************
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
    // Display the usage
    std::cout
            << "Right click: add a transition" << std::endl
            << "Left click: add a place" << std::endl
            << "Left click and Left Control key : add an arc from a place or transition" << std::endl
            << "Left click and Left Shift key : add an arc from a place or transition" << std::endl
            << "Middle click: remove a place or a transition or an arc" << std::endl
            << "+ key: add a token on the place pointed by the mouse cursor" << std::endl
            << "- key: remove a token on the place pointed by the mouse cursor" << std::endl
            << "R key: m_simulating simulation" << std::endl
            << "E key: end simulation" << std::endl
            << "C key: clear the Petri net" << std::endl;

    // Reserve memory
    m_animation_PT.reserve(128u);
    m_animation_TP.reserve(128u);

    // Precompute SFML struct for drawing places
    m_figure_place.setOrigin(sf::Vector2f(m_figure_place.getRadius(), m_figure_place.getRadius()));
    m_figure_place.setFillColor(sf::Color::White);
    m_figure_place.setOutlineThickness(2.0f);
    m_figure_place.setOutlineColor(sf::Color(244, 125, 66));

    // Precompute SFML struct for drawing tokens inside places
    m_figure_token.setOrigin(sf::Vector2f(m_figure_token.getRadius(), m_figure_token.getRadius()));
    m_figure_token.setFillColor(sf::Color::Black);

    // Precompute SFML struct for drawing transitions
    m_figure_trans.setOrigin(m_figure_trans.getSize().x / 2, m_figure_trans.getSize().y / 2);
    m_figure_trans.setFillColor(sf::Color(100, 100, 66));

    // Precompute SFML struct for drawing text (places and transitions)
    if (!m_font.loadFromFile("OpenSans-Regular.ttf"))
    {
        std::cerr << "Could not load font file ..." << std::endl;
        exit(1);
    }

    // Caption for Places
    m_text_place.setFont(m_font);
    m_text_place.setCharacterSize(FONT_SIZE);
    m_text_place.setFillColor(sf::Color(244, 125, 66));

    // Number of Tokens
    m_text_token.setFont(m_font);
    m_text_token.setCharacterSize(20);
    m_text_token.setFillColor(sf::Color::Black);
    m_text_token.setStyle(sf::Text::Bold);

    // Caption for Transitions
    m_text_trans.setFont(m_font);
    m_text_trans.setCharacterSize(FONT_SIZE);
    m_text_trans.setFillColor(sf::Color(100, 100, 66));

    // Init mouse cursor position
    m_mouse = sf::Vector2f(sf::Mouse::getPosition(window()));
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
    float d = TN_RADIUS + 1;

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
    else if ((tokens == 4u) || (tokens == 5u))
    {
        if (tokens == 5u)
        {
            d = r + 3;
            m_figure_token.setPosition(sf::Vector2f(x, y));
            window().draw(m_figure_token);
        }

        m_figure_token.setPosition(sf::Vector2f(x - d, y - d));
        window().draw(m_figure_token);

        m_figure_token.setPosition(sf::Vector2f(x + d, y - d));
        window().draw(m_figure_token);

        m_figure_token.setPosition(sf::Vector2f(x - d, y + d));
        window().draw(m_figure_token);

        m_figure_token.setPosition(sf::Vector2f(x + d, y + d));
        window().draw(m_figure_token);
    }
    else if (tokens == 6u)
    {
        d = r + 1;

        m_figure_token.setPosition(sf::Vector2f(x - 2 * d, y - d));
        window().draw(m_figure_token);

        m_figure_token.setPosition(sf::Vector2f(x + 0 + 0, y - d));
        window().draw(m_figure_token);

        m_figure_token.setPosition(sf::Vector2f(x + 2 * d, y - d));
        window().draw(m_figure_token);

        m_figure_token.setPosition(sf::Vector2f(x - 2 * d, y + d));
        window().draw(m_figure_token);

        m_figure_token.setPosition(sf::Vector2f(x + 0 + 0, y + d));
        window().draw(m_figure_token);

        m_figure_token.setPosition(sf::Vector2f(x + 2 * d, y + d));
        window().draw(m_figure_token);
    }
    else if (tokens == 7u)
    {
        d = r + 1;

        m_figure_token.setPosition(sf::Vector2f(x - 2 * d, y - d));
        window().draw(m_figure_token);

        m_figure_token.setPosition(sf::Vector2f(x + 0 + 0, y - 2 * d));
        window().draw(m_figure_token);

        m_figure_token.setPosition(sf::Vector2f(x + 2 * d, y - d));
        window().draw(m_figure_token);

        m_figure_token.setPosition(sf::Vector2f(x - 2 * d, y + d));
        window().draw(m_figure_token);

        m_figure_token.setPosition(sf::Vector2f(x + 0 + 0, y));
        window().draw(m_figure_token);

        m_figure_token.setPosition(sf::Vector2f(x + 2 * d, y + d));
        window().draw(m_figure_token);

        m_figure_token.setPosition(sf::Vector2f(x, y + 2 * d));
        window().draw(m_figure_token);
    }
    else if (tokens >= 8u)
    {
        m_text_token.setString(std::to_string(tokens));
        if (tokens < 10u)
            m_text_token.setPosition(sf::Vector2f(x - 6, y - 12));
        else if (tokens < 100u)
            m_text_token.setPosition(sf::Vector2f(x - 12, y - 12));
        else
            m_text_token.setPosition(sf::Vector2f(x - 18, y - 12));
        window().draw(m_text_token);
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
    m_text_place.setString(place.caption);
    m_text_place.setPosition(sf::Vector2f(place.x - PL_RADIUS / 2,
                                          place.y - 2 * PL_RADIUS - 5));
    window().draw(m_text_place);
}

//------------------------------------------------------------------------------
void PetriGUI::draw(Transition const& transition)
{
    // Draw the transition
    m_figure_trans.setPosition(sf::Vector2f(transition.x, transition.y));
    window().draw(m_figure_trans);

    // Draw the caption
    m_text_trans.setString(transition.caption);
    m_text_trans.setPosition(sf::Vector2f(transition.x - 16,
                                          transition.y - (TR_WIDTH / 2) - FONT_SIZE - 5));
    window().draw(m_text_trans);
}

//------------------------------------------------------------------------------
void PetriGUI::draw(Arc const& arc)
{
    if (arc.from.type == Node::Type::Place)
    {
        // Place -> Transition
        Arrow arrow(arc.from.x, arc.from.y, arc.to.x, arc.to.y);
        window().draw(arrow);
    }
    else
    {
        // Transition -> Place
        Arrow arrow(arc.from.x, arc.from.y, arc.to.x, arc.to.y);
        window().draw(arrow);
    }
}

//------------------------------------------------------------------------------
void PetriGUI::draw(float const /*dt*/)
{
    window().clear(sf::Color(0u, 0u, 102u, 255u));

    // Draw all Places
    for (auto const& p: m_petri_net.places())
    {
        draw(p);
    }

    // Draw all Transitions
    for (auto const& t: m_petri_net.transitions())
    {
        draw(t);
    }

    // Draw all Arcs
    for (auto const& a: m_petri_net.arcs())
    {
        draw(a);
    }

    // Draw the arc the user is creating
    if (m_node_from != nullptr)
    {
        Arrow arrow(m_node_from->x, m_node_from->y, m_mouse.x, m_mouse.y);
        window().draw(arrow);
    }

    if (m_animation_PT.size() > 0u)
    {
        // Draw all tokens transiting from Places to Transitions
        for (auto const& at: m_animation_PT)
        {
            m_figure_token.setPosition(sf::Vector2f(at.x, at.y));
            window().draw(m_figure_token);
        }
    }
    else
    {
        // Draw all tokens transiting from Transitions to Places
        for (auto const& at: m_animation_TP)
        {
            m_figure_token.setPosition(sf::Vector2f(at.x, at.y));
            window().draw(m_figure_token);
        }
    }

    // Swap buffer
    window().display();
}

//------------------------------------------------------------------------------
void PetriNet::cacheArcs()
{
    for (auto& trans: m_transitions)
    {
        trans.arcsIn.clear();
        trans.arcsOut.clear();

        for (auto& a: m_arcs)
        {
            if ((a.from.type == Node::Type::Place) && (a.to.id == trans.id))
                trans.arcsIn.push_back(&a);
            else if ((a.to.type == Node::Type::Place) && (a.from.id == trans.id))
                trans.arcsOut.push_back(&a);
        }
    }

    for (auto& trans: m_transitions)
    {
        std::cout << "Transition " << trans.id << " degIn: ";
        for (auto& a: trans.arcsIn)
            std::cout << "(" << a->from.key() << ", " << a->to.key() << ") ";
        std::cout << std::endl;

        std::cout << "Transition " << trans.id << " degOut: ";
        for (auto& a: trans.arcsOut)
            std::cout << "(" << a->from.key() << ", " << a->to.key() << ") ";
        std::cout << std::endl;
    }
}

//------------------------------------------------------------------------------
static size_t& tokenIn(Arc* a)
{
    return reinterpret_cast<Place*>(&(a->from))->tokens;
}

static size_t& tokenOut(Arc* a)
{
    return reinterpret_cast<Place*>(&(a->to))->tokens;
}

//------------------------------------------------------------------------------
static bool canFire(Transition const& trans)
{
    for (auto& a: trans.arcsIn)
    {
        if (tokenIn(a) == 0u)
            return false;
    }
    return true;
}

//------------------------------------------------------------------------------
AnimatedToken::AnimatedToken(Arc& arc, bool PT)
    : x(arc.from.x), y(arc.from.y), currentArc(&arc)
{
    id = PT ? arc.from.id : arc.to.id;
    magnitude = sqrtf(x * x + y * y);
}

//------------------------------------------------------------------------------
bool AnimatedToken::update(float const dt)
{
    offset += dt * ANIMATED_TOKEN_SPEED / magnitude;
    x = currentArc->from.x + (currentArc->to.x - currentArc->from.x) * offset;
    y = currentArc->from.y + (currentArc->to.y - currentArc->from.y) * offset;

    return (offset >= 1.0);
}

//------------------------------------------------------------------------------
void PetriGUI::update(float const dt) // FIXME std::chrono
{
    if (!m_simulating)
        return ;

    // Tokens have done their animation ? If yes then fire transitions.
    if ((m_animation_PT.size() == 0u) && (m_animation_TP.size() == 0u))
    {
        // For each transition check if all Places pointing to it has at least
        // one token.
        for (auto& trans: m_petri_net.transitions())
        {
            // All Places pointing to this Transition have at least one token.
            if (canFire(trans))
            {
                for (auto& a: trans.arcsIn)
                {
                    // Burn a token
                    --tokenIn(a);

                    // Add an animated tokens Places --> Transition.
                    m_animation_PT.push_back(AnimatedToken(*a, true));
                }

                // Add an animated tokens Transition --> Places.
                // note: the number of tokens will be incremented in destination
                // places when the animation will be done.
                for (auto& a: trans.arcsOut)
                {
                    m_animation_TP.push_back(AnimatedToken(*a, false));
                }
            }
        }

        // No more firing ? End of the simulation.
        if (m_animation_PT.size() == 0u)
        {
            m_simulating = false;
        }
    }
    else
    {
        // Tokens Places --> Transition are transitioning.
        if (m_animation_PT.size() > 0u)
        {
            size_t s = m_animation_PT.size();
            size_t i = s;
            while (i--)
            {
                // Reach the transition ?
                if (m_animation_PT[i].update(dt))
                {
                    // Remove it from the list
                    std::swap(m_animation_PT[i], m_animation_PT[s - 1u]);
                    m_animation_PT.pop_back();
                }
            }
        }

        // Tokens Transition --> Places are transitioning.
        else // m_animation_TP.size() > 0u
        {
            size_t s = m_animation_TP.size();
            size_t i = s;
            while (i--)
            {
                if (m_animation_TP[i].update(dt))
                {
                    ++tokenOut(m_animation_TP[i].currentArc);
                    std::swap(m_animation_TP[i], m_animation_TP[s - 1u]);
                    m_animation_TP.pop_back();
                }
            }
        }
    }
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
        if ((x >= t.x - 15.0) && (x <= t.x + TR_HEIGHT + 15.0) &&
            (y >= t.y - 15.0) && (y <= t.y + TR_WIDTH + 15.0))
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
    static bool ctrl = false;

    while (window().pollEvent(event))
    {
        switch (event.type)
        {
        case sf::Event::Closed:
            m_running = false;
            return;

        case sf::Event::KeyPressed:
            if (event.key.code == sf::Keyboard::Escape)
            {
                m_running = false;
                return ;
            }
            else if (event.key.code == sf::Keyboard::LControl)
            {
                ctrl = true;
            }
            else if (event.key.code == sf::Keyboard::C)
            {
                m_petri_net.reset();
            }
            else if (event.key.code == sf::Keyboard::R)
            {
                std::cout << "Simulation running" << std::endl;
                m_petri_net.cacheArcs();
                // TODO m_petri_net.saveTokens();
                m_simulating = true;
            }
            else if (event.key.code == sf::Keyboard::E)
            {
                std::cout << "Simulation stopped" << std::endl;
                m_simulating = false;
                // TODO m_petri_net.restoreTokens();
            }
            else if ((event.key.code == sf::Keyboard::Add) ||
                     (event.key.code == sf::Keyboard::Subtract))
            {
                Node* node = getNode(m_mouse.x, m_mouse.y);
                if ((node != nullptr) && (node->type == Node::Type::Place))
                {
                    size_t& tokens = reinterpret_cast<Place*>(node)->tokens;
                    if (event.key.code == sf::Keyboard::Add)
                    {
                        ++tokens;
                    }
                    else if (tokens > 0u)
                    {
                        --tokens;
                    }
                }
            }
            break;

        case sf::Event::KeyReleased:
            if (event.key.code == sf::Keyboard::LControl)
            {
                ctrl = false;
            }
            break;

        case sf::Event::MouseButtonPressed:
            m_node_from = m_node_to = nullptr;
            // Left button: Add place
            // Left button + key ctrl: remove place or transition
            if (event.mouseButton.button == sf::Mouse::Left)
            {
                Node* node = getNode(m_mouse.x, m_mouse.y);
                if (ctrl)
                {
                    if (node != nullptr)
                    {
                        // TODO removeNode(*node);
                    }
                }
                else
                {
                    if (node == nullptr)
                    {
                        m_petri_net.addPlace(m_mouse.x, m_mouse.y, 0u);
                    }
                }
            }
            // Right button: Add transition
            else if (event.mouseButton.button == sf::Mouse::Right)
            {
                Node* node = getNode(m_mouse.x, m_mouse.y);
                if (node == nullptr)
                {
                    m_petri_net.addTransition(m_mouse.x, m_mouse.y);
                }
            }
            // Middle button: Add arc
            else if (event.mouseButton.button == sf::Mouse::Middle)
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
