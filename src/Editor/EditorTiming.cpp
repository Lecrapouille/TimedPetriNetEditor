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

// The timing diagram of a GRAFCET: the active steps, the inputs and the
// outputs during the simulation, one row each, like the diagrams of the help
// of the qualifiers.

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#  define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "Editor/Editor.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace tpne {

//! \brief Width of the column of the names.
static constexpr float LABEL_WIDTH = 150.0f;

//------------------------------------------------------------------------------
//! \brief Value of a track at a date (the last change before it).
//------------------------------------------------------------------------------
static int valueAt(Document::Track const& track, float const time)
{
    int value = 0;
    for (auto const& change : track.changes)
    {
        if (change.first > time)
            break;
        value = change.second;
    }
    return value;
}

//------------------------------------------------------------------------------
//! \brief Step between two ticks of the time axis: 1, 2 or 5 times a power of
//! 10, about 80 pixels apart.
//------------------------------------------------------------------------------
static float tickStep(float const span, float const width)
{
    float const raw = span * 80.0f / std::max(width, 1.0f);
    float const power = std::pow(10.0f, std::floor(std::log10(raw)));
    for (float m : { 1.0f, 2.0f, 5.0f, 10.0f })
    {
        if (m * power >= raw)
            return m * power;
    }
    return 10.0f * power;
}

