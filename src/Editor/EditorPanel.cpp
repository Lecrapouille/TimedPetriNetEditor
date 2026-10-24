// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

// The control panel of a GRAFCET, drawn like the desk of a machine: push
// buttons, emergency stop, selectors, sensors, and the actuators driven by
// the actions (pilot lights, motors, heaters ...). Also the window that
// shows and edits the description of the document.

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#  define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "Editor/Editor.hpp"
#include "Editor/GrafcetHelp.hpp"
#include "imgui/imgui.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <set>

namespace tpne {

//! \brief Scale of the drawings of the control panel (smaller to fit all
//! the inputs in the window), and the same for the outputs.
static float s_scale = 1.0f;
static float s_output_scale = 1.0f;
//! \brief A length at the scale of the panel.
static float S(float const value) { return value * s_scale; }
//! \brief Size of a cell of the panel (a button and its label).
#define CELL_WIDTH S(96.0f)
#define CELL_HEIGHT S(92.0f)
//! \brief Size of a cell of a sensor value (gauge and slider).
#define VALUE_WIDTH S(170.0f)

//! \brief Push buttons kept pressed by a right click.
static std::set<std::string> s_latched;
//! \brief Engine time of the press of each push button.
static std::map<std::string, float> s_press_time;
//! \brief Wall clock time of the last click on each pulse button.
static std::map<std::string, double> s_flash;
//! \brief Angle of the fan of each motor.
static std::map<std::string, float> s_fan_angle;

//------------------------------------------------------------------------------
//! \brief Color of a cap from its name, or the default color of the kind.
//------------------------------------------------------------------------------
static ImU32 capColor(GrafcetInput const& input)
{
    std::string color = input.color;
    if (color.empty())
    {
        switch (input.kind)
        {
        case GrafcetInput::Kind::Button: color = "green"; break;
        case GrafcetInput::Kind::Pulse: color = "blue"; break;
        case GrafcetInput::Kind::Emergency: color = "red"; break;
        case GrafcetInput::Kind::Switch:
        case GrafcetInput::Kind::Selector:
        case GrafcetInput::Kind::Value:
        case GrafcetInput::Kind::Clock: color = "black"; break;
        }
    }
    if (color == "red") return IM_COL32(200, 40, 40, 255);
    if (color == "black") return IM_COL32(45, 45, 48, 255);
    if (color == "yellow") return IM_COL32(235, 200, 40, 255);
    if (color == "blue") return IM_COL32(40, 95, 205, 255);
    if (color == "white") return IM_COL32(235, 235, 235, 255);
    if (color == "orange") return IM_COL32(240, 140, 30, 255);
    return IM_COL32(40, 160, 70, 255);
}

//------------------------------------------------------------------------------
//! \brief Color of a pilot light.
//------------------------------------------------------------------------------
static ImU32 ledColor(Action::LedColor const color, float const alpha = 1.0f)
{
    ImVec4 c;
    switch (color)
    {
    case Action::LedColor::Green: c = ImVec4(0.25f, 0.95f, 0.35f, 1.0f); break;
    case Action::LedColor::Red: c = ImVec4(1.0f, 0.25f, 0.2f, 1.0f); break;
    case Action::LedColor::Orange: c = ImVec4(1.0f, 0.6f, 0.1f, 1.0f); break;
    case Action::LedColor::Yellow: c = ImVec4(1.0f, 0.92f, 0.25f, 1.0f); break;
    case Action::LedColor::Blue: c = ImVec4(0.3f, 0.6f, 1.0f, 1.0f); break;
    case Action::LedColor::White: c = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); break;
    }
    c.w = alpha;
    return ImGui::ColorConvertFloat4ToU32(c);
}

//------------------------------------------------------------------------------
//! \brief Darker or lighter color (factor < 1 darker).
//------------------------------------------------------------------------------
static ImU32 shade(ImU32 const color, float const factor)
{
    ImVec4 c = ImGui::ColorConvertU32ToFloat4(color);
    c.x = std::clamp(c.x * factor, 0.0f, 1.0f);
    c.y = std::clamp(c.y * factor, 0.0f, 1.0f);
    c.z = std::clamp(c.z * factor, 0.0f, 1.0f);
    return ImGui::ColorConvertFloat4ToU32(c);
}

//------------------------------------------------------------------------------
//! \brief Label centered under a widget, cut to the width of the cell.
//------------------------------------------------------------------------------
static void cellLabel(ImDrawList* draw, ImVec2 const& p, float const width, char const* text,
                      ImU32 const color)
{
    std::string label(text);
    while ((label.size() > 1u) && (ImGui::CalcTextSize(label.c_str()).x > width - 4.0f))
        label.pop_back();
    ImVec2 const size = ImGui::CalcTextSize(label.c_str());
    draw->AddText(ImVec2(p.x + (width - size.x) * 0.5f, p.y), color, label.c_str());
}

//------------------------------------------------------------------------------
//! \brief Metal bezel of a button: ring with a light top and a dark bottom.
//------------------------------------------------------------------------------
static void bezel(ImDrawList* draw, ImVec2 const& c, float const r)
{
    draw->AddCircleFilled(c + ImVec2(1.5f, 2.0f), r + 1.0f, IM_COL32(0, 0, 0, 70), 40);
    draw->AddCircleFilled(c, r, IM_COL32(150, 152, 158, 255), 40);
    draw->AddCircleFilled(c - ImVec2(0.0f, 1.0f), r - 2.0f, IM_COL32(200, 202, 208, 255), 40);
    draw->AddCircleFilled(c, r - S(4.0f), IM_COL32(90, 92, 98, 255), 40);
}

//------------------------------------------------------------------------------
//! \brief Round cap of a push button, lower and darker when pressed.
//------------------------------------------------------------------------------
static void cap(ImDrawList* draw, ImVec2 const& c, float const r, ImU32 const color, bool const pressed,
                bool const hovered)
{
    ImU32 const base = pressed ? shade(color, 0.7f) : (hovered ? shade(color, 1.12f) : color);
    ImVec2 const at = c + (pressed ? ImVec2(0.0f, 1.5f) : ImVec2(0.0f, 0.0f));
    float const radius = pressed ? r - 1.5f : r;
    draw->AddCircleFilled(at, radius, shade(base, 0.75f), 40);
    draw->AddCircleFilled(at - ImVec2(0.0f, 1.0f), radius - 2.0f, base, 40);
    if (!pressed)
        draw->AddCircleFilled(at - ImVec2(radius * 0.3f, radius * 0.35f), radius * 0.3f,
                              IM_COL32(255, 255, 255, 60), 24);
}

