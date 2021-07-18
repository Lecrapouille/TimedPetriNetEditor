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
#include "Howard.h"
#include <iomanip>
#include <iostream>
#include <fstream>
#include <math.h>
#include <cmath>
#include <sstream>
#include <cstring>

//------------------------------------------------------------------------------
std::atomic<size_t> Place::s_next_id{0u};
std::atomic<size_t> Transition::s_next_id{0u};

//------------------------------------------------------------------------------
// Config for rendering the Petri net
const float TRANS_WIDTH = 50.0f;  // Rectangle width for rendering Transitions
const float TRANS_HEIGHT = 10.0f;  // Rectangle height for rendering Transitions
const float PLACE_RADIUS = 25.0f; // Circle radius for rendering Places
const float TOKEN_RADIUS = 4.0f;  // Circle radius for rendering tokens
const float CAPTION_FONT_SIZE = 24.0f; // Text size used in node captions
const float TOKEN_FONT_SIZE = 20.0f; // Text size used for token numbers
const int STEP_ANGLE = 45; // Angle of rotation in degree for turning Transitions
const float BLINK_PERIOD = 0.5f; // seconds for fading colors
#define FILL_COLOR(a) sf::Color(255, 165, 0, a) // Place with tokens or fading color
#define OUTLINE_COLOR sf::Color(165, 42, 42) // Arcs, Places, Transitions

//------------------------------------------------------------------------------
//! \brief Helper structure for building sparse matrix for the exportation of
//! the Petri net to Julia language (Julia is a vectorial language mixing Matlab
//! and Python syntax but with a faster runtime) as Max-Plus dynamical linear
//! systems (State space representation).
//!
//! This class is only used for storing elements not for doing matrix
//! operations. In Julia, a sparse matrix of dimensions m x n is built with the
//! function sparse(I, J, D, n, m) where I, J are two column vectors indicating
//! coordinates of the non-zero elements, D is column vector holding values to
//! be stored. Note that in Julia indexes starts at 1, contrary to C/C++
//! starting at 0.
//------------------------------------------------------------------------------
struct SparseElement
{
    SparseElement(size_t i_, size_t j_, float d_)
        : i(i_ + 1u), j(j_ + 1u), d(d_)
    {}

    // (I,J) Coordinate
    size_t i, j;
    // Non zero element
    float d;
};

//! \brief Julia sparse matrix.
using SparseMatrix = std::vector<SparseElement>;

//! \brief Julia sparse is built as sparse(I, J, D) where I, J and D are 3
//! vectors.
std::ostream & operator<<(std::ostream &os, SparseMatrix const& matrix)
{
    std::string separator;

    os << "[";
    for (auto const& e: matrix)
    {
        os << separator << e.i;
        separator = ", ";
    }

    os << "], [";
    separator.clear();
    for (auto const& e: matrix)
    {
        os << separator << e.j;
        separator = ", ";
    }

    os << "], MP([";
    separator.clear();
    for (auto const& e: matrix)
    {
        os << separator << e.d;
        separator = ", ";
    }
    os << "])";

    return os;
}

//------------------------------------------------------------------------------
static float norm(const float xa, const float ya, const float xb, const float yb)
{
    return sqrtf((xb - xa) * (xb - xa) + (yb - ya) * (yb - ya));
}

//------------------------------------------------------------------------------
static float random(int lower, int upper)
{
    srand(time(NULL));
    return (rand() % (upper - lower + 1)) + lower;
}

//------------------------------------------------------------------------------
static const char* current_time()
{
    static char buffer[32];

    time_t current_time = ::time(nullptr);
    strftime(buffer, sizeof (buffer), "[%H:%M:%S] ", localtime(&current_time));
    return buffer;
}

//------------------------------------------------------------------------------
static bool isInput(Transition& t)
{
    return (t.arcsIn.size() == 0u) && (t.arcsOut.size() > 0u);
}

//------------------------------------------------------------------------------
static bool isOutput(Transition& t)
{
    return (t.arcsIn.size() > 0u) && (t.arcsOut.size() == 0u);
}

//------------------------------------------------------------------------------
static bool isState(Transition& t)
{
    return (t.arcsIn.size() > 0u) && (t.arcsOut.size() > 0u);
}

//------------------------------------------------------------------------------
static size_t& tokensIn(Arc& a)
{
    return reinterpret_cast<Place*>(&(a.from))->tokens;
}

//------------------------------------------------------------------------------
static size_t& tokensOut(Arc& a)
{
    return reinterpret_cast<Place*>(&(a.to))->tokens;
}

//--------------------------------------------------------------------------
static bool isEmpty(PetriNet const& petri)
{
    return (petri.places().size() == 0u) && (petri.transitions().size() == 0u);
}

//------------------------------------------------------------------------------
static uint8_t fading(sf::Clock& timer, bool restart)
{
    if (restart)
        timer.restart();

    float period = timer.getElapsedTime().asSeconds();
    if (period >= BLINK_PERIOD)
        period = BLINK_PERIOD;

    return uint8_t(255.0f - (255.0f * period / BLINK_PERIOD));
}

// *****************************************************************************
//! \brief Class allowing to draw an arrow. Arrows are needed for drawing Petri
//! arcs.
// *****************************************************************************
class Arrow : public sf::Drawable
{
public:

    Arrow(const float xa, const float ya, const float xb, const float yb,
          const uint8_t alpha)
    {
        // Arc magnitude
        const float arrowLength = norm(xa, ya, xb, yb);

        // Orientation
        const float teta = (yb - ya) / (xb - xa);
        float arrowAngle = std::atanf(teta) * 180.0f / 3.1415f; // rad -> deg
        if (xb < xa)
            arrowAngle += 180.f;
        else if (yb < ya)
            arrowAngle += 360.f;

        // Reduce the arrow magnitude to avoid entering in the place and having
        // a mush of pixels when multiple arrows are pointing on the same
        // position. To get full scaled arrow comment this block of code and
        // uncomment xa, xb, ya, yb and tailSize.
        float r = arrowLength - PLACE_RADIUS;
        float dx = ((xb - xa) * r) / arrowLength;
        float dy = ((yb - ya) * r) / arrowLength;
        float a1 = xb - dx;
        float b1 = yb - dy;
        float a2 = xa + dx;
        float b2 = ya + dy;

        // Head of the arrow
        const sf::Vector2f arrowHeadSize{ 14.f, 14.f };
        m_head = sf::ConvexShape{ 3 };
        m_head.setPoint(0, { 0.f, 0.f });
        m_head.setPoint(1, { arrowHeadSize.x, arrowHeadSize.y / 2.f });
        m_head.setPoint(2, { 0.f, arrowHeadSize.y });
        m_head.setOrigin(arrowHeadSize.x, arrowHeadSize.y / 2.f);
        m_head.setPosition(sf::Vector2f(a2, b2 /*xb, yb*/));
        m_head.setRotation(arrowAngle);

        // Tail of the arrow.
        //const sf::Vector2f tailSize{ arrowLength - arrowHeadSize.x, 2.f };
        const sf::Vector2f tailSize{ r - arrowHeadSize.x - 15, 2.f };
        m_tail = sf::RectangleShape{ tailSize };
        m_tail.setOrigin(0.f, tailSize.y / 2.f);
        m_tail.setPosition(sf::Vector2f(a1, b1 /*xa, ya*/));
        m_tail.setRotation(arrowAngle);

        if (alpha > 0u)
        {
            m_head.setFillColor(FILL_COLOR(alpha));
            m_tail.setFillColor(FILL_COLOR(alpha));
        }
        else
        {
            m_head.setFillColor(OUTLINE_COLOR);
            m_tail.setFillColor(OUTLINE_COLOR);
        }
    }

