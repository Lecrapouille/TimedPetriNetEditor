//=====================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 Quentin Quadrat <lecrapouille@gmail.com>
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

#include "PetriEditor.hpp"
#include "utils/FileDialogs.hpp"
#include "utils/Arrow.hpp"
#include "utils/Utils.hpp"
#include "Settings.hpp"
#include <iomanip>

//------------------------------------------------------------------------------
PetriEditor::PetriEditor(sf::RenderWindow& renderer, PetriNet& net)
    : GUIStates("Petri Net Editor", renderer),
      m_petri_net(net),
      m_figure_place(PLACE_RADIUS),
      m_figure_token(TOKEN_RADIUS),
      m_figure_trans(sf::Vector2f(TRANS_HEIGHT, TRANS_WIDTH)),
      m_message_bar(m_font)
{
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
    if (!m_font.loadFromFile(DATADIR"/font.ttf"))
    {
        if (!m_font.loadFromFile("data/font.ttf"))
        {
            if (!m_font.loadFromFile("font.ttf"))
            {
                std::cerr << "Could not load font file ..." << std::endl;
                exit(1);
            }
        }
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
    m_mouse = sf::Vector2f(sf::Mouse::getPosition(m_render));

    m_message_bar.setText("Welcome to timed Petri net editor");
}

//------------------------------------------------------------------------------
PetriEditor::~PetriEditor()
{
    m_render.close();
}

//------------------------------------------------------------------------------
void PetriEditor::draw(sf::Text& t, std::string const& str, float const x, float const y)
{
    t.setString(str);
    t.setPosition(x - t.getLocalBounds().width / 2.0f, y - t.getLocalBounds().height);
    m_render.draw(t);
}

//------------------------------------------------------------------------------
void PetriEditor::draw(sf::Text& t, size_t const number, float const x, float const y)
{
    PetriEditor::draw(t, std::to_string(number), x, y);
}

//------------------------------------------------------------------------------
void PetriEditor::draw(sf::Text& t, float const number, float const x, float const y)
{
    std::stringstream stream;
    stream << std::fixed << std::setprecision(2) << number;
    PetriEditor::draw(t, stream.str(), x, y);
}

//------------------------------------------------------------------------------
void PetriEditor::draw(Place const& place, uint8_t alpha)
{
    const float x = place.x;
    const float y = place.y;

    // Draw the place
    m_figure_place.setPosition(sf::Vector2f(x, y));
    m_figure_place.setFillColor(FILL_COLOR(alpha));
    m_render.draw(m_figure_place);

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
            m_render.draw(sf::CircleShape(m_figure_token));
        }
        else if (place.tokens == 2u)
        {
            m_figure_token.setPosition(sf::Vector2f(x - d, y));
            m_render.draw(sf::CircleShape(m_figure_token));

            m_figure_token.setPosition(sf::Vector2f(x + d, y));
            m_render.draw(sf::CircleShape(m_figure_token));
        }
        else if (place.tokens == 3u)
        {
            m_figure_token.setPosition(sf::Vector2f(x, y - r));
            m_render.draw(sf::CircleShape(m_figure_token));

            m_figure_token.setPosition(sf::Vector2f(x - d, y + d));
            m_render.draw(sf::CircleShape(m_figure_token));

            m_figure_token.setPosition(sf::Vector2f(x + d, y + d));
            m_render.draw(sf::CircleShape(m_figure_token));
        }
        else if ((place.tokens == 4u) || (place.tokens == 5u))
        {
            if (place.tokens == 5u)
            {
                d = r + 3.0f;
                m_figure_token.setPosition(sf::Vector2f(x, y));
                m_render.draw(sf::CircleShape(m_figure_token));
            }

            m_figure_token.setPosition(sf::Vector2f(x - d, y - d));
            m_render.draw(sf::CircleShape(m_figure_token));

            m_figure_token.setPosition(sf::Vector2f(x + d, y - d));
            m_render.draw(sf::CircleShape(m_figure_token));

            m_figure_token.setPosition(sf::Vector2f(x - d, y + d));
            m_render.draw(sf::CircleShape(m_figure_token));

            m_figure_token.setPosition(sf::Vector2f(x + d, y + d));
            m_render.draw(sf::CircleShape(m_figure_token));
        }
        else
        {
            draw(m_text_token, place.tokens, x, y);
        }
    }
}

