// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Editor/Editor.hpp"
#include "Editor/GrafcetHelp.hpp"
#include "PetriNet/Grafcet.hpp"
#include "imgui/imgui.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <map>

namespace tpne {

//------------------------------------------------------------------------------
//! \brief Small "(?)" text with a tooltip. Return true when clicked.
//------------------------------------------------------------------------------
static bool helpMarker(char const* tooltip)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", tooltip);
    return ImGui::IsItemClicked();
}

//------------------------------------------------------------------------------
//! \brief Helper to display places/steps panel.
//------------------------------------------------------------------------------
static void drawPlacesPanel(Net& net, Simulation& simulation, bool& modified,
                            bool& show_identifiers)
{
    const auto readonly = simulation.isRunning() ?
        ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_None;
    bool const grafcet = (net.type() == TypeOfNet::GRAFCET);

    ImGui::Begin(grafcet ? "Steps###Places" : "Places###Places");
    ImGui::Checkbox(grafcet ? "Show step identifiers" : "Show place identifiers", &show_identifiers);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip(grafcet ? "Draw the number of each step above its caption"
                                  : "Draw the identifier (P0, P1 ...) of each place next to its caption");
    }
    ImGui::Separator();
    ImGui::TextDisabled(grafcet ? "Captions and active steps:" : "Captions and tokens:");

    for (auto& place : net.places())
    {
        ImGui::PushID(place.key.c_str());
        ImGui::AlignTextToFramePadding();
        ImGui::SetNextItemWidth(std::max(80.0f, ImGui::GetContentRegionAvail().x - 120.0f));
        if (ImGui::InputText(place.key.c_str(), &place.caption, readonly))
            modified = true;

        // Token increment/decrement buttons
        ImGui::SameLine();
        ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
        if (ImGui::ArrowButton("##left", ImGuiDir_Left))
        {
            place.decrement();
            modified = true;
        }
        ImGui::SameLine();
        if (ImGui::ArrowButton("##right", ImGuiDir_Right))
        {
            place.increment();
            modified = true;
        }
        ImGui::PopItemFlag();

        ImGui::SameLine();
        ImGui::Text("%zu", place.tokens);

        // Capacity: 0 for unbounded.
        if (allows(net.type(), Feature::Capacities))
        {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(40.0f);
            bool const unbounded = (place.capacity == std::numeric_limits<size_t>::max());
            int capacity = unbounded ? 0 : int(std::min(place.capacity, size_t(1000000u)));
            if (ImGui::InputInt("##capacity", &capacity, 0, 0, readonly))
            {
                place.capacity = (capacity <= 0) ? std::numeric_limits<size_t>::max()
                                                 : std::max(size_t(capacity), place.tokens);
                modified = true;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Capacity: max number of tokens (0: unbounded)");
        }

        // GRAFCET: how long the step is active.
        if (grafcet && simulation.isRunning() && (place.tokens > 0u))
        {
            ImGui::SameLine();
            ImGui::TextDisabled("%.1f s", double(simulation.grafcet().stepTime(net, place.id)));
        }

        ImGui::PopID();
    }
    ImGui::End();
}

