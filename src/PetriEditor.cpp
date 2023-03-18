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

#include "PetriEditor.hpp"
#include "utils/FileDialogs.hpp"
#include "utils/Arrow.hpp"
#include "utils/Utils.hpp"
#include "utils/KeyBindings.hpp"
#include <iomanip>

//------------------------------------------------------------------------------
PetriEditor::PetriEditor(Application& application, PetriNet& net)
    : Application::GUI(application, "Editor", sf::Color::White),
      m_petri_net(net),
      m_shape_place(PLACE_RADIUS),
      m_shape_token(TOKEN_RADIUS),
      m_shape_transition(sf::Vector2f(TRANS_HEIGHT, TRANS_WIDTH)),
      m_message_bar(m_font),
      m_entry_box(m_font),
      m_grid(application.bounds())
{
    // Reserve initial memory for animated tokens
    m_animations.reserve(128u);

    // Precompute an unique SFML shape for drawing all places
    m_shape_place.setOrigin(sf::Vector2f(m_shape_place.getRadius(),
                                         m_shape_place.getRadius()));
    m_shape_place.setFillColor(sf::Color::White);
    m_shape_place.setOutlineThickness(2.0f);
    m_shape_place.setOutlineColor(OUTLINE_COLOR);

    // Precompute an unique SFML shape for drawing all tokens
    m_shape_token.setOrigin(sf::Vector2f(m_shape_token.getRadius(),
                                         m_shape_token.getRadius()));
    m_shape_token.setFillColor(sf::Color::Black);

    // Precompute an unique SFML shape for drawing all transitions
    m_shape_transition.setOrigin(m_shape_transition.getSize().x / 2,
                                 m_shape_transition.getSize().y / 2);
    m_shape_transition.setFillColor(sf::Color::White);
    m_shape_transition.setOutlineThickness(2.0f);
    m_shape_transition.setOutlineColor(OUTLINE_COLOR);

    // Precompute SFML struct for drawing text (places and transitions)
    if (!m_font.loadFromFile(data_path("font.ttf")))
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
    m_mouse = sf::Vector2f(sf::Mouse::getPosition(m_renderer));

    switch (m_petri_net.type())
    {
    case PetriNet::Type::TimedPetri:
        m_message_bar.setInfo("Welcome to timed Petri net editor! Type H key for help.");
        break;
    case PetriNet::Type::Petri:
        m_message_bar.setInfo("Welcome to Petri net editor! Type H key for help.");
        break;
    case PetriNet::Type::GRAFCET:
        m_message_bar.setInfo("Welcome to GRAFCET editor! Type H key for help.");
        break;
    default:
        m_message_bar.setInfo("Welcome! Type H key for help.");
        break;
    }
}

//------------------------------------------------------------------------------
PetriEditor::PetriEditor(Application& application, PetriNet& net, std::string const& file)
    : PetriEditor(application, net)
{
    load(file);
}

//------------------------------------------------------------------------------
bool PetriEditor::load(std::string const& file)
{
    m_petri_filename = file;
    if (!m_petri_net.load(m_petri_filename))
    {
        m_message_bar.setError(m_petri_net.message());
        m_petri_net.reset();
        return false;
    }

    m_message_bar.setInfo("Loaded with success the Petri net!");
    m_title = m_petri_filename;
    m_petri_net.modified = false;

    // Find bounds of the net to place the view
    sf::Vector2f m(800.0f, 600.0f);
    sf::Vector2f M(800.0f, 600.0f);
    for (auto& p: m_petri_net.places())
    {
        m.x = std::min(m.x, p.x);
        m.y = std::min(m.y, p.y);
        M.x = std::max(M.x, p.x);
        M.y = std::max(M.y, p.y);
    }

    // Draw all Transitions
    for (auto& t: m_petri_net.transitions())
    {
        m.x = std::min(m.x, t.x);
        m.y = std::min(m.y, t.y);
        M.x = std::max(M.x, t.x);
        M.y = std::max(M.y, t.y);
    }

    m_renderer.setView(
        sf::View(sf::Vector2f((M.x - m.x) / 2.0f, (M.y - m.y) / 2.0f),
                    sf::Vector2f(M.x - m.x + 2.0f * TRANS_WIDTH, M.y - m.y +
                                2.0f * TRANS_WIDTH)));
    sf::FloatRect r(m.x - TRANS_WIDTH, m.y - TRANS_WIDTH,
                    M.x + TRANS_WIDTH, M.y + TRANS_WIDTH);
    m_grid.resize(r);
    return true;
}

