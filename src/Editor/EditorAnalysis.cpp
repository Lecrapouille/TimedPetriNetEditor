// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

// The Analysis window: what the (max,+) algebra says about a timed event
// graph, in one dockable window with tabs (it replaces the modal dialogs that
// blocked the edition): critical circuit, dynamic linear system, counter and
// dater equations, adjacency matrices, Gantt chart of the earliest dates,
// firing dates of the simulation. Matrices are tables, ε (the (max,+) zero)
// is greyed, and each one can be copied as a Julia matrix.

#include "Editor/Editor.hpp"
#include "Editor/Theme.hpp"

#include "MaxPlus/Algorithms.hpp"
#include "MaxPlus/Flowshop.hpp"
#include "MaxPlus/SparseMatrix.hpp"
#include "imgui/imgui.h"
#include "implot/implot.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <vector>

namespace tpne {

//! \brief Value with 2 decimals.
static std::string formatTimeUnits(double const value)
{
    std::ostringstream os;
    os << std::fixed << std::setprecision(2) << value;
    return os.str();
}

//! \brief Short label of a transition: its caption, else its key.
static std::string labelOf(Transition const& transition)
{
    return transition.caption.empty() ? transition.key : transition.caption;
}

//------------------------------------------------------------------------------
//! \brief A (max,+) matrix as a table: rows and columns named, ε greyed, a
//! button to copy it as a Julia matrix. Large matrices fall back to the list
//! of their non ε entries.
//------------------------------------------------------------------------------
static void matrixTable(char const* id, SparseMatrix<MaxPlus> const& matrix,
                        std::vector<std::string> const& rows,
                        std::vector<std::string> const& columns)
{
    size_t const nr = matrix.nbRows();
    size_t const nc = matrix.nbColumns();
    ImGui::PushID(id);
    ImGui::TextDisabled("%zu x %zu", nr, nc);
    ImGui::SameLine();
    if (ImGui::SmallButton("Copy as Julia"))
    {
        std::stringstream txt;
        printSparseMatrix(txt, matrix, IndexingStyle::JuliaStyle, DisplayFormat::Dense);
        ImGui::SetClipboardText(txt.str().c_str());
    }

    ImVec4 const dim = ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
    if ((nc == 0u) || (nr == 0u))
    {
        ImGui::TextDisabled("(empty)");
    }
    else if (nc <= 48u)
    {
        ImGuiTableFlags const flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
            ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit;
        float const height = std::min(400.0f, ImGui::GetTextLineHeightWithSpacing() * float(nr + 2u));
        if (ImGui::BeginTable("##matrix", int(nc) + 1, flags, ImVec2(0.0f, height)))
        {
            ImGui::TableSetupScrollFreeze(1, 1);
            ImGui::TableSetupColumn("");
            for (size_t c = 0u; c < nc; ++c)
                ImGui::TableSetupColumn(c < columns.size() ? columns[c].c_str() : "?");
            ImGui::TableHeadersRow();
            ImGuiListClipper clipper;
            clipper.Begin(int(nr));
            while (clipper.Step())
            {
                for (int r = clipper.DisplayStart; r < clipper.DisplayEnd; ++r)
                {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(size_t(r) < rows.size() ? rows[size_t(r)].c_str() : "?");
                    for (size_t c = 0u; c < nc; ++c)
                    {
                        ImGui::TableNextColumn();
                        double const v = matrix.get(size_t(r), c).val;
                        if (std::isinf(v) && (v < 0.0))
                            ImGui::TextColored(dim, "ε");
                        else
                            ImGui::Text("%s", formatTimeUnits(v).c_str());
                    }
                }
            }
            ImGui::EndTable();
        }
    }
    else
    {
        // Too wide for a table: the non ε entries.
        if (ImGui::BeginTable("##entries", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                              ImGuiTableFlags_ScrollY, ImVec2(0.0f, 300.0f)))
        {
            ImGui::TableSetupColumn("Row");
            ImGui::TableSetupColumn("Column");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();
            for (size_t r = 0u; r < nr; ++r)
            {
                for (size_t c = 0u; c < nc; ++c)
                {
                    double const v = matrix.get(r, c).val;
                    if (std::isinf(v) && (v < 0.0))
                        continue;
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn(); ImGui::TextUnformatted(r < rows.size() ? rows[r].c_str() : "?");
                    ImGui::TableNextColumn(); ImGui::TextUnformatted(c < columns.size() ? columns[c].c_str() : "?");
                    ImGui::TableNextColumn(); ImGui::Text("%s", formatTimeUnits(v).c_str());
                }
            }
            ImGui::EndTable();
        }
    }
    ImGui::PopID();
}

//! \brief Names x1..xn.
static std::vector<std::string> names(char const* prefix, size_t n)
{
    std::vector<std::string> out;
    for (size_t i = 1u; i <= n; ++i)
        out.push_back(prefix + std::to_string(i));
    return out;
}

//------------------------------------------------------------------------------
void Editor::analysisAdjacency()
{
    SparseMatrix<MaxPlus> tokens; SparseMatrix<MaxPlus> durations;
    toAdjacencyMatrices(net(), tokens, durations);
    std::vector<std::string> labels;
    for (auto const& t : net().transitions())
        labels.push_back(labelOf(t));
    ImGui::TextWrapped("Entry (i, j): the arc from transition j to transition i "
                       "(through a place).");
    if (ImGui::BeginTabBar("##adjacency"))
    {
        if (ImGui::BeginTabItem("Durations"))
        {
            matrixTable("durations", durations, labels, labels);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Tokens"))
        {
            matrixTable("tokens", tokens, labels, labels);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

//------------------------------------------------------------------------------
void Editor::analysisSyslin()
{
    SparseMatrix<MaxPlus> D, A, B, C;
    toSysLin(net(), D, A, B, C);
    ImGui::TextUnformatted("X(k) = D ⊗ X(k) ⨁ A ⊗ X(k-1) ⨁ B ⊗ U(k)");
    ImGui::TextUnformatted("Y(k) = C ⊗ X(k)");
    ImGui::TextDisabled("k: the k-th firing (cycle), as in the Gantt chart.");
    ImGui::TextDisabled("X: internal transitions, U: inputs, Y: outputs.");
    if (ImGui::BeginTabBar("##syslin"))
    {
        if (ImGui::BeginTabItem("D"))
        {
            matrixTable("D", D, names("x", D.nbRows()), names("x", D.nbColumns()));
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("A"))
        {
            matrixTable("A", A, names("x", A.nbRows()), names("x", A.nbColumns()));
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("B"))
        {
            matrixTable("B", B, names("x", B.nbRows()), names("u", B.nbColumns()));
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("C"))
        {
            matrixTable("C", C, names("y", C.nbRows()), names("x", C.nbColumns()));
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

//------------------------------------------------------------------------------
void Editor::analysisEquations()
{
    ImGui::Checkbox("(min,+) notation", &m_states.equations_min_plus);
    ImGui::SameLine();
    ImGui::Checkbox("Captions", &m_states.equations_with_captions);
    bool const use_caption = m_states.equations_with_captions;
    bool const tropical = m_states.equations_min_plus;
    unsigned const options = (use_caption ? 1u : 0u) | (tropical ? 2u : 0u);

    if (ImGui::BeginTabBar("##equations"))
    {
        char const* tabs[] = { "Dater (max,+)", "Counter (min,+)" };
        for (size_t i = 0u; i < 2u; ++i)
        {
            if (!ImGui::BeginTabItem(tabs[i]))
                continue;
            std::string const& text = m_states.equations[i].get(net(), options, [&] {
                return (i == 0u)
                    ? showDaterEquation(net(), "", use_caption, tropical).str()
                    : showCounterEquation(net(), "", use_caption, tropical).str();
            });
            if (ImGui::SmallButton("Copy"))
                ImGui::SetClipboardText(text.c_str());
            ImGui::BeginChild("##text", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
            ImGui::TextUnformatted(text.c_str());
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

//------------------------------------------------------------------------------
void Editor::analysisWindow()
{
    ImGui::SetNextWindowSize(ImVec2(520.0f, 420.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Analysis", &m_states.show_analysis))
    {
        ImGui::End();
        return;
    }
    if (!net().layout().compact && !isEventGraph(net()))

    {
        ImGui::TextWrapped("The (max,+) analysis needs a timed event graph: every place "
                           "has one input and one output transition. Change the type of "
                           "the net (Edit > Type of net) or import a flowshop.");
        ImGui::End();
        return;
    }

    char const* tabs[] = { "Critical circuit", "Gantt", "Linear system", "Equations",
                           "Adjacency", "Firing dates" };
    if (ImGui::BeginTabBar("##analysis", ImGuiTabBarFlags_FittingPolicyScroll))
    {
        for (int i = 0; i < 6; ++i)
        {
            ImGuiTabItemFlags const flags = (m_states.analysis_tab == i)
                ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
            if (!ImGui::BeginTabItem(tabs[i], nullptr, flags))
                continue;
            switch (i)
            {
            case 0: analysisCritical(); break;
            case 1: analysisGantt(); break;
            case 2: analysisSyslin(); break;
            case 3: analysisEquations(); break;
            case 4: analysisAdjacency(); break;
            case 5: analysisPlot(); break;
            }
            ImGui::EndTabItem();
        }
        m_states.analysis_tab = -1;
        ImGui::EndTabBar();
    }
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
        for (Arc const& arc : net.arcsOut(transitions[t]))
        {
            duration = std::max(duration, double(arc.durationOrZero()));
        }
        if (duration <= 0.0)
            continue;

        std::string const name = transitions[t].caption.empty()
            ? transitions[t].key : transitions[t].caption;
        std::string row, piece;
        operationOf(name, row, piece);
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
                             piece + " k=" + std::to_string(k + 1) });
        }
    }
}

//------------------------------------------------------------------------------
void Editor::analysisCritical()
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
        ImGui::TextUnformatted(res.message.str().c_str());
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

        markArcs(net(), res.arcs);
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
                        Arc const& first = net().arcs()[res.arcs[it]];
                        Arc const& second = net().arcs()[res.arcs[it + 1u]];
                        ImGui::TextUnformatted(first.from.key().c_str());
                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(first.to.key().c_str());
                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(second.to.key().c_str());
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

}

//------------------------------------------------------------------------------
void Editor::analysisPlot()
{

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

    double const lambda = m_states.plot_lambda;

    ImGui::TextDisabled("t = %.3f s, firings = %zu, transitions = %zu, λ = %.4f",
                        sim.simulationTime(), sim.totalFirings(),
                        transitions.size(), lambda);

    ImGui::Checkbox("Subtract drift λ·k", &subtract_drift);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("In the periodic regime, a transition fires every λ: y(k) - λ·k\n"
                          "becomes flat, the remaining shows the transient and the bias.");

    // The flowshop names its outputs "out_<piece>": without such names the
    // filter would hide everything.
    bool has_outputs = false;
    for (auto const& t : transitions)
        has_outputs = has_outputs || ((t.caption.empty() ? t.key : t.caption).rfind("out_", 0) == 0);
    bool const filter_outputs = only_outputs && has_outputs;
    if (has_outputs)
    {
        ImGui::SameLine();
        ImGui::Checkbox("Outputs only (out_*)", &only_outputs);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Show only transitions whose caption starts "
                              "with \"out_\" (flowshop output convention).");
        }
    }
    if (sim.totalFirings() == 0u)
    {
        ImGui::TextColored(ImVec4(0.95f, 0.55f, 0.05f, 1.0f), "%s", sim.isRunning()
            ? "Waiting for the first firings ..."
            : "Start the simulation (Space): each firing of a transition adds a point (k, y(k)).");
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
        return;
    }

    ImPlot::SetupAxes("k (firing index)",
                      subtract_drift ? "y(k) - λ·k [time units]"
                                     : "y(k) [time units]",
                      ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
    ImPlot::SetupLegend(ImPlotLocation_NorthWest);

    // Plot one line per selected transition. The points are computed by a
    // getter while drawing: no copy of the firing dates at each frame.
    struct Line { std::vector<double> const* dates; double lambda; };
    auto const getter = [](int const idx, void* data) -> ImPlotPoint
    {
        Line const& line = *static_cast<Line const*>(data);
        double const k = double(idx + 1);
        return ImPlotPoint(k, (*line.dates)[size_t(idx)] - line.lambda * k);
    };

    for (size_t i = 0u; i < transitions.size(); ++i)
    {
        if (!selected[i])
            continue;
        if (filter_outputs)
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
}

//------------------------------------------------------------------------------
void Editor::analysisGantt()
{

    Net const& current = net();
    ImGui::SetNextItemWidth(120.0f);
    ImGui::SliderInt("Cycles", &m_states.gantt_cycles, 1, 20);
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(420.0f);
        ImGui::TextUnformatted(
            "How to read it: each transition followed by a duration is an operation. "
            "Its bar starts at the date of a firing of the transition and lasts the "
            "duration of its output arc. A flowshop transition \"M1_P1\" (piece P1 on "
            "machine M1) is drawn on the row M1 with the color of P1; other "
            "transitions get their own row.");
        ImGui::Spacing();
        ImGui::TextUnformatted(
            "k is the cycle: \"P1 k=2\" is the second time the operation is done. The "
            "dates are the earliest ones given by the (max,+) dater equation "
            "x(k) = max(x_i(k - tokens_i) + duration_i) over the input places, with the "
            "initial tokens available at time 0: the fastest schedule of the net.");
        ImGui::Spacing();
        ImGui::TextUnformatted(
            "While simulating, the red line is the time of the simulation, and the "
            "console (Simulation messages) writes each firing with the same words: "
            "\"t = 4.00  M1 starts P1 (k = 2), until 7.00\".");
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }

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
        bool const hovered = ImPlot::IsPlotHovered();
        ImPlotPoint const mouse = ImPlot::GetPlotMousePos();
        GanttBarData const* under = nullptr;
        for (auto const& bar : m_states.gantt_bars)
        {
            if (hovered && (mouse.x >= bar.start) && (mouse.x <= bar.end) &&
                (std::fabs(mouse.y - double(bar.row)) <= 0.35))
                under = &bar;
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
        // Time of the simulation: the same dates as the console.
        if (simulation().isRunning())
        {
            double const t = double(simulation().simulationTime());
            ImVec2 const a = ImPlot::PlotToPixels(t, -0.6);
            ImVec2 const b = ImPlot::PlotToPixels(t, double(rows.size()) - 0.4);
            draw->AddLine(a, b, IM_COL32(230, 50, 50, 220), 2.0f);
        }
        ImPlot::PopPlotClipRect();
        if (under != nullptr)
        {
            ImGui::BeginTooltip();
            ImGui::Text("%s: %s", rows[size_t(under->row)].c_str(), under->label.c_str());
            ImGui::Text("from %.2f to %.2f (%.2f)", under->start, under->end, under->end - under->start);
            ImGui::EndTooltip();
        }
        ImPlot::EndPlot();
    }
}

} // namespace tpne
