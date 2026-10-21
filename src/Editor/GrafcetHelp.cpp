// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

// Help on GRAFCET for the users of the editor: what each action qualifier
// does (with a timing diagram), what to write in a receptivity, what the
// inputs and outputs of the control panel are.

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#  define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "Editor/GrafcetHelp.hpp"
#include "imgui/imgui.h"

#include <algorithm>
#include <utility>
#include <vector>

namespace tpne {

//------------------------------------------------------------------------------
char const* qualifierMeaning(Action::Qualifier const qualifier)
{
    switch (qualifier)
    {
    case Action::Qualifier::N:
        return "Normal: on while the step is active (and its condition is true).";
    case Action::Qualifier::S:
        return "Set: on when the step becomes active, and it stays on (stored) "
               "until a step resets it with R.";
    case Action::Qualifier::R:
        return "Reset: switches off the action of the same name stored by S, SD or DS.";
    case Action::Qualifier::D:
        return "Delayed: on d seconds after the step became active, while it stays active.";
    case Action::Qualifier::L:
        return "Limited: on during the first d seconds of the step only.";
    case Action::Qualifier::SD:
        return "Stored then delayed: on d seconds after the activation, even if the "
               "step was left meanwhile; stays on until reset (R).";
    case Action::Qualifier::DS:
        return "Delayed then stored: if the step is still active after d seconds, on "
               "and stored until reset (R).";
    case Action::Qualifier::SL:
        return "Stored and limited: on during d seconds from the activation, even if "
               "the step was left meanwhile.";
    case Action::Qualifier::P:
        return "Pulse on activation (P1): on during one cycle when the step becomes active.";
    case Action::Qualifier::P0:
        return "Pulse on deactivation: on during one cycle when the step becomes inactive.";
    }
    return "";
}

//! \brief One signal of a timing diagram: its name and when it is on, on a
//! time line from 0 to 10.
struct Wave
{
    char const* label;
    std::vector<std::pair<float, float>> on;
    bool output;
};

//! \brief Signals of the timing diagram of a qualifier, and the duration d
//! drawn under the output (d_from = d_to: none).
static std::vector<Wave> signalsOf(Action::Qualifier const qualifier, float& d_from, float& d_to)
{
    d_from = d_to = 0.0f;
    switch (qualifier)
    {
    case Action::Qualifier::N:
        return { { "X1", { { 2.0f, 7.0f } }, false }, { "A", { { 2.0f, 7.0f } }, true } };
    case Action::Qualifier::S:
        return { { "X1: S A", { { 2.0f, 4.0f } }, false }, { "X2: R A", { { 7.0f, 9.0f } }, false },
                 { "A", { { 2.0f, 7.0f } }, true } };
    case Action::Qualifier::R:
        return { { "X1: S A", { { 1.0f, 3.0f } }, false }, { "X2: R A", { { 6.0f, 8.0f } }, false },
                 { "A", { { 1.0f, 6.0f } }, true } };
    case Action::Qualifier::D:
        d_from = 2.0f; d_to = 4.0f;
        return { { "X1", { { 2.0f, 7.0f } }, false }, { "A", { { 4.0f, 7.0f } }, true } };
    case Action::Qualifier::L:
        d_from = 2.0f; d_to = 4.0f;
        return { { "X1", { { 2.0f, 7.0f } }, false }, { "A", { { 2.0f, 4.0f } }, true } };
    case Action::Qualifier::SD:
        d_from = 2.0f; d_to = 4.0f;
        return { { "X1: SD A", { { 2.0f, 3.0f } }, false }, { "X2: R A", { { 8.0f, 9.0f } }, false },
                 { "A", { { 4.0f, 8.0f } }, true } };
    case Action::Qualifier::DS:
        d_from = 2.0f; d_to = 4.0f;
        return { { "X1: DS A", { { 2.0f, 6.0f } }, false }, { "X2: R A", { { 8.0f, 9.0f } }, false },
                 { "A", { { 4.0f, 8.0f } }, true } };
    case Action::Qualifier::SL:
        d_from = 2.0f; d_to = 5.0f;
        return { { "X1", { { 2.0f, 3.0f } }, false }, { "A", { { 2.0f, 5.0f } }, true } };
    case Action::Qualifier::P:
        return { { "X1", { { 2.0f, 7.0f } }, false }, { "A", { { 2.0f, 2.25f } }, true } };
    case Action::Qualifier::P0:
        return { { "X1", { { 2.0f, 7.0f } }, false }, { "A", { { 7.0f, 7.25f } }, true } };
    }
    return {};
}

//------------------------------------------------------------------------------
void qualifierChronogram(Action::Qualifier const qualifier, float const width)
{
    float d_from, d_to;
    std::vector<Wave> const signals = signalsOf(qualifier, d_from, d_to);
    bool const timed = (d_to > d_from);

    float const row = ImGui::GetTextLineHeight() + 6.0f;
    float const label = 64.0f;
    float const height = row * float(signals.size()) + (timed ? row * 0.8f : 0.0f);
    ImVec2 const origin = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(width, height));
    ImDrawList* draw = ImGui::GetWindowDrawList();