//------------------------------------------------------------------------------
//! \brief Scale for the next frame: smaller while the widgets are higher than
//! the room, larger (up to 1) while they leave room enough.
//------------------------------------------------------------------------------
static bool fitScale(float& scale, bool const fit, float const used, float const room)
{
    float const before = scale;
    if (!fit)
        scale = 1.0f;
    else if ((used > room) && (scale > 0.6f))
        scale = std::max(0.6f, scale * 0.96f);
    else if ((used < room * 0.8f) && (scale < 1.0f))
        scale = std::min(1.0f, scale * 1.02f);
    return scale != before;
}

//------------------------------------------------------------------------------
void Editor::collectGrafcetIo(std::vector<GrafcetInput*>& inputs, std::vector<GrafcetOutput*>& outputs)
{
    // The described inputs and outputs in their order, then the others with
    // a default description that is not kept in the document (else each
    // name typed letter by letter would stay): an edit keeps it.
    Document& doc = activeDocument();
    std::set<std::string> used_inputs;
    for (auto const& it : doc.grafcet().inputs())
        used_inputs.insert(it.first);
    std::vector<std::string> used_outputs;
    for (auto const& entry : doc.nets())
    {
        if (entry->net.type() != TypeOfNet::GRAFCET)
            continue;
        for (auto const& place : entry->net.places())
        {
            for (auto const& action : place.actions)
            {
                if (!action.name.empty() &&
                    (std::find(used_outputs.begin(), used_outputs.end(), action.name) == used_outputs.end()))
                    used_outputs.push_back(action.name);
            }
        }
    }

    inputs.clear();
    m_undescribed_inputs.clear();
    for (auto& input : doc.info().inputs)
    {
        if (used_inputs.erase(input.name) > 0u)
            inputs.push_back(&input);
    }
    for (auto const& name : used_inputs)
    {
        GrafcetInput input;
        input.name = name;
        m_undescribed_inputs.push_back(input);
        inputs.push_back(&m_undescribed_inputs.back());
    }

    outputs.clear();
    m_undescribed_outputs.clear();
    for (auto& output : doc.info().outputs)
    {
        auto const it = std::find(used_outputs.begin(), used_outputs.end(), output.name);
        if (it != used_outputs.end())
        {
            outputs.push_back(&output);
            used_outputs.erase(it);
        }
    }
    for (auto const& name : used_outputs)
    {
        GrafcetOutput output;
        output.name = name;
        m_undescribed_outputs.push_back(output);
        outputs.push_back(&m_undescribed_outputs.back());
    }
}

//------------------------------------------------------------------------------
//! \brief Value of an input: the one of the running engine (or the one
//! waiting for Apply), else its initial value.
//------------------------------------------------------------------------------
static int panelValue(Document const& doc, std::map<std::string, int> const& pending,
                      std::string const& name)
{
    if (doc.grafcet().isRunning())
    {
        auto const it = pending.find(name);
        return (it != pending.end()) ? it->second : doc.grafcet().inputValue(name);
    }
    for (auto const& entry : doc.nets())
    {
        auto const it = entry->net.inputs.find(name);
        if (it != entry->net.inputs.end())
            return it->second;
    }
    for (auto const& input : doc.info().inputs)
    {
        if (input.name == name)
            return input.initial;
    }
    return 0;
}

//------------------------------------------------------------------------------
void Editor::setPanelInput(std::string const& name, int const value, bool const running)
{
    Document& doc = activeDocument();
    if (running)
    {
        if (m_states.batch_inputs)
            m_states.pending_inputs[name] = value;
        else
            doc.setInput(name, value);
        return;
    }

    // Stopped: the initial value, saved with the document.
    for (auto const& entry : doc.nets())
    {
        if (entry->net.type() == TypeOfNet::GRAFCET)
            entry->net.inputs[name] = value;
    }
    doc.describedInput(name).initial = value;
    doc.grafcet().setInput(name, value);
    net().modified = true;
}

