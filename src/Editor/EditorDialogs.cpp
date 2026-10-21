// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Editor/Editor.hpp"
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
void Editor::showAdjacencyMatrices() const
{
    ImGui::OpenPopup("Show adjacency matrices");
    ImGui::SetNextWindowPos(m_states.viewport_center, ImGuiCond_Appearing,
                            ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Show adjacency matrices",
                               NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::Checkbox("Dense matrix", &m_states.dense_matrices);
        ImGui::PopStyleVar();

        DisplayFormat const format = m_states.dense_matrices
            ? DisplayFormat::Dense : DisplayFormat::Sparse;
        auto const matrix = [this, format](size_t const which)
        {
            SparseMatrix<MaxPlus> tokens; SparseMatrix<MaxPlus> durations;
            toAdjacencyMatrices(net(), tokens, durations);
            std::stringstream txt;
            printSparseMatrix(txt, (which == 0u) ? durations : tokens,
                              IndexingStyle::CppStyle, format);
            return txt.str();
        };

        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
        if (ImGui::BeginTabBar("adjacency", tab_bar_flags))
        {
            char const* tabs[] = { "Durations", "Tokens" };
            for (size_t i = 0u; i < 2u; ++i)
            {
                if (ImGui::BeginTabItem(tabs[i]))
                {
                    ImGui::TextUnformatted(m_states.adjacency[i].get(
                        net(), unsigned(format), [&] { return matrix(i); }).c_str());
                    ImGui::EndTabItem();
                }
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
    ImGui::OpenPopup("Counter or dater equations");
    ImGui::SetNextWindowPos(m_states.viewport_center, ImGuiCond_Appearing,
                            ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Counter or dater equations", NULL,
        ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::Checkbox("Use (min,+) operator", &m_states.equations_min_plus);
        ImGui::SameLine();
        ImGui::Checkbox("Use caption", &m_states.equations_with_captions);
        ImGui::PopStyleVar();

        bool const use_caption = m_states.equations_with_captions;
        bool const tropical_notation = m_states.equations_min_plus;
        unsigned const options = (use_caption ? 1u : 0u) | (tropical_notation ? 2u : 0u);

        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
        if (ImGui::BeginTabBar("tab counter or dater", tab_bar_flags))
        {
            if (ImGui::BeginTabItem("Counter"))
            {
                ImGui::TextUnformatted(m_states.equations[0].get(net(), options, [&] {
                    return showCounterEquation(net(), "", use_caption, tropical_notation).str();
                }).c_str());
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Dater"))
            {
                ImGui::TextUnformatted(m_states.equations[1].get(net(), options, [&] {
                    return showDaterEquation(net(), "", use_caption, tropical_notation).str();
                }).c_str());
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
        ImGui::Checkbox("Dense matrix", &m_states.dense_matrices);
        ImGui::PopStyleVar();

        ImGui::TextUnformatted("X(n) = D ⊗ X(n) ⨁ A ⊗ X(n-1) ⨁ B ⊗ U(n)\nY(n) = C ⊗ X(n)");

        DisplayFormat const format = m_states.dense_matrices
            ? DisplayFormat::Dense : DisplayFormat::Sparse;
        auto const matrix = [this, format](size_t const which)
        {
            SparseMatrix<MaxPlus> D, A, B, C;
            toSysLin(net(), D, A, B, C);
            SparseMatrix<MaxPlus> const* matrices[] = { &D, &A, &B, &C };
            std::stringstream txt;
            printSparseMatrix(txt, *matrices[which], IndexingStyle::CppStyle, format);
            return txt.str();
        };

        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
        if (ImGui::BeginTabBar("syslin", tab_bar_flags))
        {
            char const* tabs[] = { "D", "A", "B", "C" };
            for (size_t i = 0u; i < 4u; ++i)
            {
                if (ImGui::BeginTabItem(tabs[i]))
                {
                    ImGui::TextUnformatted(m_states.syslin[i].get(
                        net(), unsigned(format), [&] { return matrix(i); }).c_str());
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }

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
void Editor::showCriticalCircuit()
{
    ImGui::OpenPopup("Critical circuit analysis");
    ImGui::SetNextWindowPos(m_states.viewport_center, ImGuiCond_Appearing,
                            ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Critical circuit analysis", NULL,
                               ImGuiWindowFlags_AlwaysAutoResize))
    {
        if ((m_states.critical_cycle_net != &net()) ||
            (m_states.critical_cycle_revision != net().modified.revision()))
        {
            m_states.critical_cycle = findCriticalCycle(net());
            m_states.critical_cycle_net = &net();
            m_states.critical_cycle_revision = net().modified.revision();
        }
        CriticalCycleResult const& res = m_states.critical_cycle;

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
            m_states.critical_cycle_net = nullptr;
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

//------------------------------------------------------------------------------
void Editor::showSimulationPlot()
{
    if (!ImGui::Begin("Real-time output plot", &m_states.show_simulation_plot))
    {
        ImGui::End();
        return;
    }

    Simulation const& sim = simulation();
    Net const& current = net();
    auto const& transitions = current.transitions();

    // Per-transition selection of this net, persisted across frames. Reset
    // when the net topology changes (different number of transitions).
    std::vector<bool>& selected = m_states.plot_selection[&current];
    bool& subtract_drift = m_states.plot_subtract_drift;
    bool& only_outputs = m_states.plot_only_outputs;

    if (selected.size() != transitions.size())
    {
        selected.assign(transitions.size(), true);
    }

    bool const is_teg = (current.type() == TypeOfNet::TimedEventGraph);
    double const lambda = sim.eigenvalue();

    ImGui::TextDisabled("t = %.3f s, firings = %zu, transitions = %zu, λ = %.4f",
                        double(sim.simulationTime()), sim.totalFirings(),
                        transitions.size(), lambda);

    ImGui::Checkbox("Subtract drift λ·k", &subtract_drift);
    if (is_teg)
    {
        ImGui::SameLine();
        ImGui::Checkbox("Outputs only (out_*)", &only_outputs);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Show only transitions whose caption starts "
                              "with \"out_\" (flowshop output convention).");
        }
    }

    // Transition selector: scrollable child with checkboxes.
    if (ImGui::CollapsingHeader("Transitions"))
    {
        if (ImGui::SmallButton("All"))
            std::fill(selected.begin(), selected.end(), true);
        ImGui::SameLine();
        if (ImGui::SmallButton("None"))
            std::fill(selected.begin(), selected.end(), false);

        ImGui::BeginChild("trans_selector", ImVec2(0, 100), true);
        for (size_t i = 0u; i < transitions.size(); ++i)
        {
            bool v = selected[i];
            std::string const label = transitions[i].key +
                (transitions[i].caption.empty() ? "" : " (" + transitions[i].caption + ")");
            if (ImGui::Checkbox(label.c_str(), &v))
                selected[i] = v;
        }
        ImGui::EndChild();
    }

    ImPlotFlags const plot_flags = ImPlotFlags_NoTitle;
    if (!ImPlot::BeginPlot("##firing_dates", ImVec2(-1, -1), plot_flags))
    {
        ImGui::End();
        return;
    }

    ImPlot::SetupAxes("k (firing index)",
                      subtract_drift ? "y(k) - λ·k [time units]"
                                     : "y(k) [time units]",
                      ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
    ImPlot::SetupLegend(ImPlotLocation_NorthWest);

    // Plot one line per selected transition. The points are computed by a
    // getter while drawing: no copy of the firing dates at each frame.
    struct Line { std::vector<float> const* dates; double lambda; };
    auto const getter = [](int const idx, void* data) -> ImPlotPoint
    {
        Line const& line = *static_cast<Line const*>(data);
        double const k = double(idx + 1);
        return ImPlotPoint(k, double((*line.dates)[size_t(idx)]) - line.lambda * k);
    };

    for (size_t i = 0u; i < transitions.size(); ++i)
    {
        if (!selected[i])
            continue;
        if (only_outputs && is_teg)
        {
            // In a flowshop TEG every transition has both incoming and
            // outgoing arcs (because of the controller feedback loops), so
            // Transition::isOutput() is never true. Instead the importer
            // names the system outputs "out_<piece|machine>".
            std::string const& name = transitions[i].caption.empty()
                ? transitions[i].key : transitions[i].caption;
            if (name.rfind("out_", 0) != 0)
                continue;
        }

        auto const& dates = sim.firingDates(transitions[i].id);
        if (dates.empty())
            continue;

        Line line { &dates, subtract_drift ? lambda : 0.0 };
        std::string const& label = transitions[i].caption.empty()
            ? transitions[i].key
            : transitions[i].caption;
        ImPlot::PlotLineG(label.c_str(), getter, &line, int(dates.size()));
    }

    ImPlot::EndPlot();
    ImGui::End();
}

} // namespace tpne