//------------------------------------------------------------------------------
//! \brief Helper to display transitions panel. For GRAFCET, open_help is set
//! when the user asks for the syntax of the receptivities.
//------------------------------------------------------------------------------
static void drawTransitionsPanel(Net& net, Simulation& simulation, bool& modified,
                                 bool& show_identifiers, bool& open_help)
{
    const auto readonly = simulation.isRunning() ?
        ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_None;
    bool const grafcet = (net.type() == TypeOfNet::GRAFCET);

    ImGui::Begin("Transitions");
    ImGui::Checkbox("Show transition identifiers", &show_identifiers);
    ImGui::Separator();
    if (grafcet)
    {
        ImGui::TextDisabled("Receptivities and delays:");
        ImGui::SameLine();
        if (ImGui::SmallButton("Syntax help"))
            open_help = true;
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("a . b (and), a + b (or), /a (not), ↑a (rising edge),\n"
                              "X3 (step 3 active), 3s/X2 (timer), [temp >= 80] ...\n"
                              "Click for the whole syntax.");
    }
    else
    {
        ImGui::TextDisabled("Captions:");
    }

    for (auto& t : net.transitions())
    {
        ImGui::PushID(static_cast<int>(t.id));

        ImGui::SetNextItemWidth(std::max(80.0f, ImGui::GetContentRegionAvail().x -
                                         (grafcet ? 110.0f : 90.0f)));
        if (ImGui::InputText(t.key.c_str(), &t.caption, readonly))
            modified = true;

        // Priority when transitions are in conflict (Simulation > Conflicts).
        if (!grafcet)
        {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(40.0f);
            if (ImGui::InputInt("##priority", &t.priority, 0, 0, readonly))
                modified = true;
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Priority: with the policy \"Highest priority\" (menu Simulation >\n"
                                  "Conflicts), the highest one fires first.");
        }

        // GRAFCET transition delay (temporization)
        if (grafcet)
        {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(60.0f);
            if (ImGui::InputFloat("##delay", &t.delay, 0.0f, 0.0f, "%.1fs"))
            {
                if (t.delay < 0.0f) t.delay = 0.0f;
                modified = true;
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Delay before the transition can fire (0 = immediate).\n"
                                  "Same as writing 3s/X1 in the receptivity.");
            }

            // Timers counting during the simulation: the time operators of
            // the receptivity (30s/X1) and the delay.
            if (simulation.isRunning())
            {
                std::vector<Receptivity::Timer> timers;
                if (Receptivity const* receptivity = simulation.receptivity(t.id))
                    timers = receptivity->timers();
                if (t.delay > 0.0f)
                {
                    float const elapsed = simulation.transitionDelayTimer(t.id);
                    timers.push_back({ std::min(elapsed, t.delay), t.delay, elapsed > 0.0f, elapsed >= t.delay });
                }
                for (auto const& timer : timers)
                {
                    if (timer.duration <= 0.0f)
                        continue;
                    char text[64];
                    std::snprintf(text, sizeof(text), "%.1f / %.4g s", double(timer.elapsed), double(timer.duration));
                    ImGui::ProgressBar(timer.running ? timer.elapsed / timer.duration : (timer.output ? 1.0f : 0.0f),
                                       ImVec2(-1.0f, 0.0f), timer.running ? text : (timer.output ? "elapsed" : "waiting"));
                }
            }

            // GRAFCET receptivity validation
            if (!simulation.isRunning())
            {
                std::string const error = Receptivity::check(t.caption);
                if (!error.empty())
                {
                    ImGui::TextColored(ImVec4(0.92f, 0.22f, 0.22f, 1.0f), "%s", error.c_str());
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Syntax"))
                        open_help = true;
                }
            }
        }

        ImGui::PopID();
    }
    ImGui::End();
}

//------------------------------------------------------------------------------
//! \brief Color of the qualifier badge.
//------------------------------------------------------------------------------
static ImVec4 qualifierColor(Action::Qualifier const qualifier)
{
    switch (qualifier)
    {
    case Action::Qualifier::N: return ImVec4(0.45f, 0.55f, 0.95f, 1.0f);
    case Action::Qualifier::S: return ImVec4(0.25f, 0.70f, 0.30f, 1.0f);
    case Action::Qualifier::R: return ImVec4(0.90f, 0.30f, 0.30f, 1.0f);
    case Action::Qualifier::D:
    case Action::Qualifier::L:
    case Action::Qualifier::SD:
    case Action::Qualifier::DS:
    case Action::Qualifier::SL: return ImVec4(0.85f, 0.60f, 0.20f, 1.0f);
    case Action::Qualifier::P:
    case Action::Qualifier::P0: return ImVec4(0.70f, 0.35f, 0.80f, 1.0f);
    }
    return ImVec4(0.45f, 0.55f, 0.95f, 1.0f);
}

//------------------------------------------------------------------------------
//! \brief True for the qualifiers using Action::duration.
//------------------------------------------------------------------------------
static bool isTimed(Action::Qualifier const qualifier)
{
    return (qualifier == Action::Qualifier::D) || (qualifier == Action::Qualifier::L) ||
           (qualifier == Action::Qualifier::SD) || (qualifier == Action::Qualifier::DS) ||
           (qualifier == Action::Qualifier::SL);
}

