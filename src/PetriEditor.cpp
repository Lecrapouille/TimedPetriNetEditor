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
#include "Renderer/Arrow.hpp"
#include "utils/Utils.hpp"
#include "utils/KeyBindings.hpp"
#include "portable-file-dialogs.h"
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
    // Format for exporting
    m_exports = {
        { "Julia", { "Julia", {".jl"}, [](PetriNet const& pn, std::string const& file) -> bool { return pn.exportToJulia(file); } } },
        { "Codesys", { "Codesys", {".codesys.xml"}, [](PetriNet const& pn, std::string const& file) -> bool { return pn.exportToCodesys(file); } } },
        { "Symfony", { "Symfony", {".yaml"}, [](PetriNet const& pn, std::string const& file) -> bool { return pn.exportToSymfony(file); } } },
        { "Draw.io", { "Draw.io", {".drawio.xml"}, [](PetriNet const& pn, std::string const& file) -> bool { return pn.exportToDrawIO(file); } } },
        { "Graphviz", { "Graphviz", {".gv", ".dot"}, [](PetriNet const& pn, std::string const& file) -> bool { return pn.exportToGraphviz(file); } } },
        { "PN-Editor", { "PN-Editor", {".pns", ".pnl", ".pnk", ".pnkp"}, [](PetriNet const& pn, std::string const& file) -> bool { return pn.exportToPNEditor(file); } } },
        { "Petri-LaTeX", { "Petri-LaTeX", {".tex"}, [&](PetriNet const& pn, std::string const& file) -> bool
            {
                sf::Vector2f figure(15.0f, 15.0f); // PDF figure size
                sf::Vector2f scale(figure.x / float(m_render_texture.getSize().x),
                                   figure.y / float(m_render_texture.getSize().y));
                return pn.exportToPetriLaTeX(file, scale.x, scale.y);
            }
        } },
        { "Grafcet-LaTeX", { "Grafcet-LaTeX", {".tex"}, [](PetriNet const& pn, std::string const& file) -> bool { return pn.exportToGrafcetLaTeX(file); } } },
        { "C++", { "C++", {".hpp", ".h", ".hh", ".h++"}, [](PetriNet const& pn, std::string const& file) -> bool { return pn.exportToCpp(file); } } },
    };

    // Reserve initial memory for animated tokens
    m_animated_tokens.reserve(128u);

    // Precompute SFML struct for drawing text (places and transitions)
    if (!m_font.loadFromFile(data_path("font.ttf")))
    {
        std::cerr << "Could not load font file ..." << std::endl;
        exit(1);
    }

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

    // Caption for Places and Transitions
    m_text_caption.setFont(m_font);
    m_text_caption.setCharacterSize(CAPTION_FONT_SIZE);
    m_text_caption.setFillColor(sf::Color::Black);

    // Number of Tokens
    m_text_token.setFont(m_font);
    m_text_token.setCharacterSize(TOKEN_FONT_SIZE);
    m_text_token.setFillColor(sf::Color::Black);

    switch (m_petri_net.type())
    {
    case PetriNet::Type::Petri:
        m_message_bar.setInfo("Welcome to Petri net editor! Type H key for help.");
        break;
    case PetriNet::Type::TimedPetri:
        m_message_bar.setInfo("Welcome to timed Petri net editor! Type H key for help.");
        break;
    case PetriNet::Type::TimedGraphEvent:
        m_message_bar.setInfo("Welcome to timed graph event editor! Type H key for help.");
        break;
    case PetriNet::Type::GRAFCET:
        m_message_bar.setInfo("Welcome to GRAFCET editor! Type H key for help.");
        break;
    default:
        m_message_bar.setInfo("Welcome! Type H key for help.");
        break;
    }

    m_view = m_render_texture.getDefaultView();
}

//------------------------------------------------------------------------------
PetriEditor::PetriEditor(Application& application, PetriNet& net, std::string const& file)
    : PetriEditor(application, net)
{
    load(file);
}

//------------------------------------------------------------------------------
void PetriEditor::onConnected(int /*rc*/)
{
    std::string const pid(std::to_string(getpid()));
    std::cout << "Petri net editor " << pid << " connected to MQTT broker"
              << std::endl;

    unsubscribe(m_mqtt_topic);
    m_mqtt_topic = "pneditor-" + pid + "/" + m_petri_net.name();
    std::string message("\nYou can publish your MQTT commands to the Petri net editor ");
    m_message_bar.append(message + " the topic '" + m_mqtt_topic + "'");
    subscribe(m_mqtt_topic, MQTT::QoS::QoS0);
}