    Arrow(sf::Vector2f const& startPoint, sf::Vector2f const& endPoint, const uint8_t alpha)
        : Arrow(startPoint.x, startPoint.y, endPoint.x, endPoint.y, alpha)
    {}

private:

    void draw(sf::RenderTarget& target, sf::RenderStates /*states*/) const override final
    {
        target.draw(m_tail);
        target.draw(m_head);
    }

private:

    sf::RectangleShape m_tail;
    sf::ConvexShape m_head;
};

// *****************************************************************************
//! \brief Helper class for splitting a JSON file into sub-strings that can be
//! parsed. Indeed, we do not use third part JSON library for reading saved file
//! and load Petri nets but we read it directly since the format is very basic.
// *****************************************************************************
class Spliter
{
public:

    //! \brief Open the file to be split and memorize delimiter chars.
    //! \param[in] filepath Open the file to be split
    //! \param[in] list of delimiter chars for string separation.
    Spliter(std::string const& filepath, std::string const& del)
        : is(filepath), delimiters(del)
    {}

    //! \brief Check if the stream state is fine.
    operator bool() const
    {
        return !!is;
    }

    //! \brief Return the first interesting json string element.
    //! If no element can be split return a dummy string;
    std::string const& split()
    {
        while (true)
        {
            if (pos == std::string::npos)
            {
                if (!std::getline(is, line))
                {
                    word.clear();
                    return word;
                }
                prev = 0u;
            }

            while ((pos = line.find_first_of(delimiters, prev)) != std::string::npos)
            {
                if (pos > prev)
                {
                    word = line.substr(prev, pos - prev);
                    prev = pos + 1u;
                    return word;
                }
                prev = pos + 1u;
            }

            if (prev < line.length())
            {
                word = line.substr(prev, std::string::npos);
                return word;
            }
        }

        word.clear();
        return word;
    }

    //! \brief Return the last split string.
    std::string const& str() const
    {
        return word;
    }

private:

    std::ifstream is;
    std::string delimiters;
    std::string line;
    std::string word;
    std::size_t prev = 0u;
    std::size_t pos = std::string::npos;
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
      << "Delete key: remove a place or transition or an arc" << std::endl
      << "C key: clear the whole Petri net" << std::endl
      << "M key: move the selected place or transition" << std::endl
      << "+ key: add a token on the place pointed by the mouse cursor" << std::endl
      << "- key: remove a token on the place pointed by the mouse cursor" << std::endl
      << "R key: run (start) or stop the simulation" << std::endl
      << "S key: save the Petri net to petri.json file" << std::endl
      << "O key: load the Petri net from petri.json file" << std::endl
      << "G key: export the Petri net as Grafcet in a C++ header file" << std::endl
      << "J key: export the Petri net as Julia code" << std::endl
      << std::endl;
}

//------------------------------------------------------------------------------
PetriGUI::PetriGUI(Application &application)
    : GUI("Petri Net Editor", application),
      m_figure_place(PLACE_RADIUS),
      m_figure_token(TOKEN_RADIUS),
      m_figure_trans(sf::Vector2f(TRANS_HEIGHT, TRANS_WIDTH)),
      m_message_bar(m_font)
{
    usage();

    // Reserve memory
    m_animations.reserve(128u);

    // Precompute SFML struct for drawing places
    m_figure_place.setOrigin(sf::Vector2f(m_figure_place.getRadius(), m_figure_place.getRadius()));
    m_figure_place.setFillColor(sf::Color::White);
    m_figure_place.setOutlineThickness(2.0f);
    m_figure_place.setOutlineColor(OUTLINE_COLOR);

    // Precompute SFML struct for drawing tokens inside places
    m_figure_token.setOrigin(sf::Vector2f(m_figure_token.getRadius(), m_figure_token.getRadius()));
    m_figure_token.setFillColor(sf::Color::Black);

    // Precompute SFML struct for drawing transitions
    m_figure_trans.setOrigin(m_figure_trans.getSize().x / 2, m_figure_trans.getSize().y / 2);
    m_figure_trans.setFillColor(sf::Color::White);
    m_figure_trans.setOutlineThickness(2.0f);
    m_figure_trans.setOutlineColor(OUTLINE_COLOR);

    // Precompute SFML struct for drawing text (places and transitions)
    if (!m_font.loadFromFile("font.ttf"))
    {
        std::cerr << "Could not load font file ..." << std::endl;
        exit(1);
    }

    // Caption for Places and Transitions
    m_text_caption.setFont(m_font);
    m_text_caption.setCharacterSize(CAPTION_FONT_SIZE);
    m_text_caption.setFillColor(sf::Color::Black);

    // Number of Tokens
    m_text_token.setFont(m_font);
    m_text_token.setCharacterSize(TOKEN_FONT_SIZE);
    m_text_token.setFillColor(sf::Color::Black);

    // Init mouse cursor position
    m_mouse = sf::Vector2f(sf::Mouse::getPosition(window()));

    m_message_bar.setText("Welcome to timed Petri net editor");
}

//------------------------------------------------------------------------------
PetriGUI::~PetriGUI()
{
    window().close();
}

//------------------------------------------------------------------------------
void PetriGUI::draw(sf::Text& t, std::string const& str, float const x, float const y)
{
    t.setString(str);
    t.setPosition(x - t.getLocalBounds().width / 2.0, y - t.getLocalBounds().height);
    window().draw(t);
}

//------------------------------------------------------------------------------
void PetriGUI::draw(sf::Text& t, size_t const number, float const x, float const y)
{
    PetriGUI::draw(t, std::to_string(number), x, y);
}

//------------------------------------------------------------------------------
void PetriGUI::draw(sf::Text& t, float const number, float const x, float const y)
{
    std::stringstream stream;
    stream << std::fixed << std::setprecision(2) << number;
    PetriGUI::draw(t, stream.str(), x, y);
}

//------------------------------------------------------------------------------
void PetriGUI::draw(Place const& place, uint8_t alpha)
{
    const float x = place.x;
    const float y = place.y;

    // Draw the place
    m_figure_place.setPosition(sf::Vector2f(x, y));
    m_figure_place.setFillColor(FILL_COLOR(alpha));
    window().draw(m_figure_place);

    // Draw the caption
    draw(m_text_caption, place.caption, x,
         y - PLACE_RADIUS - CAPTION_FONT_SIZE / 2.0f - 2.0f);

    // Draw the number of tokens
    if (place.tokens > 0u)
    {
        float r = TOKEN_RADIUS;
        float d = TOKEN_RADIUS + 1.0f;

        if (place.tokens == 1u)
        {
            m_figure_token.setPosition(sf::Vector2f(x, y));
            window().draw(sf::CircleShape(m_figure_token));
        }
        else if (place.tokens == 2u)
        {
            m_figure_token.setPosition(sf::Vector2f(x - d, y));
            window().draw(sf::CircleShape(m_figure_token));

            m_figure_token.setPosition(sf::Vector2f(x + d, y));
            window().draw(sf::CircleShape(m_figure_token));
        }
        else if (place.tokens == 3u)
        {
            m_figure_token.setPosition(sf::Vector2f(x, y - r));
            window().draw(sf::CircleShape(m_figure_token));

            m_figure_token.setPosition(sf::Vector2f(x - d, y + d));
            window().draw(sf::CircleShape(m_figure_token));

            m_figure_token.setPosition(sf::Vector2f(x + d, y + d));
            window().draw(sf::CircleShape(m_figure_token));
        }
        else if ((place.tokens == 4u) || (place.tokens == 5u))
        {
            if (place.tokens == 5u)
            {
                d = r + 3.0f;
                m_figure_token.setPosition(sf::Vector2f(x, y));
                window().draw(sf::CircleShape(m_figure_token));
            }

            m_figure_token.setPosition(sf::Vector2f(x - d, y - d));
            window().draw(sf::CircleShape(m_figure_token));

            m_figure_token.setPosition(sf::Vector2f(x + d, y - d));
            window().draw(sf::CircleShape(m_figure_token));

            m_figure_token.setPosition(sf::Vector2f(x - d, y + d));
            window().draw(sf::CircleShape(m_figure_token));

            m_figure_token.setPosition(sf::Vector2f(x + d, y + d));
            window().draw(sf::CircleShape(m_figure_token));
        }
        else
        {
            draw(m_text_token, place.tokens, x, y);
        }
    }
}