//------------------------------------------------------------------------------
//! \brief Edit one action of a step: qualifier (with its timing diagram),
//! name, condition, duration, forcing orders.
//------------------------------------------------------------------------------
static void editAction(Place& place, Action& action, bool& modified, bool& open_help)
{
    // Qualifier: the list says what each one does, the diagram shows it.
    static Action::Qualifier const QUALIFIERS[] = {
        Action::Qualifier::N, Action::Qualifier::S, Action::Qualifier::R, Action::Qualifier::D,
        Action::Qualifier::L, Action::Qualifier::SD, Action::Qualifier::DS, Action::Qualifier::SL,
        Action::Qualifier::P, Action::Qualifier::P0 };
    static char const* NAMES[] = {
        "N - Normal", "S - Set (stored)", "R - Reset", "D - Delayed", "L - Limited",
        "SD - Stored then delayed", "DS - Delayed then stored", "SL - Stored and limited",
        "P1 - Pulse on activation", "P0 - Pulse on deactivation" };
    int current = 0;
    for (int i = 0; i < IM_ARRAYSIZE(QUALIFIERS); ++i)
        if (QUALIFIERS[i] == action.qualifier)
            current = i;
    ImGui::SetNextItemWidth(220.0f);
    if (ImGui::BeginCombo("Qualifier", NAMES[current]))
    {
        for (int i = 0; i < IM_ARRAYSIZE(QUALIFIERS); ++i)
        {
            if (ImGui::Selectable(NAMES[i], i == current))
            {
                action.qualifier = QUALIFIERS[i];
                modified = true;
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(300.0f);
                ImGui::TextUnformatted(qualifierMeaning(QUALIFIERS[i]));
                ImGui::PopTextWrapPos();
                qualifierChronogram(QUALIFIERS[i], 280.0f);
                ImGui::EndTooltip();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    if (helpMarker("Click for the help of all the qualifiers"))
        open_help = true;
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + std::max(200.0f, ImGui::GetContentRegionAvail().x));
    ImGui::TextDisabled("%s", qualifierMeaning(action.qualifier));
    ImGui::PopTextWrapPos();
    qualifierChronogram(action.qualifier, std::min(320.0f, ImGui::GetContentRegionAvail().x));

    if (isTimed(action.qualifier))
    {
        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::InputFloat("Duration d (s)", &action.duration, 0.5f, 1.0f, "%.1f"))
        {
            action.duration = std::max(0.0f, action.duration);
            modified = true;
        }
    }

    if (ImGui::InputText("Name", &action.name))
        modified = true;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Name of the output (actuator) set by the action: \"Motor\", \"KM1\" ...\n"
                          "Empty for an action that only forces other graphs.");

    if (ImGui::InputTextWithHint("Condition", "always", &action.condition))
        modified = true;
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Conditional action, drawn \"if Dr: Motor\": the action runs only\n"
                          "while this condition is true (same syntax as receptivities).");
    }
    std::string const condition_error = Receptivity::check(action.condition);
    if (!condition_error.empty())
        ImGui::TextColored(ImVec4(0.92f, 0.22f, 0.22f, 1.0f), "%s", condition_error.c_str());

    // LED Color of the pilot light (industrial standard colors)
    const char* colors[] = { "Green", "Red", "Orange", "Yellow", "Blue", "White" };
    int color_idx = static_cast<int>(action.color);
    ImGui::SetNextItemWidth(120.0f);
    if (ImGui::Combo("Light color", &color_idx, colors, IM_ARRAYSIZE(colors)))
    {
        action.color = static_cast<Action::LedColor>(color_idx);
        modified = true;
    }

    if (ImGui::InputText("Comment", &action.script))
        modified = true;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("What the action does (comment in the generated code)");

    // Forcing orders (hierarchical GRAFCET control)
    ImGui::SeparatorText("Forcing orders");
    for (size_t fi = 0; fi < action.forcings.size(); ++fi)
    {
        ImGui::PushID(static_cast<int>(fi));
        ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.2f, 1.0f), "%s", forcingLabel(action.forcings[fi]).c_str());
        ImGui::SameLine();
        if (ImGui::SmallButton("Remove"))
        {
            action.forcings.erase(action.forcings.begin() + static_cast<ptrdiff_t>(fi));
            modified = true;
            ImGui::PopID();
            break;
        }
        ImGui::PopID();
    }
    // Text of the new order, per step.
    static std::map<Place const*, std::string> buffers;
    std::string& buffer = buffers[&place];
    ImGui::SetNextItemWidth(160.0f);
    bool const enter = ImGui::InputTextWithHint("##new_forcing", "Production{init}", &buffer,
                                                ImGuiInputTextFlags_EnterReturnsTrue);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Forcing order while the step is active:\n"
                          "  Graph{init}  its initial steps\n"
                          "  Graph{2,5}   steps 2 and 5 only\n"
                          "  Graph{}      no active step\n"
                          "  Graph{*}     frozen");
    }
    ImGui::SameLine();
    if ((ImGui::SmallButton("Add order") || enter) && !buffer.empty())
    {
        action.forcings.push_back(parseForcing(buffer));
        buffer.clear();
        modified = true;
    }
}

