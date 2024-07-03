//=============================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2023 Quentin Quadrat <lecrapouille@gmail.com>
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
//=============================================================================

#include "TimedPetriNetEditor/PetriNet.hpp"
#include "TimedPetriNetEditor/Algorithms.hpp"
#include "TimedPetriNetEditor/SparseMatrix.hpp"
#include "Editor/DearImGui/Editor.hpp"
#include "Editor/DearImGui/Drawable.hpp"
#include "Editor/DearImGui/DearUtils.hpp"
#include "Editor/DearImGui/KeyBindings.hpp"
#include "Utils/Utils.hpp"

#ifdef WITH_MQTT
#  ifndef MQTT_BROKER_ADDR
#    error "MQTT_BROKER_ADDR shall be defined"
#  endif
#  ifndef MQTT_BROKER_PORT
#    error "MQTT_BROKER_PORT shall be defined"
#  endif
#endif

namespace tpne {

//! \brief path of the file storing dear imgui widgets. Cannot be placed
//! as member variable of Editor class.
static std::string g_ini_filename = "imgui.ini";

//--------------------------------------------------------------------------
Editor::Editor(size_t const width, size_t const height,
               std::string const& title)
    : Application(width, height, title),
      m_path(GET_DATA_PATH),
      m_simulation(m_net, m_messages),
      m_view(*this)
{
#ifdef __EMSCRIPTEN__
#  define FONT_SIZE 18.0f
#else
#  define FONT_SIZE 13.0f
#endif

    m_messages.setInfo("Path: " + m_path.toString());

    m_states.title = title;

    // Set imgui.ini loading/saving location
    ImGuiIO &io = ImGui::GetIO();
    g_ini_filename = m_path.expand("imgui.ini").c_str();
    io.IniFilename = g_ini_filename.c_str();
    // std::cout << "imgui.ini path: " << io.IniFilename << std::endl;

    // Setup fonts
    io.Fonts->AddFontFromFileTTF(m_path.expand("font.ttf").c_str(), FONT_SIZE);
    reloadFonts();

    // Theme
    ImGui::StyleColorsDark();

#ifdef WITH_MQTT
    initMQTT();
#endif
}

#ifdef WITH_MQTT
//------------------------------------------------------------------------------
bool Editor::initMQTT()
{
    // Load a Petri net using the formalism used for TPNE json files. For example
    // mosquitto_pub -h localhost -t "tpne/load" -m '{ "revision": 3, "type":
    // "Petri net", "nets": [ { "name": "hello world",
    // "places": [ { "id": 0, "caption": "P0", "tokens": 1, "x": 244, "y": 153 },
    // { "id": 1, "caption": "P1", "tokens": 0, "x": 356, "y": 260 } ],
    // "transitions": [ { "id": 0, "caption": "T0", "x": 298, "y": 207, "angle": 0 } ],
    // "arcs": [ { "from": "P0", "to": "T0" }, { "from": "T0", "to": "P1", "duration": 3 }
    // ] } ] }'
    static auto onLoadCommandReceived = [&](const mqtt::Message& msg)
    {
        if (m_simulation.running)
        {
            m_messages.setError("MQTT: cannot load new Petri net while the simulation is still in progress");
            return ;
        }
        const char* message = static_cast<const char*>(msg.payload);

        // To temporary file
        std::string path("/tmp/petri.json");
        std::ofstream file(path);
        file << message;
        file.close();

        // Import the file
        bool shall_springify;
        std::string error = loadFromFile(m_net, path, shall_springify);
        if (error.empty())
        {
            m_messages.setInfo("Loaded with success " + path);
        }
        else
        {
            m_messages.setError(error);
        }
    };

    // Start the simulation for Petri net and GRAFCET.
    // mosquitto_pub -h localhost -t "tpne/start" -m ''
    static auto onStartSimulationCommandReceived = [&](mqtt::Message const& /*msg*/)
    {
        if ((m_net.type() == TypeOfNet::TimedEventGraph) ||
            (m_net.type() == TypeOfNet::TimedPetriNet))
        {
            m_messages.setError(
                "MQTT: Please convert first to non timed net before starting simulation");
            return ;
        }
        toogleStartSimulation();
    };

    // Stop the simulation.
    // mosquitto_pub -h localhost -t "tpne/stop" -m ''
    static auto onStopSimulationCommandReceived = [&](mqtt::Message const& /*msg*/)
    {
        m_simulation.running = false;
        framerate(60);
    };

    // Fire transitions.
    // mosquitto_pub -h localhost -t "tpne/fire" -m '10100'
    static auto onFireTransitionCommandReceived = [&](mqtt::Message const& msg)
    {
        if (!m_simulation.running)
        {
            m_messages.setError("MQTT: The simulation is not running");
            return ;
        }
        const char* message = static_cast<const char*>(msg.payload);
        Net::Transitions& transitions = m_net.transitions();
        size_t i = size_t(msg.payloadlen);
        if (i == transitions.size())
        {
            while (i--)
            {
                transitions[i].receptivity = (message[i] != '0');
            }
        }
        else
        {
            m_messages.setError("MQTT: fire command length does not match number of transitions");
        }
    };

    // Subscription to MQTT messages
    auto onConnected = [&](int rc)
    {
        m_messages.setInfo("Connected to MQTT broker with return code " + std::to_string(rc));
        m_mqtt.subscribe(TOPIC_LOAD, mqtt::QoS::QoS0, onLoadCommandReceived);
        m_mqtt.subscribe(TOPIC_START, mqtt::QoS::QoS0, onStartSimulationCommandReceived);
        m_mqtt.subscribe(TOPIC_STOP, mqtt::QoS::QoS0, onStopSimulationCommandReceived);
        m_mqtt.subscribe(TOPIC_FIRE, mqtt::QoS::QoS0, onFireTransitionCommandReceived);
    };

    // Connection to the MQTT broker
    if (!m_mqtt.connect({"localhost", 1883, std::chrono::seconds(60)}, onConnected))
    {
        m_messages.setError(m_mqtt.error().message());
        return false;
    }

    return true;
}
#endif // WITH_MQTT

//------------------------------------------------------------------------------
void Editor::showStyleSelector()
{
    ImGui::OpenPopup("Theme selector");
    ImGui::SetNextWindowPos(m_states.viewport_center, ImGuiCond_Appearing,
                            ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Theme selector",
                               NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        int idx = theme();
        if (ImGui::Combo("Colors##Selector", &idx, "Dark\0Light\0Classic\0"))
        {
            theme() = ThemeId(idx);
            switch (idx)
            {
            case ThemeId::Dark: ImGui::StyleColorsDark(); break;
            case ThemeId::Light: ImGui::StyleColorsLight(); break;
            case ThemeId::Calssic: ImGui::StyleColorsClassic(); break;
            }
        }
        if (ImGui::Button("OK", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
            m_states.show_theme = false;
        }
        ImGui::EndPopup();
    }
}

//------------------------------------------------------------------------------
void Editor::setSavePath(std::string const& filepath)
{
    // If we do not have exporters for the imported file format, use
    // the default file format: json.
#if 0
    if (getExporter(extension(filepath)) == nullptr)
        m_path_to_save = baseName(filepath) + ".json";
    else
        m_path_to_save = filepath;
#else
    m_path_to_save = baseName(filepath) + ".json";
#endif
}

//------------------------------------------------------------------------------
void Editor::run(Net const& net)
{
    m_net = net;

    // Start the infinite loop
    Application::run();
}

//------------------------------------------------------------------------------
void Editor::run(std::string const& filepath)
{
    // Load Petri net file if passed with command line
    if (!filepath.empty())
    {
        bool shall_springify;
        std::string error = loadFromFile(m_net, filepath, shall_springify);
        if (error.empty())
        {
            m_messages.setInfo("Loaded with success " + filepath);
            setSavePath(filepath);
            if (shall_springify)
            {
                springify();
            }
        }
        else
        {
            m_messages.setError(error);
        }
    }

    // Start the infinite loop
    Application::run();
}

//------------------------------------------------------------------------------
void Editor::onUpdate(float const dt)
{
    if (m_net.modified)
    {
        title(m_states.title + " -- " + m_net.name + " **");
    }
    else
    {
        title(m_states.title + " -- " + m_net.name);
    }

    m_simulation.step(dt);
    m_spring.update();
}

//------------------------------------------------------------------------------
void Editor::onDraw()
{
    ImGui::DockSpaceOverViewport();

    menu();
    console();
    messagebox();
    inspector();
    view();
    //ImGui::ShowDemoWindow();
}

//------------------------------------------------------------------------------
void Editor::view()
{
    static bool open;
    if (!ImGui::Begin("Petri net", &open))
    {
        ImGui::End();
        return;
    }

    m_view.reshape();
    m_view.onHandleInput();
    m_view.drawPetriNet(m_net, m_simulation);
    ImGui::End();
}

//------------------------------------------------------------------------------
void Editor::close()
{
    m_simulation.running = false;
    m_states.do_save_as = m_net.modified;
    m_states.request_quitting = true;
}

//------------------------------------------------------------------------------
void Editor::menu()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New", nullptr, false))
            {
                // TODO
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Open", nullptr, false))
            {
                m_states.do_load = true;
            }
            if (ImGui::BeginMenu("Import from"))
            {
                for (auto const& it: importers())
                {
                    if (ImGui::MenuItem(it.format.c_str(), nullptr, false))
                    {
                        m_states.do_import_from = &it;
                    }
                }
                ImGui::EndMenu();
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Save", nullptr, false))
            {
                if (m_path_to_save == "")
                {
                    m_states.do_save_as = true;
                }
                else
                {
                    std::string error = saveToFile(m_net, m_path_to_save);
                    if (error.empty())
                    {
                        m_messages.setInfo("Saved with success " + m_path_to_save);
                        m_net.modified = false;
                    }
                    else
                    {
                        m_messages.setError(error);
                    }
                }
            }
            if (ImGui::MenuItem("Save as", nullptr, false))
            {
                m_states.do_save_as = true;
            }
            if (ImGui::BeginMenu("Export to"))
            {
                for (auto const& it: exporters())
                {
                    if (ImGui::MenuItem(it.format.c_str(), nullptr, false))
                    {
                        m_states.do_export_to = &it;
                    }
                }
                ImGui::EndMenu();
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Exit", nullptr, false))
            {
                this->close();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Actions"))
        {
            if (ImGui::BeginMenu("Type of net"))
            {
                static int current_type = int(m_net.type());
                ImGui::RadioButton("Petri net", &current_type, 0);
                ImGui::RadioButton("Timed Petri net", &current_type, 1);
                ImGui::RadioButton("Timed event graph", &current_type, 2);
                ImGui::RadioButton("GRAFCET", &current_type, 3);
                switchOfNet(TypeOfNet(current_type)); // FIXME pour Timed event graph => afficher les arcs
                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("To Canonical form"))
            {
                Net pn(m_net.type());
                toCanonicalForm(m_net, pn);
                m_net = pn;
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false))
            {
                undo();
            }

            if (ImGui::MenuItem("Redo", "Ctrl+Y", false))
            {
                redo();
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Clear net", nullptr, false))
            {
                clearNet();
            }
            if (ImGui::MenuItem("Align nodes", nullptr, false))
            {
                //editor.align();
            }
            if (ImGui::MenuItem("Show grid", nullptr, false))
            {
                m_view.grid.show ^= true;
            }
            if (ImGui::MenuItem("Take screenshot", nullptr, false))
            {
                m_states.do_screenshot = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem(m_simulation.running
                                ? "Stop simulation"
                                : "Start simulation", nullptr, false))
            {
                toogleStartSimulation();
            }
            ImGui::EndMenu();
        }

        if ((m_net.type() == TypeOfNet::TimedEventGraph) || (isEventGraph(m_net)))
        {
            if (ImGui::BeginMenu("Graph Events"))
            {
                if (ImGui::MenuItem("Show critical circuit", nullptr, false))
                {
                    m_states.do_find_critical_cycle = true;
                }
                if (ImGui::MenuItem("Show (max, +) dynamic linear system", nullptr, false))
                {
                    m_states.do_syslin = true;
                }
                if (ImGui::MenuItem("Show dater or counter equations", nullptr, false))
                {
                    m_states.do_counter_or_dater = true;
                }
                if (ImGui::MenuItem("Show adjacency matrices", nullptr, false))
                {
                    m_states.do_adjency = true;
                }
                ImGui::EndMenu();
            }
        }
        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("Help", nullptr, false))
            {
                m_states.show_help = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("About", nullptr, false))
            {
                m_states.show_about = true;
            }
            if (ImGui::MenuItem("Theme", nullptr, false))
            {
                m_states.show_theme = true;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    m_states.viewport_center = ImGui::GetMainViewport()->GetCenter();

    if (m_states.show_help) { help(); }
    if (m_states.show_about) { about(); }
    if (m_states.show_theme) { showStyleSelector(); }
    if (m_states.do_load) { loadNetFile(); }
    if (m_states.do_save_as) { saveNetAs(); }
    if (m_states.do_export_to != nullptr) { exportNetTo(*m_states.do_export_to); }
    if (m_states.do_import_from != nullptr) { importNetFrom(*m_states.do_import_from); }
    if (m_states.do_screenshot) { takeScreenshot(); }
    if (m_states.do_adjency) { showAdjacencyMatrices(); }
    if (m_states.do_counter_or_dater) { showCounterOrDaterEquation(); }
    if (m_states.do_syslin) { showDynamicLinearSystem(); }
    if (m_states.do_find_critical_cycle) { showCriticalCycles(); }
    if (m_states.request_quitting)
    {
        // Request to save the modified net before quitting, else quit the
        // application.
        if (m_net.modified) { m_states.do_save_as = true; } else { halt(); }
    }
}

//------------------------------------------------------------------------------
void Editor::showAdjacencyMatrices() const
{
    ImGui::OpenPopup("Show adjacency matrices");
    ImGui::SetNextWindowPos(m_states.viewport_center, ImGuiCond_Appearing,
                            ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Show adjacency matrices",
                               NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::Checkbox("Dense matrix", &SparseMatrix<MaxPlus>::display_as_dense);
        SparseMatrix<MaxPlus>::display_for_julia = false;
        ImGui::PopStyleVar();

        SparseMatrix<MaxPlus> tokens; SparseMatrix<MaxPlus> durations;
        toAdjacencyMatrices(m_net, tokens, durations);

        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
        if (ImGui::BeginTabBar("adjacency", tab_bar_flags))
        {
            if (ImGui::BeginTabItem("Durations"))
            {
                std::stringstream txt; txt << durations;
                ImGui::Text("%s", txt.str().c_str());
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Tokens"))
            {
                std::stringstream txt; txt << tokens;
                ImGui::Text("%s", txt.str().c_str());
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        if (ImGui::Button("OK", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
            m_states.do_adjency = false;
        }
        ImGui::EndPopup();
    }
}

//------------------------------------------------------------------------------
void Editor::showCounterOrDaterEquation() const
{
    static bool use_caption = false;
    static bool tropical_notation = false;
    //static bool show_matrix = false;

    ImGui::OpenPopup("Counter or dater equations");
    ImGui::SetNextWindowPos(m_states.viewport_center, ImGuiCond_Appearing,
                            ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Counter or dater equations", NULL,
        ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::Checkbox("Use (min,+) operator", &tropical_notation);
        ImGui::SameLine();
        ImGui::Checkbox("Use caption", &use_caption);
        ImGui::PopStyleVar();

        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
        if (ImGui::BeginTabBar("tab counter or dater", tab_bar_flags))
        {
            if (ImGui::BeginTabItem("Counter"))
            {
                ImGui::Text("%s", showCounterEquation(m_net, "", use_caption, tropical_notation)
                    .str().c_str());
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Dater"))
            {
                ImGui::Text("%s", showDaterEquation(m_net, "", use_caption, tropical_notation)
                    .str().c_str());
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        if (ImGui::Button("OK", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
            m_states.do_counter_or_dater = false;
        }
        ImGui::EndPopup();
    }
}

//------------------------------------------------------------------------------
void Editor::showDynamicLinearSystem() const
{
    ImGui::OpenPopup("(max, +) dynamic linear system");
    ImGui::SetNextWindowPos(m_states.viewport_center, ImGuiCond_Appearing,
                            ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("(max, +) dynamic linear system", NULL,
                               ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::Checkbox("Dense matrix", &SparseMatrix<MaxPlus>::display_as_dense);
        ImGui::PopStyleVar();

        SparseMatrix<MaxPlus> D; SparseMatrix<MaxPlus> A;
        SparseMatrix<MaxPlus> B; SparseMatrix<MaxPlus> C;
        toSysLin(m_net, D, A, B, C);
        SparseMatrix<MaxPlus>::display_for_julia = false;
        ImGui::Text(u8"%s", "X(n) = D . X(n) (+) A . X(n-1) (+) B . U(n)\nY(n) = C . X(n)");
        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
        if (ImGui::BeginTabBar("syslin", tab_bar_flags))
        {
            if (ImGui::BeginTabItem("D"))
            {
                std::stringstream txt; txt << D;
                ImGui::Text("%s", txt.str().c_str());
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("A"))
            {
                std::stringstream txt; txt << A;
                ImGui::Text("%s", txt.str().c_str());
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("B"))
            {
                std::stringstream txt; txt << B;
                ImGui::Text("%s", txt.str().c_str());
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("C"))
            {
                std::stringstream txt; txt << C;
                ImGui::Text("%s", txt.str().c_str());
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

#if 0
        if (m_states.plot.isEmpty())
        {
            //SparseMatrix<MaxPlus> X0;
            //SparseMatrix<MaxPlus> X = X0;
            //for (size_t i = 0; i < 10; i++)
            //{
            //    X = A * X + B * u[i,:];
            //    Y = C * X;
            //    m_states.plot.add(X, Y);
            //}
            for (size_t i = 0; i < 10; i++)
            {
                m_states.plot.add(i, 3*i);
            }
        }
        else
        {
            drawPlot("aa", "bb", m_states.plot.x(), m_states.plot.y());
        }
#endif

        if (ImGui::Button("OK", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
            m_states.do_syslin = false;
            m_states.plot.reset();
        }
        ImGui::EndPopup();
    }
}

//------------------------------------------------------------------------------
void Editor::showCriticalCycles() //const
{
    ImGui::OpenPopup("Critical Cycle");
    ImGui::SetNextWindowPos(m_states.viewport_center, ImGuiCond_Appearing,
                            ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Critical Cycle", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        CriticalCycleResult res = findCriticalCycle(m_net);
        if (!res.success)
        {
            ImGui::Text(u8"%s", res.message.str().c_str());
        }
        else
        {
            ImGui::Text("Found %zu connected components of the optimal policy", res.cycles);

            m_marked_arcs = res.arcs;
            ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
            if (ImGui::BeginTabBar("CriticalCycleResult", tab_bar_flags))
            {
                if (ImGui::BeginTabItem("Critical cycle"))
                {
                    std::stringstream txt;
                    if (m_net.type() == TypeOfNet::TimedEventGraph)
                    {
                        // Only show transitions
                        for (size_t it = 0u; it < res.arcs.size(); it += 2u)
                        {
                            txt << res.arcs[it]->from.key << " -> "
                                << res.arcs[it + 1u]->to.key
                                << std::endl;
                        }
                    }
                    else
                    {
                        // Show transitions and places
                        for (size_t it = 0u; it < res.arcs.size(); it += 2u)
                        {
                            txt << res.arcs[it]->from.key << " -> "
                                << res.arcs[it]->to.key << " -> "
                                << res.arcs[it + 1u]->to.key
                                << std::endl;
                        }
                    }
                    ImGui::Text("%s", txt.str().c_str());
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Cycle durations"))
                {
                    const auto& tr = m_net.transitions();
                    std::stringstream txt;
                    for (size_t i = 0u; i < res.durations.size(); ++i)
                    {
                        txt << "From " << tr[i].key << ": "
                            << res.durations[i]
                            << " units of time"
                            << std::endl;
                    }
                    ImGui::Text("%s", txt.str().c_str());
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Eigenvector"))
                {
                    std::stringstream txt;
                    for (auto const& it: res.eigenvector)
                    {
                        txt << it << std::endl;
                    }
                    ImGui::Text("%s", txt.str().c_str());
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }

        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
            m_states.do_find_critical_cycle = false;
        }
        ImGui::EndPopup();
    }
}

//------------------------------------------------------------------------------
void Editor::about() const
{
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::OpenPopup("About TimedPetriNetEditor");
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("About TimedPetriNetEditor", NULL,
                               ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("A timed Petri net and graph event editor and");
        ImGui::Text("simulator combined to (max,+) algebra with");
        ImGui::Text("wrapped API for Julia langage.");
        ImGui::Separator();
        std::string version("Version: " +
                            std::to_string(project::info::major_version) + '.' +
                            std::to_string(project::info::minor_version) + '.' +
                            std::to_string(project::info::patch_version));
        ImGui::Text("%s", version.c_str());
        ImGui::Separator();
        ImGui::Text("https://github.com/Lecrapouille/TimedPetriNetEditor");
        ImGui::Text("Git branch: %s", project::info::git_branch.c_str());
        ImGui::Text("Git SHA1: %s", project::info::git_sha1.c_str());
        ImGui::Text("Compiled as %s",
                    (project::info::compilation::mode == project::info::compilation::Mode::debug)
                    ? "Debug" : "Release");
        ImGui::Separator();
        ImGui::Text("Developed by Quentin Quadrat");
        ImGui::Text("Email: lecrapouille@gmail.com");
        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
            m_states.show_about = false;
        }

        ImGui::EndPopup();
    }
}

//------------------------------------------------------------------------------
void Editor::help() const
{
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::OpenPopup("Help TimedPetriNetEditor");
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Help TimedPetriNetEditor", NULL,
                               ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
        if (ImGui::BeginTabBar("help", tab_bar_flags))
        {
            if (ImGui::BeginTabItem("Mouse actions"))
            {
                std::stringstream help;
                help << "Left button pressed: add a new place." << std::endl
                     << "Right button pressed: add a new transition." << std::endl
                     << "Middle button pressed outside a node followed by middle button released on a selected node:" << std::endl
                     << "  - the arc is created as well as the origin node where its type is determined by the destination node." << std::endl
                     << "Middle button pressed on an initial selected node followed by middle button released on any node:" << std::endl
                     << "  - the arc is created as well as the destination node where its type is determined by the origin node." << std::endl
                     << "Middle button pressed on a first node followed by middle button released on a second node:" << std::endl
                     << "  - if nodes have not the same type then a simple arc is created." << std::endl
                     << "  - if nodes have the same type then an arc is created and split by an intermediate node." << std::endl
                     << "Ctrl + Middle button pressed: move the view." << std::endl;

                ImGui::Text("%s", help.str().c_str());
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Keyboard actions"))
            {
                std::stringstream help;
                help << "R: start or stop the simulation" << std::endl
                     << "Space: start or stop the simulation" << std::endl
                     << "M: move the selected place or transition" << std::endl
                     << "Delete: suppress the selected place or transition" << std::endl
                     << "+: increment the number of tokens in the selected place" << std::endl
                     << "-: decrement the number of tokens in the selected place" << std::endl;

                ImGui::Text("%s", help.str().c_str());
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Transitivity Syntax"))
            {
                std::stringstream help;
                help << "Transitivities are boolean expression beteween sensors and states of GRAFCET steps." << std::endl
                     << "The syntax used for expression is Reverse Polish Notation (RPN): operators follow their operands." << std::endl
                     << "  And operator:         ." << std::endl
                     << "  Or operator:          +" << std::endl
                     << "  Negation operator:    !" << std::endl
                     << "  State of Step 42:     X42" << std::endl
                     << "  Sensor name:          any consecutive char" << std::endl
                     << "  true operand:         true" << std::endl
                     << "  false operand:        false" << std::endl
                     << "Example:\n  X42 sensor-temp + sensor2 ! .\nmeans:\n . (Step42 or sensor-temp) and (not sensor2)" << std::endl;

                ImGui::Text("%s", help.str().c_str());
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Pathes"))
            {
                ImGui::Text("Data path: %s", m_path.toString().c_str());
                ImGui::Text("Temporary path: %s", project::info::tmp_path.c_str());
                ImGui::Text("Log path: %s", project::info::log_path.c_str());
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        if (ImGui::Button("OK", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
            m_states.show_help = false;
        }

        ImGui::EndPopup();
    }
}

//------------------------------------------------------------------------------
void Editor::console()
{
    ImGui::Begin("Console");
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.f);
    if (ImGui::Button("Clear##console_clear")) {
        clearLogs();
    }

    ImGui::PopStyleVar();
    ImGui::Spacing();
    auto const& logs = getLogs();
    size_t i = logs.size();
    while (i--)
    {
        ImGui::Separator();
        if (logs[i].level == Messages::Level::Info)
        {
            ImGui::TextColored(ImVec4(0, 255, 0, 255), "%s[info]: %s",
                               logs[i].time.c_str(), logs[i].message.c_str());
        }
        else if (logs[i].level == Messages::Level::Error)
        {
            ImGui::TextColored(ImVec4(255, 0, 0, 255), "%s[error]: %s",
                               logs[i].time.c_str(), logs[i].message.c_str());
        }
        else
        {
            ImGui::TextColored(ImVec4(255, 0, 255, 255), "%s[warn]: %s",
                               logs[i].time.c_str(), logs[i].message.c_str());
        }
    }
    ImGui::End();
}

//------------------------------------------------------------------------------
void Editor::messagebox()
{
    ImGui::Begin("Message");
    ImGui::Text("%s", getError().c_str());
    ImGui::End();
}

//------------------------------------------------------------------------------
void Editor::inspector()
{
    // InputText modify callback: modified => net shall be saved ?
    static bool modified = false;
    // InputText callback: GRAFCET transitivities modified ?
    static bool compiled = false;
    // Do not allow editing when running simulation
    const auto readonly = m_simulation.running ?
        ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_None;

    // Place captions and tokens
    {
        ImGui::Begin(m_net.type() == TypeOfNet::GRAFCET ? "Steps" : "Places");

        // Options
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::Checkbox(m_states.show_place_captions ?
                        "Show place identifiers" : "Show place captions",
                        &m_states.show_place_captions);
        ImGui::PopStyleVar();
        ImGui::Separator();

        for (auto& place: m_net.places())
        {
            ImGui::PushID(place.key.c_str());
            ImGui::AlignTextToFramePadding();
            ImGui::InputText(place.key.c_str(), &place.caption,
                readonly | ImGuiInputTextFlags_CallbackEdit,
                [](ImGuiInputTextCallbackData*)
                {
                    modified = true;
                    return 0;
                });

            // Increment/decrement tokens
            ImGui::SameLine();
            ImGui::PushButtonRepeat(true);
            if (ImGui::ArrowButton("##left", ImGuiDir_Left))
            {
                place.decrement();
                modified = true;
            }
            ImGui::SameLine();
            if (ImGui::ArrowButton("##right", ImGuiDir_Right))
            {
                place.increment();
                modified = true;
            }
            ImGui::PopButtonRepeat();

            // Caption edition
            ImGui::SameLine();
            ImGui::Text("%zu", place.tokens);

            ImGui::PopID();
        }
        ImGui::End();
    }

    // Transition captions and GRAFCET transitivities
    {
        ImGui::Begin("Transitions");
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::Checkbox(m_states.show_transition_captions
                        ? "Show transition identifiers"
                        : "Show transition captions",
                        &m_states.show_transition_captions);
        ImGui::PopStyleVar();
        ImGui::Separator();
        ImGui::Text("%s", (m_net.type() == TypeOfNet::GRAFCET) ? "Transitivities:" : "Captions:");

        // Compile transitivities for GRAFCET the initial time and each time one of transitions
        // have been edited (Currently: any InputText invalid the whole sensors. Slow but easier
        // to implement).
        compiled |= m_simulation.compiled;
        if ((m_net.type() == TypeOfNet::GRAFCET) && (!compiled))
        {
            compiled = m_simulation.generateSensors();
        }
        for (auto& t: m_net.transitions())
        {
            // Show contents of transition
            ImGui::InputText(t.key.c_str(), &t.caption,
                readonly | ImGuiInputTextFlags_CallbackEdit,
                [](ImGuiInputTextCallbackData*)
                {
                    modified = true;
                    compiled = false;
                    return 0;
                });

            // For GRAFCET and show syntax error on the transitivity
            if ((m_net.type() == TypeOfNet::GRAFCET) && (!m_simulation.running))
            {
                Simulation::Receptivities const& receptivities = m_simulation.receptivities();
                auto it = receptivities.find(t.id);
                if (it == receptivities.end())
                {
                    m_messages.setError("Could not find receptivity. This should not happened. Please report this error");
                }
                else if (!it->second.isValid())
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", it->second.error().c_str());
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", "See help for the syntax");
                }
            }
        }
        ImGui::End();

        // For GRAFCET show sensor names from transitivities
        if (m_net.type() == TypeOfNet::GRAFCET)
        {
            ImGui::Begin("Sensors");
            for (auto& it: Sensors::instance().database())
            {
                int prev_value = it.second;
                ImGui::SliderInt(it.first.c_str(), &it.second, 0, 1);
                modified = (prev_value != it.second) && (!m_simulation.running);
            }
            ImGui::End();
        }
    }

    // Arc durations
    if (m_net.type() == TypeOfNet::TimedEventGraph)
    {
        ImGui::Begin("Arcs");
        ImGui::Text("%s", "Durations:");
        for (auto& arc: m_net.arcs())
        {
            if (arc.from.type == Node::Type::Transition)
            {
                std::string text(arc.from.key + " -> " + arc.to.arcsOut[0]->to.key);
                float prev_value = arc.duration;
                ImGui::InputFloat(text.c_str(), &arc.duration, 0.01f, 1.0f, "%.3f", readonly);
                modified = (prev_value != arc.duration);
            }
        }
        ImGui::End();
    }
    else if (m_net.type() == TypeOfNet::TimedPetriNet)
    {
        ImGui::Begin("Arcs");
        ImGui::Text("%s", "Durations:");
        for (auto& arc: m_net.arcs())
        {
            std::string text(arc.from.key + " -> " + arc.to.key);
            float prev_value = arc.duration;
            ImGui::InputFloat(text.c_str(), &arc.duration, 0.01f, 1.0f, "%.3f", readonly);
            modified = (prev_value != arc.duration);
        }
        ImGui::End();
    }

    // Modified net ? If yes, set it as dirty to force its save when the app
    // is closed. Compiled receptivities ?
    m_simulation.compiled = compiled;
    m_net.modified |= modified;
    modified = false;
}

//------------------------------------------------------------------------------
void Editor::toogleStartSimulation()
{
    m_simulation.running = m_simulation.running ^ true;

    // Note: in GUI.cpp in the Application constructor, I set
    // the window to have slower framerate in the aim to have a
    // bigger discrete time and therefore AnimatedToken moving
    // with a bigger step range and avoid them to overlap when
    // i.e. two of them, carying 1 token, are arriving at almost
    // the same moment but separated by one call of this method
    // update() producing two AnimatedToken carying 1 token that
    // will be displayed at the same position instead of a
    // single AnimatedToken carying 2 tokens.
    framerate(m_simulation.running ? 30 : 60); // FPS

#ifdef WITH_MQTT
    // Simulate sensor reading value.
    auto& database = Sensors::instance().database();
    for (auto const& sensor: database)
    {
        /*
        auto const& sensor_name = sensor.first;
        m_mqtt.subscribe("tpne/" + sensor_name, MQTT::QoS::QoS0,
        [&](MQTT::Message const& msg)
        {
            const char* payload = static_cast<const char*>(msg.payload);
            auto it = database.find(sensor_name);
            if (it != database.end())
            {
                it->second = (payload[0] != '0');
            }
        });
        */
    }
#endif
}

//------------------------------------------------------------------------------
bool Editor::switchOfNet(TypeOfNet const type)
{
    if (m_simulation.running)
        return false;

    std::vector<Arc*> arcs;
    std::string error;
    if (convertTo(m_net, type, error, arcs))
        return true;

    m_messages.setError(m_net.error());
    return false;
}

//------------------------------------------------------------------------------
Node* Editor::getNode(ImVec2 const& position)
{
    Node *node = getTransition(position);
    if (m_net.type() == TypeOfNet::TimedEventGraph)
        return node;

    return (node != nullptr) ? node : getPlace(position);
}

//------------------------------------------------------------------------------
Place* Editor::getPlace(ImVec2 const& position)
{
    for (auto &place : m_net.places())
    {
        float d2 = (place.x - position.x) * (place.x - position.x) +
                   (place.y - position.y) * (place.y - position.y);
        if (d2 < PLACE_RADIUS * PLACE_RADIUS)
        {
            return &place;
        }
    }

    return nullptr;
}

//------------------------------------------------------------------------------
Transition* Editor::getTransition(ImVec2 const& position)
{
    for (auto &transition : m_net.transitions())
    {
        float d2 = (transition.x - position.x) * (transition.x - position.x) +
                   (transition.y - position.y) * (transition.y - position.y);
        if (d2 < TRANS_WIDTH * TRANS_WIDTH)
        {
            return &transition;
        }
    }

    return nullptr;
}

//------------------------------------------------------------------------------
Transition& Editor::addTransition(float const x, float const y)
{
    auto action = std::make_unique<NetModifaction>(*this);
    action->before(m_net);
    Transition& transition = m_net.addTransition(x, y);
    m_simulation.generateSensors(); // m_simulation.generateSensor(transition);
    action->after(m_net);
    m_history.add(std::move(action));
    return transition;
}

//------------------------------------------------------------------------------
void Editor::addPlace(float const x, float const y)
{
    auto action = std::make_unique<NetModifaction>(*this);
    action->before(m_net);
    m_net.addPlace(x, y);
    action->after(m_net);
    m_history.add(std::move(action));
}

//------------------------------------------------------------------------------
void Editor::removeNode(Node& node)
{
    Node::Type type = node.type;
    auto action = std::make_unique<NetModifaction>(*this);
    action->before(m_net);
    m_net.removeNode(node);
    if (type == Node::Type::Transition)
    {
        m_simulation.generateSensors();
    }
    action->after(m_net);
    m_history.add(std::move(action));
}

//------------------------------------------------------------------------------
Node& Editor::addOppositeNode(Node::Type const type, float const x,
    float const y, size_t const tokens)
{
    auto action = std::make_unique<NetModifaction>(*this);
    action->before(m_net);
    m_simulation.generateSensors();
    Node& node = m_net.addOppositeNode(type, x, y, tokens);
    if (node.type == Node::Type::Transition)
    {
        // FIXME m_simulation.generateSensor(node);
        m_simulation.generateSensors();
    }
    action->after(m_net);
    m_history.add(std::move(action));
    return node;
}

//------------------------------------------------------------------------------
void Editor::addArc(Node& from, Node& to, float const duration)
{
    auto action = std::make_unique<NetModifaction>(*this);
    action->before(m_net);
    m_net.addArc(from, to, duration);
    m_simulation.generateSensors();
    action->after(m_net);
    m_history.add(std::move(action));
}

//------------------------------------------------------------------------------
void Editor::loadNetFile()
{
    static Importer importer{"TimedPetriNetEditor", ".json", importFromJSON, false};
    importNetFrom(importer);
}

//------------------------------------------------------------------------------
void Editor::importNetFrom(Importer const& importer)
{
    if (m_simulation.running)
    {
        m_messages.setError("Cannot load during the simulation!");
        return ;
    }

    IGFD::FileDialogConfig config;
    config.path = ".";
    config.flags = ImGuiFileDialogFlags_Modal;
    ImGuiFileDialog::Instance()->OpenDialog(
        "ChooseFileDlgKey",
        "Choose the Petri file to load",
        importer.extensions.c_str(), config);

    if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey"))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            auto const filepath = ImGuiFileDialog::Instance()->GetFilePathName();
            m_marked_arcs.clear();
            m_net.clear();
            std::string error = importer.importFct(m_net, filepath);
            if (error.empty())
            {
                if (m_states.do_import_from)
                    m_messages.setInfo("Imported with success from '" + filepath + "'");
                else
                    m_messages.setInfo("Loaded with success '" + filepath + "'");
                setSavePath(filepath);
                m_net.modified = false;
                if (importer.springify)
                {
                    springify();
                }
            }
            else
            {
                m_messages.setError(error);
                m_net.clear();
                m_net.modified = true;
                m_spring.reset();
            }
        }

        // close
        m_states.do_load = false;
        m_states.do_import_from = nullptr; // FIXME think proper code: export vs save as
        ImGuiFileDialog::Instance()->Close();
    }
}

//--------------------------------------------------------------------------
void Editor::springify()
{
    m_spring.reset(m_view.size().x, m_view.size().y, m_net);
}

//--------------------------------------------------------------------------
void Editor::saveNetAs()
{
    static Exporter exporter{"TimedPetriNetEditor", ".json", exportToJSON};
    exportNetTo(exporter);
}

//------------------------------------------------------------------------------
void Editor::exportNetTo(Exporter const& exporter)
{
    if (m_simulation.running)
    {
        m_messages.setError("Cannot save during the simulation!");
        return ;
    }

    if (m_net.isEmpty())
    {
        if (m_states.request_quitting)
        {
            m_states.request_quitting = false;
            halt();
        }
        else
        {
            m_messages.setError("Cannot save dummy net!");
        }
        return ;
    }

    IGFD::FileDialogConfig config;
    config.path = ".";
    config.flags = ImGuiFileDialogFlags_Modal | ImGuiFileDialogFlags_ConfirmOverwrite;
    ImGuiFileDialog::Instance()->OpenDialog(
        "ChooseFileDlgKey",
        m_states.do_export_to ? "Choose the Petri file to save"
          : (m_states.request_quitting ? "Choose the Petri file to save before quitting"
          : "Choose the Petri file to save"),
        exporter.extensions.c_str(), config);

    if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey"))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            auto const path = ImGuiFileDialog::Instance()->GetFilePathName();
            std::string error = exporter.exportFct(m_net, path);
            if (error.empty())
            {
                if (m_states.do_export_to)
                {
                    m_messages.setInfo("Exported with success '" + path + "'");
                }
                else
                {
                    setSavePath(path);
                    m_messages.setInfo("Saved with success '" + path + "'");
                    m_net.modified = false;
                }
                if (m_states.request_quitting)
                {
                    m_states.request_quitting = false;
                    halt();
                }
            }
            else
            {
                m_messages.setError(error);
                m_net.modified = true;
            }
        }

        // Close or Cancel button.
        m_states.do_save_as = false;
        m_states.do_export_to = nullptr; // FIXME think proper code: export vs save as
        if (m_states.request_quitting)
        {
            m_states.do_save_as = true;
            m_states.request_quitting = false;
            // FIXME ajouter une pop: voulez vous vraiment perdre votre document ?
            halt();
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

//--------------------------------------------------------------------------
void Editor::takeScreenshot()
{
    std::string path;
    IGFD::FileDialogConfig config;
    config.path = ".";
    config.flags = ImGuiFileDialogFlags_Modal | ImGuiFileDialogFlags_ConfirmOverwrite;
    ImGuiFileDialog::Instance()->OpenDialog(
        "ChooseFileDlgKey",
        "Choose the PNG file to save the screenshot",
        ".png", config);

    if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey"))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            path = ImGuiFileDialog::Instance()->GetFilePathName();
            if (Application::screenshot(path))
            {
                m_messages.setInfo("Screenshot taken as file '" + path + "'");
            }
            else
            {
                m_messages.setError("Failed to save screenshot to file '" +
                                    path + "'");
            }
        }

        // close.
        m_states.do_screenshot = false;
        ImGuiFileDialog::Instance()->Close();
    }
}

//--------------------------------------------------------------------------
void Editor::clearNet()
{
    if (m_simulation.running)
        return ;

    auto action = std::make_unique<NetModifaction>(*this);
    action->before(m_net);

    m_net.reset(m_net.type());

    action->after(m_net);
    m_history.add(std::move(action));
}

//--------------------------------------------------------------------------
std::string Editor::getError() const
{
    if (m_messages.getMessages().empty())
        return {};
    return m_messages.getMessage().message;
}

//--------------------------------------------------------------------------
std::vector<Messages::TimedMessage> const& Editor::getLogs() const
{
    return m_messages.getMessages();
}

//--------------------------------------------------------------------------
void Editor::clearLogs()
{
    m_messages.clear();
}

//--------------------------------------------------------------------------
void Editor::undo()
{
    if (m_simulation.running)
        return ;

    if (!m_history.undo())
    {
        m_messages.setInfo("Cannot do more undos!");
    }
    else
    {
        m_messages.setInfo("Undo!");
        m_net.modified = true;
    }
}

//--------------------------------------------------------------------------
void Editor::redo()
{
    if (m_simulation.running)
        return ;

    if (!m_history.redo())
    {
        m_messages.setInfo("Cannot do more redos!");
    }
    else
    {
        m_messages.setInfo("Redo!");
        m_net.modified = true;
        m_simulation.compiled = false;
    }
}

//--------------------------------------------------------------------------
Editor::PetriView::PetriView(Editor& editor)
    : m_editor(editor)
{}

//--------------------------------------------------------------------------
void Editor::PetriView::Canvas::push()
{
    draw_list = ImGui::GetWindowDrawList();
    draw_list->ChannelsSplit(1);
    draw_list->PushClipRect(corners[0], corners[1], true);
}

//--------------------------------------------------------------------------
void Editor::PetriView::Canvas::pop()
{
    draw_list->PopClipRect();
}

//--------------------------------------------------------------------------
ImVec2 Editor::PetriView::Canvas::reshape()
{
    // ImDrawList API uses screen coordinates!
    corners[0] = ImGui::GetCursorScreenPos();

    // Resize canvas to what's available
    size = ImGui::GetContentRegionAvail();
    if (size.x < 50.0f) size.x = 50.0f;
    if (size.y < 50.0f) size.y = 50.0f;
    corners[1] = corners[0] + size;

    // Lock scrolled origin
    origin = corners[0] + scrolling;
    return size;
}

//--------------------------------------------------------------------------
ImVec2 Editor::PetriView::reshape()
{
    return m_canvas.reshape();
}

//--------------------------------------------------------------------------
ImVec2 Editor::PetriView::Canvas::getMousePosition()
{
    ImGuiIO &io = ImGui::GetIO();
    return io.MousePos - origin;
}

//--------------------------------------------------------------------------
bool Editor::PetriView::isMouseReleased(ImGuiMouseButton& key)
{
    ImGuiIO &io = ImGui::GetIO();
    if ((io.MousePos.x >= m_canvas.corners[0].x) &&
        (io.MousePos.x <= m_canvas.corners[1].x) &&
        (io.MousePos.y >= m_canvas.corners[0].y) &&
        (io.MousePos.y <= m_canvas.corners[1].y))
    {
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Middle))
        {
            key = ImGuiMouseButton_Middle;
            return true;
        }
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            key = ImGuiMouseButton_Left;
            return true;
        }
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
        {
            key = ImGuiMouseButton_Right;
            return true;
        }
    }
    return false;
}

//--------------------------------------------------------------------------
bool Editor::PetriView::isMouseClicked(ImGuiMouseButton& key)
{
    ImGuiIO &io = ImGui::GetIO();
    if ((io.MousePos.x >= m_canvas.corners[0].x) &&
        (io.MousePos.x <= m_canvas.corners[1].x) &&
        (io.MousePos.y >= m_canvas.corners[0].y) &&
        (io.MousePos.y <= m_canvas.corners[1].y))
    {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
        {
            key = ImGuiMouseButton_Middle;
            return true;
        }
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            key = ImGuiMouseButton_Left;
            return true;
        }
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        {
            key = ImGuiMouseButton_Right;
            return true;
        }
    }

    return false;
}

//--------------------------------------------------------------------------
bool Editor::PetriView::isMouseDraggingView(ImGuiMouseButton const& button)
{
    (void) button;

    const float mouse_threshold_for_pan = grid.menu ? -1.0f : 0.0f;
    if (ImGui::IsMouseDragging(MOUSE_BOUTON_DRAGGING_VIEW, mouse_threshold_for_pan))
    {
        return true; // m_editor.getNode(m_mouse.position) == nullptr;
    }

    return false;
}

//--------------------------------------------------------------------------
void Editor::PetriView::handleMoveNode()
{
    if (m_mouse.selection.size() == 0u)
    {
        Node* node = m_editor.getNode(m_mouse.position);
        if (node != nullptr)
        {
            m_mouse.selection.push_back(node);
            m_editor.m_net.modified = true;
        }
    }
    else
    {
        m_mouse.selection.clear();
    }
}

//--------------------------------------------------------------------------
void Editor::PetriView::handleAddNode(ImGuiMouseButton button)
{
    if (!m_editor.m_simulation.running)
    {
        // Add a new Place or a new Transition only if a node is not already
        // present.
        if (m_editor.getNode(m_mouse.position) == nullptr)
        {
            float const x = m_mouse.position.x;
            float const y = m_mouse.position.y;
            if (m_editor.m_net.type() == TypeOfNet::TimedEventGraph)
            {
                // In TimedEventGraph mode, we prefer avoiding creating
                // places because they are not displayed. So only create
                // transitions and arcs.
                m_editor.addTransition(x, y);
            }
            else
            {
                // In other mode, Petri nets have two types of nodes.
                if (button == MOUSE_BOUTON_ADD_PLACE)
                {
                    m_editor.addPlace(x, y);
                }
                else if (button == MOUSE_BOUTON_ADD_TRANSITION)
                {
                    m_editor.addTransition(x, y);
                }
            }
        }
    }
    else if (m_editor.m_net.type() == TypeOfNet::PetriNet)
    {
        // Click to fire a transition during the simulation
        Transition* transition = m_editor.getTransition(m_mouse.position);
        if (transition != nullptr)
        {
            transition->receptivity ^= true;
        }
    }
}

//--------------------------------------------------------------------------
void Editor::PetriView::handleArcOrigin()
{
    if (m_editor.m_simulation.running)
        return ;

    m_mouse.clicked_at = m_mouse.position;
    m_mouse.from = m_editor.getNode(m_mouse.position);
    m_mouse.handling_arc = !ImGui::GetIO().KeyCtrl;
    m_mouse.to = nullptr;
}

//--------------------------------------------------------------------------
void Editor::PetriView::handleArcDestination()
{
    // Finish the creation of the arc (destination node) from the mouse cursor
    m_mouse.to = m_editor.getNode(m_mouse.position);
    m_mouse.handling_arc = false;

    if (m_editor.m_net.type() == TypeOfNet::TimedEventGraph)
    {
        // In TimedEventGraph mode we only create transitions since places are
        // implicit and therefore not displayed.
        if (m_mouse.from == nullptr)
        {
            assert(m_mouse.to != nullptr);
            m_mouse.from = &m_editor.addTransition(m_mouse.clicked_at.x, m_mouse.clicked_at.y);
        }
        if (m_mouse.to == nullptr)
        {
            assert(m_mouse.from != nullptr);
            m_mouse.to = &m_editor.addTransition(m_mouse.position.x, m_mouse.position.y);
        }
    }
    else
    {
        // Arc to itself (or both nodes nullptr)
        if ((m_mouse.from == nullptr) && (m_mouse.to == nullptr))
            return ;

        else if (m_mouse.from == nullptr)
        {
            assert(m_mouse.to != nullptr);
            m_mouse.from = &m_editor.addOppositeNode(m_mouse.to->type, m_mouse.clicked_at.x, m_mouse.clicked_at.y);
        }
        else if (m_mouse.to == nullptr)
        {
            assert(m_mouse.from != nullptr);
            m_mouse.to = &m_editor.addOppositeNode(m_mouse.from->type, m_mouse.position.x, m_mouse.position.y);
        }
    }

    assert(m_mouse.from != nullptr);
    assert(m_mouse.to != nullptr);

    // The case where two nodes have the same type is managed by addArc
    m_editor.addArc(*m_mouse.from, *m_mouse.to, randomInt(1, 5));

    // Reset states
    m_mouse.from = m_mouse.to = nullptr;
}

//--------------------------------------------------------------------------
void Editor::PetriView::onHandleInput()
{
    // This will catch our interactions
    ImGui::InvisibleButton("canvas", m_canvas.size,
                           ImGuiButtonFlags_MouseButtonLeft |
                           ImGuiButtonFlags_MouseButtonRight |
                           ImGuiButtonFlags_MouseButtonMiddle);

    m_mouse.position = m_canvas.getMousePosition();
    ImGuiMouseButton button;
    if (ImGui::IsItemActive() && ImGui::IsItemHovered())
    {
        if (isMouseClicked(button))
        {
            m_editor.m_marked_arcs.clear();

            if (button == MOUSE_BOUTON_HANDLE_ARC)
            {
                handleArcOrigin();
            }
            else
            {
                handleAddNode(button);
            }
        }
        else if (ImGui::GetIO().KeyCtrl && isMouseDraggingView(button))
        {
            ImGuiIO& io = ImGui::GetIO();
            m_canvas.scrolling.x += io.MouseDelta.x;
            m_canvas.scrolling.y += io.MouseDelta.y;
        }
    }

    if (isMouseReleased(button))
    {
        m_mouse.is_dragging_view = false;
        m_mouse.selection.clear();

        if (button == MOUSE_BOUTON_HANDLE_ARC)
        {
            if (!ImGui::GetIO().KeyCtrl)
            {
                handleArcDestination();
            }
        }
    }

    if (ImGui::IsItemHovered())
    {
        if (ImGui::GetIO().KeyCtrl)
        {
            if (ImGui::IsKeyPressed(KEY_UNDO, false))
            {
                m_editor.undo();
            }
            else if (ImGui::IsKeyPressed(KEY_REDO, false))
            {
                m_editor.redo();
            }
        }
        else if (ImGui::IsKeyPressed(KEY_MOVE_PETRI_NODE, false))
        {
            handleMoveNode();
        }
        else if (ImGui::IsKeyPressed(KEY_SPRINGIFY_NET))
        {
            m_editor.springify();
        }
        // Run the animation of the Petri net
        else if (ImGui::IsKeyPressed(KEY_RUN_SIMULATION) ||
                 ImGui::IsKeyPressed(KEY_RUN_SIMULATION_ALT))
        {
            m_editor.toogleStartSimulation();
        }
        // Increment the number of tokens in the place.
        else if (ImGui::IsKeyPressed(KEY_INCREMENT_TOKENS))
        {
            Node* node = m_editor.getNode(m_mouse.position);
            if ((node != nullptr) && (node->type == Node::Type::Place))
            {
                reinterpret_cast<Place*>(node)->increment(1u);
                m_editor.m_net.modified = true;
            }
        }
        // Decrement the number of tokens in the place.
        else if (ImGui::IsKeyPressed(KEY_DECREMENT_TOKENS))
        {
            Node* node = m_editor.getNode(m_mouse.position);
            if ((node != nullptr) && (node->type == Node::Type::Place))
            {
                reinterpret_cast<Place*>(node)->decrement(1u);
                m_editor.m_net.modified = true;
            }
        }
        // Remove a node
        else if (ImGui::IsKeyPressed(KEY_DELETE_NODE))
        {
            Node* node = m_editor.getNode(m_mouse.position);
            if (node != nullptr)
            {
                m_editor.removeNode(*node);
            }
        }
    }

    if (m_editor.windowShouldClose())
    {
        m_editor.close();
    }
}

//--------------------------------------------------------------------------
void Editor::PetriView::drawPetriNet(Net& net, Simulation& simulation)
{
    const float alpha = 1.0f; // FIXME

    m_canvas.push();

    // Draw the grid
    drawGrid(m_canvas.draw_list, simulation.running);

    // Draw the Petri net
    ImVec2 const& origin = m_canvas.origin;
    for (auto const& it: net.arcs())
    {
        drawArc(m_canvas.draw_list, it, net.type(), origin, alpha);
    }
    for (auto const& it: net.places())
    {
        drawPlace(m_canvas.draw_list, it, net.type(), origin,
                  m_editor.m_states.show_place_captions, alpha);
    }
    for (auto const& it: net.transitions())
    {
        drawTransition(m_canvas.draw_list, it, net.type(), origin,
                       m_editor.m_states.show_transition_captions, alpha);
    }

    // Draw all tokens transiting from Transitions to Places
    for (auto const& it: simulation.timedTokens())
    {
        drawTimedToken(m_canvas.draw_list, it.tokens, origin.x + it.x, origin.y + it.y);
    }

    // Update node positions the user is currently moving
    for (auto& it: m_mouse.selection)
    {
        it->x = m_mouse.position.x;
        it->y = m_mouse.position.y;
    }

    // Show the arc we are creating
    if (m_mouse.handling_arc)
    {
        drawArc(m_canvas.draw_list, m_mouse.from, m_mouse.to, &m_mouse.clicked_at,
                origin, m_mouse.position);
    }

    // Draw critical cycle
    for (auto& it: m_editor.m_marked_arcs)
    {
        drawArc(m_canvas.draw_list, *it, net.type(), origin, -1.0f);
    }

    m_canvas.pop();
}

//--------------------------------------------------------------------------
void Editor::PetriView::drawGrid(ImDrawList* draw_list, bool const running)
{
    ImU32 border_color = running
       ? IM_COL32(0, 255, 0, 255) : IM_COL32(255, 255, 255, 255);
    ImU32 line_color = running
       ? IM_COL32(0, 255, 0, 40) : IM_COL32(200, 200, 200, 40);

    draw_list->ChannelsSetCurrent(0); // Background
    draw_list->AddRectFilled(m_canvas.corners[0], m_canvas.corners[1],
        ((ThemeId::Light == theme())
            ? LIGHT_THEME_PETRI_VIEW_COLOR
            : DARK_THEME_PETRI_VIEW_COLOR));
    draw_list->AddRect(m_canvas.corners[0], m_canvas.corners[1],
                       border_color);

    if (!grid.show)
        return ;

    for (float x = fmodf(m_canvas.scrolling.x, grid.step);
         x < m_canvas.size.x; x += grid.step)
    {
        draw_list->AddLine(
            ImVec2(m_canvas.corners[0].x + x, m_canvas.corners[0].y),
            ImVec2(m_canvas.corners[0].x + x, m_canvas.corners[1].y),
            line_color);
    }

    for (float y = fmodf(m_canvas.scrolling.y, grid.step);
         y < m_canvas.size.y; y += grid.step)
    {
        draw_list->AddLine(
            ImVec2(m_canvas.corners[0].x, m_canvas.corners[0].y + y),
            ImVec2(m_canvas.corners[1].x, m_canvas.corners[0].y + y),
            line_color);
    }
}

} // namespace tpne