//------------------------------------------------------------------------------
bool PetriEditor::save(bool const force)
{
    // Open the file manager GUI when forced or when the Petri net was never
    // loaded from a file.
    if (force || m_petri_filename.empty())
    {
        pfd::save_file manager("Choose the JSON file to save the Petri net",
                               "petri.json", { "JSON File", "*.json" });
        m_petri_filename = manager.result();
    }

    // The user has cancel the file manager ? Ok save anyway the Petri net as
    // temporary file.
    if (m_petri_filename.empty())
    {
        m_petri_filename = tmpPetriFile();
    }

    // Save the net. In case of success display the status on the GUI or on the
    // console.
    if (m_petri_net.save(m_petri_filename))
    {
        std::string msg = "Petri net has been saved at " + m_petri_filename;
        if (!force)
        {
            m_message_bar.setInfo(msg);
        }
        m_title = m_petri_filename;
        m_petri_net.modified = false;
        return true;
    }
    // Failed saving. Force opening the file manager next time by clearing its
    // name.
    else
    {
        if (!force)
        {
            m_message_bar.setError(m_petri_net.message());
        }
        m_petri_filename.clear();
        return false;
    }
}

//------------------------------------------------------------------------------
void PetriEditor::close()
{
    if (m_petri_net.modified)
    {
        if (!save(true))
            return ;
    }
    m_renderer.close();
}

//------------------------------------------------------------------------------
void PetriEditor::draw(sf::Text& t, std::string const& str, float const x, float const y)
{
    t.setString(str);
    t.setPosition(x - t.getLocalBounds().width / 2.0f, y - t.getLocalBounds().height);
    m_renderer.draw(t);
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
    m_shape_place.setPosition(sf::Vector2f(x, y));
    m_shape_place.setFillColor(FILL_COLOR(alpha));
    m_renderer.draw(m_shape_place);

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
            m_shape_token.setPosition(sf::Vector2f(x, y));
            m_renderer.draw(sf::CircleShape(m_shape_token));
        }
        else if (place.tokens == 2u)
        {
            m_shape_token.setPosition(sf::Vector2f(x - d, y));
            m_renderer.draw(sf::CircleShape(m_shape_token));

            m_shape_token.setPosition(sf::Vector2f(x + d, y));
            m_renderer.draw(sf::CircleShape(m_shape_token));
        }
        else if (place.tokens == 3u)
        {
            m_shape_token.setPosition(sf::Vector2f(x, y - r));
            m_renderer.draw(sf::CircleShape(m_shape_token));

            m_shape_token.setPosition(sf::Vector2f(x - d, y + d));
            m_renderer.draw(sf::CircleShape(m_shape_token));

            m_shape_token.setPosition(sf::Vector2f(x + d, y + d));
            m_renderer.draw(sf::CircleShape(m_shape_token));
        }
        else if ((place.tokens == 4u) || (place.tokens == 5u))
        {
            if (place.tokens == 5u)
            {
                d = r + 3.0f;
                m_shape_token.setPosition(sf::Vector2f(x, y));
                m_renderer.draw(sf::CircleShape(m_shape_token));
            }

            m_shape_token.setPosition(sf::Vector2f(x - d, y - d));
            m_renderer.draw(sf::CircleShape(m_shape_token));

            m_shape_token.setPosition(sf::Vector2f(x + d, y - d));
            m_renderer.draw(sf::CircleShape(m_shape_token));

            m_shape_token.setPosition(sf::Vector2f(x - d, y + d));
            m_renderer.draw(sf::CircleShape(m_shape_token));

            m_shape_token.setPosition(sf::Vector2f(x + d, y + d));
            m_renderer.draw(sf::CircleShape(m_shape_token));
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
    m_shape_transition.setPosition(sf::Vector2f(transition.x, transition.y));
    m_shape_transition.setRotation(float(transition.angle));
    if ((m_petri_net.type() == PetriNet::Type::Petri) &&
        (transition.isValidated()))
    {
        m_shape_transition.setFillColor(sf::Color::Green);
    }
    else if (transition.isEnabled())
    {
        m_shape_transition.setFillColor(sf::Color(255, 165, 0));
    }
    else
    {
        m_shape_transition.setFillColor(FILL_COLOR(alpha));
    }
    m_renderer.draw(m_shape_transition);

    // Draw the caption
    draw(m_text_caption, transition.caption, transition.x,
         transition.y - PLACE_RADIUS - CAPTION_FONT_SIZE / 2.0f - 2.0f);
}

