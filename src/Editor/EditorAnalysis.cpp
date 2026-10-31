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
// firing dates of the simulation; and, for any net, its properties
// (deadlocks, bounds, liveness, invariants). Matrices are tables, ε (the (max,+) zero)
// is greyed, and each one can be copied as a Julia matrix.

#include "Editor/Editor.hpp"
#include "Editor/Theme.hpp"
#include "Editor/FileDialogHelper.hpp"

#include "Analysis/Analysis.hpp"
#include "MaxPlus/Algorithms.hpp"
#include "MaxPlus/Flowshop.hpp"
#include "MaxPlus/SparseMatrix.hpp"
#include "imgui.h"
#include "implot.h"

#include <algorithm>
#include <cmath>
#include <fstream>
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
Editor::AnalysisKey Editor::analysisKey()
{
    Net const& current = net();
    std::vector<size_t> tokens = m_journal.isReplaying()
        ? m_journal.initialMarking(current) : simulation().initialMarking();
    return { &current, current.modified.revision(), std::move(tokens) };
}

//------------------------------------------------------------------------------
Net const& Editor::analyzedNet()
{
    Net const& current = net();
    AnalysisKey key = analysisKey();
    if (key.tokens == current.tokens())
        return current;
    if ((m_states.analyzed == nullptr) || (m_states.analyzed_key != key))
    {
        m_states.analyzed = std::make_unique<Net>(current);
        m_states.analyzed->tokens(key.tokens);
        m_states.analyzed_key = std::move(key);
    }
    return *m_states.analyzed;
}