//------------------------------------------------------------------------------
void Editor::inputWidget(GrafcetInput& input, bool const running)
{
    Document& doc = activeDocument();
    GrafcetEngine& engine = doc.grafcet();
    std::string const& name = input.name;
    int const value = panelValue(doc, m_states.pending_inputs, name);
    bool const pending = running && (m_states.pending_inputs.count(name) > 0u);
    float const width = (input.kind == GrafcetInput::Kind::Value) ? VALUE_WIDTH : CELL_WIDTH;

    ImGui::PushID(name.c_str());
    ImGui::BeginGroup();
    ImVec2 const p = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##cell", ImVec2(width, CELL_HEIGHT),
                           ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
    bool const hovered = ImGui::IsItemHovered();
    bool const held = ImGui::IsItemActive();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImU32 const text = ImGui::GetColorU32(ImGuiCol_Text);
    ImU32 const dim = ImGui::GetColorU32(ImGuiCol_TextDisabled);
    ImVec2 const c(p.x + width * 0.5f, p.y + S(32.0f));
    float const label_y = p.y + CELL_HEIGHT - ImGui::GetTextLineHeight() * 2.0f - 2.0f;
    char const* state = nullptr;
    ImU32 state_color = dim;

    switch (input.kind)
    {
    case GrafcetInput::Kind::Button:
    {
        // Press: 1. Release: 0, or a pulse if the engine did not run a cycle
        // since the press (a quick click is seen anyway). Right click: keep
        // it pressed (or release it).
        bool const latched = (s_latched.count(name) > 0u);
        if (running)
        {
            if (ImGui::IsItemActivated() && ImGui::IsMouseDown(ImGuiMouseButton_Left) && !latched)
            {
                doc.setInput(name, 1);
                s_press_time[name] = engine.time();
            }
            if (ImGui::IsItemDeactivated() && !latched && (engine.inputValue(name) != 0))
            {
                if (engine.time() == s_press_time[name])
                    doc.pulseInput(name);
                else
                    doc.setInput(name, 0);
            }
            if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
            {
                if (latched)
                    s_latched.erase(name);
                else
                    s_latched.insert(name);
                doc.setInput(name, latched ? 0 : 1);
            }
        }
        else
        {
            s_latched.erase(name);
        }
        bool const pressed = (value != 0);
        bezel(draw, c, S(24.0f));
        cap(draw, c, S(17.0f), capColor(input), pressed, hovered && running);
        if (s_latched.count(name))
            state = "held", state_color = IM_COL32(240, 150, 30, 255);
        break;
    }

    case GrafcetInput::Kind::Pulse:
    {
        if (running && ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            doc.pulseInput(name);
            s_flash[name] = ImGui::GetTime();
        }
        bool const flash = (ImGui::GetTime() - s_flash[name] < 0.15) || (value != 0);
        bezel(draw, c, S(24.0f));
        float const h = S(15.0f);
        ImVec2 const a = c - ImVec2(h, h) + (flash ? ImVec2(0.0f, 1.5f) : ImVec2());
        ImU32 const color = capColor(input);
        draw->AddRectFilled(a, a + ImVec2(2.0f * h, 2.0f * h), shade(color, flash ? 0.65f : 1.0f), S(4.0f));
        // Pulse symbol: _|‾|_
        ImVec2 const s = a + ImVec2(S(6.0f), h + S(5.0f));
        ImVec2 const wave[] = { s, s + ImVec2(S(6.0f), 0.0f), s + ImVec2(S(6.0f), -S(10.0f)),
                                s + ImVec2(S(12.0f), -S(10.0f)), s + ImVec2(S(12.0f), 0.0f), s + ImVec2(S(18.0f), 0.0f) };
        draw->AddPolyline(wave, 6, IM_COL32(255, 255, 255, 220), ImDrawFlags_None, 1.5f);
        break;
    }

    case GrafcetInput::Kind::Emergency:
    {
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            setPanelInput(name, value ? 0 : 1, running);
        bool const pushed = (value != 0);
        // Yellow disk, red mushroom head.
        draw->AddCircleFilled(c + ImVec2(1.5f, 2.0f), S(29.0f), IM_COL32(0, 0, 0, 70), 48);
        draw->AddCircleFilled(c, S(28.0f), IM_COL32(240, 200, 30, 255), 48);
        draw->AddCircle(c, S(28.0f), IM_COL32(150, 120, 10, 255), 48, 1.5f);
        cap(draw, c, pushed ? S(17.0f) : S(20.0f), IM_COL32(205, 30, 30, 255), pushed, hovered);
        state = pushed ? "pushed" : "released";
        state_color = pushed ? IM_COL32(230, 50, 50, 255) : dim;
        break;
    }

    case GrafcetInput::Kind::Switch:
    {
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            setPanelInput(name, value ? 0 : 1, running);
        // Rotary switch: black handle on 0 (left) or 1 (right).
        bezel(draw, c, S(22.0f));
        float const angle = (value ? S(45.0f) : -S(45.0f)) * S(3.14159265f) / S(180.0f);
        ImVec2 const dir(std::sin(angle), -std::cos(angle));
        ImVec2 const n(-dir.y, dir.x);
        ImU32 const color = capColor(input);
        draw->AddCircleFilled(c, S(15.0f), color, 32);
        ImVec2 const quad[] = { c + dir * S(17.0f) + n * S(4.0f), c + dir * S(17.0f) - n * S(4.0f),
                                c - dir * S(17.0f) - n * S(4.0f), c - dir * S(17.0f) + n * S(4.0f) };
        draw->AddConvexPolyFilled(quad, 4, shade(color, hovered ? 1.4f : 1.2f));
        draw->AddLine(c, c + dir * S(14.0f), IM_COL32(255, 255, 255, 230), 2.0f);
        draw->AddText(c + ImVec2(-S(30.0f), -S(28.0f)), value ? dim : text, "0");
        draw->AddText(c + ImVec2(S(25.0f), -S(28.0f)), value ? text : dim, "1");
        break;
    }

    case GrafcetInput::Kind::Clock:
    {
        // A pilot light following the square signal.
        draw->AddCircleFilled(c, S(20.0f), IM_COL32(60, 62, 68, 255), 32);
        draw->AddCircleFilled(c, S(15.0f), value ? ledColor(Action::LedColor::Yellow)
                                             : IM_COL32(90, 85, 50, 255), 32);
        char period[32];
        std::snprintf(period, sizeof(period), "%.2g s", double(input.period));
        state = nullptr;
        cellLabel(draw, ImVec2(p.x, label_y + ImGui::GetTextLineHeight()), width, period, dim);
        break;
    }

    case GrafcetInput::Kind::Value:
    {
        // Gauge: a bar filled from min to max, the value and its unit.
        float const range = float(std::max(1, input.max - input.min));
        float const ratio = std::clamp(float(value - input.min) / range, 0.0f, 1.0f);
        ImVec2 const a(p.x + S(8.0f), p.y + S(18.0f));
        ImVec2 const b(p.x + width - S(8.0f), p.y + S(34.0f));
        draw->AddRectFilled(a, b, IM_COL32(50, 52, 58, 255), S(4.0f));
        ImU32 const fill = (input.unit.find("C") != std::string::npos)
            ? ImGui::ColorConvertFloat4ToU32(ImVec4(0.3f + 0.7f * ratio, 0.55f - 0.35f * ratio, 1.0f - 0.8f * ratio, 1.0f))
            : IM_COL32(60, 150, 230, 255);
        draw->AddRectFilled(a, ImVec2(a.x + (b.x - a.x) * ratio, b.y), fill, S(4.0f));
        draw->AddRect(a, b, IM_COL32(20, 20, 20, 255), S(4.0f));
        char number[64];
        std::snprintf(number, sizeof(number), "%d %s", value, input.unit.c_str());
        ImVec2 const size = ImGui::CalcTextSize(number);
        draw->AddText(ImVec2(p.x + (width - size.x) * 0.5f, b.y + S(3.0f)), text, number);
        break;
    }

    case GrafcetInput::Kind::Selector:
        break;
    }

    // Name, and the state or what is waiting for Apply.
    cellLabel(draw, ImVec2(p.x, label_y), width, name.c_str(), text);
    if (pending)
    {
        draw->AddRect(p, p + ImVec2(width, CELL_HEIGHT), IM_COL32(240, 150, 30, 255), S(6.0f), 0, 2.0f);
        cellLabel(draw, ImVec2(p.x, label_y + ImGui::GetTextLineHeight()), width, "pending",
                  IM_COL32(240, 150, 30, 255));
    }
    else if (state != nullptr)
    {
        cellLabel(draw, ImVec2(p.x, label_y + ImGui::GetTextLineHeight()), width, state, state_color);
    }

    if (hovered && !held)
    {
        ImGui::BeginTooltip();
        ImGui::Text("%s = %d", name.c_str(), value);
        if (!input.description.empty())
            ImGui::TextUnformatted(input.description.c_str());
        ImGui::PushTextWrapPos(S(320.0f));
        ImGui::TextDisabled("%s", inputKindMeaning(input.kind));
        if (!running && ((input.kind == GrafcetInput::Kind::Button) ||
                         (input.kind == GrafcetInput::Kind::Pulse)))
            ImGui::TextDisabled("Start the simulation (Space) to press it.");
        else if (!running)
            ImGui::TextDisabled("Stopped: a click changes the initial value.");
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }

    // The slider of a value, under the gauge (not for a value computed from
    // an output while running).
    if (input.kind == GrafcetInput::Kind::Value)
    {
        ImGui::SetCursorScreenPos(ImVec2(p.x + S(8.0f), p.y + S(54.0f)));
        ImGui::SetNextItemWidth(width - S(16.0f));
        int slider = value;
        ImGui::BeginDisabled(running && !input.driven_by.empty());
        if (ImGui::SliderInt("##value", &slider, input.min, input.max, ""))
            setPanelInput(name, slider, running);
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered() && !input.driven_by.empty())
            ImGui::SetTooltip("Computed while simulating: rises while %s is on", input.driven_by.c_str());
    }
    ImGui::EndGroup();
    ImGui::PopID();
}