//------------------------------------------------------------------------------
// TBD: topic "editor/petri.json/P1" and message "=4" or "+2" or "-1"
// topic "editor/petri.json/T1" and message "1" or "0".
// "editor/petri.json and message "T1:T2;T3"
void PetriEditor::onMessageReceived(const struct mosquitto_message& msg)
{
    const char* payload = static_cast<char*>(msg.payload);
    if (payload[0] == 'T')
    {
        std::cout << "MQTT message 'T'" << std::endl;
        for (auto& t: m_petri_net.transitions())
        {
            const bool b(payload[t.id + 1u]);
            std::cout << t.key << ": " << b << std::endl;
            t.receptivity = b;
        }
    }
}

//------------------------------------------------------------------------------
void PetriEditor::clear()
{
    m_simulating = false;
    m_petri_net.clear();
    m_animated_tokens.clear();
}

//------------------------------------------------------------------------------
bool PetriEditor::load()
{
    if (!m_simulating)
    {
        pfd::open_file manager("Choose the Petri file to load", "",
                                { "JSON files", "*.json" });
        std::vector<std::string> files = manager.result();
        if (files.empty())
        {
            m_message_bar.setError("No selected file for loading");
            return false;
        }

        if (!load(files[0]))
            return false;

        // Connect to the MQTT broker
        if (!connect(MQTT_BROKER_ADDR, MQTT_BROKER_PORT))
        {
            m_message_bar.setError("Failed connecting to MQTT broker");
            return false;
        }

        return true;
    }
    else
    {
        m_message_bar.setError("Cannot load during the simulation!");
        return false;
    }
}

