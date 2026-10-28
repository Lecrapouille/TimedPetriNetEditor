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
#include "imgui/misc/cpp/imgui_stdlib.h"
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
                                   "duration and the weight of an arc. Right click on a place: its tokens "
                                   "and its max of tokens; on an arc: its kind and the color of its "
                                   "tokens. Wheel: zoom around the mouse; Shift + wheel: move the view.");
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

//------------------------------------------------------------------------------
//! \brief Resize the table of the flowshop being filled, keeping what was
//! typed. New names M<i> and C<j>, one resource of each, processing time 1.
static void resizeFlowshopForm(Editor::FlowshopForm& form, size_t machines, size_t conveyors)
{
    for (size_t m = form.machines.size(); m < machines; ++m)
        form.machines.push_back("M" + std::to_string(m + 1u));
    for (size_t c = form.conveyors.size(); c < conveyors; ++c)
        form.conveyors.push_back("C" + std::to_string(c + 1u));
    form.machines.resize(machines);
    form.conveyors.resize(conveyors);
    form.nm.resize(machines, 1);
    form.np.resize(conveyors, 1);
    form.times.resize(machines);
    for (auto& row : form.times)
        row.resize(conveyors, "1");
}

//------------------------------------------------------------------------------
//! \brief The flowshop of the table, or the error. Names: not empty, unique,
//! without space (the .flowshop file splits on them), machines without '_'
//! (the operation "<machine>_<conveyor>" is split on the first one).
static std::string flowshopOfForm(Editor::FlowshopForm const& form, Flowshop& shop)
{
    auto const checkNames = [](std::vector<std::string> const& names, char const* what,
                               bool underscore) -> std::string {
        std::set<std::string> seen;
        for (auto const& name : names)
        {
            if (name.empty())
                return std::string("A ") + what + " has no name";
            if (name.find_first_of(" \t") != std::string::npos)
                return std::string("The ") + what + " '" + name + "' has a space in its name";
            if (!underscore && (name.find('_') != std::string::npos))
                return std::string("The ") + what + " '" + name + "' has a '_' in its name";
            if (!seen.insert(name).second)
                return std::string("Two ") + what + "s are named '" + name + "'";
        }
        return {};
    };
    std::string error = checkNames(form.machines, "machine", false);
    if (error.empty())
        error = checkNames(form.conveyors, "conveyor", true);
    if (!error.empty())
        return error;

    shop = Flowshop();
    shop.nmachines = form.machines.size();
    shop.nconveyors = form.conveyors.size();
    shop.machineNames = form.machines;
    shop.conveyorNames = form.conveyors;
    for (int const n : form.nm)
        shop.nm.push_back(size_t(std::max(0, n)));
    for (int const n : form.np)
        shop.np.push_back(size_t(std::max(0, n)));
    for (size_t m = 0u; m < form.times.size(); ++m)
    {
        std::vector<float> row;
        for (size_t c = 0u; c < form.times[m].size(); ++c)
        {
            std::string const& text = form.times[m][c];
            size_t const first = text.find_first_not_of(" \t");
            std::string const value = (first == std::string::npos) ? std::string()
                : text.substr(first, text.find_last_not_of(" \t") - first + 1u);
            if (value.empty() || (value == "-") || (value == "-inf") || (value == "eps") || (value == "%0"))
            {
                row.push_back(flowshopNoOperation());
                continue;
            }
            char* end = nullptr;
            float const time = std::strtof(value.c_str(), &end);
            if ((end == value.c_str()) || (*end != '\0') || (time < 0.0f))
                return "Machine " + form.machines[m] + ", conveyor " + form.conveyors[c] +
                       ": '" + value + "' is not a duration";
            row.push_back(time);
        }
        shop.PT.push_back(row);
    }
    return checkFlowshop(shop);
}

//------------------------------------------------------------------------------
//! \brief The table filled with a flowshop (processing times as written in
//! the .csv files).
static void formOfFlowshop(Flowshop const& shop, Editor::FlowshopForm& form)
{
    form.machines = shop.machineNames;
    form.conveyors = shop.conveyorNames;
    form.nm.assign(shop.nm.begin(), shop.nm.end());
    form.np.assign(shop.np.begin(), shop.np.end());
    form.times.assign(shop.nmachines, {});
    for (size_t m = 0u; m < shop.nmachines; ++m)
    {
        for (size_t c = 0u; c < shop.nconveyors; ++c)
        {
            std::ostringstream time;
            if (!flowshopIsNoOperation(shop.PT[m][c]))
                time << shop.PT[m][c];
            form.times[m].push_back(time.str());
        }
    }
}