//------------------------------------------------------------------------------
void Editor::selectorWidget(std::vector<GrafcetInput*> const& positions, bool const running)
{
    if (positions.empty())
        return;
    Document& doc = activeDocument();
    std::string const& group = positions.front()->group;
    float const width = std::max(CELL_WIDTH * 1.5f, S(50.0f) * float(positions.size()));

    ImGui::PushID(("selector_" + group).c_str());
    ImVec2 const p = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##cell", ImVec2(width, CELL_HEIGHT));
    bool const hovered = ImGui::IsItemHovered();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImU32 const text = ImGui::GetColorU32(ImGuiCol_Text);
    ImU32 const dim = ImGui::GetColorU32(ImGuiCol_TextDisabled);
    ImVec2 const c(p.x + width * 0.5f, p.y + S(46.0f));

    // Positions on an arc above the knob, from -60 to +60 degrees.
    size_t const n = positions.size();
    auto const angleOf = [n](size_t i) {
        float const spread = (n == 1u) ? 0.0f : std::min(S(120.0f), S(45.0f) * float(n - 1u));
        float const degrees = (n == 1u) ? S(45.0f) : -spread * 0.5f + spread * float(i) / float(n - 1u);
        return degrees * S(3.14159265f) / S(180.0f);
    };
    int selected = -1;
    for (size_t i = 0u; i < n; ++i)
    {
        if (panelValue(doc, m_states.pending_inputs, positions[i]->name) != 0)
            selected = int(i);
    }

    // A click: the position nearest to the mouse (the knob itself: the next one).
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
    {
        ImVec2 const d = ImGui::GetIO().MousePos - c;
        size_t target = size_t(selected + 1) % n;
        if (d.x * d.x + d.y * d.y > S(18.0f) * S(18.0f))
        {
            float const mouse = std::atan2(d.x, -d.y);
            float best = 1e9f;
            for (size_t i = 0u; i < n; ++i)
            {
                float const gap = std::fabs(angleOf(i) - mouse);
                if (gap < best) { best = gap; target = i; }
            }
        }
        for (size_t i = 0u; i < n; ++i)
            setPanelInput(positions[i]->name, (i == target) ? 1 : 0, running);
        selected = int(target);
    }

    bezel(draw, c, S(22.0f));
    for (size_t i = 0u; i < n; ++i)
    {
        float const angle = angleOf(i);
        ImVec2 const dir(std::sin(angle), -std::cos(angle));
        ImVec2 const at = c + dir * S(34.0f);
        char const* label = positions[i]->name.c_str();
        ImVec2 const size = ImGui::CalcTextSize(label);
        // Pushed outwards by half its width, so that the names do not overlap.
        ImVec2 const center = at + ImVec2(dir.x * size.x * 0.5f, 0.0f);
        draw->AddText(center - size * 0.5f - ImVec2(0.0f, S(4.0f)), (int(i) == selected) ? text : dim, label);
        draw->AddLine(c + dir * S(23.0f), c + dir * S(26.0f), dim, 1.5f);
    }
    float const angle = (selected >= 0) ? angleOf(size_t(selected)) : 0.0f;
    ImVec2 const dir(std::sin(angle), -std::cos(angle));
    ImVec2 const nrm(-dir.y, dir.x);
    draw->AddCircleFilled(c, S(15.0f), IM_COL32(40, 40, 44, 255), 32);
    ImVec2 const quad[] = { c + dir * S(18.0f) + nrm * S(4.0f), c + dir * S(18.0f) - nrm * S(4.0f),
                            c - dir * S(18.0f) - nrm * S(4.0f), c - dir * S(18.0f) + nrm * S(4.0f) };
    draw->AddConvexPolyFilled(quad, 4, hovered ? IM_COL32(85, 85, 92, 255) : IM_COL32(60, 60, 66, 255));
    draw->AddLine(c, c + dir * S(15.0f), IM_COL32(255, 255, 255, 230), 2.0f);
    cellLabel(draw, ImVec2(p.x, p.y + CELL_HEIGHT - ImGui::GetTextLineHeight() - 2.0f), width,
              group.c_str(), text);

    if (hovered)
    {
        ImGui::BeginTooltip();
        ImGui::Text("Selector %s: one position only", group.c_str());
        for (GrafcetInput const* input : positions)
        {
            ImGui::BulletText("%s%s%s", input->name.c_str(), input->description.empty() ? "" : ": ",
                              input->description.c_str());
        }
        ImGui::EndTooltip();
    }
    ImGui::PopID();
}