//------------------------------------------------------------------------------
void PetriEditor::draw(Arc const& arc, uint8_t alpha)
{
    // Transition -> Place
    Arrow arrow(arc.from.x, arc.from.y, arc.to.x, arc.to.y, alpha);
    m_renderer.draw(arrow);

    if ((arc.from.type == Node::Type::Transition) &&
        (m_petri_net.type() == PetriNet::Type::TimedPetri))
    {
        // Draw the timing
        float x = arc.from.x + (arc.to.x - arc.from.x) / 2.0f;
        float y = arc.from.y + (arc.to.y - arc.from.y) / 2.0f;
        draw(m_text_token, arc.duration, x, y - 15);
    }
}

//------------------------------------------------------------------------------
void PetriEditor::onDraw()
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
        m_renderer.draw(arrow);
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
        m_shape_token.setPosition(at.x, at.y);
        m_renderer.draw(m_shape_token);
        draw(m_text_token, at.tokens, at.x, at.y - 16);
    }

    // Draw critical cycle
    for (auto& a: m_marked_arcs)
    {
        draw(*a, 255); // FIXME: m_marked_arcs_color
    }

    // Show the grid
    if (m_grid.show)
    {
        m_renderer.draw(m_grid);
    }

    // Draw the GUI
    m_message_bar.setSize(m_renderer.getSize());
    m_renderer.draw(m_message_bar);
    m_renderer.draw(m_entry_box);
}