//------------------------------------------------------------------------------
void PetriGUI::draw(Transition const& transition, uint8_t alpha)
{
    // Draw the transition
    m_figure_trans.setPosition(sf::Vector2f(transition.x, transition.y));
    m_figure_trans.setRotation(transition.angle);
    m_figure_trans.setFillColor(FILL_COLOR(alpha));
    window().draw(m_figure_trans);

    // Draw the caption
    draw(m_text_caption, transition.caption, transition.x,
         transition.y - PLACE_RADIUS - CAPTION_FONT_SIZE / 2.0f - 2.0f);
}

//------------------------------------------------------------------------------
void PetriGUI::draw(Arc const& arc, uint8_t alpha)
{
    // Transition -> Place
    Arrow arrow(arc.from.x, arc.from.y, arc.to.x, arc.to.y, alpha);
    window().draw(arrow);

    if (arc.from.type == Node::Type::Transition)
    {
        // Draw the timing
        float x = arc.from.x + (arc.to.x - arc.from.x) / 2.0f;
        float y = arc.from.y + (arc.to.y - arc.from.y) / 2.0f;
        draw(m_text_token, arc.duration, x, y - 15);
    }
}

//------------------------------------------------------------------------------
void PetriGUI::draw(float const /*dt*/)
{
    // Draw all Places
    for (auto& p: m_petri_net.places())
    {
        uint8_t alpha = fading(p.timer, p.tokens > 0u);
        draw(p, alpha);
    }

    // Draw all Transitions
    for (auto& t: m_petri_net.transitions())
    {
        uint8_t alpha = fading(t.timer, false);
        draw(t, alpha);
    }

    // Draw all Arcs
    for (auto& a: m_petri_net.arcs())
    {
        uint8_t alpha = fading(a.timer, false);
        draw(a, alpha);
    }

    // Draw the arc the user is creating
    if ((m_arc_from_unknown_node) || (m_node_from != nullptr))
    {
        float x = (m_arc_from_unknown_node) ? m_x : m_node_from->x;
        float y = (m_arc_from_unknown_node) ? m_y : m_node_from->y;
        Arrow arrow(x, y, m_mouse.x, m_mouse.y, 0u);
        window().draw(arrow);
    }

    // Draw the selection

    // Update the node the user is moving
    for (auto& it: m_selected_modes)
    {
        it->x = m_mouse.x;
        it->y = m_mouse.y;
    }

    // Draw all tokens transiting from Transitions to Places
    for (auto const& at: m_animations)
    {
        m_figure_token.setPosition(at.x, at.y);
        window().draw(m_figure_token);
        draw(m_text_token, at.tokens, at.x, at.y - 16);
    }

    // Draw the entry text
    m_message_bar.setSize(window().getSize());
    window().draw(m_message_bar);
}

//------------------------------------------------------------------------------
void PetriNet::generateArcsInArcsOut()
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

    // if (true)
    for (auto& p: m_places)
    {
        p.arcsIn.clear();
        p.arcsOut.clear();

        for (auto& a: m_arcs)
        {
            if ((a.from.type == Node::Type::Transition) && (a.to.id == p.id))
                p.arcsIn.push_back(&a);
            else if ((a.to.type == Node::Type::Transition) && (a.from.id == p.id))
                p.arcsOut.push_back(&a);
        }
    }
}

//------------------------------------------------------------------------------
bool PetriNet::isEventGraph()
{
    // The Petri net shall be an event graph: all places shall have a single
    // input arc and a single output arc. Else, we cannot generate the linear
    // system.
    for (auto& p: m_places)
    {
        if (!((p.arcsIn.size() == 1u) && (p.arcsOut.size() == 1u)))
        {
            // Help the user to debug the Petri net. // TODO: could be nice to
            // show directly odd arcs in red but for the moment we display on
            // the console.
            std::cerr << "Your Petri net is not an event graph. Because:"
                      << std::endl;
            for (auto& p: m_places)
            {
                if (p.arcsOut.size() != 1u)
                {
                    std::cerr << "  " << p.key
                              << ((p.arcsOut.size() > 1u)
                                  ? " has more than one output arc:"
                                  : " has no output arc");
                    for (auto const& a: p.arcsOut)
                        std::cerr << " " << a->to.key;
                    std::cerr << std::endl;
                }

                if (p.arcsIn.size() != 1u)
                {
                    std::cerr << "  " << p.key
                              << ((p.arcsIn.size() > 1u)
                                  ? " has more than one input arc:"
                                  : " has no input arc");
                    for (auto const& a: p.arcsIn)
                        std::cerr << " " << a->from.key;
                    std::cerr << std::endl;
                }
            }
            return false;
        }
    }

    return true;
}

