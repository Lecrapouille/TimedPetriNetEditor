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

#include "Editor.hpp"
#include "Remote/IRemoteControl.hpp"
#include "TimedPetriNetEditor/Algorithms.hpp"
#include "imgui/imgui.h"

namespace tpne {

//------------------------------------------------------------------------------
void Editor::menu()
{
    if (ImGui::BeginMainMenuBar())
    {
        // File menu
        menuFile();

        // View menu (NEW - contains Theme, grid options, caption options)
        menuView();

        // Actions menu
        menuActions();

        // Graph Events menu (only for event graphs)
        menuGraphEvents();

        // Help menu
        menuHelp();

        ImGui::EndMainMenuBar();
    }

    // Store viewport center for dialogs
    m_states.viewport_center = ImGui::GetMainViewport()->GetCenter();

    // Handle pending actions
    handleMenuActions();
}

//------------------------------------------------------------------------------
void Editor::menuFile()
{
    if (!ImGui::BeginMenu("File"))
        return;

    // New document options
    if (ImGui::MenuItem("New Document", nullptr, false))
    {
        m_states.request_new = true;
    }
    if (ImGui::MenuItem("New Document (Keep Current)", nullptr, false))
    {
        newDocument();
    }

    ImGui::Separator();

    // Open/Import
    if (ImGui::MenuItem("Open", nullptr, false))
    {
        m_states.file_dialog = States::FileDialog::Load;
    }
    if (ImGui::MenuItem("Open in New Document", nullptr, false))
    {
        newDocument();
        m_states.file_dialog = States::FileDialog::Load;
    }
    if (ImGui::BeginMenu("Import from"))
    {
        for (auto const& it : importers())
        {
            if (ImGui::MenuItem(it.format.c_str(), nullptr, false))
            {
                m_states.file_dialog = States::FileDialog::Import;
                m_states.pending_importer = &it;
            }
        }
        ImGui::EndMenu();
    }

    ImGui::Separator();

    // Save/Export
    if (ImGui::MenuItem("Save", nullptr, false))
    {
        saveCurrentDocument();
    }
    if (ImGui::MenuItem("Save as", nullptr, false))
    {
        m_states.file_dialog = States::FileDialog::SaveAs;
    }
    if (ImGui::BeginMenu("Export to"))
    {
        for (auto const& it : exporters())
        {
            if (ImGui::MenuItem(it.format.c_str(), nullptr, false))
            {
                m_states.file_dialog = States::FileDialog::Export;
                m_states.pending_exporter = &it;
            }
        }
        ImGui::EndMenu();
    }

    ImGui::Separator();

    // Add Net submenu (moved from Edit menu)
    if (ImGui::BeginMenu("Add Net"))
    {
        if (ImGui::MenuItem("Add Petri Net"))
        {
            addNetToActiveDocument(TypeOfNet::PetriNet,
                "PetriNet" + std::to_string(activeDocument().netCount() + 1));
        }
        if (ImGui::MenuItem("Add GRAFCET"))
        {
            addNetToActiveDocument(TypeOfNet::GRAFCET,
                "GRAFCET" + std::to_string(activeDocument().netCount() + 1));
        }
        ImGui::Separator();
        ImGui::TextDisabled("GEMMA Templates:");
        if (ImGui::MenuItem("Create GEMMA Document (3 graphs)"))
        {
            createGEMMADocument();
        }
        ImGui::EndMenu();
    }

    // Multi-document management
    if (m_documents.size() > 1)
    {
        ImGui::Separator();
        ImGui::TextDisabled("Open Documents:");
        for (size_t i = 0; i < m_documents.size(); ++i)
        {
            auto& doc = *m_documents[i];
            std::string label = doc.title();
            if (doc.isModified())
            {
                label += " *";
            }
            bool is_active = (i == m_active_document_index);
            if (ImGui::MenuItem(label.c_str(), nullptr, is_active))
            {
                setActiveDocument(i);
            }
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Close Document"))
        {
            closeDocument(&activeDocument());
        }
    }

    ImGui::Separator();

    // Exit
    if (ImGui::MenuItem("Exit", nullptr, false))
    {
        this->close();
    }

    ImGui::EndMenu();
}

//------------------------------------------------------------------------------
void Editor::menuView()
{
    if (!ImGui::BeginMenu("View"))
        return;

    // Theme (moved from Help menu)
    if (ImGui::MenuItem("Theme...", nullptr, false))
    {
        m_states.show_theme = true;
    }

    ImGui::Separator();

    // Grid options
    if (ImGui::MenuItem("Show Grid", nullptr, m_view.grid.show))
    {
        m_view.grid.show ^= true;
    }
    if (ImGui::MenuItem("Snap to Grid", "G", m_view.grid.snap))
    {
        m_view.grid.snap ^= true;
    }

    ImGui::Separator();

    // Caption options
    if (ImGui::MenuItem("Show Place Captions", nullptr, m_states.show_place_captions))
    {
        m_states.show_place_captions ^= true;
    }
    if (ImGui::MenuItem("Show Transition Captions", nullptr, m_states.show_transition_captions))
    {
        m_states.show_transition_captions ^= true;
    }

    ImGui::Separator();

    // Remote control section
    if (ImGui::BeginMenu("Remote Control"))
    {
        bool is_running = m_remote && m_remote->isRunning();
        if (ImGui::MenuItem(is_running ? "Disable" : "Enable", nullptr, is_running))
        {
            toggleRemoteControl();
        }

        // Show connection status and endpoint
        if (is_running)
        {
            ImGui::Separator();
            ImGui::TextDisabled("Endpoint: %s", m_remote->endpoint().c_str());
            if (!m_remote->error().empty())
            {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Error: %s", m_remote->error().c_str());
            }
        }
        ImGui::EndMenu();
    }

    ImGui::Separator();

    // Screenshot
    if (ImGui::MenuItem("Take Screenshot", nullptr, false))
    {
        m_states.file_dialog = States::FileDialog::Screenshot;
    }

    ImGui::EndMenu();
}

//------------------------------------------------------------------------------
void Editor::menuActions()
{
    if (!ImGui::BeginMenu("Actions"))
        return;

    // Type of net submenu
    if (ImGui::BeginMenu("Type of Net"))
    {
        static int current_type = int(net().type());
        current_type = int(net().type());
        if (ImGui::RadioButton("Petri net", &current_type, 0))
            switchOfNet(TypeOfNet::PetriNet);
        if (ImGui::RadioButton("Timed Petri net", &current_type, 1))
            switchOfNet(TypeOfNet::TimedPetriNet);
        if (ImGui::RadioButton("Timed event graph", &current_type, 2))
            switchOfNet(TypeOfNet::TimedEventGraph);
        if (ImGui::RadioButton("GRAFCET", &current_type, 3))
            switchOfNet(TypeOfNet::GRAFCET);
        ImGui::EndMenu();
    }

    if (ImGui::MenuItem("To Canonical Form"))
    {
        Net pn(net().type());
        toCanonicalForm(net(), pn);
        net() = pn;
    }

    ImGui::Separator();

    // Undo/Redo
    if (ImGui::MenuItem("Undo", "Ctrl+Z", false))
    {
        undo();
    }
    if (ImGui::MenuItem("Redo", "Ctrl+Y", false))
    {
        redo();
    }

    ImGui::Separator();

    // Edit operations
    if (ImGui::MenuItem("Clear Net", nullptr, false))
    {
        clearNet();
    }
    if (ImGui::MenuItem("Align Nodes to Grid", nullptr, false))
    {
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
        net().modified = true;
    }

    // Force-directed layout
    bool is_layout_running = m_spring.isRunning();
    if (ImGui::MenuItem(is_layout_running ? "Stop Auto-Layout" : "Auto-Layout (Force-Directed)",
                        "F", is_layout_running))
    {
        if (is_layout_running)
        {
            m_spring.reset();
            m_messages.setInfo("Auto-layout stopped");
        }
        else
        {
            springify();
            m_messages.setInfo("Auto-layout started (force-directed algorithm)");
        }
    }
    if (is_layout_running)
    {
        ImGui::TextDisabled("  Temperature: %.1f", m_spring.temperature());
    }

    ImGui::Separator();

    // Simulation controls
    if (ImGui::MenuItem(simulation().running ? "Stop Simulation" : "Start Simulation",
                        "Space", false))
    {
        toogleStartSimulation();
    }

    // Show "Start/Stop All" when document has multiple nets
    if (activeDocument().netCount() > 1)
    {
        bool any_running = activeDocument().isAnySimulationRunning();
        if (ImGui::MenuItem(any_running ? "Stop All Simulations" : "Start All Simulations",
                            nullptr, false))
        {
            toogleStartAllSimulations();
        }
    }

    // Show nets in document (when multiple)
    if (activeDocument().netCount() > 1)
    {
        ImGui::Separator();
        ImGui::TextDisabled("Nets in Document:");
        for (size_t i = 0; i < activeDocument().netCount(); ++i)
        {
            auto& entry = activeDocument().getNet(i);
            bool is_active = (i == activeDocument().activeNetIndex());
            if (ImGui::MenuItem(entry.net.name.c_str(), nullptr, is_active))
            {
                activeDocument().setActiveNetIndex(i);
            }
        }
    }

    ImGui::EndMenu();
}

//------------------------------------------------------------------------------
void Editor::menuGraphEvents()
{
    if ((net().type() != TypeOfNet::TimedEventGraph) && !isEventGraph(net()))
        return;

    if (!ImGui::BeginMenu("Graph Events"))
        return;

    if (ImGui::MenuItem("Show Critical Circuit", nullptr, false))
    {
        m_states.do_find_critical_cycle = true;
    }
    if (ImGui::MenuItem("Show (max, +) Dynamic Linear System", nullptr, false))
    {
        m_states.do_syslin = true;
    }
    if (ImGui::MenuItem("Show Dater or Counter Equations", nullptr, false))
    {
        m_states.do_counter_or_dater = true;
    }
    if (ImGui::MenuItem("Show Adjacency Matrices", nullptr, false))
    {
        m_states.do_adjency = true;
    }

    ImGui::EndMenu();
}

//------------------------------------------------------------------------------
void Editor::menuHelp()
{
    if (!ImGui::BeginMenu("Help"))
        return;

    if (ImGui::MenuItem("Help", nullptr, false))
    {
        m_states.show_help = true;
    }

    ImGui::Separator();

    if (ImGui::MenuItem("About", nullptr, false))
    {
        m_states.show_about = true;
    }

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

    // Algorithm analysis dialogs
    if (m_states.do_adjency) { showAdjacencyMatrices(); }
    if (m_states.do_counter_or_dater) { showCounterOrDaterEquation(); }
    if (m_states.do_syslin) { showDynamicLinearSystem(); }
    if (m_states.do_find_critical_cycle) { showCriticalCycles(); }

    // Quit request handling
    if (m_states.request_quitting)
    {
        if (net().modified)
        {
            m_states.show_unsaved_dialog = true;
            m_states.request_quitting = false;
        }
        else
        {
            halt();
        }
    }

    // New document request handling
    if (m_states.request_new)
    {
        if (net().modified)
        {
            m_states.show_unsaved_dialog = true;
            m_states.request_new = false;
        }
        else
        {
            m_marked_arcs.clear();
            clearNet();
            net().modified = false;
            m_path_to_save.clear();
            m_states.request_new = false;
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
        net().modified = false;
        halt();
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 0)))
    {
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