//------------------------------------------------------------------------------
void PetriEditor::onUpdate(float const dt)
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
                m_message_bar.setWarning(
                    "Petri net is empty. Simulation request ignored!");
                m_simulating = false;
            }
            else
            {
                m_state = STATE_STARTING;
            }
        }
        break;

    case STATE_STARTING:
        if (m_petri_net.type() == PetriNet::Type::Petri)
        {
            m_message_bar.setInfo(
                "Simulation has started!\n"
                "  Click on transitions for firing!\n"
                "  Press the key '+' on Places for adding tokens\n"
                "  Press the key '-' on Places for removing tokens");
        }
        else
        {
            m_message_bar.setInfo("Simulation has started!");
        }
        m_petri_net.generateArcsInArcsOut();
        m_petri_net.resetReceptivies();
        m_petri_net.shuffle_transitions(true);
        m_petri_net.backupMarks();
        m_animations.clear();
        m_state = STATE_ANIMATING;
        std::cout << current_time() << "Simulation has started!" << std::endl;
        break;

    case STATE_ENDING:
        m_message_bar.setInfo("Simulation has ended!");
        std::cout << current_time() << "Simulation has ended!"
                  << std::endl << std::endl;

        // Restore burnt tokens from the simulation
        m_petri_net.restoreMarks();
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
            // Randomize the order of fired transition.
            // TODO: filter the list to speed up ?
            auto const& transitions = m_petri_net.shuffle_transitions();

            burning = false;
            for (auto& trans: transitions)
            {
                // The theory would burn the maximum possibe of tokens that
                // we can in a single action but we can also try to burn tokens
                // one by one and randomize the transitions.
                size_t tokens = (Settings::firing == Settings::Fire::OneByOne)
                                ? size_t(trans->canFire()) // [0 .. 1] tokens
                                : trans->howManyTokensCanBurnt(); // [0 .. N] tokens
                if (tokens > 0u)
                {
                    assert(tokens <= Settings::maxTokens);
                    trans->fading.restart();

                    burning = true; // keep iterating on this loop
                    burnt = true; // At least one place has been fired

                    // Burn tokens on each predeccessor Places
                    for (auto& a: trans->arcsIn)
                    {
                        // Burn tokens
                        size_t& tks = a->tokensIn();
                        assert(tks >= tokens);
                        tks = std::min(Settings::maxTokens, tks - tokens);

                        // Invalidate the transition
                        if (m_petri_net.type() == PetriNet::Type::Petri)
                        {
                            Transition& tr = reinterpret_cast<Transition&>(a->to);
                            tr.receptivity = false;
                        }
                        a->fading.restart();
                    }

                    // Count the number of tokens for the animation
                    for (auto& a: trans->arcsOut)
                    {
                        a->count = std::min(Settings::maxTokens,
                                            a->count + tokens);
                    }
                }
            }
        } while (burning);

        // Create animated tokens with the correct number of tokens they are
        // carrying.
        if (burnt)
        {
            for (auto& a: m_petri_net.arcs()) // FIXME: speedup: trans.arcsOut
            {
                if (a.count > 0u)
                {
                    std::cout << current_time()
                              << "Transition " << a.from.caption << " burnt "
                              << a.count << " token"
                              << (a.count == 1u ? "" : "s")
                              << std::endl;
                    m_animations.push_back(AnimatedToken(a, a.count, m_petri_net.type()));
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
                              << "Place " << an.arc.to.caption
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
        else if (m_petri_net.type() != PetriNet::Type::Petri)
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
Arc* PetriEditor::getArc(float const /*x*/, float const /*y*/)
{
    return nullptr; // TODO
}

//------------------------------------------------------------------------------
// TODO: iterate backward to allowing selecting the last node inserted
Place* PetriEditor::getPlace(float const x, float const y)
{
    for (auto& p: m_petri_net.places())
    {
        float d2 = (p.x - x) * (p.x - x) + (p.y - y) * (p.y - y);
        if (d2 < PLACE_RADIUS * PLACE_RADIUS)
        {
            return &p;
        }
    }

    return nullptr;
}

//------------------------------------------------------------------------------
Transition* PetriEditor::getTransition(float const x, float const y)
{
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
Node* PetriEditor::getNode(float const x, float const y)
{
    Node *n = getPlace(x, y);
    if (n != nullptr)
        return n;
    return getTransition(x, y);
}

//------------------------------------------------------------------------------
void PetriEditor::handleKeyPressed(sf::Event const& event)
{
    // Left, right, delete, escape key binding when editing the caption of a node
    if (m_entry_box.hasFocus())
    {
        m_entry_box.onKeyPressed(event.key, m_petri_net.modified);
        return ;
    }

    // Escape key: quit the application.
    if (event.key.code == KEY_BINDIND_QUIT_APPLICATION)
    {
        close();
    }

    // Left or right Control key pressed: memorize the state
    if ((event.key.code == sf::Keyboard::LControl) ||
        (event.key.code == sf::Keyboard::RControl))
    {
        m_ctrl = true;
    }

    // Left or right Shift key pressed: memorize the state
    if ((event.key.code == sf::Keyboard::LShift) ||
        (event.key.code == sf::Keyboard::RShift))
    {
        m_shift = true;
    }

    // 'R' or SPACE key: Run the animation of the Petri net
    else if ((event.key.code == KEY_BINDIND_RUN_SIMULATION) ||
             (event.key.code == KEY_BINDIND_RUN_SIMULATION_ALT))
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
        m_application.setFramerate(m_simulating ? 30 : 60); // FPS
    }

    // 'S' key: save the Petri net to a JSON file
    else if (event.key.code == KEY_BINDIND_SAVE_PETRI_NET)
    {
        if (m_simulating)
        {
            m_message_bar.setError("Cannot save during the simulation!");
        }
        else if (m_petri_net.isEmpty())
        {
            m_message_bar.setError("Cannot save empty Petri net!");
        }
        else
        {
            // Save or save-as
            save(m_shift);
        }
    }

    // 'O' key: load the Petri net from a JSON file
    else if (event.key.code == KEY_BINDIND_LOAD_PETRI_NET)
    {
        if (!m_simulating)
        {
            pfd::open_file manager("Choose the Petri file to load", "",
                                   { "JSON files", "*.json" });
            std::vector<std::string> files = manager.result();
            if (!files.empty())
            {
                load(files[0]);
            }
        }
        else
        {
            m_message_bar.setError("Cannot load during the simulation!");
        }
    }

    // 'F1' key: take a screenshot
    else if (event.key.code == KEY_BINDIND_SCREEN_SHOT)
    {
        pfd::save_file manager("Choose the PNG file to save the screenshot",
                                "screenshot.png", { "PNG File", "*.png" });
        std::string screenshot_filename = manager.result();
        if (!m_application.screenshot(screenshot_filename))
        {
            m_message_bar.setError("Failed to save screenshot to file '" + screenshot_filename + "'");
        }
        else
        {
            m_message_bar.setInfo("Screenshot taken as file '" + screenshot_filename + "'");
        }
    }

    // 'X' key: save the Petri net as LaTeX file format
    else if (event.key.code == KEY_BINDIND_EXPORT_PETRI_TO_LATEX)
    {
        if (/*(!m_simulating) &&*/ (!m_petri_net.isEmpty()))
        {
            pfd::save_file manager("Choose the LaTeX file to export",
                                   "LateX-gen.tex",
                                   { "LaTex file", "*.tex" });
            std::string file = manager.result();
            if (!file.empty())
            {
                sf::Vector2f figure(15.0f, 15.0f); // PDF figure size
                sf::Vector2f scale(figure.x / float(m_renderer.getSize().x),
                                   figure.y / float(m_renderer.getSize().y));
                if (m_petri_net.exportToLaTeX(file, scale.x, scale.y))
                {
                    m_message_bar.setInfo(
                        "Petri net successfully exported as LaTeX file!");
                }
                else
                {
                    m_message_bar.setError(m_petri_net.message());
                }
            }
        }
        //else if (m_simulating)
        //{
        //    m_message_bar.setError("Cannot export during the simulation!");
        //}
        else if (m_petri_net.isEmpty())
        {
            m_message_bar.setWarning("Cannot export dummy Petri net!");
        }
    }

    // 'P' key: save the Petri net as graphviz file format
    else if (event.key.code == KEY_BINDIND_EXPORT_PETRI_TO_GRAPHVIZ)
    {
        if (/*(!m_simulating) &&*/ (!m_petri_net.isEmpty()))
        {
            pfd::save_file manager("Choose the Graphviz file to export",
                                   "Graphviz-gen.gv",
                                   { "Graphviz File", "*.gv *.dot" });
            std::string file = manager.result();
            if (!file.empty())
            {
                if (m_petri_net.exportToGraphviz(file))
                {
                    m_message_bar.setInfo(
                        "Petri net successfully exported as Graphviz file!");
                }
                else
                {
                    m_message_bar.setError(m_petri_net.message());
                }
            }
        }
        //else if (m_simulating)
        //{
        //    m_message_bar.setError("Cannot export during the simulation!");
        //}
        else if (m_petri_net.isEmpty())
        {
            m_message_bar.setWarning("Cannot export dummy Petri net!");
        }
    }

    // 'K' key: save the Petri net as graphviz file format
    else if (event.key.code == KEY_BINDIND_EXPORT_PETRI_TO_PNEDITOR)
    {
        if (/*(!m_simulating) &&*/ (!m_petri_net.isEmpty()))
        {
            pfd::save_file manager("Choose the PN-Editor file to export",
                                   "petri-gen.pns",
                                   { "PN-Editor File", "*.pns *.pnl *.pnk *.pnkp" });
            std::string file = manager.result();
            if (!file.empty())
            {
                if (m_petri_net.exportToPNEditor(file))
                {
                    m_message_bar.setInfo(
                        "Petri net successfully exported as Graphviz file!");
                }
                else
                {
                    m_message_bar.setError(m_petri_net.message());
                }
            }
        }
        //else if (m_simulating)
        //{
        //    m_message_bar.setError("Cannot export during the simulation!");
        //}
        else if (m_petri_net.isEmpty())
        {
            m_message_bar.setWarning("Cannot export dummy Petri net!");
        }
    }

    // 'G' key: save the Petri net as grafcet in a C++ header file
    else if (event.key.code == KEY_BINDIND_EXPORT_PETRI_TO_GRAFCET)
    {
        if ((!m_simulating) && (!m_petri_net.isEmpty()))
        {
            pfd::save_file manager("Choose the C++ header file to export as Grafcet",
                                   "Grafcet-gen.hpp",
                                   { "C++ Header File", "*.hpp *.h *.hh *.h++" });
            std::string file = manager.result();
            if (!file.empty())
            {
                if (m_petri_net.exportToCpp(file, "generated"))
                {
                    m_message_bar.setInfo(
                        "The Petri net has successfully exported as grafcet as "
                        "C++ header file!");
                }
                else
                {
                    m_message_bar.setError(m_petri_net.message());
                }
            }
        }
        else if (m_simulating)
        {
            m_message_bar.setError("Cannot export during the simulation!");
        }
        else if (m_petri_net.isEmpty())
        {
            m_message_bar.setWarning("Cannot export dummy Petri net!");
        }
    }

    // 'J' key: save the Petri net as graph event in a Julia script file
    else if (event.key.code == KEY_BINDIND_EXPORT_PETRI_TO_JULIA)
    {
        if ((!m_simulating) && (!m_petri_net.isEmpty()))
        {
            pfd::save_file manager("Choose the Julia file to export as graph event",
                                   "GraphEvent-gen.jl",
                                   { "Julia File", "*.jl" });
            std::string file = manager.result();
            if (!file.empty())
            {
                if (m_petri_net.exportToJulia(file))
                {
                    m_message_bar.setInfo(
                        "The Petri net has successfully exported as graph event"
                        " as Julia file!");
                }
                else
                {
                    m_message_bar.setError(m_petri_net.message());
                    m_marked_arcs = m_petri_net.markedArcs();
                    m_marked_arcs_color = sf::Color::Red;
                }
            }
        }
        else if (m_simulating)
        {
            m_message_bar.setWarning("Cannot export during the simulation!");
        }
        else if (m_petri_net.isEmpty())
        {
            m_message_bar.setWarning("Cannot export dummy Petri net!");
        }
    }

    // 'C' key: show critical graph (only for event graph)
    else if (event.key.code == KEY_BINDIND_SHOW_CRITICAL_CYCLE)
    {
        m_simulating = false;
        if (m_petri_net.findCriticalCycle(m_marked_arcs))
        {
            m_message_bar.setInfo(m_petri_net.message());
            m_marked_arcs_color = sf::Color(255, 165, 0);
            std::cout << m_petri_net.message();
        }
        else
        {
            m_message_bar.setError(m_petri_net.message());
            m_marked_arcs_color = sf::Color::Red;
        }
    }

    // 'Z' key: erase the Petri net
    else if (event.key.code == KEY_BINDIND_ERASE_PETRI_NET)
    {
        m_simulating = false;
        m_petri_net.reset();
        m_animations.clear();
    }

    // 'M' key: Move the selected node
    else if (event.key.code == KEY_BINDIND_MOVE_PETRI_NODE)
    {
        if (m_selected_modes.size() == 0u)
        {
            Node* node = getNode(m_mouse.x, m_mouse.y);
            if (node != nullptr)
            {
                m_selected_modes.push_back(node);
                m_petri_net.modified = true;
            }
        }
        else
        {
            m_selected_modes.clear();
        }
    }

    // 'L' key: create the arc from the selected node
    else if (event.key.code == KEY_BINDIND_ARC_FROM_NODE)
    {
        if ((m_node_from == nullptr) && (!m_arc_from_unknown_node))
            handleArcOrigin();
        else
            handleArcDestination();
    }

    // Delete a node. TODO: implement arc deletion
    else if (event.key.code == KEY_BINDING_DELETE_PETRI_ELEMENT)
    {
        Node* node = getNode(m_mouse.x, m_mouse.y);
        if (node != nullptr)
            m_petri_net.removeNode(*node);
    }

#if 0
    // Uncomment to check graphically generated cannical net
    else if (event.key.code == sf::Keyboard::W)
    {
        if (isEventGraph(m_marked_arcs))
        {
            m_marked_arcs.clear();
            PetriNet pn(m_petri_net.type());
            m_petri_net.toCanonicalForm(pn);
            m_petri_net = pn;
        }
    }
#endif

    // '+' key: increase the number of tokens in the place.
    // '-' key: decrease the number of tokens in the place.
    else if ((event.key.code == KEY_BINDIND_INCREMENT_TOKENS) ||
             (event.key.code == KEY_BINDIND_DECREMENT_TOKENS))
    {
        Node* node = getNode(m_mouse.x, m_mouse.y);
        if ((node != nullptr) && (node->type == Node::Type::Place))
        {
            size_t& tokens = reinterpret_cast<Place*>(node)->tokens;
            if (event.key.code == KEY_BINDIND_INCREMENT_TOKENS)
                tokens = std::min(Settings::maxTokens, tokens + 1u);
            else if (tokens > 0u)
                --tokens;
            m_petri_net.modified = true;
        }
    }

    // 'Up' key or 'Down': rotate the transition CW or CCW.
    else if ((event.key.code == KEY_BINDIND_ROTATE_CW) ||
             (event.key.code == KEY_BINDIND_ROTATE_CCW))
    {
        Transition* transition = getTransition(m_mouse.x, m_mouse.y);
        if (transition != nullptr)
        {
            transition->angle += (
                (event.key.code == sf::Keyboard::PageDown) ? STEP_ANGLE : -STEP_ANGLE);
            transition->angle = transition->angle % 360;
            if (transition->angle < 0)
                transition->angle += 360;
            m_petri_net.modified = true;
        }
    }

    // 'E' key: Is Event graph ?
    else if (event.key.code == KEY_BINDIND_IS_EVENT_GRAPH)
    {
        m_petri_net.generateArcsInArcsOut();
        if (m_petri_net.isEventGraph(m_marked_arcs))
        {
            m_message_bar.setInfo(m_petri_net.message());
        }
        else
        {
            m_message_bar.setError(m_petri_net.message());
        }
    }

    // 'H' key: Show the help
    else if (event.key.code == KEY_BINDIND_SHOW_HELP)
    {
        m_message_bar.setInfo(PetriEditor::help().str());
    }

    // 'A' for aligning nodes on a grid
    else if (event.key.code == KEY_BINDIND_ALIGN_NODES)
    {
        alignElements();
        m_message_bar.setInfo("Nodes have been aligned !");
    }

    // 'D' show grid
    else if (event.key.code == KEY_BINDIND_SHOW_GRID)
    {
        m_grid.show ^= true;
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
                    if (!m_petri_net.addArc(*m_node_from, n, duration))
                    {
                        m_message_bar.setError(m_petri_net.message());
                    }
                    m_node_from = &n;
                }
                else
                {
                    Place& n = m_petri_net.addPlace(x, y);
                    if (!m_petri_net.addArc(*m_node_from, n, duration))
                    {
                        m_message_bar.setError(m_petri_net.message());
                    }
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
    if (!m_petri_net.addArc(*m_node_from, *m_node_to, duration))
    {
        m_message_bar.setError(m_petri_net.message());
    }

    // Reset states
    m_node_from = m_node_to = nullptr;
    m_selected_modes.clear();
    m_arc_from_unknown_node = false;
}

//------------------------------------------------------------------------------
void PetriEditor::alignElements()
{
    const int D = int(TRANS_WIDTH);

    for (auto& transition: m_petri_net.transitions())
    {
        transition.x = transition.x - float(int(transition.x) % D);
        transition.y = transition.y - float(int(transition.y) % D);
    }

    for (auto& place: m_petri_net.places())
    {
        place.x = place.x - float(int(place.x) % D);
        place.y = place.y - float(int(place.y) % D);
    }
}

//------------------------------------------------------------------------------
bool PetriEditor::clickedOnCaption()
{
    static std::string duration;

    for (auto& place: m_petri_net.places())
    {
        if (m_entry_box.canFocusOn(place.caption, nullptr, place.x, place.y, m_mouse))
        {
            return true;
        }
    }

    for (auto& transition: m_petri_net.transitions())
    {
        if (m_entry_box.canFocusOn(transition.caption, nullptr, transition.x, transition.y, m_mouse))
        {
            return true;
        }
    }

    for (auto& arc: m_petri_net.arcs())
    {
        if (arc.from.type == Node::Type::Place)
            continue;

        // Since durations are float, this is more complex: we have to convert
        // float to string and do not convert it back to float because values
        // after the dot makes things wrong. For example removing the last 0 of
        // 2.00 will restore its 0 (+ risk of adding epsilon due to IEEE754 norm)
        const float x = arc.from.x + (arc.to.x - arc.from.x) / 2.0f;
        const float y = arc.from.y + (arc.to.y - arc.from.y) / 2.0f + 2.0f;
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << arc.duration;
        duration = ss.str();
        if (m_entry_box.canFocusOn(duration, &arc.duration, x, y, m_mouse))
        {
            std::cout << "Duration: " << duration << std::endl;
            return true;
        }
    }

    return false;
}

//------------------------------------------------------------------------------
void PetriEditor::handleMouseButton(sf::Event const& event)
{
    m_marked_arcs.clear();

    if (m_entry_box.hasFocus())
    {
        m_entry_box.onMousePressed(m_mouse);
        return ;
    }

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
        else if (clickedOnCaption())
        {
            // Nothing to do
        }
        else
        {
            if (!m_simulating)
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
            else if (m_petri_net.type() == PetriNet::Type::Petri)
            {
                // Click to fire a transition
                Transition* transition = getTransition(m_mouse.x, m_mouse.y);
                if (transition != nullptr)
                {
                    transition->receptivity ^= true;
                }
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
void PetriEditor::onHandleInput()
{
    sf::Event event;
    m_mouse = sf::Vector2f(sf::Mouse::getPosition(m_renderer));

    while (m_renderer.pollEvent(event))
    {
        switch (event.type)
        {
        case sf::Event::Closed:
            close();
            break;
        case sf::Event::KeyPressed:
            m_marked_arcs.clear();
            handleKeyPressed(event);
            break;
        case sf::Event::TextEntered:
            m_entry_box.onTextEntered(event.text.unicode);
            break;
        case sf::Event::KeyReleased:
            m_ctrl = false;
            break;
        case sf::Event::MouseButtonPressed:
            handleMouseButton(event);
            break;
        case sf::Event::MouseButtonReleased:
            handleMouseButton(event);
            break;
        case sf::Event::MouseWheelMoved:
            //zoom += (float(event.mouseWheel.delta) / 10.0f);
            //std::cout << "Zoom: " << zoom << "\n";
            break;
        case sf::Event::Resized:
            {
                sf::FloatRect r(0.0f, 0.0f, float(event.size.width), float(event.size.height));
                m_renderer.setView(sf::View(r));
                m_grid.resize(r);
            }
            break;
        default:
            break;
        }
    }

    if (m_petri_net.modified)
    {
        m_renderer.setTitle(m_title + " **");
    }
    else
    {
        m_renderer.setTitle(m_title);
    }
}

//------------------------------------------------------------------------------
std::stringstream PetriEditor::help()
{
    std::stringstream ss;
    ss << "GUI commands for:" << std::endl
       << "  Left mouse button pressed: add a place" << std::endl
       << "  Right mouse button pressed: add a transition" << std::endl
       << "  Middle mouse button pressed: add an arc with the selected place or transition as origin" << std::endl
       << "  Middle mouse button release: end the arc with the selected place or transition as destination" << std::endl
       << "  " << to_str(KEY_BINDIND_ARC_FROM_NODE) << " key: add an arc with the selected place or transition as origin" << std::endl
       << "  " << to_str(KEY_BINDING_DELETE_PETRI_ELEMENT) << " key: remove a place or transition or an arc" << std::endl
       << "  " << to_str(KEY_BINDIND_ERASE_PETRI_NET) << " key: clear the whole Petri net" << std::endl
       << "  " << to_str(KEY_BINDIND_MOVE_PETRI_NODE) << " key: move the selected place or transition" << std::endl
       << "  " << to_str(KEY_BINDIND_ALIGN_NODES) << " key: align nodes" << std::endl
       << "  " << to_str(KEY_BINDIND_SHOW_GRID) << " key: show the grid" << std::endl
       << "  " << to_str(KEY_BINDIND_INCREMENT_TOKENS) << " key: add a token on the place pointed by the mouse cursor" << std::endl
       << "  " << to_str(KEY_BINDIND_DECREMENT_TOKENS) << " key: remove a token on the place pointed by the mouse cursor" << std::endl
       << "  " << to_str(KEY_BINDIND_RUN_SIMULATION) << " key: run (start) or stop the simulation" << std::endl
       << "  " << to_str(KEY_BINDIND_RUN_SIMULATION_ALT) << " key: run (start) or stop the simulation" << std::endl
       << "  " << to_str(KEY_BINDIND_IS_EVENT_GRAPH) << " key: is net an event graph ?" << std::endl
       << "  " << to_str(KEY_BINDIND_SHOW_CRITICAL_CYCLE) << " key: show critical circuit" << std::endl
       << "  " << to_str(KEY_BINDIND_SAVE_PETRI_NET) << " key: save the Petri net to petri.json file" << std::endl
       << "  " << to_str(KEY_BINDIND_LOAD_PETRI_NET) << " key: load the Petri net from petri.json file" << std::endl
       << "  " << to_str(KEY_BINDIND_EXPORT_PETRI_TO_GRAPHVIZ) << " key: export the Petri net as Graphviz file" << std::endl
       << "  " << to_str(KEY_BINDIND_EXPORT_PETRI_TO_LATEX) << " key: export the Petri net as LaTeX file" << std::endl
       << "  " << to_str(KEY_BINDIND_EXPORT_PETRI_TO_GRAFCET) << " key: export the Petri net as GRAFCET in a C++ header file" << std::endl
       << "  " << to_str(KEY_BINDIND_EXPORT_PETRI_TO_JULIA) << " key: export the Petri net as Julia code" << std::endl
       << "  " << to_str(KEY_BINDIND_EXPORT_PETRI_TO_PNEDITOR) << " key: export the Petri net as for https://gitlab.com/porky11/pn-editor" << std::endl
       << "  " << to_str(KEY_BINDIND_SCREEN_SHOT) << " key: take a screenshot of the view" << std::endl
       ;
    return ss;
}