//------------------------------------------------------------------------------
// TODO temporary: for the moment we only support Petri nets where Places have
// 0 or 1 tokens and places linked to inputs and outputs shall have 0 token.
bool PetriNet::exportToJulia(std::string const& filename)
{
    std::ofstream file(filename);
    if (!file)
    {
        std::cerr << "Failed to export the Petri net to '" << filename
                  << "'. Reason was " << strerror(errno) << std::endl;
        return false;
    }

    // Generate the Julia header
    file << "using MaxPlus, SparseArrays" << std::endl << std::endl;

    // Update arcs in/out for all transitions to be sure to generate the correct
    // net.
    generateArcsInArcsOut(/*arcs: true*/);

    if (!isEventGraph())
        return false;

    // Count the number of inputs, outputs and states for creating matrices.
    size_t nb_states = 0u;
    size_t nb_inputs = 0u;
    size_t nb_outputs = 0u;

    file << "## Petri Transitions:" << std::endl;

    // Show and count system inputs
    for (auto& t: m_transitions)
    {
        if (isInput(t))
        {
            t.mi = nb_inputs;
            nb_inputs += 1u;
            file << "# " << t.key << ": input (U"
                      << nb_inputs << ")" << std::endl;
        }
    }

    // Show and count system states
    for (auto& t: m_transitions)
    {
        if (isState(t))
        {
            t.mi = nb_states;
            nb_states += 1u;
            file << "# " << t.key << ": state (X"
                      << nb_states << ")" << std::endl;
        }
    }

    // Show and count system outputs
    for (auto& t: m_transitions)
    {
        if (isOutput(t))
        {
            t.mi = nb_outputs;
            nb_outputs += 1u;
            file << "# " << t.key << ": output (Y" << nb_outputs
                      << ")" << std::endl;
        }
    }

    // Graph representation
    file << std::endl;
    file << "## Timed graph event depict as two graph adjacency matrices:" << std::endl;
    file << "# Nodes are Transitions." << std::endl;
    file << "# Arcs are Places and therefore have tokens and durations" << std::endl;
    SparseMatrix N, T;
    for (auto& p: m_places)
    {
        // Since we are sure we are an event graph: places have a single input
        // arc and a aingle output arc.
        assert(p.arcsIn.size() == 1u);
        assert(p.arcsIn[0]->from.type == Node::Type::Transition);
        assert(p.arcsOut.size() == 1u);
        assert(p.arcsOut[0]->to.type == Node::Type::Transition);

        Transition& from = *reinterpret_cast<Transition*>(&(p.arcsIn[0]->from));
        Transition& to = *reinterpret_cast<Transition*>(&(p.arcsOut[0]->to));

        file << "# Arc " << p.key << ": " << from.key << " -> " << to.key
             << " (Duration: " << p.arcsIn[0]->duration
             << ", Tokens: " << p.tokens << ")" << std::endl;

        // Note origin and destination are inverted because of matrix product:
        // T * x with x a column vector
        T.push_back(SparseElement(to.id, from.id, p.arcsIn[0]->duration));
        N.push_back(SparseElement(to.id, from.id, p.tokens));
    }
    size_t const nnodes = m_transitions.size();
    file << "N = sparse(" << N << ", " << nnodes << ", " << nnodes << ") # Tokens" << std::endl;
    file << "T = sparse(" << T << ", " << nnodes << ", " << nnodes << ") # Durations" << std::endl;

    // Show the event graph to its Max-Plus counter form
    file << std::endl;
    file << "## Timed event graph represented as its counter form:" << std::endl;
    for (auto& t: m_transitions)
    {
        if (t.arcsIn.size() == 0u)
            continue;

        file << "# " << t.key << "(t) = min(";
        std::string separator1;
        for (auto& ai: t.arcsIn)
        {
            file << separator1;
            file << tokensIn(*ai) << " + ";
            std::string separator2;
            for (auto& ao: ai->from.arcsIn)
            {
                file << separator2;
                file << ao->from.key << "(t - " << ao->duration << ")";
                separator2 = ", ";
            }
            separator1 = ", ";
        }
        file << ");" << std::endl;
    }

    // Show the event graph to its Max-Plus dater form
    file << std::endl;
    file << "## Timed event graph represented as its dater form:" << std::endl;
    for (auto& t: m_transitions)
    {
        if (t.arcsIn.size() == 0u)
            continue;

        file << "# " << t.key << "(n) = max(";
        std::string separator1;
        for (auto& ai: t.arcsIn)
        {
            file << separator1;
            std::string separator2;
            for (auto& ao: ai->from.arcsIn)
            {
                file << separator2;
                file << ao->duration << " + " << ao->from.key
                          << "(n - " << tokensIn(*ai) << ")";
                separator2 = ", ";
            }
            separator1 = ", ";
        }
        file << ");" << std::endl;
    }

    file << std::endl;
    file << "## Max-Plus implicit linear dynamic system of the dater form:" << std::endl;
    file << "# X(n) = D X(n) (+) A X(n-1) + B U(n)" << std::endl;
    file << "# Y(n) = C X(n)" << std::endl;

    // Compute the syslin as Julia code using the Max-Plus package
    // X(n) = D X(n) (+) A X(n-1) + B U(n)
    // Y(n) = C X(n)
    SparseMatrix A, D, B, C;
    for (auto& arc: m_arcs)
    {
        if (arc.from.type == Node::Type::Place)
            continue;

        Transition& t = *reinterpret_cast<Transition*>(&(arc.from));
        if (isInput(t))
        {
            // System inputs: B U(n)
            B.push_back(SparseElement(t.mi, t.mi, arc.duration));
        }
        else // States or outputs
        {
            Place& p = *reinterpret_cast<Place*>(&(arc.to));
            for (auto& a: p.arcsOut)
            {
                Transition& td = *reinterpret_cast<Transition*>(&(a->to));

                if (isState(td))
                {
                    // Systems states: X(n) = D X(n) (+) A X(n-1)
                    if (p.tokens == 1u)
                    {
                        A.push_back(SparseElement(td.mi, t.mi, arc.duration));
                    }
                    else
                    {
                        D.push_back(SparseElement(td.mi, t.mi, arc.duration));
                    }
                }
                else if (isOutput(td))
                {
                    // System outputs: Y(n) = C X(n)
                    C.push_back(SparseElement(t.mi, t.mi, arc.duration));
                }
            }
        }
    }

    // Julia Max-Plus Linear system
    file << "D = sparse(" << D << ", " << nb_states << ", " << nb_states << ") # States without tokens" << std::endl;
    file << "A = sparse(" << A << ", " << nb_states << ", " << nb_states << ") # States with 1 token" << std::endl;
    file << "B = sparse(" << B << ", " << nb_inputs << ", " << nb_inputs << ") # Inputs" << std::endl;
    file << "C = sparse(" << C << ", " << nb_outputs << ", " << nb_outputs << ") # Outputs" << std::endl;
    file << "S = MPSysLin(A, B, C, D)" << std::endl;

    // Semi-Howard
    file << std::endl;
    file << "#" << std::endl;
    file << "TODO l,v = semihoward(S.D, S.A)" << std::endl;

    return true;
}

//------------------------------------------------------------------------------
bool PetriNet::showCriticalCycle()
{
    // Update arcs in/out for all transitions to be sure to generate the correct
    // net.
    generateArcsInArcsOut(/*arcs: true*/);

    if (!isEventGraph())
        return false;

    // Reserve memory
    size_t const nnodes = m_transitions.size();
    size_t const narcs = m_places.size();
    std::vector<double> N; N.reserve(narcs);
    std::vector<double> T; T.reserve(narcs);
    std::vector<int> IJ; IJ.reserve(2u * narcs);

    for (auto& p: m_places)
    {
        // Since we are sure we are an event graph: places have a single input
        // arc and a aingle output arc.
        assert(p.arcsIn.size() == 1u);
        assert(p.arcsIn[0]->from.type == Node::Type::Transition);
        assert(p.arcsOut.size() == 1u);
        assert(p.arcsOut[0]->to.type == Node::Type::Transition);

        Transition& from = *reinterpret_cast<Transition*>(&(p.arcsIn[0]->from));
        Transition& to = *reinterpret_cast<Transition*>(&(p.arcsOut[0]->to));

        std::cout << "# Arc " << p.key << ": " << from.key << " -> " << to.key
                  << " (Duration: " << p.arcsIn[0]->duration << ", Tokens: "
                  << p.tokens << ")" << std::endl;

        IJ.push_back(to.id); // Transposed is needed
        IJ.push_back(from.id);
        T.push_back(p.arcsIn[0]->duration);
        N.push_back(p.tokens);
    }

    std::vector<double> V(nnodes); // bias
    std::vector<double> chi(nnodes); // cycle time vector
    std::vector<int> policy(nnodes); // optimal policy
    int ncomponents; //
    int niterations; // nb of iteration of the algorithm
    int verbosemode = 0;
    int res = Semi_Howard(IJ.data(), T.data(), N.data(),
                          nnodes, narcs,
                          chi.data(), V.data(), policy.data(), &niterations,
                          &ncomponents, verbosemode);

    std::cout << "V=";
    for (auto const& it: V)
    {
        std::cout << ' ' << it;
    }
    std::cout << std::endl << "CHI=";
    for (auto const& it: chi)
    {
        std::cout << ' ' << it;
    }
    std::cout << std::endl << "Nb Policy=" << ncomponents;
    std::cout << std::endl << "POLICY=";
    for (auto const& it: policy)
    {
        std::cout << ' ' << it;
    }
    std::cout << std::endl;

    return res == 0;
}

