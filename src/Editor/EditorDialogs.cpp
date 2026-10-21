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
#include "PetriNet/Flowshop.hpp"
#include "Editor/Theme.hpp"

#include "PetriNet/Algorithms.hpp"
#include "PetriNet/SparseMatrix.hpp"
#include "imgui/imgui.h"
#include "implot/implot.h"
#include <algorithm>
#include <iomanip>
#include <set>
#include <sstream>
#include <vector>

namespace tpne {

namespace {

std::string formatTimeUnits(double const value)
{
    std::ostringstream os;
    os << std::fixed << std::setprecision(2) << value;
    return os.str();
}

} // namespace

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
            if (ImGui::BeginTabItem("Mouse and keys"))
            {
                ImGui::TextUnformatted(
                    "No mode to choose: what a button does depends on what is under the\n"
                    "mouse, and on whether the mouse moves before the button is released.\n"
                    "The status bar always says what the buttons would do now.\n");
                ImGuiTableFlags const flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                              ImGuiTableFlags_SizingFixedFit;
                if (ImGui::BeginTable("##mouse", 3, flags))
                {
                    ImGui::TableSetupColumn("Under the mouse");
                    ImGui::TableSetupColumn("Click");
                    ImGui::TableSetupColumn("Drag");
                    ImGui::TableHeadersRow();
                    char const* rows[][3] = {
                        { "nothing, left button", "add a place (first clears the selection)", "select in a rectangle" },
                        { "nothing, right button", "add a transition", "move the view" },
                        { "node, left button", "select it (Ctrl: add/remove)", "move it and the selection" },
                        { "node, right button", "menu of the node", "draw an arc from it" },
                        { "arc, left button", "select it", "select in a rectangle" },
                        { "anything, middle button", "", "move the view" },
                    };
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
                ImGui::TextUnformatted(
                    "\nAn arc dropped in the void creates a node of the other kind there;\n"
                    "between two nodes of the same kind, a node is inserted.\n\n"
                    "Double click: rename a node (receptivity of a GRAFCET transition),\n"
                    "              duration of an arc.\n"
                    "Wheel: zoom around the mouse. Shift+wheel: move the view.\n"
                    "F: fit the net.  G: snap to the grid.  L: spring layout.\n"
                    "+ / -: tokens of the place under the mouse.\n"
                    "Del, Backspace: delete.  Arrows: move the selection.\n"
                    "Ctrl+Z / Ctrl+Y: undo, redo.  Ctrl+C / X / V / A.\n"
                    "R, F5: start or stop the simulation.  Space+drag: move the view.  Esc: cancel.");
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
            if (ImGui::BeginTabItem("Receptivity Syntax"))
            {
                ImGui::TextUnformatted(
                    "GRAFCET receptivities are boolean expressions (IEC 60848) on inputs,\n"
                    "steps and time. Precedence: not, and, or. Parentheses are allowed.\n\n"
                    "  And:               a . b    (also a * b, a and b)\n"
                    "  Or:                a + b    (also a | b, a or b)\n"
                    "  Not:               /a       (also !a, not a)\n"
                    "  Constants:         1  0  true  false\n"
                    "  Step 42 active:    X42\n"
                    "  Step of a graph:   Safety:X3\n"
                    "  Rising edge:       ↑a       (also ^a, rising(a))\n"
                    "  Falling edge:      ↓a       (also falling(a))\n"
                    "  Time:              3s/X2    (true 3 s after X2 became active)\n"
                    "                     2s/a/5s  (and false 5 s after a became false)\n"
                    "                     units: ms s min h\n"
                    "  Integer input:     [count >= 3]   (= != < <= > >=)\n"
                    "  Input names:       Dcy, sensor_2, holding(cube) ...\n\n"
                    "Example:\n  X42 . (start + ↑button) . /emergency\n\n"
                    "Actions: N continuous, S set, R reset, D delayed, L limited,\n"
                    "SD, DS, SL stored and timed, P1/P0 pulse on (de)activation.\n"
                    "A condition makes an action conditional.");
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Pathes"))
            {
                ImGui::Text("Data path: %s", m_path.toString().c_str());
                ImGui::Text("Temporary path: %s", project::info::paths::tmp.c_str());
                ImGui::Text("Log path: %s", project::info::paths::log.c_str());
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

} // namespace tpne
