// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Editor/Editor.hpp"
#include "MaxPlus/Flowshop.hpp"
#include "Editor/Theme.hpp"
#include "Editor/GrafcetHelp.hpp"

#include "MaxPlus/Algorithms.hpp"
#include "MaxPlus/SparseMatrix.hpp"
#include "imgui/imgui.h"
#include "implot/implot.h"
#include <algorithm>
#include <iomanip>
#include <set>
#include <sstream>
#include <vector>

namespace tpne {


//------------------------------------------------------------------------------
void Editor::showStyleSelector()
{
    ImGui::OpenPopup("Theme selector");
    ImGui::SetNextWindowPos(m_states.viewport_center, ImGuiCond_Appearing,
                            ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Theme selector",
                               NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        int idx = static_cast<int>(theme());
        if (ImGui::Combo("Colors##Selector", &idx, "Dark\0Light\0Classic\0"))
        {
            theme() = static_cast<ThemeId>(idx);
            switch (theme())
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
                            std::to_string(project::info::version::major) + '.' +
                            std::to_string(project::info::version::minor) + '.' +
                            std::to_string(project::info::version::patch));
        ImGui::Text("%s", version.c_str());
        ImGui::Separator();
        ImGui::Text("https://github.com/Lecrapouille/TimedPetriNetEditor");
        ImGui::Text("Git branch: %s", project::info::git::branch.c_str());
        ImGui::Text("Git SHA1: %s", project::info::git::sha1.c_str());
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

//! \brief Table of rows of cells, in the help.
static void helpTable(char const* id, std::initializer_list<char const*> headers,
                      std::initializer_list<std::initializer_list<char const*>> rows)
{
    ImGuiTableFlags const flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                  ImGuiTableFlags_SizingFixedFit;
    if (!ImGui::BeginTable(id, int(headers.size()), flags))
        return;
    for (char const* header : headers)
        ImGui::TableSetupColumn(header);
    ImGui::TableHeadersRow();
    for (auto const& row : rows)
    {
        ImGui::TableNextRow();
        for (char const* cell : row)
        {
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(cell);
        }
    }
    ImGui::EndTable();
}

//------------------------------------------------------------------------------
void Editor::help() const
{
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::OpenPopup("Help TimedPetriNetEditor");
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImVec2 const work = ImGui::GetMainViewport()->WorkSize;
    ImGui::SetNextWindowSize(ImVec2(std::min(760.0f, work.x - 40.0f), std::min(620.0f, work.y - 40.0f)),
                             ImGuiCond_Appearing);
    if (ImGui::BeginPopupModal("Help TimedPetriNetEditor", NULL, ImGuiWindowFlags_None))
    {
        // A tab asked by a menu or a button comes in front once.
        int const tab = m_states.help_tab;
        m_states.help_tab = -1;
        auto const flags = [tab](int i) {
            return (tab == i) ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
        };
        ImGui::BeginChild("##help", ImVec2(0.0f, -ImGui::GetFrameHeightWithSpacing()));
        if (ImGui::BeginTabBar("help"))
        {
            if (ImGui::BeginTabItem("Mouse and keys", nullptr, flags(0)))
            {
                ImGui::TextWrapped("No mode to choose: what a button does depends on what is under "
                                   "the mouse, and on whether the mouse moves before the button is "
                                   "released. The left button draws (places, arcs, selection), the "
                                   "right button moves (nodes, view). The status bar always says "
                                   "what the buttons would do now.");
                ImGui::Spacing();
                helpTable("##mouse", { "Under the mouse", "Click", "Drag" }, {
                    { "nothing, left button", "add a place (first clears the selection)", "select in a rectangle" },
                    { "nothing, right button", "add a transition", "move the view" },
                    { "node, left button", "select it (Ctrl: add/remove)", "draw an arc from it" },
                    { "node, right button", "menu of the node", "move it and the selection" },
                    { "arc, left button", "select it", "select in a rectangle" },
                    { "arc, right button", "menu of the arc", "move the view" },
                    { "anything, middle button", "", "move the view" },
                });
                ImGui::Spacing();
                ImGui::TextWrapped("An arc released in the void creates a node of the other kind there; "
                                   "between two nodes of the same kind, a node is inserted. Double click: "
                                   "rename a node (the receptivity of a GRAFCET transition), or edit the "
                                   "duration of an arc. Wheel: zoom around the mouse; Shift + wheel: move "
                                   "the view.");
                ImGui::Spacing();
                helpTable("##keys", { "Key", "Action" }, {
                    { "Space", "start or stop the simulation" },
                    { "F", "fit the net in its window" },
                    { "G", "snap the nodes to the grid" },
                    { "L", "spring layout (again to stop)" },
                    { "+ / -", "tokens of the place under the mouse" },
                    { "Delete, Backspace", "delete the selection, else what is under the mouse" },
                    { "Arrows (Shift: by 1)", "move the selection" },
                    { "Esc", "cancel the current drag, else clear the selection" },
                    { "Ctrl+Z, Ctrl+Y", "undo, redo" },
                    { "Ctrl+C, X, V, A", "copy, cut, paste at the mouse, select all" },
                    { "Ctrl+N, O, S, Shift+S, Q", "new, open, save, save as, quit" },
                    { "Ctrl+=, Ctrl+-, Ctrl+0", "zoom in, zoom out, fit" },
                    { "F7", "GRAFCET: apply the inputs changed together" },
                    { "F1", "this help" },
                });
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("GRAFCET receptivities", nullptr, flags(1)))
            {
                receptivitiesHelp();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("GRAFCET actions", nullptr, flags(2)))
            {
                qualifiersHelp();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("GRAFCET control panel", nullptr, flags(3)))
            {
                controlPanelHelp();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Paths", nullptr, flags(4)))
            {
                ImGui::Text("Data path: %s", m_path.toString().c_str());
                ImGui::Text("Temporary path: %s", project::info::paths::tmp.c_str());
                ImGui::Text("Log path: %s", project::info::paths::log.c_str());
                ImGui::Text("Settings: %s", settingsFolder().string().c_str());
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::EndChild();

        if (ImGui::Button("OK", ImVec2(120, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape, false))
        {
            ImGui::CloseCurrentPopup();
            m_states.show_help = false;
        }

        ImGui::EndPopup();
    }
}

} // namespace tpne