//------------------------------------------------------------------------------
bool PetriNet::exportToCpp(std::string const& filename, std::string const& name)
{
    std::ofstream file(filename);
    if (!file)
    {
        std::cerr << "Failed to export the Petri net to '" << filename
                  << "'. Reason was " << strerror(errno) << std::endl;
        return false;
    }

    std::string upper_name(name);
    std::for_each(upper_name.begin(), upper_name.end(), [](char & c) {
        c = ::toupper(c);
    });

    // Update arcs in/out for all transitions to be sure to generate the correct
    // net.
    generateArcsInArcsOut();

    file << "// This file has been generated and you should avoid editing it." << std::endl;
    file << "// Note: the code generator is still experimental !" << std::endl;
    file << "" << std::endl;
    file << "#ifndef GENERATED_GRAFCET_" << upper_name << "_HPP" << std::endl;
    file << "#  define GENERATED_GRAFCET_" << upper_name << "_HPP" << std::endl;
    file << "" << std::endl;
    file << "#  include <iostream>" << std::endl;
    file << "" << std::endl;
    file << "namespace " << name << " {" << std::endl;

    file << R"PN(
class Grafcet
{
public:

    Grafcet()
    {
        initIO();
        reset();
    }

    void reset()
    {
        // Initial states
)PN";

    for (size_t i = 0; i < m_places.size(); ++i)
    {
        file << "        X[" << m_places[i].id << "] = "
             << (m_places[i].tokens ? "true; " : "false;")
             << std::endl;
    }

    file << R"PN(    }

    void update()
    {
)PN";

    file << "        // Do actions of enabled steps" << std::endl;
    for (size_t p = 0u; p < m_places.size(); ++p)
    {
        file << "        if (X[" << p << "]) { X" << p << "(); }"
             << std::endl;
    }

    file << std::endl;
    file << "        // Read inputs (TODO)" << std::endl << std::endl;
    file << "        // Update transitions" << std::endl;

    for (size_t t = 0u; t < m_transitions.size(); ++t)
    {
        Transition& trans = m_transitions[t];
        file << "        T[" << trans.id << "] =";
        for (size_t a = 0; a < trans.arcsIn.size(); ++a)
        {
            Arc& arc = *trans.arcsIn[a];
            if (a > 0u) {  file << " &&"; }
            file << " X[" << arc.from.id << "]";
        }
        file << " && T"  << trans.id << "();";
        file << " // " << trans.key << std::endl;
                                                         }

    file  << std::endl << "        // Update steps" << std::endl;
    for (size_t t = 0u; t < m_transitions.size(); ++t)
    {
        Transition& trans = m_transitions[t];
        file << "        if (T[" << t << "]) {" << std::endl;
        for (size_t a = 0; a < trans.arcsIn.size(); ++a)
        {
            Arc& arc = *trans.arcsIn[a];
            file << "            X[" << arc.from.id
                 << "] = false; // Disable " << arc.from.key << std::endl;
        }
        for (size_t a = 0; a < trans.arcsOut.size(); ++a)
        {
            Arc& arc = *trans.arcsOut[a];
            file << "            X[" << arc.to.id
                 << "] = true; // Enable " << arc.to.key << std::endl;
        }
        file << "        }" << std::endl << std::endl;
    }

    file << "        // Set output values (TODO)" << std::endl;

    file << R"PN(    }

    void debug()
    {
       std::cout << "Transitions:" << std::endl;
       for (size_t i = 0u; i < MAX_TRANSITIONS; ++i)
          std::cout << "  T[" << i << "] = " << T[i] << std::endl;

       //std::cout << "Inputs:" << std::endl;
       //for (size_t i = 0u; i < MAX_INPUTS; ++i)
       //   std::cout << "  I[" << i << "] = " << I[i] << std::endl;

       std::cout << "Steps:" << std::endl;
       for (size_t i = 0u; i < MAX_STEPS; ++i)
          std::cout << "  X[" << i << "] = " << X[i] << std::endl;

       //std::cout << "Outputs:" << std::endl;
       //for (size_t i = 0u; i < MAX_OUTPUTS; ++i)
       //   std::cout << "  O[" << i << "] = " << O[i] << std::endl;
    }

private:

    //! \brief Fonction not generated to let the user initializing
    //! inputs (i.e. TTL gpio, ADC ...) and outputs (i.e. TTL gpio,
    //! PWM ...)
    void initIO();
)PN";

    for (auto const& t: m_transitions)
    {
        file << "    //! \\brief Fonction not generated to let the user writting the" << std::endl;
        file << "    //! transitivity for the Transition " << t.id <<  " depending of the system" << std::endl;
        file << "    //! inputs I[]." << std::endl;
        file << "    //! \\return true if the transition is enabled." << std::endl;
        file << "    bool T" << t.id << "() const;" << std::endl;
    }

    for (auto const& p: m_places)
    {
        file << "    //! \\brief Fonction not generated to let the user writting the" << std::endl;
        file << "    //! reaction for the Step " << p.id << std::endl;
        file << "    void X" << p.id << "();" << std::endl;
    }

    file << R"PN(
private:

)PN";

    file << "    static const size_t MAX_STEPS = " << m_places.size() << "u;"  << std::endl;
    file << "    static const size_t MAX_TRANSITIONS = " << m_transitions.size() << "u;" << std::endl;
    file << "    //static const size_t MAX_INPUTS = X;" << std::endl;
    file << "    //static const size_t MAX_OUTPUTS = Y;" << std::endl;
    file << "" << std::endl;
    file << "    //! \\brief Steps"  << std::endl;
    file << "    bool X[MAX_STEPS];" << std::endl;
    file << "    //! \\brief Transitions"  << std::endl;
    file << "    bool T[MAX_TRANSITIONS];" << std::endl;
    file << "    //! \\brief Inputs"  << std::endl;
    file << "    //uint16_t I[MAX_INPUTS];" << std::endl;
    file << "    //! \\brief Outputs"  << std::endl;
    file << "    //uint16_t O[MAX_OUTPUTS];" << std::endl;
    file << "};" << std::endl;
    file << "" << std::endl;
    file << "} // namespace " << name << std::endl;
    file << "#endif // GENERATED_GRAFCET_" << upper_name << "_HPP" << std::endl;

    std::cerr << "Petri net saved into file '" << filename << "'" << std::endl;
    return true;
}

//------------------------------------------------------------------------------
bool PetriNet::save(std::string const& filename)
{
    std::string separator;
    std::ofstream file(filename);

    if (isEmpty(*this))
    {
        std::cerr << "I'll not save empty net" << std::endl;
        return false;
    }

    if (!file)
    {
        std::cerr << "Failed saving the Petri net in '" << filename
                  << "'. Reason was " << strerror(errno) << std::endl;
        return false;
    }

    file << "{\n  \"places\": [";
    for (auto const& p: m_places)
    {
        file << separator << '\"' << p.key << ','
             << p.x << ',' << p.y << ',' << p.tokens << '\"';
        separator = ", ";
    }
    file << "],\n  \"transitions\": [";
    separator = "";
    for (auto const& t: m_transitions)
    {
        file << separator << '\"' << t.key << ','
             << t.x << ',' << t.y << ',' << t.angle << '\"';
        separator = ", ";
    }
    file << "],\n  \"arcs\": [";
    separator = "";
    for (auto const& a: m_arcs)
    {
        file << separator << '\"' << a.from.key << ','
             << a.to.key << ',' << a.duration << '\"';
        separator = ", ";
    }
    file << "]\n}";

    std::cerr << "Petri net saved into file '" << filename << "'" << std::endl;
    return true;
}

