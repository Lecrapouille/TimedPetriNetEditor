//=============================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2026 Quentin Quadrat <lecrapouille@gmail.com>
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

#include "Editor/Editor.hpp"
#include "Editor/EditorState.hpp"
#include "Editor/Theme.hpp"
#include "Remote/IRemoteControl.hpp"
#include "PetriNet/Algorithms.hpp"
#include "PetriNet/Layout.hpp"
#include "imgui/imgui.h"

#include <algorithm>
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
    ImGuiIO const& io = ImGui::GetIO();
    if (io.WantTextInput)
        return;
    auto const pressed = [](ImGuiKeyChord chord) {
        return ImGui::Shortcut(chord, ImGuiInputFlags_RouteGlobal);
    };
    if (pressed(ImGuiMod_Ctrl | ImGuiKey_N)) m_states.request_new = true;
    if (pressed(ImGuiMod_Ctrl | ImGuiKey_O)) m_states.request_load = true;
    if (pressed(ImGuiMod_Ctrl | ImGuiKey_S)) saveCurrentDocument();
    if (pressed(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S)) m_states.file_dialog = States::FileDialog::SaveAs;
    if (pressed(ImGuiMod_Ctrl | ImGuiKey_Q)) close();
    if (pressed(ImGuiKey_F5)) toogleStartSimulation();
    if (pressed(ImGuiMod_Ctrl | ImGuiKey_Equal)) m_view.zoomBy(1.25f);
    if (pressed(ImGuiMod_Ctrl | ImGuiKey_Minus)) m_view.zoomBy(0.8f);
    if (pressed(ImGuiMod_Ctrl | ImGuiKey_0))
        activeDocument().activeNet().view_state.center_request = Document::ViewState::CENTER_FRAMES;
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
        ImGui::SetTooltip("Start or stop the simulation (R, F5)");

    // Type of the active net.
    ImGui::SetNextItemWidth(150.0f);
    int type = int(net().type());
    ImGui::BeginDisabled(running);
    if (ImGui::Combo("##type", &type, "Petri net\0Timed Petri net\0Timed event graph\0GRAFCET\0"))
        switchOfNet(TypeOfNet(type));
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
    if ((net().type() == TypeOfNet::TimedEventGraph) || isEventGraph(net()))
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
        struct Kind { char const* label; TypeOfNet type; };
        for (Kind const& kind : { Kind{ "Petri net", TypeOfNet::PetriNet },
                                  Kind{ "Timed Petri net", TypeOfNet::TimedPetriNet },
                                  Kind{ "Timed event graph", TypeOfNet::TimedEventGraph },
                                  Kind{ "GRAFCET", TypeOfNet::GRAFCET } })
        {
            if (ImGui::MenuItem(kind.label))
            {
                newDocument();
                switchOfNet(kind.type);
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
        struct Kind { char const* label; TypeOfNet type; };
        for (Kind const& kind : { Kind{ "Petri net", TypeOfNet::PetriNet },
                                  Kind{ "Timed Petri net", TypeOfNet::TimedPetriNet },
                                  Kind{ "Timed event graph", TypeOfNet::TimedEventGraph },
                                  Kind{ "GRAFCET", TypeOfNet::GRAFCET } })
        {
            if (ImGui::MenuItem(kind.label, nullptr, net().type() == kind.type))
                switchOfNet(kind.type);
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
        net().generateArcsInArcsOut();
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
            place.x = m_view.grid.snapValue(place.x);
            place.y = m_view.grid.snapValue(place.y);
        }
        for (auto& trans : net().transitions())
        {
            trans.x = m_view.grid.snapValue(trans.x);
            trans.y = m_view.grid.snapValue(trans.y);
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
        m_view.zoomBy(1.25f);
    if (ImGui::MenuItem("Zoom out", "Ctrl+-, wheel"))
        m_view.zoomBy(0.8f);

    ImGui::Separator();
    if (ImGui::MenuItem("Grid", nullptr, m_view.grid.show))
        m_view.grid.show ^= true;
    if (ImGui::MenuItem("Snap to the grid", "G", m_view.grid.snap))
        m_view.grid.snap ^= true;
    if (ImGui::MenuItem("Place captions", nullptr, m_states.show_place_captions))
        m_states.show_place_captions ^= true;
    if (ImGui::MenuItem("Transition captions", nullptr, m_states.show_transition_captions))
        m_states.show_transition_captions ^= true;

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
    if (ImGui::MenuItem(running ? "Stop" : "Start", "R, F5"))
        toogleStartSimulation();
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

    bool const teg = (net().type() == TypeOfNet::TimedEventGraph) || isEventGraph(net());
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
                    m_marked_arcs = cc.arcs;
            }
        }
    }
    ImGui::EndDisabled();
    ImGui::EndMenu();
}

//------------------------------------------------------------------------------
void Editor::menuHelp()
{
    if (!ImGui::BeginMenu("Help"))
        return;
    if (ImGui::MenuItem("Mouse, keys and syntax"))
        m_states.show_help = true;
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
    }
    else if (activeDocument().netCount() > 1)
    {
        std::vector<Net> nets;
        auto& doc = activeDocument();
        nets.reserve(doc.netCount());
        for (size_t i = 0; i < doc.netCount(); ++i)
            nets.push_back(doc.getNet(i).net);

        std::string error = exportAllNetsToJSON(nets, m_path_to_save);
        if (error.empty())
        {
            m_messages.setInfo("Saved with success " + m_path_to_save);
            doc.setModified(false);
            for (size_t i = 0; i < doc.netCount(); ++i)
                doc.getNet(i).net.modified = false;
        }
        else
        {
            m_messages.setError(error);
        }
    }
    else
    {
        std::string error = saveToFile(net(), m_path_to_save);
        if (error.empty())
        {
            m_messages.setInfo("Saved with success " + m_path_to_save);
            net().modified = false;
        }
        else
        {
            m_messages.setError(error);
        }
    }
}

} // namespace tpne