//------------------------------------------------------------------------------
//! \brief Table to choose how the inputs and outputs are drawn.
//------------------------------------------------------------------------------
static bool editIo(Document& doc, std::vector<GrafcetInput*> const& inputs, std::vector<GrafcetOutput*> const& outputs)
{
    bool modified = false;
    ImGuiTableFlags const flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                  ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX;
    ImGui::SeparatorText("Inputs");
    if (ImGui::BeginTable("##edit_inputs", 4, flags))
    {
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Kind");
        ImGui::TableSetupColumn("Settings");
        ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();
        static char const* KINDS[] = { "switch", "button", "pulse", "emergency", "selector", "value", "clock" };
        static char const* COLORS[] = { "", "green", "red", "black", "yellow", "blue", "white", "orange" };
        for (GrafcetInput* input : inputs)
        {
            ImGui::PushID(input->name.c_str());
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(input->name.c_str());
            ImGui::TableNextColumn();
            int kind = int(input->kind);
            ImGui::SetNextItemWidth(95.0f);
            if (ImGui::Combo("##kind", &kind, KINDS, IM_ARRAYSIZE(KINDS)))
            {
                input->kind = GrafcetInput::Kind(kind);
                if ((input->kind == GrafcetInput::Kind::Selector) && input->group.empty())
                    input->group = "Mode";
                modified = true;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", inputKindMeaning(input->kind));
            ImGui::TableNextColumn();
            switch (input->kind)
            {
            case GrafcetInput::Kind::Selector:
                ImGui::SetNextItemWidth(110.0f);
                modified |= ImGui::InputTextWithHint("##group", "group", &input->group);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Inputs of the same group are the positions of one selector");
                break;
            case GrafcetInput::Kind::Value:
                ImGui::SetNextItemWidth(110.0f);
                modified |= ImGui::DragIntRange2("##range", &input->min, &input->max, 1.0f, -100000, 100000,
                                                 "min %d", "max %d");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(45.0f);
                modified |= ImGui::InputTextWithHint("##unit", "unit", &input->unit);
                ImGui::SetNextItemWidth(110.0f);
                modified |= ImGui::InputTextWithHint("##driven", "driven by output", &input->driven_by);
                if (!input->driven_by.empty())
                {
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(55.0f);
                    modified |= ImGui::DragFloat("##rise", &input->rise, 0.1f, 0.0f, 1000.0f, "+%.1f/s");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(55.0f);
                    modified |= ImGui::DragFloat("##fall", &input->fall, 0.1f, 0.0f, 1000.0f, "-%.1f/s");
                }
                break;
            case GrafcetInput::Kind::Clock:
                ImGui::SetNextItemWidth(110.0f);
                modified |= ImGui::DragFloat("##period", &input->period, 0.05f, 0.05f, 3600.0f, "period %.2f s");
                break;
            case GrafcetInput::Kind::Button:
            case GrafcetInput::Kind::Switch:
            case GrafcetInput::Kind::Pulse:
            case GrafcetInput::Kind::Emergency:
            {
                int color = 0;
                for (int i = 0; i < IM_ARRAYSIZE(COLORS); ++i)
                    if (input->color == COLORS[i])
                        color = i;
                ImGui::SetNextItemWidth(95.0f);
                if (ImGui::Combo("##color", &color, COLORS, IM_ARRAYSIZE(COLORS)))
                {
                    input->color = COLORS[color];
                    modified = true;
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Color of the cap (empty: from the kind)");
                break;
            }
            }
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1.0f);
            modified |= ImGui::InputText("##description", &input->description);
            ImGui::PopID();
            // Edited: kept in the document.
            if (modified && (findInput(doc.info(), input->name) == nullptr))
                doc.describedInput(input->name) = *input;
        }
        ImGui::EndTable();
    }

    ImGui::SeparatorText("Outputs");
    if (ImGui::BeginTable("##edit_outputs", 3, flags))
    {
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Kind");
        ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();
        static char const* KINDS[] = { "light", "motor", "heater", "valve", "horn" };
        for (GrafcetOutput* output : outputs)
        {
            ImGui::PushID(output->name.c_str());
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(output->name.c_str());
            ImGui::TableNextColumn();
            int kind = int(output->kind);
            ImGui::SetNextItemWidth(95.0f);
            if (ImGui::Combo("##kind", &kind, KINDS, IM_ARRAYSIZE(KINDS)))
            {
                output->kind = GrafcetOutput::Kind(kind);
                modified = true;
            }
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1.0f);
            modified |= ImGui::InputText("##description", &output->description);
            ImGui::PopID();
            if (modified && (findOutput(doc.info(), output->name) == nullptr))
                doc.describedOutput(output->name) = *output;
        }
        ImGui::EndTable();
    }
    return modified;
}