//------------------------------------------------------------------------------
bool PetriNet::load(std::string const& filename)
{
    bool found_places = false;
    bool found_transitions = false;
    bool found_arcs = false;

    Spliter s(filename, " \",");

    if (!s)
    {
        std::cerr << "Failed opening '" << filename << "'. Reason was '"
                  << strerror(errno) << "'" << std::endl;
        return false;
    }

    if (s.split() != "{")
    {
        std::cerr << "Token { missing. Bad JSON file" << std::endl;
        return false;
    }

    reset();

    while (s)
    {
        s.split();
        if ((s.str() == "places") && (s.split() == ":") && (s.split() == "["))
        {
            found_places = true;
            while (s.split() != "]")
            {
                size_t id = atoi(s.str().c_str() + 1u);
                float x = stof(s.split());
                float y = stof(s.split());
                size_t t = stoi(s.split());
                addPlace(id, x, y, t);
            }
        }
        else if ((s.str() == "transitions") && (s.split() == ":") && (s.split() == "["))
        {
            found_transitions = true;
            while (s.split() != "]")
            {
                size_t id = atoi(s.str().c_str() + 1u);
                float x = stof(s.split());
                float y = stof(s.split());
                int a = stoi(s.split());
                addTransition(id, x, y, a);
            }
        }
        else if ((s.str() == "arcs") && (s.split() == ":") && (s.split() == "["))
        {
            found_arcs = true;
            while (s.split() != "]")
            {
                Node* from = findNode(s.str());
                if (!from)
                {
                    std::cerr << "Origin node " << s.str() << " not found" << std::endl;
                    return false;
                }

                Node* to = findNode(s.split());
                if (!to)
                {
                    std::cerr << "Destination node " << s.str() << " not found" << std::endl;
                    return false;
                }

                float duration = stof(s.split());
                addArc(*from, *to, duration);
            }
        }
        else if (s.str() == "}")
        {
            if (!found_places)
                std::cerr << "The JSON file did not contained Places" << std::endl;
            if (!found_transitions)
                std::cerr << "The JSON file did not contained Transitions" << std::endl;
            if (!found_arcs)
                std::cerr << "The JSON file did not contained Arcs" << std::endl;

            if (!(found_places && found_transitions && found_arcs))
                return false;
        }
        else if (s.str() != "")
        {
            std::cerr << "Key " << s.str() << " is not a valid token" << std::endl;
            return false;
        }
    }

    return true;
}

//------------------------------------------------------------------------------
Node* PetriNet::findNode(std::string const& key)
{
    if (key[0] == 'P')
    {
        for (auto& p: m_places)
        {
            if (p.key == key)
                return &p;
        }
        return nullptr;
    }

    if (key[0] == 'T')
    {        for (auto& t: m_transitions)
        {
            if (t.key == key)
                return &t;
        }
        return nullptr;
    }

    std::cerr << "Node key shall start with 'P' or 'T'" << std::endl;
    return nullptr;
}

//------------------------------------------------------------------------------
void PetriNet::removeNode(Node& node)
{
    // Remove all arcs linked to this node.
    // Note: For fastest deletion, we simply swap the undesired arc with the
    // latest arc in the container. To do that, we have to iterate from the end
    // of the container.
    size_t s = m_arcs.size();
    size_t i = s;
    while (i--)
    {
        if ((m_arcs[i].to == node) || (m_arcs[i].from == node))
        {
            // Found the undesired arc: make the latest element take its
            // location in the container.
            m_arcs[i] = m_arcs[m_arcs.size() - 1u];
            m_arcs.pop_back();
        }
    }

    // Search and remove the node.
    // Note: For fastest deletion, we simply swap the undesired node with the
    // latest node in the container. To do that, we have to iterate from the end
    // of the container.
    if (node.type == Node::Type::Place)
    {
        size_t i = m_places.size();
        while (i--)
        {
            // Found the undesired node: make the latest element take its
            // location in the container. But before doing this we have to
            // restore references on impacted arcs.
            if (m_places[i].id == node.id)
            {
                // Swap element but keep the ID of the removed element
                Place& pi = m_places[i];
                Place& pe = m_places[m_places.size() - 1u];
                m_places[i] = Place(pi.id, pe.x, pe.y, pe.tokens);
                Place::s_next_id -= 2u;

                // Update the references to nodes of the arc
                for (auto& a: m_arcs) // TODO optim: use in/out arcs but they may not be generated
                {
                    if (a.to == pe)
                        a = Arc(a.from, m_places[i], a.duration);
                    if (a.from == pe)
                        a = Arc(m_places[i], a.to, a.duration);
                }

                m_places.pop_back();
            }
        }
    }
    else
    {
        size_t i = m_transitions.size();
        while (i--)
        {
            if (m_transitions[i].id == node.id)
            {
                Transition& ti = m_transitions[i];
                Transition& te = m_transitions[m_transitions.size() - 1u];
                m_transitions[i] = Transition(ti.id, te.x, te.y, te.angle);
                Transition::s_next_id -= 2u;

                for (auto& a: m_arcs) // TODO idem
                {
                    if (a.to == te)
                        a = Arc(a.from, m_transitions[i], a.duration);
                    if (a.from == te)
                        a = Arc(m_transitions[i], a.to, a.duration);
                }

                m_transitions.pop_back();
            }
        }
    }

    // Restore in arcs and out arcs for each node
    generateArcsInArcsOut();
}

//------------------------------------------------------------------------------
static size_t canFire(Transition const& trans)
{
    if (trans.arcsIn.size() == 0u)
        return 0u;

#if 1 // Version 1: return 0 or 1 token

    for (auto& a: trans.arcsIn)
    {
        if (tokensIn(*a) == 0u)
            return 0u;
    }
    return 1u;

#else // Version 2: return the maximum possibe of tokens that can be burnt

    size_t burnt = static_cast<size_t>(-1);

    for (auto& a: trans.arcsIn)
    {
        size_t tokens = tokensIn(*a);
        if (tokens == 0u)
            return 0u;

        if (tokens < burnt)
            burnt = tokens;
    }
    return burnt;

#endif
}

//------------------------------------------------------------------------------
AnimatedToken::AnimatedToken(Arc& arc_, size_t tokens_)
    : x(arc_.from.x), y(arc_.from.y), tokens(tokens_), arc(arc_)
{
    assert(arc.from.type == Node::Type::Transition);
    assert(arc.to.type == Node::Type::Place);

    // Note: we are supposing the norm and duration is never updated by
    // the user during the simulation.
    magnitude = norm(arc.from.x, arc.from.y, arc.to.x, arc.to.y);
    speed = magnitude / arc.duration;
}

//------------------------------------------------------------------------------
bool AnimatedToken::update(float const dt)
{
    offset += dt * speed / magnitude;
    x = arc.from.x + (arc.to.x - arc.from.x) * offset;
    y = arc.from.y + (arc.to.y - arc.from.y) * offset;

    return (offset >= 1.0);
}

