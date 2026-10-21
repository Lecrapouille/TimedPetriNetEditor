// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

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

//------------------------------------------------------------------------------
//! \brief Fill the bars of the Gantt chart: one bar per firing of a transition
//! followed by a duration (an operation). A caption "<machine>_<piece>" (the
//! flowshop convention) puts the bar on the row of the machine, colored by
//! piece; other transitions get their own row.
static void computeGantt(Net const& net, int const cycles,
                         std::vector<std::string>& rows,
                         std::vector<Editor::GanttBarData>& bars, std::string& error)
{
    rows.clear();
    bars.clear();
    std::vector<std::vector<double>> dates;
    error = earliestDates(net, size_t(cycles), dates);
    if (!error.empty())
        return;

    std::map<std::string, int> row_of;
    std::map<std::string, int> color_of;
    auto const& transitions = net.transitions();
    for (size_t t = 0u; t < transitions.size(); ++t)
    {
        double duration = 0.0;
        for (Arc const* arc : transitions[t].arcsOut)
        {
            if (!std::isnan(arc->duration))
                duration = std::max(duration, double(arc->duration));
        }
        if (duration <= 0.0)
            continue;

        std::string const name = transitions[t].caption.empty()
            ? transitions[t].key : transitions[t].caption;
        size_t const underscore = name.find('_');
        bool const flowshop = (underscore != std::string::npos) && (name.rfind("out_", 0) != 0);
        std::string const row = flowshop ? name.substr(0, underscore) : name;
        std::string const piece = flowshop ? name.substr(underscore + 1u) : name;
        if (row_of.find(row) == row_of.end())
        {
            row_of[row] = int(rows.size());
            rows.push_back(row);
        }
        if (color_of.find(piece) == color_of.end())
            color_of[piece] = int(color_of.size());
        for (int k = 0; k < cycles; ++k)
        {
            bars.push_back({ row_of[row], color_of[piece], dates[t][size_t(k)],
                             dates[t][size_t(k)] + duration,
                             piece + " #" + std::to_string(k + 1) });
        }
    }
}

//------------------------------------------------------------------------------
void Editor::showGanttChart()
{
    if (!ImGui::Begin("Gantt chart", &m_states.show_gantt))
    {
        ImGui::End();
        return;
    }

    Net const& current = net();
    ImGui::SetNextItemWidth(120.0f);
    ImGui::SliderInt("Cycles", &m_states.gantt_cycles, 1, 20);
    ImGui::SameLine();
    ImGui::TextDisabled("Earliest dates of the (max,+) dater equation; tokens available at 0.");

    if ((m_states.gantt_net != &current) ||
        (m_states.gantt_revision != current.modified.revision()) ||
        (m_states.gantt_computed_cycles != m_states.gantt_cycles))
    {
        std::vector<GanttBarData> bars;
        computeGantt(current, m_states.gantt_cycles, m_states.gantt_rows, bars,
                     m_states.gantt_error);
        m_states.gantt_bars.clear();
        for (auto const& bar : bars)
            m_states.gantt_bars.push_back({ bar.row, bar.color, bar.start, bar.end, bar.label });
        m_states.gantt_net = &current;
        m_states.gantt_revision = current.modified.revision();
        m_states.gantt_computed_cycles = m_states.gantt_cycles;
    }
    if (!m_states.gantt_error.empty())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", m_states.gantt_error.c_str());
        ImGui::End();
        return;
    }

    auto const& rows = m_states.gantt_rows;
    double end = 1.0;
    for (auto const& bar : m_states.gantt_bars)
        end = std::max(end, bar.end);

    if (ImPlot::BeginPlot("##gantt", ImVec2(-1, -1), ImPlotFlags_NoLegend))
    {
        std::vector<double> ticks(rows.size());
        std::vector<char const*> labels(rows.size());
        for (size_t i = 0u; i < rows.size(); ++i)
        {
            ticks[i] = double(i);
            labels[i] = rows[i].c_str();
        }
        ImPlot::SetupAxes("time", nullptr, 0, ImPlotAxisFlags_Invert);
        if (!rows.empty())
            ImPlot::SetupAxisTicks(ImAxis_Y1, ticks.data(), int(rows.size()), labels.data());
        ImPlot::SetupAxesLimits(0.0, end, -0.6, double(rows.size()) - 0.4, ImPlotCond_Always);

        ImDrawList* draw = ImPlot::GetPlotDrawList();
        ImPlot::PushPlotClipRect();
        for (auto const& bar : m_states.gantt_bars)
        {
            ImVec2 const a = ImPlot::PlotToPixels(bar.start, bar.row - 0.35);
            ImVec2 const b = ImPlot::PlotToPixels(bar.end, bar.row + 0.35);
            ImVec4 color = ImPlot::GetColormapColor(bar.color);
            draw->AddRectFilled(a, b, ImGui::GetColorU32(color), 3.0f);
            draw->AddRect(a, b, IM_COL32(0, 0, 0, 160), 3.0f);
            ImVec2 const size = ImGui::CalcTextSize(bar.label.c_str());
            if (std::fabs(b.x - a.x) > size.x + 4.0f)
            {
                draw->AddText(ImVec2(std::min(a.x, b.x) + 3.0f, (a.y + b.y - size.y) / 2.0f),
                              IM_COL32(0, 0, 0, 255), bar.label.c_str());
            }
        }
        ImPlot::PopPlotClipRect();
        ImPlot::EndPlot();
    }
    ImGui::End();
}

} // namespace tpne