//------------------------------------------------------------------------------
void Editor::controlPanel()
{
    if (!ImGui::Begin("Control panel"))
    {
        ImGui::End();
        return;
    }

    Document& doc = activeDocument();
    bool const running = doc.grafcet().isRunning();
    std::vector<GrafcetInput*> inputs;
    std::vector<GrafcetOutput*> outputs;
    collectGrafcetIo(inputs, outputs);

    // State and options.
    if (running)
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.4f, 1.0f), "Running  t = %.1f s", double(doc.grafcet().time()));
    else
        ImGui::TextDisabled("Stopped: initial values");
    ImGui::SameLine();
    ImGui::Checkbox("Apply together", &m_states.batch_inputs);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Switches, selectors and values change at the next Apply (F7),\n"
                          "all in the same instant. Push buttons act at once.");
    if (m_states.batch_inputs && running)
    {
        ImGui::SameLine();
        ImGui::BeginDisabled(m_states.pending_inputs.empty());
        bool const apply = ImGui::SmallButton("Apply (F7)");
        ImGui::EndDisabled();
        if (apply || (ImGui::IsKeyPressed(ImGuiKey_F7, false) && !m_states.pending_inputs.empty()))
        {
            for (auto const& it : m_states.pending_inputs)
                doc.setInput(it.first, it.second);
            m_states.pending_inputs.clear();
        }
    }
    if (!running || !m_states.batch_inputs)
        m_states.pending_inputs.clear();
    ImGui::SameLine();
    ImGui::Checkbox("Edit", &m_states.edit_io);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Choose the kind (push button, selector, sensor ...),\n"
                          "the color and the description of each input and output");
    ImGui::SameLine();
    ImGui::Checkbox("Fit", &m_states.fit_panel);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Make the buttons smaller to see all of them in the window");
    ImGui::SameLine();
    if (ImGui::SmallButton("?"))
        openHelp(3);
    ImGui::Separator();

    if (inputs.empty())
    {
        ImGui::TextDisabled("No input: write receptivities on the transitions.");
        ImGui::End();
        return;
    }

    if (m_states.edit_io)
    {
        if (editIo(activeDocument(), inputs, outputs))
            net().modified = true;
        ImGui::End();
        return;
    }

    // Widgets side by side, in sections (that can be folded), going to the
    // next line when full. Drawn smaller when they do not fit.
    if (!m_states.fit_panel)
        s_scale = 1.0f;
    float const widgets_top = ImGui::GetCursorPosY();
    ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * s_scale);
    float const right = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
    bool first = true;
    auto const place = [&](float width) {
        if (!first && (ImGui::GetItemRectMax().x + ImGui::GetStyle().ItemSpacing.x + width <= right))
            ImGui::SameLine();
        first = false;
    };
    auto const section = [&](char const* title, std::initializer_list<GrafcetInput::Kind> kinds) {
        bool any = false;
        for (GrafcetInput* input : inputs)
            any = any || (std::find(kinds.begin(), kinds.end(), input->kind) != kinds.end());
        if (!any)
            return;
        if (!ImGui::CollapsingHeader(title, ImGuiTreeNodeFlags_DefaultOpen))
            return;
        first = true;
        for (GrafcetInput* input : inputs)
        {
            if (std::find(kinds.begin(), kinds.end(), input->kind) == kinds.end())
                continue;
            place((input->kind == GrafcetInput::Kind::Value) ? VALUE_WIDTH : CELL_WIDTH);
            inputWidget(*input, running);
        }
    };

    section("Safety", { GrafcetInput::Kind::Emergency });
    section("Buttons", { GrafcetInput::Kind::Button, GrafcetInput::Kind::Pulse });
    section("Switches", { GrafcetInput::Kind::Switch });

    // Selectors: one widget per group.
    std::map<std::string, std::vector<GrafcetInput*>> groups;
    std::vector<std::string> order;
    for (GrafcetInput* input : inputs)
    {
        if (input->kind != GrafcetInput::Kind::Selector)
            continue;
        if (groups.find(input->group) == groups.end())
            order.push_back(input->group);
        groups[input->group].push_back(input);
    }
    if (!order.empty() && ImGui::CollapsingHeader("Selectors", ImGuiTreeNodeFlags_DefaultOpen))
    {
        first = true;
        for (auto const& group : order)
        {
            place(std::max(CELL_WIDTH * 1.5f, 50.0f * float(groups[group].size())));
            selectorWidget(groups[group], running);
        }
    }

    section("Sensors", { GrafcetInput::Kind::Value });
    section("Clocks", { GrafcetInput::Kind::Clock });
    ImGui::PopFont();
    if (fitScale(s_scale, m_states.fit_panel, ImGui::GetCursorPosY() - widgets_top,
             ImGui::GetWindowHeight() - widgets_top - ImGui::GetStyle().WindowPadding.y))
        m_busy_frames = std::max(m_busy_frames, 2);
    ImGui::End();
}