    ImU32 const text = ImGui::GetColorU32(ImGuiCol_Text);
    ImU32 const dim = ImGui::GetColorU32(ImGuiCol_TextDisabled);
    ImU32 const grid = ImGui::GetColorU32(ImGuiCol_Separator, 0.5f);
    ImU32 const out = IM_COL32(60, 180, 90, 255);
    ImU32 const out_fill = IM_COL32(60, 180, 90, 60);
    ImU32 const step_fill = ImGui::GetColorU32(ImGuiCol_FrameBgActive, 0.6f);
    float const x0 = origin.x + label;
    float const span = std::max(40.0f, width - label - 6.0f);
    auto const at = [&](float t) { return x0 + span * t / 10.0f; };

    // Faint lines at each change, through all the signals.
    for (Wave const& signal : signals)
    {
        for (auto const& on : signal.on)
        {
            for (float t : { on.first, on.second })
                draw->AddLine(ImVec2(at(t), origin.y), ImVec2(at(t), origin.y + row * float(signals.size())),
                              grid);
        }
    }

    for (size_t i = 0u; i < signals.size(); ++i)
    {
        Wave const& signal = signals[i];
        float const top = origin.y + row * float(i) + 4.0f;
        float const bottom = origin.y + row * float(i + 1u) - 4.0f;
        draw->AddText(ImVec2(origin.x, top - 1.0f), signal.output ? out : text, signal.label);

        // Square wave: low, up at each start, down at each end.
        std::vector<ImVec2> points { ImVec2(at(0.0f), bottom) };
        for (auto const& on : signal.on)
        {
            float const end = std::max(on.second, on.first + 0.08f);
            draw->AddRectFilled(ImVec2(at(on.first), top), ImVec2(at(end), bottom),
                                signal.output ? out_fill : step_fill);
            points.push_back(ImVec2(at(on.first), bottom));
            points.push_back(ImVec2(at(on.first), top));
            points.push_back(ImVec2(at(end), top));
            points.push_back(ImVec2(at(end), bottom));
        }
        points.push_back(ImVec2(at(10.0f), bottom));
        draw->AddPolyline(points.data(), int(points.size()), signal.output ? out : text,
                          ImDrawFlags_None, signal.output ? 2.0f : 1.5f);
    }

    // The duration d of the timed qualifiers, under the output.
    if (timed)
    {
        float const y = origin.y + row * float(signals.size()) + row * 0.35f;
        float const a = at(d_from);
        float const b = at(d_to);
        draw->AddLine(ImVec2(a, y), ImVec2(b, y), dim, 1.0f);
        draw->AddLine(ImVec2(a, y - 4.0f), ImVec2(a, y + 4.0f), dim, 1.0f);
        draw->AddLine(ImVec2(b, y - 4.0f), ImVec2(b, y + 4.0f), dim, 1.0f);
        ImVec2 const size = ImGui::CalcTextSize("d");
        draw->AddText(ImVec2((a + b - size.x) * 0.5f, y - size.y + 1.0f), dim, "d");
    }
}