//------------------------------------------------------------------------------
//! \brief Helper to display GRAFCET actions panel.
//------------------------------------------------------------------------------
static void drawGrafcetActionsPanel(Net& net, Simulation& simulation, bool& modified,
                                    bool& show_boxes, bool& open_help)
{
    ImGui::Begin("Actions");
    ImGui::Checkbox("Show the action boxes", &show_boxes);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Hidden: a small tag with the number of actions, for a compact net");
    ImGui::SameLine();
    if (ImGui::SmallButton("Qualifiers help"))
        open_help = true;
    ImGui::Separator();

    for (auto& place : net.places())
    {
        bool is_active = (place.tokens > 0);
        if (is_active)
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(100, 220, 100, 255));
        bool open = ImGui::TreeNode(place.key.c_str(), "X%zu  %s%s  (%zu)", place.id,
            place.caption.c_str(), is_active ? "  [active]" : "", place.actions.size());
        if (is_active)
            ImGui::PopStyleColor();
        if (!open)
            continue;

        ImGui::PushID(place.key.c_str());

        // List of the actions: a click selects the one to edit.
        int& selected = *ImGui::GetStateStorage()->GetIntRef(ImGui::GetID("selected_action"), 0);
        if (selected >= static_cast<int>(place.actions.size()))
            selected = 0;
        for (size_t i = 0; i < place.actions.size(); ++i)
        {
            Action& action = place.actions[i];
            ImGui::PushID(static_cast<int>(i));
            bool const running = simulation.isRunning() && simulation.actionState(place.id, i).active;
            std::string label;
            if (!action.condition.empty())
                label += "if " + action.condition + ": ";
            label += action.name;
            for (auto const& forcing : action.forcings)
                label += (label.empty() ? "" : "  ") + forcingLabel(forcing);
            ImGui::TextColored(qualifierColor(action.qualifier), "%-2s", action.forcings.empty() || !action.name.empty()
                               ? qualifierToStr(action.qualifier) : "F");
            ImGui::SameLine();
            if (ImGui::Selectable((label + "##action").c_str(), selected == int(i)))
                selected = int(i);
            if (running)
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.3f, 0.85f, 0.4f, 1.0f), "on");
            }
            // Timed qualifiers: how far the duration d is.
            if (simulation.isRunning() && isTimed(action.qualifier) && (action.duration > 0.0f))
            {
                Simulation::ActionState const& state = simulation.actionState(place.id, i);
                if (state.timer > 0.0f)
                {
                    char text[64];
                    std::snprintf(text, sizeof(text), "%.1f / %.4g s", double(std::min(state.timer, action.duration)),
                                  double(action.duration));
                    ImGui::ProgressBar(std::min(state.timer / action.duration, 1.0f), ImVec2(-1.0f, 0.0f), text);
                }
            }
            ImGui::PopID();
        }

        if (ImGui::SmallButton("+ Add action"))
        {
            Action new_action;
            new_action.name = "Action" + std::to_string(place.actions.size() + 1);
            place.actions.push_back(new_action);
            selected = int(place.actions.size()) - 1;
            modified = true;
        }
        if (!place.actions.empty())
        {
            ImGui::SameLine();
            if (ImGui::SmallButton("Delete this action"))
            {
                place.actions.erase(place.actions.begin() + selected);
                if (selected > 0)
                    selected--;
                modified = true;
            }
        }

        if (selected < static_cast<int>(place.actions.size()))
        {
            ImGui::Separator();
            editAction(place, place.actions[size_t(selected)], modified, open_help);
        }

        ImGui::PopID();
        ImGui::TreePop();
    }

    ImGui::End();
}