//------------------------------------------------------------------------------
//! \brief Draw an actuator in a cell: what it is, on or off.
//------------------------------------------------------------------------------
static void actuator(ImDrawList* draw, GrafcetOutput const& output, ImVec2 const& p, bool const on,
                     Action::LedColor const color, float const dt)
{
    ImVec2 const c(p.x + CELL_WIDTH * 0.5f, p.y + S(32.0f));
    ImU32 const metal = IM_COL32(120, 124, 132, 255);
    ImU32 const dark = IM_COL32(55, 57, 62, 255);
    ImU32 const line = IM_COL32(25, 25, 28, 255);

    switch (output.kind)
    {
    case GrafcetOutput::Kind::Light:
    {
        draw->AddCircleFilled(c, S(22.0f), dark, 40);
        if (on)
            draw->AddCircleFilled(c, S(24.0f), ledColor(color, 0.25f), 40);
        draw->AddCircleFilled(c, S(16.0f), on ? ledColor(color) : shade(ledColor(color), 0.3f), 40);
        if (on)
            draw->AddCircleFilled(c - ImVec2(S(5.0f), S(5.0f)), S(5.0f), IM_COL32(255, 255, 255, 120), 16);
        break;
    }

    case GrafcetOutput::Kind::Motor:
    {
        // Housing with cooling fins, a fan turning while on.
        float& angle = s_fan_angle[output.name];
        if (on)
            angle += dt * S(9.0f);
        draw->AddRectFilled(c + ImVec2(-S(20.0f), S(16.0f)), c + ImVec2(S(20.0f), S(24.0f)), dark, 2.0f);
        draw->AddCircleFilled(c, S(22.0f), metal, 40);
        draw->AddCircle(c, S(22.0f), line, 40, 1.5f);
        draw->AddCircleFilled(c, S(17.0f), on ? IM_COL32(70, 140, 220, 255) : dark, 40);
        for (int k = 0; k < 4; ++k)
        {
            float const a = angle + float(k) * S(3.14159265f) * 0.5f;
            ImVec2 const d(std::cos(a), std::sin(a));
            ImVec2 const n(-d.y, d.x);
            ImVec2 const blade[] = { c + n * 2.0f, c + d * S(15.0f) + n * S(5.0f), c + d * S(15.0f) - n * 1.0f, c - n * 2.0f };
            draw->AddConvexPolyFilled(blade, 4, IM_COL32(230, 232, 236, 255));
        }
        draw->AddCircleFilled(c, S(3.5f), line, 16);
        draw->AddText(c + ImVec2(S(14.0f), -S(24.0f)), on ? IM_COL32(70, 140, 220, 255) : line, "M");
        break;
    }

    case GrafcetOutput::Kind::Heater:
    {
        // Resistor in zigzag, glowing while on, with heat waves.
        float const t = float(ImGui::GetTime());
        ImU32 const hot = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.35f + 0.1f * std::sin(t * S(6.0f)), 0.1f, 1.0f));
        draw->AddRectFilled(c - ImVec2(S(24.0f), S(14.0f)), c + ImVec2(S(24.0f), S(14.0f)), dark, S(4.0f));
        std::vector<ImVec2> zigzag;
        for (int k = 0; k <= 8; ++k)
            zigzag.push_back(c + ImVec2(-S(20.0f) + S(5.0f) * float(k), (k % 2) ? -S(8.0f) : S(8.0f)));
        if (on)
            draw->AddPolyline(zigzag.data(), int(zigzag.size()), IM_COL32(255, 120, 40, 90),
                              ImDrawFlags_None, S(7.0f));
        draw->AddPolyline(zigzag.data(), int(zigzag.size()), on ? hot : metal, ImDrawFlags_None, 2.5f);
        if (on)
        {
            for (int k = -1; k <= 1; ++k)
            {
                std::vector<ImVec2> wave;
                for (int j = 0; j <= 6; ++j)
                {
                    float const y = -S(18.0f) - float(j) * 2.0f;
                    wave.push_back(c + ImVec2(float(k) * S(12.0f) + 2.5f * std::sin(t * S(5.0f) + float(j)), y));
                }
                draw->AddPolyline(wave.data(), int(wave.size()), IM_COL32(255, 140, 60, 180),
                                  ImDrawFlags_None, 1.5f);
            }
        }
        break;
    }

    case GrafcetOutput::Kind::Valve:
    {
        // Cylinder: the rod goes out while on.
        float const rod = on ? S(18.0f) : 2.0f;
        draw->AddRectFilled(c + ImVec2(-S(26.0f), -S(9.0f)), c + ImVec2(S(4.0f), S(9.0f)), metal, S(3.0f));
        draw->AddRect(c + ImVec2(-S(26.0f), -S(9.0f)), c + ImVec2(S(4.0f), S(9.0f)), line, S(3.0f), 0, 1.5f);
        draw->AddRectFilled(c + ImVec2(-S(14.0f) + rod - 2.0f, -S(7.0f)), c + ImVec2(-S(10.0f) + rod - 2.0f, S(7.0f)), dark);
        draw->AddRectFilled(c + ImVec2(-S(10.0f) + rod - 2.0f, -2.5f), c + ImVec2(S(8.0f) + rod, 2.5f),
                            on ? IM_COL32(70, 140, 220, 255) : IM_COL32(200, 202, 208, 255));
        draw->AddRectFilled(c + ImVec2(S(8.0f) + rod, -S(6.0f)), c + ImVec2(S(11.0f) + rod, S(6.0f)), dark);
        break;
    }

    case GrafcetOutput::Kind::Horn:
    {
        ImVec2 const body[] = { c + ImVec2(-S(18.0f), -S(6.0f)), c + ImVec2(-S(10.0f), -S(6.0f)), c + ImVec2(2.0f, -S(16.0f)),
                                c + ImVec2(2.0f, S(16.0f)), c + ImVec2(-S(10.0f), S(6.0f)), c + ImVec2(-S(18.0f), S(6.0f)) };
        draw->AddConvexPolyFilled(body, 6, on ? IM_COL32(240, 160, 40, 255) : metal);
        draw->AddPolyline(body, 6, line, ImDrawFlags_Closed, 1.5f);
        if (on)
        {
            float const t = float(ImGui::GetTime());
            for (int k = 1; k <= 3; ++k)
            {
                float const r = S(6.0f) * float(k) + S(4.0f) * std::fmod(t * 2.0f, 1.0f);
                draw->PathArcTo(c + ImVec2(S(4.0f), 0.0f), r, -0.8f, 0.8f, 12);
                draw->PathStroke(IM_COL32(240, 160, 40, 200), ImDrawFlags_None, 2.0f);
            }
        }
        break;
    }
    }
}

//------------------------------------------------------------------------------
void Editor::outputsPanel()
{
    if (!ImGui::Begin("Outputs"))
    {
        ImGui::End();
        return;
    }

    Document& doc = activeDocument();
    bool const running = doc.grafcet().isRunning();
    std::vector<GrafcetInput*> inputs;
    std::vector<GrafcetOutput*> outputs;
    collectGrafcetIo(inputs, outputs);
    if (outputs.empty())
    {
        ImGui::TextDisabled("No output: give a name to the actions of the steps.");
        ImGui::End();
        return;
    }

    // Color of the pilot lights: the one of their first action.
    std::map<std::string, Action::LedColor> colors;
    for (auto const& entry : doc.nets())
    {
        for (auto const& place : entry->net.places())
            for (auto const& action : place.actions)
                colors.emplace(action.name, action.color);
    }

    // Drawn at their own scale, to fit their window.
    float const panel_scale = s_scale;
    s_scale = s_output_scale;
    float const widgets_top = ImGui::GetCursorPosY();
    ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * s_scale);
    float const right = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
    float const dt = ImGui::GetIO().DeltaTime;
    bool first = true;
    for (GrafcetOutput const* output : outputs)
    {
        if (!first && (ImGui::GetItemRectMax().x + ImGui::GetStyle().ItemSpacing.x + CELL_WIDTH <= right))
            ImGui::SameLine();
        first = false;

        bool const on = running && doc.grafcet().output(output->name);
        ImGui::PushID(output->name.c_str());
        ImVec2 const p = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("##cell", ImVec2(CELL_WIDTH, CELL_HEIGHT));
        ImDrawList* draw = ImGui::GetWindowDrawList();
        actuator(draw, *output, p, on, colors[output->name], dt);
        float const label_y = p.y + CELL_HEIGHT - ImGui::GetTextLineHeight() * 2.0f - 2.0f;
        cellLabel(draw, ImVec2(p.x, label_y), CELL_WIDTH, output->name.c_str(),
                  ImGui::GetColorU32(ImGuiCol_Text));
        cellLabel(draw, ImVec2(p.x, label_y + ImGui::GetTextLineHeight()), CELL_WIDTH, on ? "ON" : "off",
                  on ? IM_COL32(60, 190, 90, 255) : ImGui::GetColorU32(ImGuiCol_TextDisabled));
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("%s: %s", output->name.c_str(), on ? "on" : "off");
            if (!output->description.empty())
                ImGui::TextUnformatted(output->description.c_str());
            ImGui::TextDisabled("%s", outputKindMeaning(output->kind));
            ImGui::EndTooltip();
        }
        ImGui::PopID();
    }
    ImGui::PopFont();
    if (fitScale(s_output_scale, m_states.fit_panel, ImGui::GetCursorPosY() - widgets_top,
             ImGui::GetWindowHeight() - widgets_top - ImGui::GetStyle().WindowPadding.y))
        m_busy_frames = std::max(m_busy_frames, 2);
    s_scale = panel_scale;
    if (running)
        m_busy_frames = std::max(m_busy_frames, 1);
    ImGui::End();
}