//------------------------------------------------------------------------------
// TODO: could be nice to separate simulation from animation
void PetriGUI::update(float const dt)
{
    bool burnt = false;
    bool burning = false;
    States state = m_state;

    switch (state)
    {
    case STATE_IDLE:
        if (m_simulating)
        {
            if (isEmpty(m_petri_net))
            {
                m_message_bar.setText("Petri net is empty. Simulation request ignored!");
                std::cerr << "Petri net is empty. Simulation request ignored!"
                          << std::endl;
                m_simulating = false;
            }
            else
            {
                m_state = STATE_STARTING;
            }
        }
        break;

    case STATE_STARTING:
        m_message_bar.setText("Simulation has started!");
        std::cout << current_time() << "Simulation has started!" << std::endl;
        // Backup tokens for each places since the simulation will burn them
        for (auto& p: m_petri_net.places())
        {
            p.m_backup_tokens = p.tokens;
        }

        // Populate in and out arcs for all transitions to avoid to look after
        // them.
        m_petri_net.generateArcsInArcsOut();
        m_animations.clear();
        m_state = STATE_ANIMATING;
        break;

    case STATE_ENDING:
        m_message_bar.setText("Simulation has ended!");
        std::cout << current_time() << "Simulation has ended!"
                  << std::endl << std::endl;
        // Restore burnt tokens from the simulation
        for (auto& p: m_petri_net.places())
        {
            p.tokens = p.m_backup_tokens;
        }

        m_animations.clear();
        m_state = STATE_IDLE;
        break;

    case STATE_ANIMATING:
        m_state = m_simulating ? STATE_ANIMATING : STATE_ENDING;

        // For each transition check if it is activated (all incoming Places
        // have at least one token to burn. Note: since here we care Petri but
        // not Grafcet the transitivity is always true). If yes, we will burn
        // the maximum possible of tokens in a single step for the animation.
        // But in the aim to divide tokens the most kindly over the maximum
        // transitions possible we have to iterate and burn tokens one by one.
        do
        {
            burning = false;
            for (auto& trans: m_petri_net.transitions()) // FIXME: filter the list to speed up
            {
                size_t tokens = canFire(trans); // [0 .. 1] tokens
                if (tokens > 0u)
                {
                    trans.timer.restart();

                    burning = true; // keep iterating on this loop
                    burnt = true; // At least one place has been fired

                    // Burn a single token on each Places above
                    for (auto& a: trans.arcsIn)
                    {
                        tokensIn(*a) -= 1u;
                        a->timer.restart();
                    }

                    // Count the number of tokens for the animation
                    for (auto& a: trans.arcsOut)
                    {
                        a->count += 1u;
                        // FIXME: speedup: store trans.arcsOut
                    }
                }
            }
        } while (burning);

        // Create animeted tokens with the correct number of tokens they are
        // carrying.
        if (burnt)
        {
            for (auto& a: m_petri_net.arcs()) // FIXME: speedup: trans.arcsOut
            {
                if (a.count > 0u)
                {
                    std::cout << current_time()
                              << a.from.key << " burnt "
                              << a.count << " token"
                              << (a.count == 1u ? "" : "s")
                              << std::endl;
                    m_animations.push_back(AnimatedToken(a, a.count));
                    a.timer.restart();
                    a.count = 0u;
                }
            }
        }

        // Tokens Transition --> Places are transitioning.
        if (m_animations.size() > 0u)
        {
            size_t i = m_animations.size();
            while (i--)
            {
                AnimatedToken& an = m_animations[i];
                if (an.update(dt))
                {
                    // Animated token reached its ddestination: Place
                    std::cout << current_time()
                              << "Place " << an.arc.to.key
                              << " got " << an.tokens << " token"
                              << (an.tokens == 1u ? "" : "s")
                              << std::endl;

                    // Drop the number of tokens it was carrying.
                    tokensOut(an.arc) += an.tokens;
                    // Remove it
                    m_animations[i] = m_animations[m_animations.size() - 1u];
                    m_animations.pop_back();
                }
            }
        }
        else
        {
            std::cout << current_time() << "The simulation cannot burn tokens."
                      << std::endl;
            m_simulating = false;
            m_state = STATE_ENDING;
        }
        break;

    default:
        std::cerr << "Odd state in the state machine doing the "
                  << "animation of the Petri net." << std::endl;
        exit(1);
        break;
    }
}

//------------------------------------------------------------------------------
Node* PetriGUI::getNode(float const x, float const y)
{
    // TODO: iterate backward to allowing selecting the last node inserted
    for (auto& p: m_petri_net.places())
    {
        float d2 = (p.x - x) * (p.x - x) + (p.y - y) * (p.y - y);
        if (d2 < PLACE_RADIUS * PLACE_RADIUS)
        {
            return &p;
        }
    }

    for (auto& t: m_petri_net.transitions())
    {
        // Working but sometimes less precise
        //if ((x >= t.x - 15.0) && (x <= t.x + TRANS_HEIGHT + 15.0) &&
        //    (y >= t.y - 15.0) && (y <= t.y + TRANS_WIDTH + 15.0))
        float d2 = (t.x - x) * (t.x - x) + (t.y - y) * (t.y - y);
        if (d2 < TRANS_WIDTH * TRANS_WIDTH)
        {
            return &t;
        }
    }

    return nullptr;
}

//------------------------------------------------------------------------------
void PetriGUI::handleKeyPressed(sf::Event const& event)
{
    // Escape key: quit the application.
    if (event.key.code == sf::Keyboard::Escape)
        m_running = false;

    // Left or right Control key pressed: memorize the state
    else if ((event.key.code == sf::Keyboard::LControl) ||
             (event.key.code == sf::Keyboard::RControl))
    {
        m_ctrl = true;
    }

    // 'R' key: Run the animation of the Petri net
    else if (event.key.code == sf::Keyboard::R)
    {
        m_simulating = m_simulating ^ true;

        // Note: in GUI.cpp in the Application constructor, I set
        // the window to have slower framerate in the aim to have a
        // bigger discrete time and therefore AnimatedToken moving
        // with a bigger step range and avoid them to overlap when
        // i.e. two of them, carying 1 token, are arriving at almost
        // the same moment but separated by one call of this method
        // update() producing two AnimatedToken carying 1 token that
        // will be displayed at the same position instead of a
        // single AnimatedToken carying 2 tokens.
        window().setFramerateLimit(m_simulating ? 30 : 60); // FPS
    }

    // 'S' key: save the Petri net to a JSON file
    else if (event.key.code == sf::Keyboard::S)
    {
        if (!m_simulating)
        {
            if (m_petri_net.save("petri.json"))
            {
                m_message_bar.setText("Petri net has been saved!");
            }
            else
            {
                m_message_bar.setText("Failed saving the Petri net!");
            }
        }
        else
        {
            m_message_bar.setText("Cannot save during the simulation!");
            std::cerr << "Cannot save during the simulation"
                      << std::endl;
        }
    }

    // 'O' key: load the Petri net to a JSON file
    else if (event.key.code == sf::Keyboard::O)
    {
        if (!m_simulating)
        {
            if (m_petri_net.load("petri.json"))
            {
                m_message_bar.setText("Loaded with success the Petri net!");
            }
            else
            {
                m_message_bar.setText("Failed loading the Petri net!");
            }
        }
        else
        {
            m_message_bar.setText("Cannot save during the simulation!");
            std::cerr << "Cannot save during the simulation"
                      << std::endl;
        }
    }

    // 'G' key: save the Petri net as grafcet in a C++ header file
    else if (event.key.code == sf::Keyboard::G)
    {
        if (m_petri_net.exportToCpp("Grafcet-gen.hpp", "generated"))
        {
            m_message_bar.setText("The Petri net has successfully exported as grafcet as C++ header file!");
        }
        else
        {
            m_message_bar.setText("Could not export the Petri net to C++ header file!");
        }
    }

     // 'J' key: save the Petri net as graph event in a Julia script file
    else if (event.key.code == sf::Keyboard::J)
    {
        if (m_petri_net.exportToJulia("GraphEvent-gen.jl"))
        {
            m_message_bar.setText("The Petri net has successfully exported as graph event as Julia file!");
        }
        else
        {
            m_message_bar.setText("Could not export the Petri net to Julia file!");
        }
    }

    // 'C' key: erase the Petri net
    else if (event.key.code == sf::Keyboard::C)
    {
        m_simulating = false;
        m_petri_net.reset();
        m_animations.clear();
    }

    // 'M' key: Move the selected node
    else if (event.key.code == sf::Keyboard::M)
    {
        if (m_selected_modes.size() == 0u)
        {
            Node* node = getNode(m_mouse.x, m_mouse.y);
            if (node != nullptr)
                m_selected_modes.push_back(node);
        }
        else
        {
            m_selected_modes.clear();
        }
    }

    // 'L' key: Move the selected node
    else if (event.key.code == sf::Keyboard::L)
    {
        if ((m_node_from == nullptr) && (!m_arc_from_unknown_node))
            handleArcOrigin();
        else
            handleArcDestination();
    }

    // Delete a node. TODO: implement arc deletion
    if (event.key.code == sf::Keyboard::Delete)
    {
        Node* node = getNode(m_mouse.x, m_mouse.y);
        if (node != nullptr)
            m_petri_net.removeNode(*node);
    }

    // '+' key: increase the number of tokens in the place.
    // '-' key: decrease the number of tokens in the place.
    else if ((event.key.code == sf::Keyboard::Add) ||
             (event.key.code == sf::Keyboard::Subtract))
    {
        Node* node = getNode(m_mouse.x, m_mouse.y);
        if ((node != nullptr) && (node->type == Node::Type::Place))
        {
            size_t& tokens = reinterpret_cast<Place*>(node)->tokens;
            if (event.key.code == sf::Keyboard::Add)
                ++tokens;
            else if (tokens > 0u)
                --tokens;
        }
    }

    else if ((event.key.code == sf::Keyboard::PageUp) ||
             (event.key.code == sf::Keyboard::PageDown))
    {
        Node* node = getNode(m_mouse.x, m_mouse.y);
        if ((node != nullptr) && (node->type == Node::Type::Transition))
        {
            Transition& t = *reinterpret_cast<Transition*>(node);
            t.angle += (event.key.code == sf::Keyboard::PageDown ? STEP_ANGLE : -STEP_ANGLE);
            t.angle = t.angle % 360;
            if (t.angle < 0)
                t.angle += 360;
        }
    }
}