//------------------------------------------------------------------------------
void Editor::timingDiagram()
{
    // First time: next to the console.
    if (ImGuiWindow* console = ImGui::FindWindowByName("Console"))
        ImGui::SetNextWindowDockID(console->DockId, ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Timing diagram"))
    {
        ImGui::End();
        return;
    }

    Document& doc = activeDocument();
    auto const& tracks = doc.tracks();
    float const now = doc.recordedTime();
    bool const running = doc.grafcet().isRunning();

    // Options.
    ImGui::Checkbox("Steps", &m_states.timing_steps);
    ImGui::SameLine();
    ImGui::Checkbox("Inputs", &m_states.timing_inputs);
    ImGui::SameLine();
    ImGui::Checkbox("Outputs", &m_states.timing_outputs);
    ImGui::SameLine();
    ImGui::Checkbox("Only what changed", &m_states.timing_used_only);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Hide the steps never active and the signals always 0");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(130.0f);
    ImGui::SliderFloat("##span", &m_states.timing_span, 2.0f, 600.0f, "last %.0f s", ImGuiSliderFlags_Logarithmic);
    ImGui::SameLine();
    ImGui::Checkbox("Follow", &m_states.timing_follow);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Show the last seconds of the simulation; else drag the time line");
    if (!m_states.timing_follow)
    {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(std::max(80.0f, ImGui::GetContentRegionAvail().x));
        float const last = std::max(now, m_states.timing_span);
        m_states.timing_end = std::clamp(m_states.timing_end, m_states.timing_span, last);
        ImGui::SliderFloat("##end", &m_states.timing_end, m_states.timing_span, last, "until %.1f s");
    }

    if (tracks.empty())
    {
        ImGui::TextDisabled("Start the simulation (Space): the active steps, the inputs and the "
                            "outputs are drawn here while the GRAFCET runs.");
        ImGui::End();
        return;
    }

    // Rows: steps (graph by graph), inputs, outputs.
    std::vector<Document::Track const*> rows;
    for (Document::Track::Kind kind : { Document::Track::Kind::Step, Document::Track::Kind::Input,
                                        Document::Track::Kind::Output })
    {
        bool const shown = (kind == Document::Track::Kind::Step) ? m_states.timing_steps
                         : (kind == Document::Track::Kind::Input) ? m_states.timing_inputs
                         : m_states.timing_outputs;
        if (!shown)
            continue;
        std::vector<Document::Track const*> group;
        for (auto const& track : tracks)
        {
            if ((track.kind == kind) && (track.used || !m_states.timing_used_only))
                group.push_back(&track);
        }
        std::stable_sort(group.begin(), group.end(), [](auto const* a, auto const* b) {
            return a->graph < b->graph;
        });
        rows.insert(rows.end(), group.begin(), group.end());
    }

    float const span = m_states.timing_span;
    float const end = m_states.timing_follow ? std::max(now, span) : m_states.timing_end;
    float const start = end - span;
    float const row = ImGui::GetTextLineHeight() + 8.0f;
    float const axis = ImGui::GetTextLineHeight() + 6.0f;

    ImGui::BeginChild("##timing", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None);
    ImVec2 const origin = ImGui::GetCursorScreenPos();
    float const width = std::max(100.0f, ImGui::GetContentRegionAvail().x);
    ImGui::InvisibleButton("##timing_area", ImVec2(width, axis + row * float(rows.size())));
    bool const hovered = ImGui::IsItemHovered();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    float const x0 = origin.x + LABEL_WIDTH;
    float const span_px = width - LABEL_WIDTH - 8.0f;
    auto const xOf = [&](float t) { return x0 + (t - start) / span * span_px; };

    ImU32 const text = ImGui::GetColorU32(ImGuiCol_Text);
    ImU32 const dim = ImGui::GetColorU32(ImGuiCol_TextDisabled);
    ImU32 const grid = ImGui::GetColorU32(ImGuiCol_Separator, 0.45f);
    ImVec2 const clip_min = ImGui::GetWindowPos();
    ImVec2 const clip_max = clip_min + ImGui::GetWindowSize();

    // Time axis on top, and the grid.
    float const tick = tickStep(span, span_px);
    for (float t = std::ceil(start / tick) * tick; t <= end + 1e-3f; t += tick)
    {
        if (t < 0.0f)
            continue;
        float const x = xOf(t);
        char label[32];
        std::snprintf(label, sizeof(label), "%g s", double(t));
        draw->AddText(ImVec2(x + 2.0f, origin.y), dim, label);
        draw->AddLine(ImVec2(x, origin.y + axis - 2.0f), ImVec2(x, origin.y + axis + row * float(rows.size())), grid);
    }

    // One row per signal: a square wave (steps blue, inputs orange, outputs
    // green), a curve with its value for a sensor.
    for (size_t r = 0u; r < rows.size(); ++r)
    {
        Document::Track const& track = *rows[r];
        float const top = origin.y + axis + row * float(r) + 3.0f;
        float const bottom = top + row - 6.0f;
        if ((bottom < clip_min.y) || (top > clip_max.y))
            continue;
        ImU32 color = IM_COL32(70, 140, 230, 255);
        if (track.kind == Document::Track::Kind::Input)
            color = IM_COL32(235, 150, 40, 255);
        else if (track.kind == Document::Track::Kind::Output)
            color = IM_COL32(60, 185, 90, 255);
        ImU32 const fill = (color & 0x00FFFFFF) | 0x50000000;
        draw->AddText(ImVec2(origin.x, top - 1.0f), text, track.name.c_str());

        // A sensor: its range, else 0 / 1.
        int low = 0;
        int high = 1;
        bool analog = false;
        if (track.kind == Document::Track::Kind::Input)
        {
            for (auto const& input : doc.info().inputs)
            {
                if ((input.name == track.name) && (input.kind == GrafcetInput::Kind::Value))
                {
                    low = input.min;
                    high = std::max(input.min + 1, input.max);
                    analog = true;
                }
            }
        }
        auto const yOf = [&](int value) {
            float const ratio = std::clamp(float(value - low) / float(high - low), 0.0f, 1.0f);
            return bottom - (bottom - top) * ratio;
        };

        draw->PushClipRect(ImVec2(x0, top - 1.0f), ImVec2(x0 + span_px, bottom + 1.0f), true);
        std::vector<ImVec2> points;
        int value = valueAt(track, start);
        points.push_back(ImVec2(xOf(start), yOf(value)));
        for (auto const& change : track.changes)
        {
            if (change.first <= start)
                continue;
            if (change.first > end)
                break;
            float const x = xOf(change.first);
            if (!analog && (value != 0))
                draw->AddRectFilled(ImVec2(points.back().x, top), ImVec2(x, bottom), fill);
            points.push_back(ImVec2(x, yOf(value)));
            value = change.second;
            points.push_back(ImVec2(x, yOf(value)));
        }
        float const last_x = xOf(std::min(end, std::max(now, start)));
        if (!analog && (value != 0))
            draw->AddRectFilled(ImVec2(points.back().x, top), ImVec2(last_x, bottom), fill);
        points.push_back(ImVec2(last_x, yOf(value)));
        draw->AddPolyline(points.data(), int(points.size()), color, ImDrawFlags_None, 1.6f);
        draw->PopClipRect();
        if (analog)
        {
            char number[32];
            std::snprintf(number, sizeof(number), "%d", value);
            draw->AddText(ImVec2(origin.x + LABEL_WIDTH - ImGui::CalcTextSize(number).x - 6.0f, top - 1.0f),
                          dim, number);
        }
    }

    // The time under the mouse: a line, and the values in a tooltip.
    if (hovered)
    {
        float const mouse_x = ImGui::GetIO().MousePos.x;
        if (mouse_x >= x0)
        {
            float const t = start + (mouse_x - x0) / span_px * span;
            draw->AddLine(ImVec2(mouse_x, origin.y), ImVec2(mouse_x, origin.y + axis + row * float(rows.size())),
                          IM_COL32(230, 50, 50, 200), 1.5f);
            ImGui::BeginTooltip();
            ImGui::Text("t = %.2f s", double(t));
            size_t const r = size_t(std::max(0.0f, (ImGui::GetIO().MousePos.y - origin.y - axis) / row));
            if (r < rows.size())
                ImGui::Text("%s = %d", rows[r]->name.c_str(), valueAt(*rows[r], t));
            ImGui::EndTooltip();
        }
    }
    ImGui::EndChild();
    if (running)
        m_busy_frames = std::max(m_busy_frames, 1);
    ImGui::End();
}

} // namespace tpne