//------------------------------------------------------------------------------
void Editor::documentPanel()
{
    if (!ImGui::Begin("Document"))
    {
        ImGui::End();
        return;
    }

    // Read: the texts wrapped. Edit: the fields (no wrapping in them).
    static bool editing = false;
    Document& doc = activeDocument();
    bool const running = doc.isAnySimulationRunning();
    bool modified = false;
    float const lines = ImGui::GetTextLineHeight();

    ImGui::TextDisabled("%s - %zu net(s), %s", doc.title().c_str(), doc.netCount(),
                        to_str(net().type()).c_str());
    ImGui::SameLine();
    ImGui::BeginDisabled(running);
    ImGui::Checkbox("Edit", &editing);
    ImGui::EndDisabled();
    if (running)
        editing = false;

    ImGui::SeparatorText("What the document is about");
    if (editing)
    {
        modified |= ImGui::InputTextMultiline("##document", &doc.info().description,
                                              ImVec2(-1.0f, lines * 6.0f));
    }
    else if (doc.info().description.empty())
    {
        ImGui::TextDisabled("No description: check Edit to write one.");
    }
    else
    {
        ImGui::TextWrapped("%s", doc.info().description.c_str());
    }

    ImGui::SeparatorText("Nets");
    for (size_t i = 0u; i < doc.netCount(); ++i)
    {
        Net& n = doc.getNet(i).net;
        ImGui::PushID(int(i));
        bool const active = (i == doc.activeNetIndex());
        if (ImGui::RadioButton("##active", active))
            doc.setActiveNetIndex(i);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Show this net in the inspector");
        ImGui::SameLine();
        if (editing)
        {
            ImGui::SetNextItemWidth(-1.0f);
            modified |= ImGui::InputText("##name", &n.name);
            modified |= ImGui::InputTextMultiline("##description", &n.description,
                                                  ImVec2(-1.0f, lines * 3.5f));
        }
        else
        {
            ImGui::TextUnformatted(n.name.c_str());
            if (!n.description.empty())
            {
                ImGui::Indent();
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
                ImGui::TextWrapped("%s", n.description.c_str());
                ImGui::PopStyleColor();
                ImGui::Unindent();
            }
        }
        ImGui::PopID();
    }

    // Colors of the tokens of the active net (runtime): results of the
    // functions and colors of the user.
    if (allows(net().type(), Feature::Colors))
    {
        ImGui::SeparatorText("Colors of the tokens");
        ImGui::TextDisabled("Success, Failed, Canceled: results of the functions.");
        std::vector<std::string> const user = net().colors.user();
        if (user.empty())
            ImGui::TextDisabled("No color of yours.");
        for (auto const& name : user)
            ImGui::BulletText("%s", name.c_str());
        static std::string new_color;
        ImGui::BeginDisabled(running);
        ImGui::SetNextItemWidth(160.0f);
        bool const add = ImGui::InputTextWithHint("##newcolor", "new color", &new_color,
                                                  ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::SameLine();
        if ((ImGui::Button("Add") || add) && !new_color.empty())
        {
            net().colors.add(new_color);
            new_color.clear();
            modified = true;
        }
        ImGui::EndDisabled();
        ImGui::TextDisabled("Give them to the arcs: right click on an arc.");
    }

    // The inputs and outputs of a GRAFCET, with their description.
    if (net().type() == TypeOfNet::GRAFCET)
    {
        std::vector<GrafcetInput*> inputs;
        std::vector<GrafcetOutput*> outputs;
        collectGrafcetIo(inputs, outputs);
        ImGui::SeparatorText("Inputs and outputs");
        ImGuiTableFlags const table = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                      ImGuiTableFlags_SizingFixedFit;
        if (ImGui::BeginTable("##io", 3, table))
        {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Kind");
            ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();
            for (GrafcetInput const* input : inputs)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn(); ImGui::TextUnformatted(input->name.c_str());
                ImGui::TableNextColumn(); ImGui::TextDisabled("in: %s", inputKindToStr(input->kind));
                ImGui::TableNextColumn(); ImGui::TextWrapped("%s", input->description.c_str());
            }
            for (GrafcetOutput const* output : outputs)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn(); ImGui::TextUnformatted(output->name.c_str());
                ImGui::TableNextColumn(); ImGui::TextDisabled("out: %s", outputKindToStr(output->kind));
                ImGui::TableNextColumn(); ImGui::TextWrapped("%s", output->description.c_str());
            }
            ImGui::EndTable();
        }
        ImGui::TextDisabled("Edit them with the Edit box of the control panel.");
    }

    if (modified)
        net().modified = true;
    ImGui::End();
}

} // namespace tpne