//------------------------------------------------------------------------------
void qualifiersHelp()
{
    ImGui::TextWrapped("An action is written in a box on the right of its step: its qualifier "
                       "(the letter), its name, and an optional condition (\"if Dr: Motor\": "
                       "the action runs only while Dr is true). The output of the same name "
                       "is on while one of its actions is on. In the diagrams, X1 is the step "
                       "of the action and A its output.");
    ImGui::Spacing();
    ImGuiTableFlags const flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                  ImGuiTableFlags_SizingStretchProp;
    if (!ImGui::BeginTable("##qualifiers", 3, flags))
        return;
    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 30.0f);
    ImGui::TableSetupColumn("Meaning", ImGuiTableColumnFlags_WidthStretch, 1.0f);
    ImGui::TableSetupColumn("Timing diagram", ImGuiTableColumnFlags_WidthFixed, 280.0f);
    ImGui::TableHeadersRow();
    for (Action::Qualifier q : { Action::Qualifier::N, Action::Qualifier::S, Action::Qualifier::R,
                                 Action::Qualifier::D, Action::Qualifier::L, Action::Qualifier::SD,
                                 Action::Qualifier::DS, Action::Qualifier::SL, Action::Qualifier::P,
                                 Action::Qualifier::P0 })
    {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(qualifierToStr(q));
        ImGui::TableNextColumn();
        ImGui::TextWrapped("%s", qualifierMeaning(q));
        ImGui::TableNextColumn();
        qualifierChronogram(q, 270.0f);
    }
    ImGui::EndTable();

    ImGui::Spacing();
    ImGui::SeparatorText("Forcing orders");
    ImGui::TextWrapped("A step can force another graph (hierarchy of a GEMMA): the order is "
                       "drawn in a double frame. While the step is active, the forced graph "
                       "takes the given situation and does not evolve.");
    if (ImGui::BeginTable("##forcings", 2, flags))
    {
        ImGui::TableSetupColumn("Order", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("Meaning");
        ImGui::TableHeadersRow();
        char const* rows[][2] = {
            { "Production{init}", "the initial steps of Production only" },
            { "Production{2,5}", "steps 2 and 5 of Production only" },
            { "Production{}", "no active step in Production" },
            { "Production{*}", "Production frozen in its current situation" },
        };
        for (auto const& r : rows)
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextUnformatted(r[0]);
            ImGui::TableNextColumn(); ImGui::TextUnformatted(r[1]);
        }
        ImGui::EndTable();
    }
}

//------------------------------------------------------------------------------
void receptivitiesHelp()
{
    ImGui::TextWrapped("A receptivity is the condition written on the right of a transition "
                       "(IEC 60848). The transition fires when all the steps above it are "
                       "active and its receptivity is true. Precedence: not, then and, then "
                       "or; parentheses are allowed. An empty receptivity is always true.");
    ImGui::Spacing();
    ImGuiTableFlags const flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                  ImGuiTableFlags_SizingStretchProp;
    if (!ImGui::BeginTable("##receptivities", 3, flags))
        return;
    ImGui::TableSetupColumn("Write", ImGuiTableColumnFlags_WidthFixed, 150.0f);
    ImGui::TableSetupColumn("Means");
    ImGui::TableSetupColumn("Also", ImGuiTableColumnFlags_WidthFixed, 150.0f);
    ImGui::TableHeadersRow();
    char const* rows[][3] = {
        { "a . b", "a and b", "a * b, a and b" },
        { "a + b", "a or b", "a | b, a or b" },
        { "/a", "not a", "!a, not a" },
        { "1   0", "always true, never true", "true, false" },
        { "X3", "step 3 of this graph is active", "" },
        { "Safety:X3", "step 3 of the graph Safety is active", "" },
        { "↑a", "a rises (true during one instant)", "^a, rising(a)" },
        { "↓a", "a falls (true during one instant)", "falling(a)" },
        { "3s/X2", "true 3 s after step 2 became active", "500ms/a, 2min/b" },
        { "2s/a/5s", "true 2 s after a rose, false 5 s after it fell", "units: ms s min h" },
        { "[temp >= 80]", "comparison of an integer input", "=  !=  <  <=  >  >=" },
        { "Dcy", "an input (button, sensor ...)", "sensor_2, holding(cube)" },
    };
    for (auto const& r : rows)
    {
        ImGui::TableNextRow();
        for (char const* cell : r)
        {
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(cell);
        }
    }
    ImGui::EndTable();
    ImGui::Spacing();
    ImGui::TextDisabled("Example:");
    ImGui::SameLine();
    ImGui::TextUnformatted("Control:X4 . ↑Dcy . /Au");
    ImGui::TextDisabled("Type the arrows with ^ (rising); a transition also has a delay field.");
}