//------------------------------------------------------------------------------
void Editor::analysisAdjacency()
{
    SparseMatrix<MaxPlus> tokens; SparseMatrix<MaxPlus> durations;
    toAdjacencyMatrices(analyzedNet(), tokens, durations);
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
    toSysLin(analyzedNet(), D, A, B, C);
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
            std::string const& text = m_states.equations[i].get(analysisKey(), options, [&] {
                Net const& analyzed = analyzedNet();
                return (i == 0u)
                    ? showDaterEquation(analyzed, "", use_caption, tropical).str()
                    : showCounterEquation(analyzed, "", use_caption, tropical).str();
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
bool Editor::analysisTab(char const* name, int const tab)
{
    bool const asked = (m_states.analysis_tab == tab);
    if (!ImGui::BeginTabItem(name, nullptr, asked ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None))
        return false;
    if (asked)
        m_states.analysis_tab = -1;
    return true;
}

//------------------------------------------------------------------------------
void Editor::analysisWindow()
{
    // Closed by its cross (opened again by the menu Analysis). Tabs: the
    // properties of any net (Petri net theory, time not considered), then for
    // a timed net "(max,+)", with its own tabs for the algebra.
    // Tabs (openAnalysis()): 0 to 5 in (max,+), 6 to 9 properties.
    if (!m_states.show_analysis)
        return;
    ImGui::SetNextWindowSize(ImVec2(520.0f, 420.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Analysis", &m_states.show_analysis) && ImGui::BeginTabBar("##analysis", ImGuiTabBarFlags_FittingPolicyScroll))
    {
        analysisProperties();
        if (net().type() == TypeOfNet::TimedPetriNet)
            analysisMaxPlus();
        ImGui::EndTabBar();
    }
    // A (max,+) tab asked for a net that is not timed.
    if ((net().type() != TypeOfNet::TimedPetriNet) && (m_states.analysis_tab >= 0) &&
        (m_states.analysis_tab < 6))
        m_states.analysis_tab = -1;
    ImGui::End();
}

//------------------------------------------------------------------------------
void Editor::analysisMaxPlus()
{
    // One tab "(max,+)" with its own row of tabs. The algebra needs an event
    // graph: else the tab tells why not, with the faulty arcs.
    bool const asked = (m_states.analysis_tab >= 0) && (m_states.analysis_tab < 6);
    if (!ImGui::BeginTabItem("(max,+)", nullptr, asked ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None))
        return;

    std::string error;
    std::vector<size_t> arcs;
    if (!isEventGraph(net(), error, arcs))
    {
        if (asked)
            m_states.analysis_tab = -1;
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextUnformatted("The (max,+) algebra needs a timed event graph: every place has "
                               "one input and one output transition, arcs with no weight, "
                               "inhibition, reset nor color, places with no capacity.");
        ImGui::Spacing();
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(palette().critical), "%s", error.c_str());
        ImGui::Spacing();
        ImGui::TextUnformatted("Change the net, or import a flowshop (File > Import > Flowshop).");
        ImGui::PopTextWrapPos();
        if (!arcs.empty() && ImGui::Button("Show the faulty arcs"))
            markArcs(net(), arcs);
        ImGui::EndTabItem();
        return;
    }

    if (ImGui::BeginTabBar("##maxplus", ImGuiTabBarFlags_FittingPolicyScroll))
    {
        char const* tabs[] = { "Critical circuit", "Gantt", "Linear system", "Equations",
                               "Adjacency", "Firing dates" };
        for (int i = 0; i < 6; ++i)
        {
            if (!analysisTab(tabs[i], i))
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
        ImGui::EndTabBar();
    }
    ImGui::EndTabItem();
}

//------------------------------------------------------------------------------
//! \brief "2 P0 + P1" for an invariant over the nodes (their captions).
template<class Nodes>
static std::string invariantText(Nodes const& nodes, std::vector<int64_t> const& x)
{
    std::string s;
    for (size_t i = 0u; i < x.size(); ++i)
    {
        if (x[i] == 0)
            continue;
        if (!s.empty())
            s += " + ";
        if (x[i] != 1)
            s += std::to_string(x[i]) + " ";
        s += (nodes[i].caption.empty() || (nodes[i].caption == nodes[i].key))
            ? nodes[i].key : nodes[i].key + " (" + nodes[i].caption + ")";
    }
    return s;
}

//------------------------------------------------------------------------------
//! \brief A colored badge: "yes" in green, "no" in red, else gray.
static void badge(char const* label, int const state, std::string const& text)
{
    Palette const& pal = palette();
    ImU32 const color = (state > 0) ? pal.active : (state < 0) ? pal.critical : pal.secondary;
    ImGui::TextUnformatted(label);
    ImGui::SameLine(ImGui::GetFontSize() * 9.0f);
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(color), "%s", text.c_str());
}

//------------------------------------------------------------------------------
void Editor::analysisProperties()
{
    // Properties of the net (time not considered): computed on demand, the
    // reachability graph can be big; again at each change of a small net.
    // From the tokens before the simulation: a running one moves them.
    auto& cache = m_states.petri;
    Net& current = net();
    Net const& analyzed = analyzedNet();
    AnalysisKey const key = analysisKey();
    bool const stale = (cache.key != key);
    auto compute = [&]() {
        cache = States::PetriProperties{};
        cache.key = key;
        size_t const max = size_t(m_states.max_markings);
        cache.properties = properties(analyzed, max);
        std::string note;
        cache.p_invariants = pInvariants(analyzed, note);
        cache.t_invariants = tInvariants(analyzed, cache.invariants_note);
        if (cache.invariants_note.empty())
            cache.invariants_note = note;
        cache.report = report(analyzed, max);
    };
    if (stale && (current.places().size() + current.transitions().size() <= 60u))
        compute(); // small net: follow the edition
    auto ready = [&]() { return (cache.key.net == &current) && !cache.report.empty(); };

    Properties const& prop = cache.properties;
    Palette const& pal = palette();
    auto names = [&current](std::vector<size_t> const& ids) {
        std::string s;
        for (size_t t : ids)
        {
            if (t < current.transitions().size())
                s += (s.empty() ? "" : ", ") + current.transitions()[t].key;
        }
        return s;
    };

    // On top of each tab of the properties: compute, the graph of the
    // markings, the marks on the net. False: nothing computed yet.
    auto toolbar = [&]() -> bool {
        m_states.properties_shown = true;
        if (ImGui::Button((stale || !ready()) ? "Compute" : "Compute again"))
            compute();
        ImGui::SameLine();
        ImGui::SetNextItemWidth(110.0f);
        if (ImGui::InputInt("max markings", &m_states.max_markings, 1000, 10000))
            m_states.max_markings = std::clamp(m_states.max_markings, 10, 10000000);
        if (!ready())
        {
            ImGui::TextWrapped("Deadlocks, dead transitions, bounds, liveness and invariants of the net "
                               "(durations and GRAFCET rules not considered): press Compute.");
            return false;
        }
        if (ImGui::SmallButton("Graph of the markings"))
            showMarkingsGraph(analyzed);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Draw the graph of the markings in a window (and export it to dot)");
        ImGui::SameLine();
        ImGui::Checkbox("Show on the net", &m_states.mark_properties);
        if (stale)
            ImGui::TextColored(ImVec4(0.95f, 0.55f, 0.05f, 1.0f), "The net changed: compute again.");
        ImGui::Separator();
        return true;
    };

    // Summary, and the deadlock found.
    if (analysisTab("Summary", 6))
    {
        if (toolbar())
        {
            badge("Markings", prop.complete ? 1 : 0, std::to_string(prop.states) + (prop.complete ? " (all)" : " (stopped: maybe more)"));
            badge("Deadlock", prop.deadlock ? -1 : (prop.complete ? 1 : 0),
                  prop.deadlock ? "yes" : (prop.complete ? "none" : "none found"));
            badge("Dead transitions", prop.dead_transitions.empty() ? 1 : -1,
                  prop.dead_transitions.empty() ? "none" : names(prop.dead_transitions));
            std::string unbounded;
            size_t k = 0u;
            for (size_t p = 0u; p < prop.bounds.size(); ++p)
            {
                if (prop.bounds[p] == OMEGA)
                    unbounded += (unbounded.empty() ? "" : ", ") + current.places()[p].key;
                else
                    k = std::max(k, prop.bounds[p]);
            }
            badge("Bounded", prop.bounded ? 1 : (unbounded.empty() ? 0 : -1),
                  prop.bounded ? "yes (" + std::to_string(k) + "-bounded)" : (unbounded.empty() ? "unknown" : "no: " + unbounded));
            badge("Live", !prop.complete ? 0 : (prop.live ? 1 : -1),
                  !prop.complete ? "unknown" : (prop.live ? "yes" : "no: " + names(prop.not_live)));
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            for (auto const& note : prop.notes)
                ImGui::TextWrapped("%s", note.c_str());
            ImGui::PopStyleColor();

            if (prop.deadlock)
            {
                ImGui::SeparatorText("Deadlock");
                std::string marking;
                for (size_t p = 0u; p < prop.deadlock_marking.size(); ++p)
                {
                    if (prop.deadlock_marking[p] > 0u)
                        marking += (marking.empty() ? "" : ", ") + current.places()[p].key + "=" +
                                   std::to_string(prop.deadlock_marking[p]);
                }
                ImGui::TextWrapped("Marking: %s", marking.empty() ? "no token" : marking.c_str());
                ImGui::TextWrapped("Reached %s %s", prop.deadlock_path.empty() ? "at once" : "after firing",
                                   names(prop.deadlock_path).c_str());
                ImGui::BeginDisabled(prop.deadlock_path.empty() || activeDocument().isAnySimulationRunning());
                if (ImGui::Button("Replay the path in the Journal"))
                {
                    m_journal.showFirings(current, "path to the deadlock", prop.deadlock_path);
                    ImGui::SetWindowFocus("Journal");
                }
                ImGui::EndDisabled();
            }
        }
        ImGui::EndTabItem();
    }

    // Bounds.
    if (analysisTab("Bounds", 7))
    {
        if (toolbar() &&
            ImGui::BeginTable("##bounds", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("Place");
            ImGui::TableSetupColumn("Max tokens");
            ImGui::TableHeadersRow();
            for (size_t p = 0u; (p < prop.bounds.size()) && (p < current.places().size()); ++p)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%s %s", current.places()[p].key.c_str(), current.places()[p].caption.c_str());
                ImGui::TableNextColumn();
                if (prop.bounds[p] == OMEGA)
                    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(pal.critical), "unbounded");
                else
                    ImGui::Text("%zu", prop.bounds[p]);
            }
            ImGui::EndTable();
        }
        ImGui::EndTabItem();
    }

    // Invariants: a click marks their nodes.
    size_t const offset = cache.p_invariants.size();
    if (analysisTab("Invariants", 8))
    {
        if (toolbar())
        {
            if (!cache.invariants_note.empty())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
                ImGui::TextWrapped("%s", cache.invariants_note.c_str());
                ImGui::PopStyleColor();
            }
            Marking const m0 = analyzed.tokens();
            ImGui::TextDisabled("P-invariants: the weighted tokens of these places never change");
            if (cache.p_invariants.empty())
                ImGui::BulletText("none");
            for (size_t i = 0u; i < cache.p_invariants.size(); ++i)
            {
                auto const& y = cache.p_invariants[i];
                int64_t c = 0;
                for (size_t p = 0u; (p < y.size()) && (p < m0.size()); ++p)
                    c += y[p] * int64_t(m0[p]);
                std::string const label = invariantText(current.places(), y) + " = " + std::to_string(c) +
                                          "##p" + std::to_string(i);
                if (ImGui::Selectable(label.c_str(), cache.marked_invariant == int(i)))
                    cache.marked_invariant = (cache.marked_invariant == int(i)) ? -1 : int(i);
            }
            ImGui::TextDisabled("T-invariants: firing these transitions gives the marking back");
            if (cache.t_invariants.empty())
                ImGui::BulletText("none");
            for (size_t i = 0u; i < cache.t_invariants.size(); ++i)
            {
                std::string const label = invariantText(current.transitions(), cache.t_invariants[i]) +
                                          "##t" + std::to_string(i);
                int const id = int(offset + i);
                if (ImGui::Selectable(label.c_str(), cache.marked_invariant == id))
                    cache.marked_invariant = (cache.marked_invariant == id) ? -1 : id;
            }
        }
        ImGui::EndTabItem();
    }

    // The text of tpne-check.
    if (analysisTab("Report", 9))
    {
        if (toolbar())
        {
            if (ImGui::SmallButton("Copy the report"))
                ImGui::SetClipboardText(cache.report.c_str());
            ImGui::BeginChild("##report", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders,
                              ImGuiWindowFlags_HorizontalScrollbar);
            ImGui::TextUnformatted(cache.report.c_str());
            ImGui::EndChild();
        }
        ImGui::EndTabItem();
    }

    // Marks on the net, while a tab of the properties is shown: dead
    // transitions, transitions not live, unbounded places, the nodes of the
    // invariant selected.
    if (!ready())
    {
        m_marks.clear("analysis");
        return;
    }
    std::vector<NodeMark> marks;
    if (m_states.mark_properties)
    {
        for (size_t t : prop.not_live)
        {
            bool const dead = std::find(prop.dead_transitions.begin(), prop.dead_transitions.end(), t) !=
                              prop.dead_transitions.end();
            if (!dead && (t < current.transitions().size()))
                marks.push_back({ current.transitions()[t].nodeId(), pal.enabled, "not live" });
        }
        for (size_t t : prop.dead_transitions)
        {
            if (t < current.transitions().size())
                marks.push_back({ current.transitions()[t].nodeId(), pal.critical, "dead" });
        }
        for (size_t p = 0u; (p < prop.bounds.size()) && (p < current.places().size()); ++p)
        {
            if (prop.bounds[p] == OMEGA)
                marks.push_back({ current.places()[p].nodeId(), pal.critical, "unbounded" });
        }
    }
    if (cache.marked_invariant >= 0)
    {
        size_t const i = size_t(cache.marked_invariant);
        if (i < offset)
        {
            auto const& y = cache.p_invariants[i];
            for (size_t p = 0u; (p < y.size()) && (p < current.places().size()); ++p)
            {
                if (y[p] != 0)
                    marks.push_back({ current.places()[p].nodeId(), pal.selection,
                                      (y[p] == 1) ? "" : "x" + std::to_string(y[p]) });
            }
        }
        else if (i - offset < cache.t_invariants.size())
        {
            auto const& x = cache.t_invariants[i - offset];
            for (size_t t = 0u; (t < x.size()) && (t < current.transitions().size()); ++t)
            {
                if (x[t] != 0)
                    marks.push_back({ current.transitions()[t].nodeId(), pal.selection,
                                      (x[t] == 1) ? "" : "x" + std::to_string(x[t]) });
            }
        }
    }
    m_marks.set(current, "analysis", std::move(marks));
}

//------------------------------------------------------------------------------
//! \brief Fill the bars of the Gantt chart: one bar per firing of a transition
//! followed by a duration (an operation). A caption "<machine>_<conveyor>" (the
//! flowshop convention) puts the bar on the row of the machine, colored by
//! conveyor; other transitions get their own row.
static void computeGantt(Net const& net, int const cycles,
                         std::vector<std::string>& rows, std::vector<std::string>& conveyors,
                         std::vector<Editor::GanttBarData>& bars, std::string& error)
{
    rows.clear();
    conveyors.clear();
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
        std::string row, conveyor;
        operationOf(name, row, conveyor);
        if (row_of.find(row) == row_of.end())
        {
            row_of[row] = int(rows.size());
            rows.push_back(row);
        }
        if (color_of.find(conveyor) == color_of.end())
        {
            color_of[conveyor] = int(color_of.size());
            conveyors.push_back(conveyor);
        }
        for (int k = 0; k < cycles; ++k)
        {
            bars.push_back({ row_of[row], color_of[conveyor], dates[t][size_t(k)],
                             dates[t][size_t(k)] + duration,
                             conveyor + " k=" + std::to_string(k + 1) });
        }
    }
}

//------------------------------------------------------------------------------
void Editor::showMarkingsGraph(Net const& analyzed)
{
    size_t const max = size_t(m_states.max_markings);
    ReachabilityGraph const reach = reachabilityGraph(analyzed, max);
    std::string const title = analyzed.name.empty() ? "net" : analyzed.name;
    // The coverability graph when the reachable markings are too many.
    CoverabilityGraph const cover = reach.complete ? CoverabilityGraph{} : coverabilityGraph(analyzed, max);
    if (!reach.complete && cover.complete)
        m_states.markings.set(analyzed, cover.states, cover.edges, {}, title + " (coverability)",
                              toDot(analyzed, cover.states, cover.edges));
    else
        m_states.markings.set(analyzed, reach.states, reach.edges, reach.colored, title,
                              toDot(analyzed, reach.states, reach.edges, reach.colored));
    m_states.markings.open = true;
}

//------------------------------------------------------------------------------
void Editor::markingsWindow()
{
    MarkingsGraph& graph = m_states.markings;
    ImGui::SetNextWindowSize(ImVec2(700.0f, 500.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Graph of the markings", &graph.open))
    {
        ImGui::End();
        return;
    }
    ImGui::Text("%s", graph.title().c_str());
    ImGui::SameLine();
    if (ImGui::SmallButton("Layout again"))
        graph.relayout();
    ImGui::SameLine();
    if (ImGui::SmallButton("Export to dot..."))
        FileDialogHelper::openSave("MarkingsDlgKey", "Save the graph of the markings", ".dot,.gv");
    FileDialogHelper::display("MarkingsDlgKey", [this](std::string const& path) {
        std::ofstream file(path);
        if (file << m_states.markings.dot())
        {
            file.close();
            FileDialogHelper::download(path);
            m_messages.setInfo("Graph of the markings saved in '" + path + "' (dot -Tsvg to draw it)");
        }
        else
            m_messages.setError("Cannot write '" + path + "'");
    });
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Bold: the initial marking. Red: a deadlock. Drag a box or the view;\n"
                          "wheel: zoom. A marking lists the tokens of P0, P1 ... with their\n"
                          "colors when they are not all Success.");
    graph.draw();
    ImGui::End();
}

//------------------------------------------------------------------------------
void Editor::analysisCritical()
{
    AnalysisKey key = analysisKey();
    if (m_states.critical_cycle_key != key)
    {
        m_states.critical_cycle = findCriticalCycle(analyzedNet());
        m_states.critical_cycle_key = std::move(key);
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

    // The flowshop names its outputs "out_<conveyor>" and "out_<machine>", its
    // operations "<machine>_<conveyor>": the conveyors are known by the
    // operations, and their outputs are the ones worth plotting. Without such
    // names the filter would hide everything.
    auto const name = [](Transition const& t) -> std::string const& {
        return t.caption.empty() ? t.key : t.caption;
    };
    std::set<std::string> conveyors;
    bool has_outputs = false;
    for (auto const& t : transitions)
    {
        std::string machine, conveyor;
        operationOf(name(t), machine, conveyor);
        if (machine != conveyor)
            conveyors.insert(conveyor);
        has_outputs = has_outputs || (name(t).rfind("out_", 0) == 0);
    }
    auto const shown = [&](Transition const& t) {
        std::string const& n = name(t);
        if (n.rfind("out_", 0) != 0)
            return false;
        return conveyors.empty() || (conveyors.count(n.substr(4u)) != 0u);
    };
    bool const filter_outputs = only_outputs && has_outputs;
    if (has_outputs)
    {
        ImGui::SameLine();
        ImGui::Checkbox(conveyors.empty() ? "Outputs only (out_*)" : "Conveyors only (out_<conveyor>)",
                        &only_outputs);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(conveyors.empty()
                ? "Show only transitions whose caption starts with \"out_\"\n"
                  "(flowshop output convention)."
                : "Show only the outputs of the conveyors of the flowshop:\n"
                  "a part leaves the factory at each firing of out_<conveyor>.");
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
        // In a flowshop TEG every transition has both incoming and outgoing
        // arcs (because of the controller feedback loops), so
        // Transition::isOutput() is never true: the outputs are known by
        // their name.
        if (filter_outputs && !shown(transitions[i]))
            continue;

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
            "duration of its output arc. A flowshop transition \"M1_C1\" (conveyor C1 on "
            "machine M1) is drawn on the row M1 with the color of C1; other "
            "transitions get their own row.");
        ImGui::Spacing();
        ImGui::TextUnformatted(
            "Why only the machines of a flowshop? The other transitions have no "
            "duration: M1 (the machine is free again), C1 (the conveyor is free again) and "
            "out_... (end of a cycle) take no time, so they have no bar. They still "
            "count in the dates: they pass the tokens from a cycle to the next.");
        ImGui::Spacing();
        ImGui::TextUnformatted(
            "k is the cycle: \"C1 k=2\" is the second time the operation is done. The "
            "dates are the earliest ones given by the (max,+) dater equation "
            "x(k) = max(x_i(k - tokens_i) + duration_i) over the input places, with the "
            "initial tokens available at time 0: the fastest schedule of the net.");
        ImGui::Spacing();
        ImGui::TextUnformatted(
            "While simulating, the red line is the time of the simulation, and the "
            "console (Simulation messages) writes each firing with the same words: "
            "\"t = 4.00  M1 starts C1 (k = 2), until 7.00\".");
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }

    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    ImGui::TextWrapped("Rows: the machines of a flowshop (M1_C1 on M1), else the transitions "
                       "with a duration; the others take no time and have no bar.");
    ImGui::PopStyleColor();

    // The earliest dates from the tokens before the simulation.
    AnalysisKey key = analysisKey();
    if ((m_states.gantt_key != key) ||
        (m_states.gantt_computed_cycles != m_states.gantt_cycles))
    {
        computeGantt(analyzedNet(), m_states.gantt_cycles, m_states.gantt_rows,
                     m_states.gantt_conveyors, m_states.gantt_bars, m_states.gantt_error);
        m_states.gantt_key = std::move(key);
        m_states.gantt_computed_cycles = m_states.gantt_cycles;
    }
    if (!m_states.gantt_error.empty())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", m_states.gantt_error.c_str());
        return;
    }

    // The colors: the conveyors of a flowshop (none when each transition has
    // its row: the rows name them), wrapped on the width of the window.
    auto const& conveyors = m_states.gantt_conveyors;
    for (size_t i = 0u; (conveyors != m_states.gantt_rows) && (i < conveyors.size()); ++i)
    {
        float const size = ImGui::GetTextLineHeight();
        float const width = size + ImGui::GetStyle().ItemSpacing.x + ImGui::CalcTextSize(conveyors[i].c_str()).x;
        if ((i > 0u) && (ImGui::GetItemRectMax().x + ImGui::GetStyle().ItemSpacing.x + width <
                         ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x))
            ImGui::SameLine();
        ImVec2 const p = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddRectFilled(p, p + ImVec2(size, size),
            ImGui::GetColorU32(ImPlot::GetColormapColor(int(i))), 3.0f);
        ImGui::Dummy(ImVec2(size, size));
        ImGui::SameLine();
        ImGui::TextUnformatted(conveyors[i].c_str());
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