//------------------------------------------------------------------------------
bool PetriEditor::load(std::string const& file)
{
    m_petri_filename = file;
    if (!m_petri_net.load(m_petri_filename))
    {
        m_message_bar.setError(m_petri_net.message());
        m_petri_net.clear();
        return false;
    }

    m_message_bar.setInfo("Loaded with success the Petri net file '" + file + "'");
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

    m_render_texture.setView(
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
    m_render_texture.draw(t);
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
    // In graph event we "compress" the graph by not displaying places.
    if (m_petri_net.type() == PetriNet::Type::TimedGraphEvent)
        return ;

    const float x = place.x;
    const float y = place.y;

    // Draw the place
    m_shape_place.setPosition(sf::Vector2f(x, y));
    m_shape_place.setFillColor(FILL_COLOR(alpha));
    m_render_texture.draw(m_shape_place);

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
            m_render_texture.draw(sf::CircleShape(m_shape_token));
        }
        else if (place.tokens == 2u)
        {
            m_shape_token.setPosition(sf::Vector2f(x - d, y));
            m_render_texture.draw(sf::CircleShape(m_shape_token));

            m_shape_token.setPosition(sf::Vector2f(x + d, y));
            m_render_texture.draw(sf::CircleShape(m_shape_token));
        }
        else if (place.tokens == 3u)
        {
            m_shape_token.setPosition(sf::Vector2f(x, y - r));
            m_render_texture.draw(sf::CircleShape(m_shape_token));

            m_shape_token.setPosition(sf::Vector2f(x - d, y + d));
            m_render_texture.draw(sf::CircleShape(m_shape_token));

            m_shape_token.setPosition(sf::Vector2f(x + d, y + d));
            m_render_texture.draw(sf::CircleShape(m_shape_token));
        }
        else if ((place.tokens == 4u) || (place.tokens == 5u))
        {
            if (place.tokens == 5u)
            {
                d = r + 3.0f;
                m_shape_token.setPosition(sf::Vector2f(x, y));
                m_render_texture.draw(sf::CircleShape(m_shape_token));
            }

            m_shape_token.setPosition(sf::Vector2f(x - d, y - d));
            m_render_texture.draw(sf::CircleShape(m_shape_token));

            m_shape_token.setPosition(sf::Vector2f(x + d, y - d));
            m_render_texture.draw(sf::CircleShape(m_shape_token));

            m_shape_token.setPosition(sf::Vector2f(x - d, y + d));
            m_render_texture.draw(sf::CircleShape(m_shape_token));

            m_shape_token.setPosition(sf::Vector2f(x + d, y + d));
            m_render_texture.draw(sf::CircleShape(m_shape_token));
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
    m_render_texture.draw(m_shape_transition);

    // Draw the caption
    draw(m_text_caption, transition.caption, transition.x,
         transition.y - PLACE_RADIUS - CAPTION_FONT_SIZE / 2.0f - 2.0f);
}

//------------------------------------------------------------------------------
void PetriEditor::draw(Arc const& arc, uint8_t alpha)
{
    if (m_petri_net.type() == PetriNet::Type::TimedGraphEvent)
    {
        // In graph event we "compress" the graph by not displaying places.
        if (arc.from.type == Node::Type::Place)
            return ;
        // So we draw arrows between Transition to Transition using the
        // property of graph event: there is only one place.
        assert(arc.to.arcsOut.size() == 1u && "malformed graph event");
        Node& next = arc.to.arcsOut[0]->to;
        Arrow arrow(arc.from.x, arc.from.y, next.x, next.y, alpha);
        m_render_texture.draw(arrow);

        // Print the timing / tokens
        float x = arc.from.x + (next.x - arc.from.x) / 2.0f;
        float y = arc.from.y + (next.y - arc.from.y) / 2.0f;
        std::stringstream stream;
        stream << std::fixed << std::setprecision(2) << arc.duration << ", "
               << arc.to.key << "(" << reinterpret_cast<Place&>(arc.to).tokens << ")";
        draw(m_text_token, stream.str(), x, y - 15);
    }
    else
    {
        // Transition -> Place
        Arrow arrow(arc.from.x, arc.from.y, arc.to.x, arc.to.y, alpha);
        m_render_texture.draw(arrow);

        if ((arc.from.type == Node::Type::Transition) &&
            (m_petri_net.type() == PetriNet::Type::TimedPetri))
        {
            // Print the timing for timed petri net
            float x = arc.from.x + (arc.to.x - arc.from.x) / 2.0f;
            float y = arc.from.y + (arc.to.y - arc.from.y) / 2.0f;
            draw(m_text_token, arc.duration, x, y - 15);
        }
    }
}

//------------------------------------------------------------------------------
void PetriEditor::onDraw()
{
    onDrawIMGui();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.f, 0.f});
    ImGui::Begin("editor"); // FIXME editer plusieurs Petri

    const ImVec2 region_size(ImGui::GetContentRegionAvail());
    m_render_texture.create(static_cast<unsigned>(region_size.x),
                            static_cast<unsigned>(region_size.y));
    m_render_texture.clear(sf::Color::White);
    sf::FloatRect r(0.0f, 0.0f, float(region_size.x), float(region_size.y));
    m_view.reset(r);
    m_render_texture.setView(m_view);

    renderScene(m_render_texture);
    m_render_texture.display();

    ImGui::ImageButton(m_render_texture, 0);
    m_is_hovered = ImGui::IsItemHovered();

    // move the planning scene around by dragging mouse Right-click
    //if (is_hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Right))
    //{
    //    view_move_xy_.x -= io.MouseDelta.x;
    //    view_move_xy_.y -= io.MouseDelta.y;
    //}

    // Update the current mouse position in planning scene panel
    const ImVec2 p(ImGui::GetCursorScreenPos());
    const ImVec2 origin(p.x /*- view_move_xy_.*/, p.y /*- view_move_xy_.y*/);
    ImGuiIO& io = ImGui::GetIO();
    sf::Vector2i mouse_pos_in_canvas(io.MousePos.x - origin.x,
                                     io.MousePos.y - origin.y + region_size.y);
    m_mouse = m_render_texture.mapPixelToCoords(mouse_pos_in_canvas);

    ImGui::End();
    ImGui::PopStyleVar();
}

//------------------------------------------------------------------------------
void PetriEditor::renderScene(sf::RenderTexture& r)
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
        r.draw(arrow);
    }

    // Draw the selection

    // Update the node the user is moving
    for (auto& it: m_selected_modes)
    {
        it->x = m_mouse.x;
        it->y = m_mouse.y;
    }

    // Draw all tokens transiting from Transitions to Places
    for (auto const& at: m_animated_tokens)
    {
        m_shape_token.setPosition(at.x, at.y);
        r.draw(m_shape_token);
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
        r.draw(m_grid);
    }

    // Draw the GUI
    m_message_bar.setSize(r.getSize());
    //r.draw(m_message_bar);
    r.draw(m_entry_box);
}