//------------------------------------------------------------------------------
char const* inputKindMeaning(GrafcetInput::Kind const kind)
{
    switch (kind)
    {
    case GrafcetInput::Kind::Switch: return "Switch: a click turns it on or off, and it stays.";
    case GrafcetInput::Kind::Button:
        return "Push button: on while it is pressed (one cycle at least). Right click: "
               "keep it pressed.";
    case GrafcetInput::Kind::Pulse: return "Pulse: on during one cycle of the GRAFCET for each click.";
    case GrafcetInput::Kind::Emergency:
        return "Emergency stop: a click pushes it (on) and it stays pushed until a "
               "second click pulls it back.";
    case GrafcetInput::Kind::Selector:
        return "Selector: the inputs of the same group are its positions; one only "
               "is on (Auto / Manu).";
    case GrafcetInput::Kind::Value:
        return "Value of a sensor (temperature, level, counter), set with a slider, or "
               "computed from an output (\"driven by\": goes up while the output is "
               "on, down else). Read it with [name >= 80].";
    case GrafcetInput::Kind::Clock: return "Clock: on during half of its period, then off.";
    }
    return "";
}

//------------------------------------------------------------------------------
char const* outputKindMeaning(GrafcetOutput::Kind const kind)
{
    switch (kind)
    {
    case GrafcetOutput::Kind::Light: return "Pilot light of the color of its actions.";
    case GrafcetOutput::Kind::Motor: return "Motor: its fan turns while it is on.";
    case GrafcetOutput::Kind::Heater: return "Heater: its resistor glows while it is on.";
    case GrafcetOutput::Kind::Valve: return "Valve or cylinder: open (rod out) while it is on.";
    case GrafcetOutput::Kind::Horn: return "Horn or buzzer: sound waves while it is on.";
    }
    return "";
}

//------------------------------------------------------------------------------
void controlPanelHelp()
{
    ImGui::TextWrapped("The control panel shows the inputs of the GRAFCET as the buttons of a "
                       "machine, and the Outputs window its actuators. Inputs read by the "
                       "receptivities appear by themselves as switches: \"Edit\" on the "
                       "panel chooses their kind, their color and their description (saved "
                       "with the document). While simulating, a click acts at once; with "
                       "\"Apply together\", the changes wait for Apply (F7) so that several "
                       "inputs change in the same instant.");
    ImGui::Spacing();
    ImGuiTableFlags const flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                  ImGuiTableFlags_SizingStretchProp;
    if (ImGui::BeginTable("##inputs", 2, flags))
    {
        ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthFixed, 90.0f);
        ImGui::TableSetupColumn("Behavior");
        ImGui::TableHeadersRow();
        for (GrafcetInput::Kind kind : { GrafcetInput::Kind::Button, GrafcetInput::Kind::Switch,
                                         GrafcetInput::Kind::Pulse, GrafcetInput::Kind::Emergency,
                                         GrafcetInput::Kind::Selector, GrafcetInput::Kind::Value,
                                         GrafcetInput::Kind::Clock })
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextUnformatted(inputKindToStr(kind));
            ImGui::TableNextColumn(); ImGui::TextWrapped("%s", inputKindMeaning(kind));
        }
        ImGui::EndTable();
    }
    ImGui::Spacing();
    if (ImGui::BeginTable("##outputs", 2, flags))
    {
        ImGui::TableSetupColumn("Output", ImGuiTableColumnFlags_WidthFixed, 90.0f);
        ImGui::TableSetupColumn("Drawn as");
        ImGui::TableHeadersRow();
        for (GrafcetOutput::Kind kind : { GrafcetOutput::Kind::Light, GrafcetOutput::Kind::Motor,
                                          GrafcetOutput::Kind::Heater, GrafcetOutput::Kind::Valve,
                                          GrafcetOutput::Kind::Horn })
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextUnformatted(outputKindToStr(kind));
            ImGui::TableNextColumn(); ImGui::TextWrapped("%s", outputKindMeaning(kind));
        }
        ImGui::EndTable();
    }
    ImGui::Spacing();
    ImGui::TextDisabled("In the JSON file:");
    ImGui::TextUnformatted(
        "\"inputs\": [ { \"name\": \"Dcy\", \"kind\": \"button\", \"color\": \"green\",\n"
        "              \"description\": \"Start a cycle\" },\n"
        "            { \"name\": \"Auto\", \"kind\": \"selector\", \"group\": \"Mode\" },\n"
        "            { \"name\": \"Temperature\", \"kind\": \"value\", \"min\": 20, \"max\": 120,\n"
        "              \"unit\": \"°C\", \"driven_by\": \"Oven\", \"rise\": 5, \"fall\": 1 } ],\n"
        "\"outputs\": [ { \"name\": \"Oven\", \"kind\": \"heater\" } ]");
}

} // namespace tpne