//------------------------------------------------------------------------------
void PetriEditor::draw(Transition const& transition, uint8_t alpha)
{
    // Draw the transition
    m_figure_trans.setPosition(sf::Vector2f(transition.x, transition.y));
    m_figure_trans.setRotation(float(transition.angle));
    m_figure_trans.setFillColor(FILL_COLOR(alpha));
    m_render.draw(m_figure_trans);

    // Draw the caption
    draw(m_text_caption, transition.caption, transition.x,
         transition.y - PLACE_RADIUS - CAPTION_FONT_SIZE / 2.0f - 2.0f);
}

//------------------------------------------------------------------------------
void PetriEditor::draw(Arc const& arc, uint8_t alpha)
{
    // Transition -> Place
    Arrow arrow(arc.from.x, arc.from.y, arc.to.x, arc.to.y, alpha);
    m_render.draw(arrow);

    if (arc.from.type == Node::Type::Transition)
    {
        // Draw the timing
        float x = arc.from.x + (arc.to.x - arc.from.x) / 2.0f;
        float y = arc.from.y + (arc.to.y - arc.from.y) / 2.0f;
        draw(m_text_token, arc.duration, x, y - 15);
    }
}

//------------------------------------------------------------------------------
void PetriEditor::draw()
{
    // Draw all Places
    for (auto& p: m_petri_net.places())
    {
        uint8_t alpha = fading(p.fading, p.tokens > 0u, FADING_PERIOD);
        draw(p, alpha);
    }

    // Draw all Transitions
    for (auto& t: m_petri_net.transitions())
    {
        uint8_t alpha = fading(t.fading, false, FADING_PERIOD);
        draw(t, alpha);
    }

    // Draw all Arcs
    for (auto& a: m_petri_net.arcs())
    {
        uint8_t alpha = fading(a.fading, false, FADING_PERIOD);
        draw(a, alpha);
    }

    // Draw the arc the user is creating
    if ((m_arc_from_unknown_node) || (m_node_from != nullptr))
    {
        float x = (m_arc_from_unknown_node) ? m_x : m_node_from->x;
        float y = (m_arc_from_unknown_node) ? m_y : m_node_from->y;
        Arrow arrow(x, y, m_mouse.x, m_mouse.y, 0u);
        m_render.draw(arrow);
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
        m_render.draw(m_figure_token);
        draw(m_text_token, at.tokens, at.x, at.y - 16);
    }

    // Draw critical cycle
    for (auto& a: m_petri_net.m_critical)
    {
        draw(*a, 255);
    }

    // Draw the entry text
    m_message_bar.setSize(m_render.getSize());
    m_render.draw(m_message_bar);
}


