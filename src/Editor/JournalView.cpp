// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Editor/JournalView.hpp"
#include "Editor/FileDialogHelper.hpp"
#include "Editor/Theme.hpp"
#include "Runtime/Journal.hpp"

#include "imgui/imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

namespace tpne {

//! \brief Source of the marks of the nodes.
static char const* const MARKS = "journal";

//------------------------------------------------------------------------------
//! \brief Name of the transition in the journal (as the runtime writes it).
static std::string const& nameOf(Transition const& transition)
{
    return transition.caption.empty() ? transition.key : transition.caption;
}

//------------------------------------------------------------------------------
void JournalView::numberLast()
{
    LogEntry const& last = m_entries.back();
    size_t k = 0u;
    if (last.event == LogEntry::Event::Scheduled)
    {
        for (auto const& e : m_entries)
        {
            if ((e.event == LogEntry::Event::Scheduled) && (e.transition == last.transition) &&
                (e.instance == last.instance))
                ++k;
        }
    }
    m_k.push_back(k);
}

//------------------------------------------------------------------------------
void JournalView::record(Net const& net, size_t const transition, double const date)
{
    if (transition >= net.transitions().size())
        return;
    if (m_source != "simulation")
    {
        m_entries.clear();
        m_k.clear();
        m_source = "simulation";
    }

    // A new run of the net: its previous entries go.
    for (auto it = m_entries.rbegin(); it != m_entries.rend(); ++it)
    {
        if (it->instance != net.name)
            continue;
        if (date < it->date)
        {
            size_t i = 0u;
            for (size_t j = 0u; j < m_entries.size(); ++j)
            {
                if (m_entries[j].instance != net.name)
                {
                    m_entries[i] = m_entries[j];
                    m_k[i++] = m_k[j];
                }
            }
            m_entries.resize(i);
            m_k.resize(i);
        }
        break;
    }

    std::string const& name = nameOf(net.transitions()[transition]);
    m_entries.push_back({ net.name, name, LogEntry::Event::Scheduled, Success, date });
    numberLast();
    m_entries.push_back({ net.name, name, LogEntry::Event::Completed, Success, date });
    numberLast();
    m_dirty = true;
}

//------------------------------------------------------------------------------
void JournalView::append(LogEntry const& entry)
{
    m_entries.push_back(entry);
    numberLast();
    m_dirty = true;
}

//------------------------------------------------------------------------------
void JournalView::live(std::string const& source, Net& net)
{
    clear();
    m_live = source;
    if (source.empty())
        return;
    m_source = source;
    m_follow = true;
    m_replay = true;
    m_replay_net = &net;
    m_saved_tokens = net.tokens();
    m_instance.clear();
    m_dirty = true;
}

//------------------------------------------------------------------------------
void JournalView::showFirings(Net& net, std::string const& source, std::vector<size_t> const& transitions)
{
    clear();
    for (size_t i = 0u; i < transitions.size(); ++i)
    {
        if (transitions[i] >= net.transitions().size())
            continue;
        std::string const& name = nameOf(net.transitions()[transitions[i]]);
        double const date = double(i + 1u);
        append({ net.name, name, LogEntry::Event::Scheduled, Success, date });
        append({ net.name, name, LogEntry::Event::Completed, Success, date });
    }
    m_source = source;
    m_follow = false;
    m_replay = true;
    m_replay_net = &net;
    m_saved_tokens = net.tokens();
    m_instance = net.name;
    m_cursor = 0.0;
    m_dirty = true;
}

//------------------------------------------------------------------------------
void JournalView::snapshot(std::string const& instance, double const date, Marking const& tokens)
{
    m_snapshots[instance] = { date, tokens, m_entries.size() };
    m_dirty = true;
}

//------------------------------------------------------------------------------
void JournalView::clear()
{
    stopReplay();
    m_entries.clear();
    m_k.clear();
    m_snapshots.clear();
    m_selected = size_t(-1);
    m_error.clear();
}

//------------------------------------------------------------------------------
std::string JournalView::load(std::string const& path, Net const& net)
{
    std::ifstream file(path);
    if (!file)
        return "Cannot read the journal '" + path + "'";
    std::stringstream text;
    text << file.rdbuf();

    std::vector<LogEntry> entries;
    std::string const error = fromJsonLines(text.str(), net.colors, entries);
    if (!error.empty())
        return "Journal '" + path + "': " + error;

    m_entries.clear();
    m_k.clear();
    for (auto const& entry : entries)
        append(entry);
    m_source = path;
    m_selected = size_t(-1);
    m_instance.clear();
    return {};
}

//------------------------------------------------------------------------------
std::string JournalView::save(std::string const& path, Net const& net) const
{
    // JSON lines, CSV or XES (process mining), by the extension.
    return saveJournal(m_entries, net.colors, path);
}

//------------------------------------------------------------------------------
void JournalView::stopReplay()
{
    Net* net = m_replay_net;
    if (m_replay && (net != nullptr) && (m_saved_tokens.size() == net->places().size()))
    {
        for (size_t p = 0u; p < net->places().size(); ++p)
            net->places()[p].tokens = m_saved_tokens[p];
    }
    m_replay = false;
    m_playing = false;
    m_replay_net = nullptr;
    m_saved_tokens.clear();
    m_live.clear();
}

//------------------------------------------------------------------------------
void JournalView::replay(Net& net, NodeMarks& marks)
{
    if (!m_dirty)
        return;
    m_dirty = false;

    // The marking of the instance at the cursor: the journal replayed from the
    // tokens the net had, or the ones the program gave (Runtime::restore()
    // does it with the colors).
    size_t since = 0u;
    auto const snapshot = m_snapshots.find(m_instance);
    if ((snapshot != m_snapshots.end()) && (snapshot->second.date <= m_cursor) &&
        (snapshot->second.tokens.size() == net.places().size()))
    {
        since = snapshot->second.entries;
        for (size_t p = 0u; p < net.places().size(); ++p)
            net.places()[p].tokens = snapshot->second.tokens[p];
    }
    else
    {
        for (size_t p = 0u; p < net.places().size(); ++p)
            net.places()[p].tokens = m_saved_tokens[p];
    }
    std::vector<LogEntry> past;
    std::map<std::string, size_t> running;
    for (size_t i = since; i < m_entries.size(); ++i)
    {
        LogEntry const& e = m_entries[i];
        if ((e.instance != m_instance) || (e.date > m_cursor))
            continue;
        past.push_back(e);
        if (e.event == LogEntry::Event::Scheduled)
            ++running[e.transition];
        else if ((e.event == LogEntry::Event::Completed) && (running[e.transition] > 0u))
            --running[e.transition];
    }
    Runtime runtime(net);
    std::shared_ptr<Instance> instance = runtime.restore(m_instance, past, Runtime::Recovery::Fail);
    Marking const counts = instance->marking().counts();
    for (size_t p = 0u; (p < counts.size()) && (p < net.places().size()); ++p)
        net.places()[p].tokens = counts[p];

    // Transitions whose function runs at the cursor, and the one selected.
    Palette const& pal = palette();
    std::vector<NodeMark> nodes;
    for (auto const& it : running)
    {
        size_t const t = runtime.transitionOf(it.first);
        if ((t != size_t(-1)) && (it.second > 0u))
            nodes.push_back({ net.transitions()[t].nodeId(), pal.active, "running" });
    }
    if (m_selected < m_entries.size())
    {
        size_t const t = runtime.transitionOf(m_entries[m_selected].transition);
        if (t != size_t(-1))
            nodes.push_back({ net.transitions()[t].nodeId(), pal.selection, "" });
    }
    marks.set(net, MARKS, std::move(nodes));
}

//------------------------------------------------------------------------------
void JournalView::toolbar(Net& net, bool const busy, NodeMarks& marks)
{
    if (ImGui::Button("Clear"))
    {
        clear();
        marks.clear(MARKS);
    }
    ImGui::SameLine();
    if (ImGui::Button("Open...") || request_open)
    {
        request_open = false;
        FileDialogHelper::openLoad("JournalOpen", "Open a journal", ".jsonl,.json,.log");
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(m_entries.empty());
    if (ImGui::Button("Save...") || (request_save && !m_entries.empty()))
        FileDialogHelper::openSave("JournalSave", "Save the journal", ".jsonl,.csv,.xes");
    request_save = false;
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::Checkbox("Follow", &m_follow);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(160.0f);
    ImGui::InputTextWithHint("##journal_filter", "Filter", &m_filter);
    ImGui::SameLine();
    ImGui::TextDisabled("%zu entries%s%s", m_entries.size(), m_source.empty() ? "" : "  from ",
                        m_source.c_str());

    FileDialogHelper::display("JournalOpen", [&](std::string const& path) {
        stopReplay();
        m_error = load(path, net);
    });
    FileDialogHelper::display("JournalSave", [&](std::string const& path) {
        m_error = save(path, net);
    });
    if (!m_error.empty())
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(palette().critical), "%s", m_error.c_str());

    // Replay: a cursor on the dates of an instance.
    std::set<std::string> instances;
    double first = 0.0, last = 0.0;
    for (auto const& e : m_entries)
        instances.insert(e.instance);
    for (auto const& it : m_snapshots)
        instances.insert(it.first);
    if (isLive() && m_follow)
    {
        // Following a program: the instance of the net shown (the graph of
        // the same name of a GRAFCET), else the one that moved last.
        std::string const follow = (instances.count(net.name) != 0u) ? net.name
            : (!m_entries.empty() ? m_entries.back().instance : m_instance);
        if (follow != m_instance)
        {
            m_instance = follow;
            m_dirty = true;
        }
    }
    if (m_instance.empty() || (instances.count(m_instance) == 0u))
        m_instance = (instances.count(net.name) != 0u) ? net.name
                   : (instances.empty() ? std::string() : *instances.begin());
    for (auto const& e : m_entries)
    {
        if (e.instance != m_instance)
            continue;
        last = std::max(last, e.date);
    }
    if (auto const it = m_snapshots.find(m_instance); it != m_snapshots.end())
        last = std::max(last, it->second.date);

    ImGui::BeginDisabled(busy || (m_entries.empty() && m_snapshots.empty()));
    bool on = m_replay;
    if (ImGui::Checkbox("Replay", &on))
    {
        if (on)
        {
            m_replay = true;
            m_replay_net = &net;
            m_saved_tokens = net.tokens();
            m_cursor = last;
            m_dirty = true;
        }
        else
        {
            stopReplay();
            marks.clear(MARKS);
        }
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip("Show the marking of the net at a date of the journal");
    if (!m_replay)
        return;

    ImGui::SameLine();
    ImGui::SetNextItemWidth(140.0f);
    if (ImGui::BeginCombo("##instance", m_instance.c_str()))
    {
        for (auto const& name : instances)
        {
            if (ImGui::Selectable(name.c_str(), name == m_instance))
            {
                m_instance = name;
                m_dirty = true;
            }
        }
        ImGui::EndCombo();
    }

    // Dates of the events of the instance, to jump from one to the next.
    std::vector<double> dates;
    for (auto const& e : m_entries)
    {
        if (e.instance == m_instance)
            dates.push_back(e.date);
    }
    std::sort(dates.begin(), dates.end());
    dates.erase(std::unique(dates.begin(), dates.end()), dates.end());
    auto jump = [&](bool const forward) {
        if (dates.empty())
            return;
        auto it = forward ? std::upper_bound(dates.begin(), dates.end(), m_cursor)
                          : std::lower_bound(dates.begin(), dates.end(), m_cursor);
        if (!forward)
            m_cursor = (it == dates.begin()) ? first : *(--it);
        else if (it != dates.end())
            m_cursor = *it;
        m_dirty = true;
    };
    ImGui::SameLine();
    if (ImGui::ArrowButton("##begin", ImGuiDir_Left))
    {
        m_cursor = first;
        m_dirty = true;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("<"))
        jump(false);
    ImGui::SameLine();
    float cursor = float(m_cursor);
    ImGui::SetNextItemWidth(std::max(120.0f, ImGui::GetContentRegionAvail().x - 170.0f));
    if (ImGui::SliderFloat("##cursor", &cursor, float(first), std::max(float(last), float(first) + 0.001f), "t = %.3f"))
    {
        m_cursor = double(cursor);
        m_dirty = true;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton(">"))
        jump(true);
    ImGui::SameLine();
    if (ImGui::ArrowButton("##end", ImGuiDir_Right))
    {
        m_cursor = last;
        m_dirty = true;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton(m_playing ? "Pause" : "Play"))
    {
        m_playing = !m_playing;
        if (m_playing && (m_cursor >= last))
            m_cursor = first;
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60.0f);
    ImGui::DragFloat("##speed", &m_speed, 0.05f, 0.1f, 100.0f, "x%.1f");
    if (isLive() && m_follow && (m_cursor != last))
    {
        m_cursor = last;
        m_dirty = true;
    }
    if (m_playing)
    {
        m_cursor = std::min(last, m_cursor + double(ImGui::GetIO().DeltaTime * m_speed));
        m_playing = (m_cursor < last);
        m_dirty = true;
    }
    replay(net, marks);
}

//------------------------------------------------------------------------------
void JournalView::table(Net& net, NodeMarks& marks)
{
    ImGuiTableFlags const flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV |
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit;
    if (!ImGui::BeginTable("##journal", 6, flags))
        return;
    float const em = ImGui::GetFontSize();
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("Date", ImGuiTableColumnFlags_WidthFixed, 5.0f * em);
    ImGui::TableSetupColumn("Instance", ImGuiTableColumnFlags_WidthFixed, 9.0f * em);
    ImGui::TableSetupColumn("Transition", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("k", ImGuiTableColumnFlags_WidthFixed, 2.5f * em);
    ImGui::TableSetupColumn("Event", ImGuiTableColumnFlags_WidthFixed, 6.0f * em);
    ImGui::TableSetupColumn("Result", ImGuiTableColumnFlags_WidthFixed, 6.0f * em);
    ImGui::TableHeadersRow();

    // Rows shown: the filter on the instance and the transition.
    std::vector<size_t> rows;
    rows.reserve(m_entries.size());
    for (size_t i = 0u; i < m_entries.size(); ++i)
    {
        LogEntry const& e = m_entries[i];
        if (m_filter.empty() || (e.instance.find(m_filter) != std::string::npos) ||
            (e.transition.find(m_filter) != std::string::npos))
            rows.push_back(i);
    }

    Palette const& pal = palette();
    ImGuiListClipper clipper;
    clipper.Begin(int(rows.size()));
    while (clipper.Step())
    {
        for (int r = clipper.DisplayStart; r < clipper.DisplayEnd; ++r)
        {
            size_t const i = rows[size_t(r)];
            LogEntry const& e = m_entries[i];
            bool const future = m_replay && ((e.date > m_cursor) || (e.instance != m_instance));
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            if (future)
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            char date[32];
            std::snprintf(date, sizeof(date), "%.3f##%zu", e.date, i);
            if (ImGui::Selectable(date, m_selected == i, ImGuiSelectableFlags_SpanAllColumns))
            {
                m_selected = i;
                if (m_replay)
                {
                    m_instance = e.instance;
                    m_cursor = e.date;
                }
                m_dirty = true;
            }
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(e.instance.c_str());
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(e.transition.c_str());
            ImGui::TableNextColumn();
            if (m_k[i] > 0u)
                ImGui::Text("%zu", m_k[i]);
            ImGui::TableNextColumn();
            std::string const event = to_str(e.event);
            ImGui::TextUnformatted(event.c_str());
            ImGui::TableNextColumn();
            if (e.event == LogEntry::Event::Completed)
            {
                std::string const result = net.colors.name(e.result);
                if (e.result == Failed)
                    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(pal.critical), "%s", result.c_str());
                else
                    ImGui::TextUnformatted(result.c_str());
            }
            if (future)
                ImGui::PopStyleColor();
        }
    }
    if (m_follow && (!m_replay || isLive()) && (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f))
        ImGui::SetScrollHereY(1.0f);
    ImGui::EndTable();

    // Out of the replay, the selected entry marks its transition.
    if (!m_replay)
    {
        std::vector<NodeMark> nodes;
        if (m_selected < m_entries.size())
        {
            Runtime const runtime(net);
            size_t const t = runtime.transitionOf(m_entries[m_selected].transition);
            if (t != size_t(-1))
                nodes.push_back({ net.transitions()[t].nodeId(), pal.selection, "" });
        }
        marks.set(net, MARKS, std::move(nodes));
    }
}

//------------------------------------------------------------------------------
void JournalView::draw(Net& net, bool const busy, NodeMarks& marks)
{
    // The replayed net changed (another net is active, it is simulated): it
    // gets its tokens back.
    if (m_replay && ((m_replay_net != &net) || busy ||
                     (m_saved_tokens.size() != net.places().size())))
    {
        stopReplay();
        marks.clear(MARKS);
    }

    if (!ImGui::Begin("Journal"))
    {
        ImGui::End();
        return;
    }
    toolbar(net, busy, marks);
    table(net, marks);
    ImGui::End();
}

} // namespace tpne
