// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Editor/Editor.hpp"
#include "Editor/EditorState.hpp"
#include "Editor/Theme.hpp"
#include "Remote/IRemoteControl.hpp"
#include "MaxPlus/Algorithms.hpp"
#include "Editor/TopDownLayout.hpp"
#include "Editor/KeyBindings.hpp"
#include "Editor/FileDialogHelper.hpp"
#include "Runtime/Journal.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdio>
#include <cmath>
#include <map>
#include <vector>

namespace tpne {

//------------------------------------------------------------------------------
void Editor::menu()
{
    if (ImGui::BeginMainMenuBar())
    {
        menuFile();
        menuEdit();
        menuView();
        menuSimulation();
        menuGraphEvents();
        menuDebug();
        menuHelp();
        toolbar();
        ImGui::EndMainMenuBar();
    }

    // Store viewport center for dialogs
    m_states.viewport_center = ImGui::GetMainViewport()->GetCenter();

    shortcuts();
    handleMenuActions();
}

//------------------------------------------------------------------------------
void Editor::shortcuts()
{
    // Space starts and stops the simulation, wherever the focus is (but in a
    // text field). The editor owns the key: the keyboard navigation of Dear
    // ImGui would else also press the last clicked button with it.
    static ImGuiID const SPACE_OWNER = ImHashStr("SimulationKey");
    ImGui::SetKeyOwner(KEY_RUN_SIMULATION, SPACE_OWNER);

    ImGuiIO const& io = ImGui::GetIO();
    if (io.WantTextInput)
        return;
    if (ImGui::IsKeyPressed(KEY_RUN_SIMULATION, ImGuiInputFlags_None, SPACE_OWNER) &&
        !io.KeyCtrl && !io.KeyAlt && !ImGui::IsAnyItemActive())
    {
        toogleStartSimulation();
    }

    auto const pressed = [](ImGuiKeyChord chord) {
        return ImGui::Shortcut(chord, ImGuiInputFlags_RouteGlobal);
    };
    Document::ViewState& view = activeDocument().activeNet().view_state;
    if (pressed(ImGuiMod_Ctrl | ImGuiKey_N)) m_states.request_new = true;
    if (pressed(ImGuiMod_Ctrl | ImGuiKey_O)) m_states.request_load = true;
    if (pressed(ImGuiMod_Ctrl | ImGuiKey_S)) saveCurrentDocument();
    if (pressed(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S)) m_states.file_dialog = States::FileDialog::SaveAs;
    if (pressed(ImGuiMod_Ctrl | ImGuiKey_Q)) close();
    if (pressed(ImGuiMod_Ctrl | ImGuiKey_Equal)) view.zoom_request *= 1.25f;
    if (pressed(ImGuiMod_Ctrl | ImGuiKey_Minus)) view.zoom_request *= 0.8f;
    if (pressed(ImGuiMod_Ctrl | ImGuiKey_0))
        view.center_request = Document::ViewState::CENTER_FRAMES;
    if (pressed(ImGuiKey_F1)) openHelp(0);
    if (pressed(ImGuiKey_P) && activeDocument().isAnySimulationRunning()) m_states.paused ^= true;
}

//------------------------------------------------------------------------------
void Editor::toolbar()
{
    // Quick controls on the right of the menus.
    ImGui::Separator();
    bool const running = simulation().isRunning();
    ImGui::PushStyleColor(ImGuiCol_Text, running ? ImVec4(0.97f, 0.32f, 0.29f, 1.0f)
                                                 : ImVec4(0.25f, 0.73f, 0.31f, 1.0f));
    if (ImGui::MenuItem(running ? "■ Stop" : "▶ Run"))
        toogleStartSimulation();
    ImGui::PopStyleColor();
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Start or stop the simulation (Space)");

    // Pause and speed of the simulation (to follow the Gantt chart, the
    // timing diagram ...).
    ImGui::BeginDisabled(!running);
    if (ImGui::MenuItem(m_states.paused ? "Resume##toolbar" : "Pause##toolbar", nullptr, m_states.paused))
        m_states.paused ^= true;
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip("Pause or resume the simulation (P)");
    static float const SPEEDS[] = { 0.1f, 0.25f, 0.5f, 1.0f, 2.0f, 5.0f, 10.0f, 20.0f, 50.0f };
    char speed[32];
    std::snprintf(speed, sizeof(speed), "x%g", double(m_states.time_scale));
    ImGui::SetNextItemWidth(70.0f);
    if (ImGui::BeginCombo("##speed", speed))
    {
        for (float value : SPEEDS)
        {
            std::snprintf(speed, sizeof(speed), "x%g", double(value));
            if (ImGui::Selectable(speed, value == m_states.time_scale))
                m_states.time_scale = value;
        }
        ImGui::EndCombo();
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Speed of the simulation time (x2: 30s/X1 lasts 15 s)");

    // Type of the active net.
    ImGui::SetNextItemWidth(150.0f);
    int kind = net().layout().compact ? 2 : (net().type() == TypeOfNet::GRAFCET) ? 3 : int(net().type());
    ImGui::BeginDisabled(running);
    if (ImGui::Combo("##type", &kind, "Petri net\0Timed Petri net\0Timed event graph\0GRAFCET\0"))
    {
        switchOfNet((kind == 3) ? TypeOfNet::GRAFCET : (kind == 0) ? TypeOfNet::PetriNet
                                                                   : TypeOfNet::TimedPetriNet, kind == 2);
    }
    ImGui::EndDisabled();

    if (ImGui::MenuItem("Fit##toolbar"))
        activeDocument().activeNet().view_state.center_request = Document::ViewState::CENTER_FRAMES;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Fit the net in its window (F)");
    if (ImGui::MenuItem("Grid##toolbar", nullptr, m_view.grid.show))
        m_view.grid.show ^= true;
    if (ImGui::MenuItem("Snap##toolbar", nullptr, m_view.grid.snap))
        m_view.grid.snap ^= true;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Snap the nodes to the grid when they move (G)");
    if (net().layout().compact || isEventGraph(net()))
    {
        // Not "Analysis": the menu of the same name is in this menu bar.
        if (ImGui::MenuItem("Analysis window##toolbar", nullptr, m_states.show_analysis))
            m_states.show_analysis ^= true;
    }
}

//------------------------------------------------------------------------------
void Editor::menuFile()
{
    if (!ImGui::BeginMenu("File"))
        return;

    if (ImGui::BeginMenu("New"))
    {
        struct Kind { char const* label; TypeOfNet type; bool compact; };
        for (Kind const& kind : { Kind{ "Petri net", TypeOfNet::PetriNet, false },
                                  Kind{ "Timed Petri net", TypeOfNet::TimedPetriNet, false },
                                  Kind{ "Timed event graph", TypeOfNet::TimedPetriNet, true },
                                  Kind{ "GRAFCET", TypeOfNet::GRAFCET, false } })
        {
            if (ImGui::MenuItem(kind.label))
            {
                newDocument();
                switchOfNet(kind.type, kind.compact);
                net().name = kind.label;
            }
        }
        ImGui::Separator();
        if (ImGui::MenuItem("GEMMA (3 GRAFCETs)"))
        {
            newDocument();
            createGEMMADocument();
        }
        ImGui::EndMenu();
    }
    if (ImGui::MenuItem("Clear the document", "Ctrl+N"))
        m_states.request_new = true;

    ImGui::Separator();
    if (ImGui::MenuItem("Open...", "Ctrl+O"))
        m_states.request_load = true;
    if (ImGui::MenuItem("Open in a new document..."))
    {
        newDocument();
        m_states.file_dialog = States::FileDialog::Load;
    }
    if (ImGui::BeginMenu("Import"))
    {
        for (auto const& it : importers())
        {
            if (ImGui::MenuItem(it.format.c_str()))
            {
                m_states.file_dialog = States::FileDialog::Import;
                m_states.pending_importer = &it;
            }
        }
        ImGui::EndMenu();
    }

    ImGui::Separator();
    if (ImGui::MenuItem("Save", "Ctrl+S"))
        saveCurrentDocument();
    if (ImGui::MenuItem("Save as...", "Ctrl+Shift+S"))
        m_states.file_dialog = States::FileDialog::SaveAs;
    if (ImGui::BeginMenu("Export"))
    {
        for (auto const& it : exporters())
        {
            if (ImGui::MenuItem(it.format.c_str()))
            {
                m_states.file_dialog = States::FileDialog::Export;
                m_states.pending_exporter = &it;
            }
        }
        ImGui::EndMenu();
    }
    if (ImGui::MenuItem("Screenshot..."))
        m_states.file_dialog = States::FileDialog::Screenshot;

    if (m_documents.size() > 1u)
    {
        ImGui::Separator();
        if (ImGui::BeginMenu("Documents"))
        {
            for (size_t i = 0; i < m_documents.size(); ++i)
            {
                auto& doc = *m_documents[i];
                std::string label = doc.title() + (doc.isModified() ? " *" : "") + "##doc" + std::to_string(i);
                if (ImGui::MenuItem(label.c_str(), nullptr, i == m_active_document_index))
                    setActiveDocument(i);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Close this document"))
                closeDocument(&activeDocument());
            ImGui::EndMenu();
        }
    }

    ImGui::Separator();
    if (ImGui::MenuItem("Quit", "Ctrl+Q"))
        this->close();

    ImGui::EndMenu();
}

//------------------------------------------------------------------------------
void Editor::menuEdit()
{
    if (!ImGui::BeginMenu("Edit"))
        return;

    bool const running = simulation().isRunning();
    ImGui::BeginDisabled(running);
    if (ImGui::MenuItem("Undo", "Ctrl+Z"))
        undo();
    if (ImGui::MenuItem("Redo", "Ctrl+Y"))
        redo();
    ImGui::Separator();
    ImGui::TextDisabled("Copy, cut, paste: Ctrl+C, X, V on the canvas");

    ImGui::Separator();
    if (ImGui::BeginMenu("Type of net"))
    {
        struct Kind { char const* label; TypeOfNet type; bool compact; };
        for (Kind const& kind : { Kind{ "Petri net", TypeOfNet::PetriNet, false },
                                  Kind{ "Timed Petri net", TypeOfNet::TimedPetriNet, false },
                                  Kind{ "Timed event graph", TypeOfNet::TimedPetriNet, true },
                                  Kind{ "GRAFCET", TypeOfNet::GRAFCET, false } })
        {
            bool const current = (net().type() == kind.type) && (net().layout().compact == kind.compact);
            if (ImGui::MenuItem(kind.label, nullptr, current))
                switchOfNet(kind.type, kind.compact);
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Add a net to the document"))
    {
        if (ImGui::MenuItem("Petri net"))
            addNetToActiveDocument(TypeOfNet::PetriNet, "PetriNet" + std::to_string(activeDocument().netCount() + 1));
        if (ImGui::MenuItem("GRAFCET"))
            addNetToActiveDocument(TypeOfNet::GRAFCET, "GRAFCET" + std::to_string(activeDocument().netCount() + 1));
        ImGui::EndMenu();
    }
    if (ImGui::MenuItem("To canonical form"))
    {
        Net pn(net().type());
        toCanonicalForm(net(), pn);
        net() = pn;
        m_view.clearAllSelections();
    }

    ImGui::Separator();
    if (ImGui::MenuItem("Arrange top-down", nullptr, false, !net().isEmpty()))
    {
        auto action = std::make_unique<NetModificationAction>([this]() -> Net& { return net(); });
        action->before(net());
        arrangeTopDown(net());
        action->after(net());
        m_history.add(std::move(action));
        activeDocument().activeNet().view_state.center_request = Document::ViewState::CENTER_FRAMES;
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Place the nodes in layers from the initial steps, as a GRAFCET is drawn");
    bool const layout = m_force_directed.isRunning();
    if (ImGui::MenuItem(layout ? "Stop the spring layout" : "Spring layout", "L", layout))
    {
        if (layout)
            m_force_directed.reset();
        else
            springify();
    }
    if (ImGui::MenuItem("Align to the grid"))
    {
        auto action = std::make_unique<NetModificationAction>([this]() -> Net& { return net(); });
        action->before(net());
        for (auto& place : net().places())
        {
            Point& p = net().position(place);
            p.x = m_view.grid.snapValue(p.x);
            p.y = m_view.grid.snapValue(p.y);
        }
        for (auto& trans : net().transitions())
        {
            Point& p = net().position(trans);
            p.x = m_view.grid.snapValue(p.x);
            p.y = m_view.grid.snapValue(p.y);
        }
        action->after(net());
        m_history.add(std::move(action));
        net().modified = true;
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Clear the net"))
    {
        clearNet();
        m_view.clearAllSelections();
    }
    ImGui::EndDisabled();
    ImGui::EndMenu();
}

//------------------------------------------------------------------------------
void Editor::menuView()
{
    if (!ImGui::BeginMenu("View"))
        return;

    if (ImGui::MenuItem("Fit the net", "F"))
        activeDocument().activeNet().view_state.center_request = Document::ViewState::CENTER_FRAMES;
    if (ImGui::MenuItem("Zoom in", "Ctrl+=, wheel"))
        activeDocument().activeNet().view_state.zoom_request *= 1.25f;
    if (ImGui::MenuItem("Zoom out", "Ctrl+-, wheel"))
        activeDocument().activeNet().view_state.zoom_request *= 0.8f;

    ImGui::Separator();
    if (ImGui::MenuItem("Grid", nullptr, m_view.grid.show))
        m_view.grid.show ^= true;
    if (ImGui::MenuItem("Snap to the grid", "G", m_view.grid.snap))
        m_view.grid.snap ^= true;
    if (ImGui::MenuItem("Place identifiers", nullptr, m_states.show_place_identifiers))
        m_states.show_place_identifiers ^= true;
    if (ImGui::MenuItem("Transition identifiers", nullptr, m_states.show_transition_identifiers))
        m_states.show_transition_identifiers ^= true;
    if (ImGui::MenuItem("GRAFCET action boxes", nullptr, m_states.show_action_boxes))
        m_states.show_action_boxes ^= true;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Hidden: a small tag with the number of actions, for a compact net");

    ImGui::Separator();
    if (ImGui::BeginMenu("Theme"))
    {
        char const* names[] = { "Dark", "Light", "Classic" };
        for (int i = 0; i < 3; ++i)
        {
            if (ImGui::MenuItem(names[i], nullptr, int(theme()) == i))
            {
                theme() = ThemeId(i);
                applyTheme();
            }
        }
        ImGui::EndMenu();
    }
    if (ImGui::MenuItem("Reset the layout of the windows"))
        m_states.reset_layout = true;

    ImGui::Separator();
    if (ImGui::BeginMenu("Remote control"))
    {
#  if TPNE_USE_ZEROMQ
        bool is_running = m_remote && m_remote->isRunning();
        if (ImGui::MenuItem(is_running ? "Disable" : "Enable", nullptr, is_running))
            toggleRemoteControl();
        if (is_running)
        {
            ImGui::Separator();
            ImGui::TextDisabled("Endpoint: %s", m_remote->endpoint().c_str());
            if (!m_remote->error().empty())
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Error: %s", m_remote->error().c_str());
        }
#  else
        ImGui::MenuItem("Disabled at build time", nullptr, false, false);
#  endif
        ImGui::EndMenu();
    }
    ImGui::EndMenu();
}

//------------------------------------------------------------------------------
void Editor::menuSimulation()
{
    if (!ImGui::BeginMenu("Simulation"))
        return;

    bool const running = simulation().isRunning();
    if (ImGui::MenuItem(running ? "Stop" : "Start", "Space"))
        toogleStartSimulation();
    if (ImGui::MenuItem(m_states.paused ? "Resume" : "Pause", "P", m_states.paused, running))
        m_states.paused ^= true;
    if (ImGui::BeginMenu("Speed"))
    {
        for (float value : { 0.1f, 0.25f, 0.5f, 1.0f, 2.0f, 5.0f, 10.0f, 20.0f, 50.0f })
        {
            char label[32];
            std::snprintf(label, sizeof(label), "x%g", double(value));
            if (ImGui::MenuItem(label, nullptr, value == m_states.time_scale))
                m_states.time_scale = value;
        }
        ImGui::EndMenu();
    }

    // Conflicts between transitions (Petri nets and timed nets): the policy
    // and the seed of the random choices, to replay a simulation.
    if (ImGui::BeginMenu("Conflicts", net().type() != TypeOfNet::GRAFCET))
    {
        Engine::Options options = simulation().options();
        static std::pair<Engine::Policy, char const*> const POLICIES[] = {
            { Engine::Policy::Random, "Random" },
            { Engine::Policy::Priority, "Highest priority" },
            { Engine::Policy::FirstDeclared, "First declared" } };
        for (auto const& it : POLICIES)
        {
            if (ImGui::MenuItem(it.second, nullptr, options.policy == it.first, !running))
                options.policy = it.first;
        }
        ImGui::Separator();
        ImGui::BeginDisabled(running || (options.policy != Engine::Policy::Random));
        ImGui::SetNextItemWidth(180.0f);
        ImGui::InputScalar("Seed", ImGuiDataType_U64, &options.seed);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("0: a new seed at each simulation (written in the console)");
        if ((simulation().seed() != 0u) && ImGui::MenuItem("Replay the last simulation"))
            options.seed = simulation().seed();
        ImGui::EndDisabled();
        for (auto& entry : activeDocument().nets())
            entry->simulation->options(options);
        ImGui::EndMenu();
    }

    if (activeDocument().netCount() > 1)
    {
        bool any_running = activeDocument().isAnySimulationRunning();
        if (ImGui::MenuItem(any_running ? "Stop all the nets" : "Start all the nets"))
            toogleStartAllSimulations();
        ImGui::Separator();
        ImGui::TextDisabled("Nets of the document");
        for (size_t i = 0; i < activeDocument().netCount(); ++i)
        {
            auto& entry = activeDocument().getNet(i);
            std::string const label = entry.net.name + "##net" + std::to_string(i);
            if (ImGui::MenuItem(label.c_str(), nullptr, i == activeDocument().activeNetIndex()))
                activeDocument().setActiveNetIndex(i);
        }
    }
    ImGui::EndMenu();
}

//------------------------------------------------------------------------------
void Editor::menuGraphEvents()
{
    if (!ImGui::BeginMenu("Analysis"))
        return;

    bool const teg = net().layout().compact || isEventGraph(net());

    if (!teg)
        ImGui::TextDisabled("Needs a timed event graph");
    ImGui::BeginDisabled(!teg);
    char const* tabs[] = { "Critical circuit (cycle time)", "Gantt chart (earliest dates)",
                           "(max,+) linear system", "Dater and counter equations",
                           "Adjacency matrices", "Firing dates of the simulation" };
    for (int i = 0; i < 6; ++i)
    {
        if (ImGui::MenuItem(tabs[i]))
        {
            openAnalysis(i);
            if (i == 0)
            {
                CriticalCycleResult const cc = findCriticalCycle(net());
                if (cc.success)
                    markArcs(net(), cc.arcs);
            }
        }
    }
    ImGui::EndDisabled();
    ImGui::EndMenu();
}

//------------------------------------------------------------------------------
void Editor::menuDebug()
{
    if (!ImGui::BeginMenu("Debug"))
        return;

    // A program running the net (runtime, generated GRAFCET) publishes what
    // it does: the editor draws it.
    if (!m_monitor.isConnected())
    {
        ImGui::TextDisabled("Program publishing its journal (ZeroMQ):");
        ImGui::SetNextItemWidth(220.0f);
        ImGui::InputText("##endpoint", &m_monitor_endpoint);
        ImGui::SameLine();
        if (ImGui::Button("Connect"))
        {
            if (m_monitor.connect(m_monitor_endpoint))
            {
                m_journal.live(m_monitor_endpoint, net());
                m_messages.setInfo("Connected to " + m_monitor_endpoint + ": the Journal follows the program");
                ImGui::SetWindowFocus("Journal");
            }
            else
                m_messages.setError(m_monitor.error());
            ImGui::CloseCurrentPopup();
        }
    }
    else if (ImGui::MenuItem(("Disconnect from " + m_monitor.endpoint()).c_str()))
    {
        m_monitor.disconnect();
        m_journal.stopReplay();
        m_marks.clear("runtime");
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Open a manifest..."))
        FileDialogHelper::openLoad("ManifestDlgKey", "Open the manifest of a program", ".json");
    if (ImGui::MenuItem("Open a journal..."))
        m_journal.request_open = true;
    if (ImGui::MenuItem("Save the journal..."))
        m_journal.request_save = true;
    if (ImGui::MenuItem("Clear the marks of the nodes"))
    {
        m_marks.clear("runtime");
        m_marks.clear("journal");
        m_marks.clear("analysis");
    }
    ImGui::EndMenu();
}

//------------------------------------------------------------------------------
void Editor::programWindow()
{
    FileDialogHelper::display("ManifestDlgKey", [this](std::string const& path) {
        std::ifstream file(path);
        std::stringstream text;
        text << file.rdbuf();
        Manifest manifest;
        std::string const error = fromManifest(text.str(), manifest);
        if (!error.empty())
        {
            m_messages.setError("'" + path + "': " + error);
            return;
        }
        RuntimeMonitor::mark(net(), manifest, m_marks);
        m_messages.setInfo(std::to_string(manifest.unregistered.size()) +
                           " transitions without function in '" + path + "'");
    });

    if (!m_monitor.isConnected())
        return;
    if (!ImGui::Begin("Program"))
    {
        ImGui::End();
        return;
    }
    ImGui::Text("Connected to %s", m_monitor.endpoint().c_str());
    ImGui::SameLine();
    if (ImGui::SmallButton("Disconnect"))
    {
        m_monitor.disconnect();
        m_journal.stopReplay();
        m_marks.clear("runtime");
        ImGui::End();
        return;
    }
    Manifest const& manifest = m_monitor.manifest(net().name);
    if (manifest.net.empty() && manifest.transitions.empty())
        ImGui::TextDisabled("No manifest received yet");
    else
    {
        ImGui::Text("Net: %s", manifest.net.c_str());
        if (manifest.transitions.size() != net().transitions().size())
            ImGui::TextColored(ImVec4(0.95f, 0.55f, 0.05f, 1.0f),
                               "The program has %zu transitions, this net %zu: is it the same net?",
                               manifest.transitions.size(), net().transitions().size());
        if (!manifest.unregistered.empty())
        {
            ImGui::TextDisabled("Transitions without function (they fire at once):");
            for (auto const& name : manifest.unregistered)
                ImGui::BulletText("%s", name.c_str());
        }
    }
    if (!m_monitor.io().empty())
    {
        ImGui::SeparatorText("Inputs and outputs");
        for (auto const& line : m_monitor.io())
            ImGui::TextUnformatted(line.c_str());
    }
    ImGui::End();
}

//------------------------------------------------------------------------------
void Editor::menuHelp()
{
    if (!ImGui::BeginMenu("Help"))
        return;
    if (ImGui::MenuItem("Mouse and keys", "F1"))
        openHelp(0);
    if (ImGui::MenuItem("GRAFCET receptivities"))
        openHelp(1);
    if (ImGui::MenuItem("GRAFCET actions (qualifiers)"))
        openHelp(2);
    if (ImGui::MenuItem("GRAFCET control panel"))
        openHelp(3);
    ImGui::Separator();
    if (ImGui::MenuItem("About"))
        m_states.show_about = true;
    ImGui::EndMenu();
}

//------------------------------------------------------------------------------
void Editor::handleMenuActions()
{
    // Modal info dialogs
    if (m_states.show_help) { help(); }
    if (m_states.show_about) { about(); }
    if (m_states.show_theme) { showStyleSelector(); }

    // File dialogs (mutually exclusive)
    switch (m_states.file_dialog)
    {
        case States::FileDialog::Load:
            loadNetFile();
            break;
        case States::FileDialog::SaveAs:
            saveNetAs();
            break;
        case States::FileDialog::Export:
            if (m_states.pending_exporter != nullptr)
                exportNetTo(*m_states.pending_exporter);
            break;
        case States::FileDialog::Import:
            if (m_states.pending_importer != nullptr)
                importNetFrom(*m_states.pending_importer);
            break;
        case States::FileDialog::Screenshot:
            takeScreenshot();
            break;
        case States::FileDialog::None:
            break;
    }

    if ((m_states.request_save_frame > 0) && (ImGui::GetFrameCount() >= m_states.request_save_frame))
    {
        m_states.request_save_frame = 0;
        saveCurrentDocument();
    }

    // Analysis window (dockable, not modal)
    if (m_states.show_analysis) { analysisWindow(); }

    // Quit request handling
    if (m_states.request_quitting)
    {
        if (hasUnsavedChanges() &&
            m_states.file_dialog != States::FileDialog::SaveAs)
        {
            m_states.show_unsaved_dialog = true;
        }
        else if (!hasUnsavedChanges())
        {
            m_states.request_quitting = false;
            halt();
        }
    }

    // New document request handling
    if (m_states.request_new)
    {
        if (hasUnsavedChanges() &&
            m_states.file_dialog != States::FileDialog::SaveAs)
        {
            m_states.show_unsaved_dialog = true;
        }
        else
        {
            m_marked_arcs.clear();
            clearNet();
            activeDocument().info() = DocumentInfo{};
            discardUnsavedChanges();
            m_states.request_new = false;
        }
    }

    // Open (load) request handling: prompt to save the current document first if
    // it has unsaved changes, then open the load file dialog.
    if (m_states.request_load)
    {
        if (hasUnsavedChanges() &&
            m_states.file_dialog != States::FileDialog::SaveAs)
        {
            m_states.show_unsaved_dialog = true;
        }
        else
        {
            m_states.request_load = false;
            m_states.file_dialog = States::FileDialog::Load;
        }
    }

    // Unsaved changes dialog
    if (m_states.show_unsaved_dialog)
    {
        ImGui::OpenPopup("Unsaved Changes");
        m_states.show_unsaved_dialog = false;
    }
    showUnsavedChangesDialog();
}

//------------------------------------------------------------------------------
void Editor::showUnsavedChangesDialog()
{
    if (!ImGui::BeginPopupModal("Unsaved Changes", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
        return;

    ImGui::Text("The document has unsaved changes.");
    ImGui::Text("What do you want to do?");
    ImGui::Separator();

    if (ImGui::Button("Save", ImVec2(120, 0)))
    {
        m_states.file_dialog = States::FileDialog::SaveAs;
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Don't Save", ImVec2(120, 0)))
    {
        discardUnsavedChanges();
        fulfillPendingDocumentRequest();
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 0)))
    {
        m_states.request_quitting = false;
        m_states.request_new = false;
        m_states.request_load = false;
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

//------------------------------------------------------------------------------
void Editor::saveCurrentDocument()
{
    if (m_path_to_save == "")
    {
        m_states.file_dialog = States::FileDialog::SaveAs;
        return;
    }
    if (stopSimulationFor("save"))
    {
        // Saved at the next frame, once the marking is restored.
        m_states.request_save_frame = ImGui::GetFrameCount() + 1;
        return;
    }
    std::string const error = writeDocument(m_path_to_save);
    if (error.empty())
        m_messages.setInfo("Saved with success " + m_path_to_save);
    else
        m_messages.setError(error);
}

} // namespace tpne