//------------------------------------------------------------------------------
void PetriEditor::update(float const dt)
{
    bool burnt = false;
    bool burning = false;
    States state = m_state;

    switch (state)
    {
    case STATE_IDLE:
        if (m_simulating)
        {
            if (m_petri_net.isEmpty())
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
                size_t tokens = trans.canFire(); // [0 .. 1] tokens
                if (tokens > 0u)
                {
                    trans.fading.restart();

                    burning = true; // keep iterating on this loop
                    burnt = true; // At least one place has been fired

                    // Burn a single token on each Places above
                    for (auto& a: trans.arcsIn)
                    {
                        a->tokensIn() -= 1u;
                        a->fading.restart();
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
                    a.fading.restart();
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
                    an.arc.tokensOut() += an.tokens;
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
Node* PetriEditor::getNode(float const x, float const y)
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
void PetriEditor::handleKeyPressed(sf::Event const& event)
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
        m_render.setFramerateLimit(m_simulating ? 30 : 60); // FPS
    }

    // 'S' key: save the Petri net to a JSON file
    else if (event.key.code == sf::Keyboard::S)
    {
        if ((!m_simulating) && (!m_petri_net.isEmpty()))
        {
            pfd::save_file manager("Choose the JSON file to save the Petri net", "~/petri.json",
                                   { "JSON File", "*.json" });
            std::string file = manager.result();
            if (!file.empty())
            {
                if (m_petri_net.save(file))
                {
                    m_message_bar.setText("Petri net has been saved!");
                }
                else
                {
                    m_message_bar.setText("Failed saving the Petri net!");
                }
            }
        }
        else if (m_simulating)
        {
            m_message_bar.setText("Cannot save during the simulation!");
            std::cerr << "Cannot save during the simulation"
                      << std::endl;
        }
        else if (m_petri_net.isEmpty())
        {
            m_message_bar.setText("Cannot save empty Petri net!");
            std::cerr << "Cannot save empty Petri net"
                      << std::endl;
        }
    }

    // 'O' key: load the Petri net to a JSON file
    else if (event.key.code == sf::Keyboard::O)
    {
        if (!m_simulating)
        {
            pfd::open_file manager("Choose the Petri file to load", "",
                                   { "JSON Files", "*.json" });
            std::vector<std::string> files = manager.result();
            if (!files.empty())
            {
                if (m_petri_net.load(files[0]))
                {
                    m_message_bar.setText("Loaded with success the Petri net!");
                }
                else
                {
                    m_message_bar.setText("Failed loading the Petri net!");
                    m_petri_net.reset();
                }
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
        if ((!m_simulating) && (!m_petri_net.isEmpty()))
        {
            pfd::save_file manager("Choose the C++ header file to export as Grafcet", "~/Grafcet-gen.hpp",
                                   { "C++ Header File", "*.hpp *.h *.hh *.h++" });
            std::string file = manager.result();
            if (!file.empty())
            {
                if (m_petri_net.exportToCpp(file, "generated"))
                {
                    m_message_bar.setText("The Petri net has successfully exported as grafcet as C++ header file!");
                }
                else
                {
                    m_message_bar.setText("Could not export the Petri net to C++ header file!");
                }
            }
        }
        else if (m_simulating)
        {
            m_message_bar.setText("Cannot export during the simulation!");
            std::cerr << "Cannot export during the simulation"
                      << std::endl;
        }
        else if (m_petri_net.isEmpty())
        {
            m_message_bar.setText("Cannot export empty Petri net!");
            std::cerr << "Cannot export empty Petri net"
                      << std::endl;
        }
    }

     // 'J' key: save the Petri net as graph event in a Julia script file
    else if (event.key.code == sf::Keyboard::J)
    {
        if ((!m_simulating) && (!m_petri_net.isEmpty()))
        {
            pfd::save_file manager("Choose the Julia file to export as graph event", "~/GraphEvent-gen.jl",
                                   { "Julia File", "*.jl" });
            std::string file = manager.result();
            if (!file.empty())
            {
                if (m_petri_net.exportToJulia(file))
                {
                    m_message_bar.setText("The Petri net has successfully exported as graph event as Julia file!");
                }
                else
                {
                    m_message_bar.setText("Could not export the Petri net to Julia file!");
                }
            }
        }
        else if (m_simulating)
        {
            m_message_bar.setText("Cannot export during the simulation!");
            std::cerr << "Cannot export during the simulation"
                      << std::endl;
        }
        else if (m_petri_net.isEmpty())
        {
            m_message_bar.setText("Cannot export empty Petri net!");
            std::cerr << "Cannot export empty Petri net"
                      << std::endl;
        }
    }

    // 'C' key: show critical graph (only for event graph)
    else if (event.key.code == sf::Keyboard::C)
    {
        m_simulating = false;
        if (!m_petri_net.showCriticalCycle())
        {
            m_message_bar.setText("Failed to show critical cycle");
        }
    }

    // 'Z' key: erase the Petri net
    else if (event.key.code == sf::Keyboard::Z)
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
    else if (event.key.code == sf::Keyboard::Delete)
    {
        Node* node = getNode(m_mouse.x, m_mouse.y);
        if (node != nullptr)
            m_petri_net.removeNode(*node);
    }

    // FIXME TEMPORARY
    else if (event.key.code == sf::Keyboard::W)
    {
        PetriNet pn;
        m_petri_net.toCanonicalForm(pn);
        m_petri_net = pn;
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
void PetriEditor::handleArcOrigin()
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
void PetriEditor::handleArcDestination()
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
void PetriEditor::handleMouseButton(sf::Event const& event)
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
void PetriEditor::handleInput()
{
    sf::Event event;
    m_mouse = sf::Vector2f(sf::Mouse::getPosition(m_render));

    while (m_running && m_render.pollEvent(event))
    {
        switch (event.type)
        {
        case sf::Event::Closed:
            m_running = false;
            break;
        case sf::Event::KeyPressed:
            m_petri_net.m_critical.clear();
            handleKeyPressed(event);
            break;
        case sf::Event::KeyReleased:
            m_ctrl = false;
            break;
        case sf::Event::MouseButtonPressed:
        case sf::Event::MouseButtonReleased:
            m_petri_net.m_critical.clear();
            handleMouseButton(event);
            break;
        case sf::Event::Resized:
            sf::FloatRect(0.0f, 0.0f, float(event.size.width), float(event.size.height));
            m_render.setView(sf::View(sf::FloatRect(0.0f, 0.0f, float(event.size.width), float(event.size.height))));
            break;
        default:
            break;
        }
    }
}