//------------------------------------------------------------------------------
void PetriGUI::handleArcOrigin()
{
    m_node_from = getNode(m_mouse.x, m_mouse.y);
    if (m_node_from == nullptr)
    {
        if ((m_petri_net.places().size() != 0u) ||
            (m_petri_net.transitions().size() != 0u))
        {
            // We do not yet know the type of the destination node so create
            // intermediate information.
            m_x = m_mouse.x;
            m_y = m_mouse.y;
            m_arc_from_unknown_node = true;
        }
    }

    // Reset states
    m_node_to  = nullptr;
    m_selected_modes.clear();
}

//------------------------------------------------------------------------------
void PetriGUI::handleArcDestination()
{
    // Finish the creation of the arc (destination node)
    m_node_to = getNode(m_mouse.x, m_mouse.y);

    // The user grab no nodes: abort
    if ((m_node_from == nullptr) && (m_node_to == nullptr))
        return ;

    // Reached the destination node
    if (m_node_to != nullptr)
    {
        if (m_node_from != nullptr)
        {
            if (m_node_to->type == m_node_from->type)
            {
                // The user tried to link two nodes of the same type: this is
                // forbidden but we allow it by creating the intermediate node
                // of oposing type.
                float x = m_node_to->x + (m_node_from->x - m_node_to->x) / 2.0f;
                float y = m_node_to->y + (m_node_from->y - m_node_to->y) / 2.0f;
                float duration = random(1, 5);
                if (m_node_to->type == Node::Type::Place)
                {
                    Transition& n = m_petri_net.addTransition(x, y);
                    m_petri_net.addArc(*m_node_from, n, duration);
                    m_node_from = &n;
                }
                else
                {
                    Place& n = m_petri_net.addPlace(x, y);
                    m_petri_net.addArc(*m_node_from, n, duration);
                    m_node_from = &n;
                }
            }
        }
        else
        {
            // The user did not click on a node but released mouse on a node. We
            // create the origin node before creating the arc.
            if (m_arc_from_unknown_node)
            {
                if (m_node_to->type == Node::Type::Place)
                    m_node_from = &m_petri_net.addTransition(m_x, m_y);
                else
                    m_node_from = &m_petri_net.addPlace(m_x, m_y);
            }
        }
    }
    else if (m_node_from != nullptr)
    {
        // The user did not click on a node but released mouse on a node. We
        // create the origin node before creating the arc.
        float x = m_mouse.x;
        float y = m_mouse.y;
        if (m_node_from->type == Node::Type::Place)
            m_node_to = &m_petri_net.addTransition(x, y);
        else
            m_node_to = &m_petri_net.addPlace(x, y);
    }

    // Create the arc. Note: the duration value is only used
    // for arc Transition --> Place.
    float duration = random(1, 5);
    m_petri_net.addArc(*m_node_from, *m_node_to, duration);

    // Reset states
    m_node_from = m_node_to = nullptr;
    m_selected_modes.clear();
    m_arc_from_unknown_node = false;
}

//------------------------------------------------------------------------------
void PetriGUI::handleMouseButton(sf::Event const& event)
{
    // The 'M' key was pressed. Reset the state but do not add new node!
    if ((m_selected_modes.size() != 0u) &&
        (event.type == sf::Event::MouseButtonPressed))
    {
        m_node_from = m_node_to = nullptr;
        m_selected_modes.clear();
        return ;
    }

    if (event.mouseButton.button == sf::Mouse::Middle)
    {
        // Add new arc
        // Middle button: create arcs. On key pressed: define the origin node. On
        // key released: define the destination node. Several cases are managed:
        //   - origin and destination nodes exist and are different type (Place ->
        //     Transition or Transition -> Place): then create directly the arc.
        //   - the origin node is selected but not the destination node: then create
        //     the destination node of different type and then create the arc.
        //   - the origin node is not selected but not the destination node: then
        //     create the origine node of different type and then create the arc.
        if (event.type == sf::Event::MouseButtonPressed)
            handleArcOrigin();
        else if (event.type == sf::Event::MouseButtonReleased)
            handleArcDestination();
    }
    else if (event.type == sf::Event::MouseButtonPressed)
    {
        // Node selection
        if (m_ctrl)
        {
            m_x = m_mouse.x;
            m_y = m_mouse.y;
        }

        // End the arc started with the 'L' key
        else if ((m_node_from != nullptr) || (m_arc_from_unknown_node))
        {
            // left mouse button: ends the arc.
            // right mouse button: abort the arc
            if (event.mouseButton.button == sf::Mouse::Left)
                handleArcDestination();
        }
        else
        {
            // Add a new Place or a new Transition only if a node is
            // not already present
            if (getNode(m_mouse.x, m_mouse.y) == nullptr)
            {
                if (event.mouseButton.button == sf::Mouse::Left)
                    m_petri_net.addPlace(m_mouse.x, m_mouse.y);
                else if (event.mouseButton.button == sf::Mouse::Right)
                    m_petri_net.addTransition(m_mouse.x, m_mouse.y);
            }
        }

        // Reset states
        m_node_from = m_node_to = nullptr;
        m_selected_modes.clear();
        m_arc_from_unknown_node = false;
    }
    else if ((event.type == sf::Event::MouseButtonReleased) && (m_ctrl))
    {
        // Node selection
        for (auto& p: m_petri_net.places())
        {
            if ((p.x >= m_x) && (p.x <= m_mouse.x) &&
                (p.y >= m_y) && (p.y <= m_mouse.y))
            {
                m_selected_modes.push_back(&p);
            }
        }
        for (auto& t: m_petri_net.transitions())
        {
            if ((t.x >= m_x) && (t.x <= m_mouse.x) &&
                (t.y >= m_y) && (t.y <= m_mouse.y))
            {
                m_selected_modes.push_back(&t);
            }
        }
    }
}

//------------------------------------------------------------------------------
void PetriGUI::handleInput()
{
    sf::Event event;
    m_mouse = sf::Vector2f(sf::Mouse::getPosition(window()));

    while (m_running && window().pollEvent(event))
    {
        switch (event.type)
        {
        case sf::Event::Closed:
            m_running = false;
            break;
        case sf::Event::KeyPressed:
            handleKeyPressed(event);
            break;
        case sf::Event::KeyReleased:
            m_ctrl = false;
            break;
        case sf::Event::MouseButtonPressed:
        case sf::Event::MouseButtonReleased:
            handleMouseButton(event);
            break;
        case sf::Event::Resized:
            sf::FloatRect(0.0f, 0.0f, event.size.width, event.size.height);
            window().setView(sf::View(sf::FloatRect(0.0f, 0.0f, event.size.width, event.size.height)));
            break;
        default:
            break;
        }
    }
}