//------------------------------------------------------------------------------
void Editor::newFlowshopDialog()
{
    FlowshopForm& form = m_states.flowshop;
    if (form.machines.empty())
        resizeFlowshopForm(form, 2u, 3u);

    ImGui::OpenPopup("New flowshop");
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(760.0f, 400.0f), ImGuiCond_Appearing);
    if (!ImGui::BeginPopupModal("New flowshop", nullptr))
        return;

    int machines = int(form.machines.size());
    int conveyors = int(form.conveyors.size());
    ImGui::SetNextItemWidth(100.0f);
    bool resized = ImGui::InputInt("Machines", &machines);
    ImGui::SameLine(0.0f, 24.0f);
    ImGui::SetNextItemWidth(100.0f);
    resized |= ImGui::InputInt("Conveyors", &conveyors);
    if (resized)
        resizeFlowshopForm(form, size_t(std::clamp(machines, 1, 64)), size_t(std::clamp(conveyors, 1, 64)));
    ImGui::SameLine(0.0f, 24.0f);
    Flowshop current;
    bool const is_flowshop = flowshopOfNet(net(), current).empty();
    ImGui::BeginDisabled(!is_flowshop);
    if (ImGui::Button("From the active net"))
    {
        formOfFlowshop(current, form);
        form.error.clear();
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip(is_flowshop ? "Fill the table with the flowshop of the active net"
                                      : "The active net is not a flowshop");

    float const footer = ImGui::GetFrameHeightWithSpacing() * (form.error.empty() ? 1.5f : 2.5f);
    ImGuiTableFlags const flags = ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY |
                                  ImGuiTableFlags_SizingFixedFit;
    if (ImGui::BeginTable("##flowshop", int(form.conveyors.size()) + 1, flags, ImVec2(0.0f, -footer)))
    {
        // The names and their units in the first row and column, as headers:
        // the other cells are only processing times.
        ImU32 const header = ImGui::GetColorU32(ImGuiCol_TableHeaderBg);
        auto const named = [](std::string& name, int& units, float width) {
            ImGui::SetNextItemWidth(width);
            ImGui::InputText("##name", &name);
            ImGui::SameLine(0.0f, 4.0f);
            ImGui::TextDisabled("x");
            ImGui::SameLine(0.0f, 4.0f);
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::InputInt("##units", &units, 0, 0))
                units = std::max(0, units);
        };
        ImGui::TableSetupScrollFreeze(1, 1);
        ImGui::TableSetupColumn("Machine", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        for (size_t c = 0u; c < form.conveyors.size(); ++c)
            ImGui::TableSetupColumn(form.conveyors[c].c_str(), ImGuiTableColumnFlags_WidthFixed, 120.0f);

        // First row: the conveyors, how many of each.
        ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, header);
        ImGui::TableNextColumn();
        ImGui::TextDisabled("Machine \\ Conveyor (?)");
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(360.0f);
            ImGui::TextUnformatted(
                "Each part visits the machines from top to bottom, each machine works on "
                "the conveyors from left to right.");
            ImGui::Spacing();
            ImGui::TextUnformatted(
                "Name x n: the factory owns n machines, or conveyors, of this kind (the "
                "tokens of its cycle).");
            ImGui::Spacing();
            ImGui::TextUnformatted(
                "Cells: processing time of the conveyor on the machine, in the time unit "
                "of the net (the durations of the arcs). Empty or '-': the conveyor does "
                "not visit the machine.");
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
        for (size_t c = 0u; c < form.conveyors.size(); ++c)
        {
            ImGui::TableNextColumn();
            ImGui::PushID(int(c));
            named(form.conveyors[c], form.np[c], 60.0f);
            ImGui::PopID();
        }

        // A row per machine: its name, how many, the processing times.
        for (size_t m = 0u; m < form.machines.size(); ++m)
        {
            ImGui::PushID(int(m) + 1000);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, header);
            named(form.machines[m], form.nm[m], 90.0f);
            for (size_t c = 0u; c < form.conveyors.size(); ++c)
            {
                ImGui::TableNextColumn();
                ImGui::PushID(int(c));
                ImGui::SetNextItemWidth(-FLT_MIN);
                ImGui::InputTextWithHint("##time", "-", &form.times[m][c]);
                ImGui::PopID();
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    if (!form.error.empty())
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(palette().critical), "%s", form.error.c_str());
    bool close = false;
    if (ImGui::Button("Create", ImVec2(120.0f, 0.0f)))
    {
        Flowshop shop;
        form.error = flowshopOfForm(form, shop);
        if (form.error.empty())
        {
            newDocument();
            net().clear();
            buildFlowshop(net(), shop);
            net().name = "Flowshop";
            net().modified = true;
            activeDocument().activeNet().view_state.center_request = Document::ViewState::CENTER_FRAMES;
            m_busy_frames = BUSY_FRAMES;
            m_messages.setInfo("Created a flowshop of " + std::to_string(shop.nmachines) + " machines and " +
                               std::to_string(shop.nconveyors) + " conveyors");
            close = true;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f)) || ImGui::IsKeyPressed(ImGuiKey_Escape, false))
        close = true;
    if (close)
    {
        form.error.clear();
        form.open = false;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

} // namespace tpne
