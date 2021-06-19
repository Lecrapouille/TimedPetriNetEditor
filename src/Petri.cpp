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
#include <fstream>
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
const float TOKEN_ANIMATION_DURATION = 2.0f;

//------------------------------------------------------------------------------
static float norm(const float xa, const float ya, const float xb, const float yb)
{
    return sqrtf((xb - xa) * (xb - xa) + (yb - ya) * (yb - ya));
}

// *****************************************************************************
//! \brief Allow to draw an arrow needed for drawing Petri arcs.
// *****************************************************************************
class Arrow : public sf::Drawable
{
public:

    Arrow(const float xa, const float ya, const float xb, const float yb)
    {
        // Arc magnitude
        const float arrowLength = norm(xa, ya, xb, yb);

        // Orientation
        const float teta = (yb - ya) / (xb - xa);
        float arrowAngle = std::atan(teta) * 180.0f / 3.1415f; // rad -> deg
        if (xb < xa)
            arrowAngle += 180.f;
        else if (yb < ya)
            arrowAngle += 360.f;

        // Reduce the arrow magnitude to avoid entering in the place and having
        // a mush of pixels when multiple arrows are pointing on the same
        // position. To get full scaled arrow comment this block of code and
        // uncomment xa, xb, ya, yb and tailSize.
        float r = arrowLength - PL_RADIUS;
        float dx = ((xb - xa) * r) / arrowLength;
        float dy = ((yb - ya) * r) / arrowLength;
        float a1 = xb - dx;
        float b1 = yb - dy;
        float a2 = xa + dx;
        float b2 = ya + dy;

        // Head of the arrow
        const sf::Vector2f arrowHeadSize{ 14.f, 14.f };
        m_arrowHead = sf::ConvexShape{ 3 };
        m_arrowHead.setPoint(0, { 0.f, 0.f });
        m_arrowHead.setPoint(1, { arrowHeadSize.x, arrowHeadSize.y / 2.f });
        m_arrowHead.setPoint(2, { 0.f, arrowHeadSize.y });
        m_arrowHead.setOrigin(arrowHeadSize.x, arrowHeadSize.y / 2.f);
        m_arrowHead.setPosition(sf::Vector2f(a2, b2 /*xb, yb*/));
        m_arrowHead.setRotation(arrowAngle);
        m_arrowHead.setFillColor(sf::Color(244, 125, 66));

        // Tail of the arrow.
        //const sf::Vector2f tailSize{ arrowLength - arrowHeadSize.x, 2.f };
        const sf::Vector2f tailSize{ r - arrowHeadSize.x - 15, 2.f };
        m_tail = sf::RectangleShape{ tailSize };
        m_tail.setOrigin(0.f, tailSize.y / 2.f);
        m_tail.setPosition(sf::Vector2f(a1, b1 /*xa, ya*/));
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
static void usage()
{
    std::cout
            << "Left mouse button pressed: add a place" << std::endl
            << "Right mouse button pressed: add a transition" << std::endl
            << "Middle mouse button pressed: add an arc with the selected place or transition as origin" << std::endl
            << "Middle mouse button release: end the arc with the selected place or transition as destination" << std::endl
            << "L key: add an arc with the selected place or transition as origin" << std::endl
            << "M key: move the selected place or transition" << std::endl
            << "+ key: add a token on the place pointed by the mouse cursor" << std::endl
            << "- key: remove a token on the place pointed by the mouse cursor" << std::endl
            << "R key: m_simulating simulation" << std::endl
            << "E key: end simulation" << std::endl
            << "S key: save the Petri net as petri.json file" << std::endl
            << "O key: load the Petri net from petri.json file" << std::endl
            << "Delete key: remove a place or transition or an arc" << std::endl
            << "C key: clear the Petri net" << std::endl;
}

//------------------------------------------------------------------------------
PetriGUI::PetriGUI(Application &application)
    : GUI("Petri Net Editor", application),
      m_figure_place(PL_RADIUS),
      m_figure_token(TN_RADIUS),
      m_figure_trans(sf::Vector2f(TR_HEIGHT, TR_WIDTH))
{
    usage();

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
void PetriGUI::displayNumber(size_t const tokens, float const x, float const y)
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
        displayNumber(tokens, x, y);
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

    // Update the node the user is moving
    if (m_moving_node != nullptr)
    {
        m_moving_node->x = m_mouse.x;
        m_moving_node->y = m_mouse.y;
    }

    if (m_animation_PT.size() > 0u)
    {
        // Draw all tokens transiting from Places to Transitions
        for (auto const& at: m_animation_PT)
        {
            m_figure_token.setPosition(sf::Vector2f(at.x, at.y));
            window().draw(m_figure_token);
            displayNumber(at.tokens, at.x, at.y - 16);
        }
    }
    else
    {
        // Draw all tokens transiting from Transitions to Places
        for (auto const& at: m_animation_TP)
        {
            m_figure_token.setPosition(sf::Vector2f(at.x, at.y));
            window().draw(m_figure_token);
            displayNumber(at.tokens, at.x, at.y - 16);
        }
    }
}

//------------------------------------------------------------------------------
bool PetriNet::save(std::string const& filename)
{
    std::string separator;
    std::ofstream file(filename);

    file << "{\n  \"places\": [";
    for (auto const& p: m_places)
    {
        file << separator << '\"' << p.key() << ','
             << p.x << ',' << p.y << ',' << p.tokens << '\"';
        separator = ", ";
    }
    file << "],\n  \"trans\": [";
    separator = "";
    for (auto const& t: m_transitions)
    {
        file << separator << '\"' << t.key() << ','
             << t.x << ',' << t.y << '\"';
        separator = ", ";
    }
    file << "],\n  \"arcs\": [";
    separator = "";
    for (auto const& a: m_arcs)
    {
        file << separator << '\"' << a.from.key() << ','
             << a.to.key() << '\"';
        separator = ", ";
    }
    file << "]\n}";

    std::cout << "Petri net saved into file '" << filename << "'" << std::endl;
    return true;
}

//------------------------------------------------------------------------------
bool PetriNet::load(std::string const& filename)
{
    return false; // TODO
}

void PetriNet::removeNode(Node& node)
{
    // Remove all arcs linked to this node
    size_t s = m_arcs.size();
    size_t i = s;
    while (i--)
    {
        if ((m_arcs[i].to.id == node.id) || (m_arcs[i].from.id == node.id))
        {
            m_arcs[i] = m_arcs[s - 1u];
            m_arcs.pop_back();
        }
    }

    // Search and remove the node
    if (node.type == Node::Type::Place)
    {
        size_t s = m_places.size();
        size_t i = s;
        while (i--)
        {
            if (m_places[i].id == node.id)
            {
                m_places[i] = m_places[s - 1u];
                m_places.pop_back();
            }
        }
    }
    else
    {
        size_t s = m_transitions.size();
        size_t i = s;
        while (i--)
        {
            if (m_transitions[i].id == node.id)
            {
                m_transitions[i] = m_transitions[s - 1u];
                m_transitions.pop_back();
            }
        }
    }

    // Restore arcs
    // cacheArcs();
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
static size_t canFire(Transition const& trans)
{
    size_t burnt = static_cast<size_t>(-1);

    for (auto& a: trans.arcsIn)
    {
        size_t tokens = tokenIn(a);
        if (tokens == 0u)
            return 0u;
        if (tokens < burnt)
            burnt = tokens;
    }
    return burnt;
}

//------------------------------------------------------------------------------
AnimatedToken::AnimatedToken(Arc& arc, size_t tok, bool PT)
    : x(arc.from.x), y(arc.from.y), tokens(tok), currentArc(&arc),
      magnitude(norm(arc.from.x, arc.from.y, arc.to.x, arc.to.y))
{
    id = PT ? arc.from.id : arc.to.id;
}

//------------------------------------------------------------------------------
bool AnimatedToken::update(float const dt)
{
    float speed = magnitude / TOKEN_ANIMATION_DURATION;
    offset += dt * speed / magnitude;
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
            size_t tokens = canFire(trans);
            if (tokens > 0u)
            {
                for (auto& a: trans.arcsIn)
                {
                    // Burn tokens
                    tokenIn(a) -= tokens;

                    // Add an animated tokens Places --> Transition.
                    m_animation_PT.push_back(AnimatedToken(*a, tokens, true));
                }

                // Add an animated tokens Transition --> Places.
                // note: the number of tokens will be incremented in destination
                // places when the animation will be done.
                for (auto& a: trans.arcsOut)
                {
                    m_animation_TP.push_back(AnimatedToken(*a, tokens, false));
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
                    m_animation_PT[i] = m_animation_PT[s - 1u];
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
                    tokenOut(m_animation_TP[i].currentArc) += m_animation_TP[i].tokens;
                    m_animation_TP[i] = m_animation_TP[s - 1u];
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

    while (window().pollEvent(event))
    {
        switch (event.type)
        {
        // Window close button clicked
        case sf::Event::Closed:
            m_running = false;
            return;

        case sf::Event::KeyPressed:
            // Escape key: qui the application.
            if (event.key.code == sf::Keyboard::Escape)
            {
                m_running = false;
                return ;
            }
            // Left or right Control key pressed: memorize the state
            else if ((event.key.code == sf::Keyboard::LControl) ||
                     (event.key.code == sf::Keyboard::RControl))
            {
                m_ctrl = true;
            }
            // 'S' key: save the Petri net to a JSON file
            else if (event.key.code == sf::Keyboard::S)
            {
                m_petri_net.save("petri.json");
            }
            // 'O' key: load the Petri net to a JSON file
            else if (event.key.code == sf::Keyboard::O)
            {
                m_petri_net.load("petri.json");
            }
            // 'C' key: erase the Petri net
            else if (event.key.code == sf::Keyboard::C)
            {
                m_petri_net.reset();
            }
            // 'M' key: Move the selected node
            else if (event.key.code == sf::Keyboard::M)
            {
                m_moving_node =
                        (m_moving_node != nullptr) ?
                        nullptr : getNode(m_mouse.x, m_mouse.y);
            }
            // 'L' key: create an arc from the selected node
            else if (event.key.code == sf::Keyboard::L)
            {
                m_node_from = getNode(m_mouse.x, m_mouse.y);
            }
            //
            if (event.key.code == sf::Keyboard::Delete)
            {
                Node* node = getNode(m_mouse.x, m_mouse.y);
                if (node != nullptr)
                {
                    m_petri_net.removeNode(*node);
                }
            }
            // 'R' key: Run the animation of the Petri net
            else if (event.key.code == sf::Keyboard::R)
            {
                if (!m_simulating)
                {
                    std::cout << "Simulation running" << std::endl;
                    m_petri_net.cacheArcs();
                    // TODO m_petri_net.saveTokens();
                    m_simulating = true;
                }
            }
            // 'E' key: End the animation of the Petri net
            else if (event.key.code == sf::Keyboard::E)
            {
                if (m_simulating)
                {
                    std::cout << "Simulation stopped" << std::endl;
                    m_simulating = false;
                    // TODO m_petri_net.restoreTokens();
                }
            }
            // '+' or '-' key: increase or decrease the number of tokens in the
            // place.
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
            // Left or right Control Key released: unmemorize the state
            if ((event.key.code == sf::Keyboard::LControl) ||
                (event.key.code == sf::Keyboard::RControl))
            {
                m_ctrl = false;
            }
            break;

        case sf::Event::MouseButtonPressed:
            // Left button: Add a Place node
            // Right button: Add a Transition node
            if ((event.mouseButton.button == sf::Mouse::Left) ||
                (event.mouseButton.button == sf::Mouse::Right))
            {
                m_node_to = getNode(m_mouse.x, m_mouse.y);
                if (m_node_from == nullptr)
                {
                    if (m_node_to != nullptr)
                    {
                        // Forbid to add a node inside another node
                    }
                    else if (event.mouseButton.button == sf::Mouse::Left)
                    {
                        m_petri_net.addPlace(m_mouse.x, m_mouse.y);
                    }
                    else if (event.mouseButton.button == sf::Mouse::Right)
                    {
                        m_petri_net.addTransition(m_mouse.x, m_mouse.y);
                    }
                }
                else if (m_node_to != nullptr)
                {
                    m_petri_net.addArc(*m_node_from, *m_node_to);
                }
                m_node_from = m_node_to = m_moving_node = nullptr;
            }
            // Middle button: Add arc
            else if (event.mouseButton.button == sf::Mouse::Middle)
            {
                m_node_from =
                        (m_node_from != nullptr) ?
                        nullptr : getNode(m_mouse.x, m_mouse.y);
                m_node_to = m_moving_node = nullptr;
            }
            break;

        case sf::Event::MouseButtonReleased:
            if (m_node_from != nullptr)
            {
                // Finish the creation of the arc (destination node)
                m_node_to = getNode(m_mouse.x, m_mouse.y);
                if (m_node_to == nullptr)
                {
                    // No destination node selected ? Create one !
                    if (m_node_from->type == Node::Type::Place)
                        m_node_to = &m_petri_net.addTransition(m_mouse.x, m_mouse.y);
                    else
                        m_node_to = &m_petri_net.addPlace(m_mouse.x, m_mouse.y);
                }
                m_petri_net.addArc(*m_node_from, *m_node_to);
            }
            m_node_from = m_node_to = nullptr;
            break;

        default:
            break;
        }
    }
}
