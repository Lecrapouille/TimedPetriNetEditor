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
#include "Editor/Theme.hpp"

#include "PetriNet/Algorithms.hpp"
#include "PetriNet/SparseMatrix.hpp"
#include "imgui/imgui.h"
#include <iomanip>
#include <set>
#include <sstream>

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
void Editor::showAdjacencyMatrices() const
{
    ImGui::OpenPopup("Show adjacency matrices");
    ImGui::SetNextWindowPos(m_states.viewport_center, ImGuiCond_Appearing,
                            ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Show adjacency matrices",
                               NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        static bool display_as_dense = false;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::Checkbox("Dense matrix", &display_as_dense);
        ImGui::PopStyleVar();

        SparseMatrix<MaxPlus> tokens; SparseMatrix<MaxPlus> durations;
        toAdjacencyMatrices(net(), tokens, durations);

        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
        if (ImGui::BeginTabBar("adjacency", tab_bar_flags))
        {
            if (ImGui::BeginTabItem("Durations"))
            {
                std::stringstream txt;
                printSparseMatrix(txt, durations, IndexingStyle::CppStyle,
                                  display_as_dense ? DisplayFormat::Dense : DisplayFormat::Sparse);
                ImGui::Text("%s", txt.str().c_str());
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Tokens"))
            {
                std::stringstream txt;
                printSparseMatrix(txt, tokens, IndexingStyle::CppStyle,
                                  display_as_dense ? DisplayFormat::Dense : DisplayFormat::Sparse);
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
                ImGui::Text("%s", showCounterEquation(net(), "", use_caption, tropical_notation)
                    .str().c_str());
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Dater"))
            {
                ImGui::Text("%s", showDaterEquation(net(), "", use_caption, tropical_notation)
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
        static bool display_as_dense = false;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::Checkbox("Dense matrix", &display_as_dense);
        ImGui::PopStyleVar();

        static SparseMatrix<MaxPlus> cached_D, cached_A, cached_B, cached_C;
        static bool cached_valid = false;
        if (!cached_valid)
        {
            toSysLin(net(), cached_D, cached_A, cached_B, cached_C);
            cached_valid = true;
        }

        SparseMatrix<MaxPlus>& D = cached_D;
        SparseMatrix<MaxPlus>& A = cached_A;
        SparseMatrix<MaxPlus>& B = cached_B;
        SparseMatrix<MaxPlus>& C = cached_C;
        ImGui::Text(u8"%s", "X(n) = D . X(n) (+) A . X(n-1) (+) B . U(n)\nY(n) = C . X(n)");

        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
        if (ImGui::BeginTabBar("syslin", tab_bar_flags))
        {
            if (ImGui::BeginTabItem("D"))
            {
                std::stringstream txt;
                printSparseMatrix(txt, D, IndexingStyle::CppStyle,
                                  display_as_dense ? DisplayFormat::Dense : DisplayFormat::Sparse);
                ImGui::Text("%s", txt.str().c_str());
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("A"))
            {
                std::stringstream txt;
                printSparseMatrix(txt, A, IndexingStyle::CppStyle,
                                  display_as_dense ? DisplayFormat::Dense : DisplayFormat::Sparse);
                ImGui::Text("%s", txt.str().c_str());
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("B"))
            {
                std::stringstream txt;
                printSparseMatrix(txt, B, IndexingStyle::CppStyle,
                                  display_as_dense ? DisplayFormat::Dense : DisplayFormat::Sparse);
                ImGui::Text("%s", txt.str().c_str());
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("C"))
            {
                std::stringstream txt;
                printSparseMatrix(txt, C, IndexingStyle::CppStyle,
                                  display_as_dense ? DisplayFormat::Dense : DisplayFormat::Sparse);
                ImGui::Text("%s", txt.str().c_str());
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        if (ImGui::Button("OK", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
            m_states.do_syslin = false;
            m_states.plot.reset();
            cached_valid = false;
        }
        ImGui::EndPopup();
    }
}

//------------------------------------------------------------------------------
void Editor::showCriticalCircuit()
{
    ImGui::OpenPopup("Critical circuit analysis");
    ImGui::SetNextWindowPos(m_states.viewport_center, ImGuiCond_Appearing,
                            ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Critical circuit analysis", NULL,
                               ImGuiWindowFlags_AlwaysAutoResize))
    {
        static CriticalCycleResult cached;
        static bool cached_valid = false;
        if (!cached_valid)
        {
            cached = findCriticalCycle(net());
            cached_valid = true;
        }
        CriticalCycleResult const& res = cached;

        if (!res.success)
        {
            ImGui::Text(u8"%s", res.message.str().c_str());
        }
        else
        {
            ImGui::TextWrapped(
                "Howard policy for this timed event graph. "
                "Highlighted arcs on the canvas form the critical circuit(s).");
            ImGui::Spacing();
            ImGui::Text("Strongly connected components: %zu", res.cycles);

            std::set<double> const lambdas(res.durations.begin(), res.durations.end());
            if (!lambdas.empty())
            {
                if (lambdas.size() == 1u)
                {
                    ImGui::Text("Cycle time λ: %s time units.",
                                formatTimeUnits(*lambdas.begin()).c_str());
                }
                else
                {
                    std::ostringstream os;
                    auto it = lambdas.begin();
                    os << formatTimeUnits(*it++);
                    for (; it != lambdas.end(); ++it)
                        os << ", " << formatTimeUnits(*it);
                    ImGui::Text("Cycle times λ: %s time units.", os.str().c_str());
                }
            }

            m_marked_arcs = res.arcs;
            ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
            if (ImGui::BeginTabBar("CriticalCycleResult", tab_bar_flags))
            {
                if (ImGui::BeginTabItem("Circuit arcs"))
                {
                    ImGuiTableFlags const flags =
                        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                        ImGuiTableFlags_SizingFixedFit;
                    if (ImGui::BeginTable("CriticalCircuitArcs", 3, flags))
                    {
                        ImGui::TableSetupColumn("From");
                        ImGui::TableSetupColumn("Place");
                        ImGui::TableSetupColumn("To");
                        ImGui::TableHeadersRow();
                        for (size_t it = 0u; it < res.arcs.size(); it += 2u)
                        {
                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImGui::TextUnformatted(res.arcs[it]->from.key.c_str());
                            ImGui::TableNextColumn();
                            ImGui::TextUnformatted(res.arcs[it]->to.key.c_str());
                            ImGui::TableNextColumn();
                            ImGui::TextUnformatted(res.arcs[it + 1u]->to.key.c_str());
                        }
                        ImGui::EndTable();
                    }
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Bias"))
                {
                    auto const& transitions = net().transitions();
                    ImGuiTableFlags const flags =
                        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                        ImGuiTableFlags_SizingFixedFit;
                    if (ImGui::BeginTable("CriticalCircuitBias", 2, flags))
                    {
                        ImGui::TableSetupColumn("Transition");
                        ImGui::TableSetupColumn("Bias [time units]");
                        ImGui::TableHeadersRow();
                        size_t const n = std::min(transitions.size(),
                                                  res.eigenvector.size());
                        for (size_t i = 0; i < n; ++i)
                        {
                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImGui::TextUnformatted(transitions[i].key.c_str());
                            ImGui::TableNextColumn();
                            ImGui::TextUnformatted(
                                formatTimeUnits(res.eigenvector[i]).c_str());
                        }
                        ImGui::EndTable();
                    }
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Help"))
                {
                    float const help_wrap = ImGui::GetCursorPos().x + 420.0f;
                    ImGui::PushTextWrapPos(help_wrap);
                    ImGui::TextDisabled("Cycle time λ");
                    ImGui::TextWrapped(
                        "Max-plus eigenvalue of the timed event graph (same time "
                        "units as arc durations). Average duration of one turn around "
                        "a circuit in the optimal (Howard) policy—the bottleneck of "
                        "the steady periodic regime.");
                    ImGui::Spacing();
                    ImGui::TextDisabled("Critical circuit");
                    ImGui::TextWrapped(
                        "Arcs of the optimal policy that lie on a cycle and set λ. "
                        "They are highlighted on the canvas (listed in the "
                        "Circuit arcs tab).");
                    ImGui::Spacing();
                    ImGui::TextDisabled("Bias");
                    ImGui::TextWrapped(
                        "Initial time offset of each transition in the optimal "
                        "policy, in the same time units. Transitions on the same "
                        "circuit share the same λ but may have different biases.");
                    ImGui::PopTextWrapPos();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }

        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
            m_states.do_find_critical_circuit = false;
            cached_valid = false;
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