//------------------------------------------------------------------------------
void PetriEditor::onUpdate(float const dt)
{
    if (m_petri_net.modified)
    {
        m_renderer.setTitle(m_title + " **");
    }
    else
    {
        m_renderer.setTitle(m_title);
    }

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
        m_petri_net.getTokens(m_marks);
        m_animated_tokens.clear();
        m_state = STATE_ANIMATING;
        std::cout << current_time() << "Simulation has started!" << std::endl;
        break;

    case STATE_ENDING:
        m_message_bar.setInfo("Simulation has ended!");
        std::cout << current_time() << "Simulation has ended!"
                  << std::endl << std::endl;

        // Restore burnt tokens from the simulation
        m_petri_net.setTokens(m_marks);
        m_animated_tokens.clear();
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
            auto& transitions = m_petri_net.shuffle_transitions();

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

                    // Transition source
                    if (trans->isInput())
                    {
                        burning = false;
                        trans->receptivity = false;
                    }
                    else
                    {
                        // Burn tokens on each predecessor Places
                        for (auto& a: trans->arcsIn)
                        {
                            // Burn tokens
                            size_t& tks = a->tokensIn();
                            assert(tks >= tokens);
                            tks = std::min(Settings::maxTokens, tks - tokens);

                            // Invalidate transitions of previous places
                            if (m_petri_net.type() == PetriNet::Type::Petri)
                            {
                                Transition& tr = reinterpret_cast<Transition&>(a->to);
                                tr.receptivity = false;
                            }
                            a->fading.restart();
                        }
                    }

                    // Count the number of tokens for the animation
                    for (auto& a: trans->arcsOut)
                    {
                        a->count = std::min(Settings::maxTokens, a->count + tokens);
                    }
                }
            }
        } while (burning);

        // Create animated tokens with the correct number of tokens they are
        // carrying.
        if (burnt)
        {
            for (auto& a: m_petri_net.arcs()) // FIXME: speedup: trans->arcsOut
            {
                if (a.count > 0u) // number of tokens carried by a single animation
                {
                    std::cout << current_time()
                              << "Transition " << a.from.caption << " burnt "
                              << a.count << " token"
                              << (a.count == 1u ? "" : "s")
                              << std::endl;
                    m_animated_tokens.push_back(AnimatedToken(a, a.count, m_petri_net.type()));
                    a.fading.restart();
                    a.count = 0u;
                }
            }
        }

        // Tokens Transition --> Places are transitioning.
        if (m_animated_tokens.size() > 0u)
        {
            size_t i = m_animated_tokens.size();
            while (i--)
            {
                AnimatedToken& an = m_animated_tokens[i];
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

                    // Transition source. In Petri net we keep using the mouse to
                    // fire source transition to generate a single token by mouse
                    // click while in other mode the transition fires once the
                    // animation ends.
                    if (m_petri_net.type() != PetriNet::Type::Petri)
                    {
                        Transition& t = reinterpret_cast<Transition&>(an.arc.from);
                        if (t.isInput())
                        {
                            t.receptivity = true;
                        }
                    }

                    // Remove it
                    m_animated_tokens[i] = m_animated_tokens[m_animated_tokens.size() - 1u];
                    m_animated_tokens.pop_back();
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
bool PetriEditor::exports(std::string const& format)
{
    if (m_petri_net.isEmpty())
    {
        m_message_bar.setWarning("Cannot export dummy Petri net!");
        return false;
    }

    //if (m_simulating)
    //{
    //    m_message_bar.setError("Cannot export during the simulation!");
    //    return false;
    //}

    if (m_exports.find(format) == m_exports.end())
    {
        m_message_bar.setError("Unknown exporting file format: " + format);
        return false;
    }

    Export& exp = m_exports[format];
    std::string all_extensions;
    std::string c("*");
    for (auto& it: exp.extensions)
    {
        all_extensions += c;
        all_extensions += it;
        c = " *";
    }

    pfd::save_file manager("Choose the " + exp.what + " file to export",
                            "petri-gen" + exp.extensions[0],
                            { exp.what + " file", all_extensions });
    std::string file = manager.result();
    if (file.empty())
    {
        m_message_bar.setError("Unselected file");
        return false;
    }

    m_petri_net.generateArcsInArcsOut();
    bool res = exp.exports(m_petri_net, file);
    if (res)
    {
        m_message_bar.setInfo(
            "Petri net successfully exported as " + exp.what + " file!");
        return true;
    }
    else
    {
        m_message_bar.setError(m_petri_net.message());
        return false;
    }
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
    else if ((event.key.code == KEY_BINDING_RUN_SIMULATION) ||
             (event.key.code == KEY_BINDING_RUN_SIMULATION_ALT))
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
    else if (event.key.code == KEY_BINDING_SAVE_PETRI_NET)
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
    else if (event.key.code == KEY_BINDING_LOAD_PETRI_NET)
        load();
    else if (event.key.code == KEY_BINDING_SCREEN_SHOT)
        screenshot();

    // 'F' key: load a flowshop net from a text file
    else if (event.key.code == KEY_BINDING_LOAD_FLOWSHOP)
    {
        if (!m_simulating)
        {
            pfd::open_file manager("Choose the Petri file to load", "",
                                   { "Flowshop files", "*.flowshop" });
            std::vector<std::string> files = manager.result();
            if (!files.empty())
            {
                if (m_petri_net.importFlowshop(files[0])) // FIXME a merger avec load()
                {
                    m_message_bar.setInfo("Flowshop successfully imported!");
                }
                else
                {
                    m_message_bar.setError(m_petri_net.message());
                }
            }
        }
        else
        {
            m_message_bar.setError("Cannot load during the simulation!");
        }
    }

    // 'C' key: show critical graph (only for event graph)
    else if (event.key.code == KEY_BINDING_SHOW_CRITICAL_CYCLE)
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
    else if (event.key.code == KEY_BINDING_ERASE_PETRI_NET)
    {
        clear();
    }

    // 'M' key: Move the selected node
    else if (event.key.code == KEY_BINDING_MOVE_PETRI_NODE)
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
    else if (event.key.code == KEY_BINDING_ARC_FROM_NODE)
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
    else if ((event.key.code == KEY_BINDING_INCREMENT_TOKENS) ||
             (event.key.code == KEY_BINDING_DECREMENT_TOKENS))
    {
        Node* node = getNode(m_mouse.x, m_mouse.y);
        if ((node != nullptr) && (node->type == Node::Type::Place))
        {
            size_t& tokens = reinterpret_cast<Place*>(node)->tokens;
            if (event.key.code == KEY_BINDING_INCREMENT_TOKENS)
                tokens = std::min(Settings::maxTokens, tokens + 1u);
            else if (tokens > 0u)
                --tokens;
            m_petri_net.modified = true;
        }
    }

    // 'Up' key or 'Down': rotate the transition CW or CCW.
    else if ((event.key.code == KEY_BINDING_ROTATE_CW) ||
             (event.key.code == KEY_BINDING_ROTATE_CCW))
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
    else if (event.key.code == KEY_BINDING_IS_EVENT_GRAPH)
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
    else if (event.key.code == KEY_BINDING_SHOW_HELP)
    {
        m_message_bar.setInfo(PetriEditor::help().str());
    }

    // 'A' for aligning nodes on a grid
    else if (event.key.code == KEY_BINDING_ALIGN_NODES)
    {
        align();
        m_message_bar.setInfo("Nodes have been aligned !");
    }

    // 'D' show grid
    else if (event.key.code == KEY_BINDING_SHOW_GRID)
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
        if (m_petri_net.type() == PetriNet::Type::TimedGraphEvent)
        {
            // With timed event graph we have to add implicit places.
            float px = x + (m_node_from->x - x) / 2.0f;
            float py = y + (m_node_from->y - y) / 2.0f;
            float duration = random(1, 5);
            Place& n = m_petri_net.addPlace(px, py);
            if (!m_petri_net.addArc(*m_node_from, n, duration))
            {
                m_message_bar.setError(m_petri_net.message());
            }
            m_node_from = &n;
        }
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
void PetriEditor::align()
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

    // Edit node titles or edit duration on arcs.
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
void PetriEditor::onHandleInput(sf::Event const& event)
{
    switch (event.type)
    {
    case sf::Event::Closed:
        close();
        break;
    case sf::Event::KeyPressed:
        // Move the view if not editing text
        if (!m_entry_box.hasFocus())
        {
            if (event.key.code == sf::Keyboard::Right)
            {
                m_view.move(10.0f, 0.0f);
                return ;
            }
            else if (event.key.code == sf::Keyboard::Left)
            {
                m_view.move(-10.0f, 0.0f);
                return ;
            }
            else if (event.key.code == sf::Keyboard::Up)
            {
                m_view.move(0.0f, 10.0f);
                return ;
            }
            else if (event.key.code == sf::Keyboard::Down)
            {
                m_view.move(0.0f, -10.0f);
                return ;
            }
        }

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
        if (m_is_hovered)
            handleMouseButton(event);
        break;
    case sf::Event::MouseButtonReleased:
        if (m_is_hovered)
            handleMouseButton(event);
        break;
    case sf::Event::MouseWheelScrolled:
        if (m_is_hovered)
            applyZoom(event.mouseWheelScroll.delta);
        break;
    default:
        break;
    }
}

//------------------------------------------------------------------------------
void PetriEditor::applyZoom(float const delta)
{
    constexpr float factor = 1.1f;
    constexpr float inv_factor = 1.0f / factor;

    if (delta < 0.0f)
    {
        // Factor > 1 makes the view bigger (objects appear smaller)
        m_view.zoom(factor);
        m_zoom_level *= factor;
    }
    else
    {
        // Factor < 1 makes the view smaller (objects appear bigger)
        m_view.zoom(inv_factor);
        m_zoom_level *= inv_factor;
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
       << "  Middle mouse button scroll: zoom/unzoom the view" << std::endl
       << "  " << to_str(KEY_BINDING_ARC_FROM_NODE) << " key: add an arc with the selected place or transition as origin" << std::endl
       << "  " << to_str(KEY_BINDING_DELETE_PETRI_ELEMENT) << " key: remove a place or transition or an arc" << std::endl
       << "  " << to_str(KEY_BINDING_ERASE_PETRI_NET) << " key: clear the whole Petri net" << std::endl
       << "  " << to_str(KEY_BINDING_MOVE_PETRI_NODE) << " key: move the selected place or transition" << std::endl
       << "  " << to_str(KEY_BINDING_ALIGN_NODES) << " key: align nodes" << std::endl
       << "  " << to_str(KEY_BINDING_SHOW_GRID) << " key: show the grid" << std::endl
       << "  " << to_str(KEY_BINDING_INCREMENT_TOKENS) << " key: add a token on the place pointed by the mouse cursor" << std::endl
       << "  " << to_str(KEY_BINDING_DECREMENT_TOKENS) << " key: remove a token on the place pointed by the mouse cursor" << std::endl
       << "  " << to_str(KEY_BINDING_RUN_SIMULATION) << " key: run (start) or stop the simulation" << std::endl
       << "  " << to_str(KEY_BINDING_RUN_SIMULATION_ALT) << " key: run (start) or stop the simulation" << std::endl
       << "  " << to_str(KEY_BINDING_IS_EVENT_GRAPH) << " key: is net an event graph ?" << std::endl
       << "  " << to_str(KEY_BINDING_SHOW_CRITICAL_CYCLE) << " key: show critical circuit" << std::endl
       << "  " << to_str(KEY_BINDING_SAVE_PETRI_NET) << " key: save the Petri net to petri.json file" << std::endl
       << "  " << to_str(KEY_BINDING_LOAD_PETRI_NET) << " key: load the Petri net from petri.json file" << std::endl
       << "  " << to_str(KEY_BINDING_SCREEN_SHOT) << " key: take a screenshot of the view" << std::endl
       << "  " << to_str(sf::Keyboard::Right) << " / " << to_str(sf::Keyboard::Left) << " keys: move the view right/left" << std::endl
       << "  " << to_str(sf::Keyboard::Up) << " / " << to_str(sf::Keyboard::Down) << " keys: move the view up/down" << std::endl
       ;
    return ss;
}

//------------------------------------------------------------------------------
bool PetriEditor::screenshot()
{
    pfd::save_file manager("Choose the PNG file to save the screenshot",
                            "screenshot.png", { "PNG File", "*.png" });
    std::string screenshot_filename = manager.result();
    // Do not call this one because DearImGui stole it
    //if (!m_application.screenshot(screenshot_filename))
    if (!m_render_texture.getTexture().copyToImage().saveToFile(screenshot_filename))
    {
        m_message_bar.setError("Failed to save screenshot to file '" + screenshot_filename + "'");
        return false;
    }
    else
    {
        m_message_bar.setInfo("Screenshot taken as file '" + screenshot_filename + "'");
        return true;
    }
}