//------------------------------------------------------------------------------
//! \brief Arcs of Petri nets: kind (from a place), weight and duration (from a
//! transition, timed nets).
//------------------------------------------------------------------------------
static void drawArcsTable(Net& net, Simulation& simulation, bool& modified)
{
    bool const timed = (net.type() == TypeOfNet::TimedPetriNet);
    ImGui::Begin("Arcs");
    ImGui::BeginDisabled(simulation.isRunning());
    ImGuiTableFlags const flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit;
    if (ImGui::BeginTable("ArcsTable", timed ? 4 : 3, flags))
    {
        ImGui::TableSetupColumn("Arc");
        ImGui::TableSetupColumn("Kind");
        ImGui::TableSetupColumn("Weight");
        if (timed)
            ImGui::TableSetupColumn("Duration");
        ImGui::TableHeadersRow();
        static char const* KINDS[] = { "normal", "inhibitor", "reset" };
        for (size_t i = 0u; i < net.arcs().size(); ++i)
        {
            Arc& arc = net.arcs()[i];
            ImGui::PushID(static_cast<int>(i));
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%s -> %s", arc.from.key().c_str(), arc.to.key().c_str());
            ImGui::TableNextColumn();
            if (arc.from.type == Node::Type::Place)
            {
                int kind = static_cast<int>(arc.kind);
                ImGui::SetNextItemWidth(90.0f);
                if (ImGui::Combo("##kind", &kind, KINDS, IM_ARRAYSIZE(KINDS)))
                {
                    arc.kind = static_cast<ArcKind>(kind);
                    modified = true;
                }
            }
            ImGui::TableNextColumn();
            int weight = int(std::min(arc.weight, size_t(1000000u)));
            ImGui::SetNextItemWidth(70.0f);
            if (ImGui::InputInt("##weight", &weight))
            {
                arc.weight = size_t(std::max(1, weight));
                modified = true;
            }
            if (timed)
            {
                ImGui::TableNextColumn();
                if (arc.from.type == Node::Type::Transition)
                {
                    float duration = arc.durationOrZero();
                    ImGui::SetNextItemWidth(90.0f);
                    if (ImGui::InputFloat("##duration", &duration, 0.0f, 0.0f, "%.3f"))
                    {
                        arc.duration = std::max(0.0f, duration);
                        modified = true;
                    }
                }
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::EndDisabled();
    ImGui::End();
}

//------------------------------------------------------------------------------
//! \brief Helper to display arc durations panel. Only the arcs going out of a
//! transition have a duration (the time of the tokens on their way).
//------------------------------------------------------------------------------
static void drawArcsPanel(Net& net, Simulation& simulation, bool& modified)
{
    const auto readonly = simulation.isRunning() ?
        ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_None;

    if ((net.type() == TypeOfNet::PetriNet) || (net.type() == TypeOfNet::TimedPetriNet))
    {
        drawArcsTable(net, simulation, modified);
        return;
    }
    if (net.type() != TypeOfNet::TimedEventGraph)
        return;

    ImGui::Begin("Arcs");
    ImGui::TextDisabled("Durations (transition -> place):");
    for (auto& arc : net.arcs())
    {
        if (arc.from.type != Node::Type::Transition)
            continue;
        auto const next = net.arcsOut(net.node(arc.to));
        std::string const text = (net.type() == TypeOfNet::TimedEventGraph) && !next.empty()
            ? arc.from.key() + " -> " + next.front().to.key()
            : arc.from.key() + " -> " + arc.to.key();
        float duration = arc.durationOrZero();
        if (ImGui::InputFloat(text.c_str(), &duration, 0.01f, 1.0f, "%.3f", readonly))
        {
            arc.duration = std::max(0.0f, duration);
            modified = true;
        }
    }
    ImGui::End();
}

//------------------------------------------------------------------------------
void Editor::inspector()
{
    bool modified = false;
    bool receptivity_help = false;
    bool qualifier_help = false;

    drawPlacesPanel(net(), simulation(), modified, m_states.show_place_identifiers);
    drawTransitionsPanel(net(), simulation(), modified, m_states.show_transition_identifiers,
                         receptivity_help);
    if (net().type() == TypeOfNet::GRAFCET)
    {
        drawGrafcetActionsPanel(net(), simulation(), modified, m_states.show_action_boxes,
                                qualifier_help);
        controlPanel();
        outputsPanel();
        timingDiagram();
    }
    drawArcsPanel(net(), simulation(), modified);
    documentPanel();

    if (receptivity_help)
        openHelp(1);
    if (qualifier_help)
        openHelp(2);
    if (modified)
        net().modified = true;
}

} // namespace tpne
