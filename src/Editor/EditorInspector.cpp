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
#include "PetriNet/Receptivities.hpp"
#include "imgui/imgui.h"
#include <map>

namespace tpne {

//------------------------------------------------------------------------------
//! \brief Helper to display places/steps panel.
//------------------------------------------------------------------------------
static void drawPlacesPanel(Net& net, Simulation& simulation, bool& modified,
                            bool& show_place_captions)
{
    const auto readonly = simulation.running ?
        ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_None;

    ImGui::Begin(net.type() == TypeOfNet::GRAFCET ? "Steps" : "Places");

    // Caption toggle option
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::Checkbox(show_place_captions ?
                    "Show place identifiers" : "Show place captions",
                    &show_place_captions);
    ImGui::PopStyleVar();
    ImGui::Separator();

    // Place list with token controls
    for (auto& place : net.places())
    {
        ImGui::PushID(place.key.c_str());
        ImGui::AlignTextToFramePadding();
        ImGui::InputText(place.key.c_str(), &place.caption,
            readonly | ImGuiInputTextFlags_CallbackEdit,
            [](ImGuiInputTextCallbackData*) { return 0; });

        // Token increment/decrement buttons
        ImGui::SameLine();
        ImGui::PushButtonRepeat(true);
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
        ImGui::PopButtonRepeat();

        ImGui::SameLine();
        ImGui::Text("%zu", place.tokens);

        ImGui::PopID();
    }
    ImGui::End();
}

//------------------------------------------------------------------------------
//! \brief Helper to display transitions panel.
//------------------------------------------------------------------------------
static void drawTransitionsPanel(Net& net, Simulation& simulation, [[maybe_unused]] bool& modified,
                                 bool& compiled, bool& show_transition_captions)
{
    const auto readonly = simulation.running ?
        ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_None;

    ImGui::Begin("Transitions");

    // Caption toggle option
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::Checkbox(show_transition_captions
                    ? "Show transition identifiers"
                    : "Show transition captions",
                    &show_transition_captions);
    ImGui::PopStyleVar();
    ImGui::Separator();
    ImGui::Text("%s", (net.type() == TypeOfNet::GRAFCET) ? "Transitivities:" : "Captions:");

    // Compile transitivities for GRAFCET
    compiled |= simulation.compiled;
    if ((net.type() == TypeOfNet::GRAFCET) && (!compiled))
    {
        compiled = simulation.generateSensors();
    }

    // Transition list
    for (auto& t : net.transitions())
    {
        ImGui::InputText(t.key.c_str(), &t.caption,
            readonly | ImGuiInputTextFlags_CallbackEdit,
            [](ImGuiInputTextCallbackData*)
            {
                return 0;
            });

        // GRAFCET receptivity validation
        if ((net.type() == TypeOfNet::GRAFCET) && (!simulation.running))
        {
            Simulation::Receptivities const& receptivities = simulation.receptivities();
            auto it = receptivities.find(t.id);
            if (it == receptivities.end())
            {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f),
                    "Receptivity not compiled for %s", t.key.c_str());
            }
            else if (!it->second.isValid())
            {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", it->second.error().c_str());
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", "See help for the syntax");
            }
        }
    }
    ImGui::End();
}

//------------------------------------------------------------------------------
//! \brief Helper to display GRAFCET inputs panel.
//------------------------------------------------------------------------------
static void drawGrafcetInputsPanel(Simulation& simulation, bool& modified)
{
    static std::map<std::string, int> pending_values;
    static bool immediate_mode = false;

    ImGui::Begin("Inputs");

    if (simulation.running)
    {
        ImGui::Checkbox("Immediate mode", &immediate_mode);
        ImGui::SameLine();
        if (ImGui::Button("Apply (F7)") || ImGui::IsKeyPressed(ImGuiKey_F7))
        {
            for (auto& it : pending_values)
            {
                auto db_it = Sensors::instance().database().find(it.first);
                if (db_it != Sensors::instance().database().end())
                {
                    db_it->second = it.second;
                }
            }
        }
    }
    else
    {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Set initial values:");
    }

    ImGui::Separator();

    // Sensor buttons
    for (auto& it : Sensors::instance().database())
    {
        if (pending_values.find(it.first) == pending_values.end())
        {
            pending_values[it.first] = it.second;
        }

        int& pending = pending_values[it.first];
        int current = it.second;

        ImGui::PushID(it.first.c_str());

        bool is_on = simulation.running
            ? (immediate_mode ? (current != 0) : (pending != 0))
            : (current != 0);
        bool changed = simulation.running && (is_on != (current != 0));

        // Color styling
        if (changed)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(255, 200, 100, 255));
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 0, 255));
        }
        else if (is_on)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(100, 200, 100, 255));
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 0, 255));
        }

        if (ImGui::Button(is_on ? "ON " : "OFF", ImVec2(50, 25)))
        {
            if (simulation.running)
            {
                if (immediate_mode)
                {
                    it.second = it.second ? 0 : 1;
                    pending = it.second;
                }
                else
                {
                    pending = pending ? 0 : 1;
                }
            }
            else
            {
                it.second = it.second ? 0 : 1;
                pending = it.second;
                modified = true;
            }
        }

        if (changed || is_on)
        {
            ImGui::PopStyleColor(2);
        }

        ImGui::SameLine();
        ImGui::Text("%s", it.first.c_str());

        if (changed && !immediate_mode && simulation.running)
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "(pending)");
        }

        ImGui::PopID();
    }

    ImGui::End();
}

//------------------------------------------------------------------------------
//! \brief Helper to display GRAFCET outputs panel.
//------------------------------------------------------------------------------
static void drawGrafcetOutputsPanel(Net& net)
{
    ImGui::Begin("Outputs");
    ImGui::TextWrapped("Actuator states from active actions.");
    ImGui::Separator();

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Structure to hold actuator state and color
    struct ActuatorInfo {
        bool active = false;
        Action::LedColor color = Action::LedColor::Green;
    };

    // Collect actuator states from actions (using action.active computed by simulation)
    std::map<std::string, ActuatorInfo> actuator_states;
    for (const auto& place : net.places())
    {
        for (const auto& action : place.actions)
        {
            if (!action.name.empty())
            {
                // Use action.active which is computed by Simulation::updateActions()
                // This properly handles all qualifiers (N, S, R, D, L, SD, DS, SL, P)
                auto it = actuator_states.find(action.name);
                if (it == actuator_states.end())
                {
                    actuator_states[action.name] = {action.active, action.color};
                }
                else
                {
                    // OR the states (multiple actions can control the same actuator)
                    it->second.active = it->second.active || action.active;
                }
            }
        }
    }

    // Helper to convert LedColor to ImU32
    auto ledColorToImU32 = [](Action::LedColor color, bool active) -> ImU32 {
        if (!active) return IM_COL32(60, 60, 60, 255);
        switch (color) {
            case Action::LedColor::Green:  return IM_COL32(50, 255, 50, 255);
            case Action::LedColor::Red:    return IM_COL32(255, 50, 50, 255);
            case Action::LedColor::Orange: return IM_COL32(255, 165, 0, 255);
            case Action::LedColor::Yellow: return IM_COL32(255, 255, 50, 255);
            case Action::LedColor::Blue:   return IM_COL32(50, 150, 255, 255);
            case Action::LedColor::White:  return IM_COL32(255, 255, 255, 255);
        }
        return IM_COL32(50, 255, 50, 255);
    };

    auto ledColorToGlow = [](Action::LedColor color) -> ImU32 {
        switch (color) {
            case Action::LedColor::Green:  return IM_COL32(50, 255, 50, 60);
            case Action::LedColor::Red:    return IM_COL32(255, 50, 50, 60);
            case Action::LedColor::Orange: return IM_COL32(255, 165, 0, 60);
            case Action::LedColor::Yellow: return IM_COL32(255, 255, 50, 60);
            case Action::LedColor::Blue:   return IM_COL32(50, 150, 255, 60);
            case Action::LedColor::White:  return IM_COL32(255, 255, 255, 60);
        }
        return IM_COL32(50, 255, 50, 60);
    };

    auto ledColorToTextColor = [](Action::LedColor color) -> ImVec4 {
        switch (color) {
            case Action::LedColor::Green:  return ImVec4(0.4f, 1.0f, 0.4f, 1.0f);
            case Action::LedColor::Red:    return ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
            case Action::LedColor::Orange: return ImVec4(1.0f, 0.65f, 0.0f, 1.0f);
            case Action::LedColor::Yellow: return ImVec4(1.0f, 1.0f, 0.4f, 1.0f);
            case Action::LedColor::Blue:   return ImVec4(0.4f, 0.6f, 1.0f, 1.0f);
            case Action::LedColor::White:  return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        }
        return ImVec4(0.4f, 1.0f, 0.4f, 1.0f);
    };

    if (actuator_states.empty())
    {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No actions defined");
    }
    else
    {
        for (const auto& [name, info] : actuator_states)
        {
            ImVec2 led_pos = ImGui::GetCursorScreenPos();
            const float led_radius = 8.0f;
            ImVec2 led_center(led_pos.x + led_radius + 4.0f, led_pos.y + 10.0f);

            ImU32 led_color = ledColorToImU32(info.color, info.active);

            if (info.active)
            {
                draw_list->AddCircleFilled(led_center, led_radius + 3.0f, ledColorToGlow(info.color));
            }
            draw_list->AddCircleFilled(led_center, led_radius, led_color);
            draw_list->AddCircle(led_center, led_radius, IM_COL32(100, 100, 100, 255), 32, 1.5f);

            ImGui::Dummy(ImVec2(led_radius * 2 + 12.0f, 20.0f));
            ImGui::SameLine();

            if (info.active)
                ImGui::TextColored(ledColorToTextColor(info.color), "%s", name.c_str());
            else
                ImGui::Text("%s", name.c_str());
        }
    }

    ImGui::End();
}

//------------------------------------------------------------------------------
//! \brief Helper to display GRAFCET actions panel.
//------------------------------------------------------------------------------
static void drawGrafcetActionsPanel(Net& net, Simulation& simulation, bool& modified)
{
    ImGui::Begin("Actions");
    ImGui::TextWrapped("Actions for each step. Right-click step on canvas > Edit Actions.");
    ImGui::Separator();

    for (auto& place : net.places())
    {
        bool is_active = (place.tokens > 0);
        if (is_active)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(100, 255, 100, 255));
        }

        bool open = ImGui::TreeNode(place.key.c_str(), "%s: %s %s",
            place.key.c_str(), place.caption.c_str(),
            is_active ? "[ACTIVE]" : "");

        if (is_active)
        {
            ImGui::PopStyleColor();
        }

        if (open)
        {
            ImGui::PushID(place.key.c_str());

            // List actions
            for (size_t i = 0; i < place.actions.size(); ++i)
            {
                Action& action = place.actions[i];
                ImGui::PushID(static_cast<int>(i));

                // Qualifier badge
                const char* qual_str = qualifierToStr(action.qualifier);
                ImVec4 badge_color;
                switch (action.qualifier) {
                    case Action::Qualifier::N: badge_color = ImVec4(0.4f, 0.4f, 0.8f, 1.0f); break;
                    case Action::Qualifier::S: badge_color = ImVec4(0.2f, 0.7f, 0.2f, 1.0f); break;
                    case Action::Qualifier::R: badge_color = ImVec4(0.8f, 0.2f, 0.2f, 1.0f); break;
                    case Action::Qualifier::D:
                    case Action::Qualifier::L:
                    case Action::Qualifier::SD:
                    case Action::Qualifier::DS:
                    case Action::Qualifier::SL: badge_color = ImVec4(0.7f, 0.5f, 0.2f, 1.0f); break;
                    case Action::Qualifier::P: badge_color = ImVec4(0.6f, 0.2f, 0.6f, 1.0f); break;
                }

                ImGui::TextColored(badge_color, "[%s]", qual_str);
                ImGui::SameLine();
                ImGui::Text("%s", action.name.c_str());
                if (!action.script.empty())
                {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(%s)", action.script.c_str());
                }

                // Timer for timed qualifiers
                if (action.qualifier == Action::Qualifier::D ||
                    action.qualifier == Action::Qualifier::L ||
                    action.qualifier == Action::Qualifier::SD ||
                    action.qualifier == Action::Qualifier::DS ||
                    action.qualifier == Action::Qualifier::SL)
                {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "[%.1fs]", action.duration);
                }

                // Active state indicator
                if (simulation.running && action.active)
                {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), " ACTIVE");
                }

                ImGui::PopID();
            }

            // Add action button
            if (ImGui::SmallButton("+ Add Action"))
            {
                Action new_action;
                new_action.name = "Action" + std::to_string(place.actions.size() + 1);
                place.actions.push_back(new_action);
                modified = true;
            }

            // Edit selected action
            if (!place.actions.empty())
            {
                ImGui::Separator();
                ImGui::Text("Edit action:");

                static int selected_action = 0;
                if (selected_action >= static_cast<int>(place.actions.size()))
                    selected_action = 0;

                std::vector<const char*> action_names;
                for (auto& a : place.actions)
                    action_names.push_back(a.name.c_str());

                ImGui::Combo("##action_select", &selected_action,
                    action_names.data(), static_cast<int>(action_names.size()));

                if (selected_action < static_cast<int>(place.actions.size()))
                {
                    Action& action = place.actions[selected_action];

                    // Qualifier selector
                    const char* qualifiers[] = {
                        "N - Normal", "S - Set (Stored)", "R - Reset",
                        "D - Delayed", "L - Limited", "SD - Stored & Delayed",
                        "DS - Delayed & Stored", "SL - Stored & Limited", "P - Pulse"
                    };
                    int qual_idx = static_cast<int>(action.qualifier);
                    if (ImGui::Combo("Qualifier", &qual_idx, qualifiers, IM_ARRAYSIZE(qualifiers)))
                    {
                        action.qualifier = static_cast<Action::Qualifier>(qual_idx);
                        modified = true;
                    }

                    // LED Color selector (industrial standard colors)
                    const char* colors[] = {
                        "Green", "Red", "Orange", "Yellow", "Blue", "White"
                    };
                    int color_idx = static_cast<int>(action.color);
                    if (ImGui::Combo("LED Color", &color_idx, colors, IM_ARRAYSIZE(colors)))
                    {
                        action.color = static_cast<Action::LedColor>(color_idx);
                        modified = true;
                    }

                    // Name input
                    char name_buf[128];
                    strncpy(name_buf, action.name.c_str(), sizeof(name_buf) - 1);
                    if (ImGui::InputText("Name", name_buf, sizeof(name_buf)))
                    {
                        action.name = name_buf;
                        modified = true;
                    }

                    // Script/Description input (used as comment in generated code)
                    char script_buf[256];
                    strncpy(script_buf, action.script.c_str(), sizeof(script_buf) - 1);
                    if (ImGui::InputText("Description", script_buf, sizeof(script_buf)))
                    {
                        action.script = script_buf;
                        modified = true;
                    }
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("Description of the action (used as comment in generated code)");
                    }

                    // Duration for timed qualifiers
                    if (action.qualifier == Action::Qualifier::D ||
                        action.qualifier == Action::Qualifier::L ||
                        action.qualifier == Action::Qualifier::SD ||
                        action.qualifier == Action::Qualifier::DS ||
                        action.qualifier == Action::Qualifier::SL)
                    {
                        if (ImGui::InputFloat("Duration (s)", &action.duration, 0.1f, 1.0f))
                        {
                            modified = true;
                        }
                    }

                    // Delete button
                    if (ImGui::SmallButton("Delete Action"))
                    {
                        place.actions.erase(place.actions.begin() + selected_action);
                        if (selected_action > 0) selected_action--;
                        modified = true;
                    }
                }
            }

            ImGui::PopID();
            ImGui::TreePop();
        }
    }

    ImGui::End();
}

//------------------------------------------------------------------------------
//! \brief Helper to display arc durations panel.
//------------------------------------------------------------------------------
static void drawArcsPanel(Net& net, Simulation& simulation, bool& modified)
{
    const auto readonly = simulation.running ?
        ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_None;

    if (net.type() == TypeOfNet::TimedEventGraph)
    {
        ImGui::Begin("Arcs");
        ImGui::Text("%s", "Durations:");
        for (auto& arc : net.arcs())
        {
            if (arc.from.type == Node::Type::Transition)
            {
                std::string text(arc.from.key + " -> " + arc.to.arcsOut[0]->to.key);
                float prev_value = arc.duration;
                ImGui::InputFloat(text.c_str(), &arc.duration, 0.01f, 1.0f, "%.3f", readonly);
                modified = (prev_value != arc.duration);
            }
        }
        ImGui::End();
    }
    else if (net.type() == TypeOfNet::TimedPetriNet)
    {
        ImGui::Begin("Arcs");
        ImGui::Text("%s", "Durations:");
        for (auto& arc : net.arcs())
        {
            std::string text(arc.from.key + " -> " + arc.to.key);
            float prev_value = arc.duration;
            ImGui::InputFloat(text.c_str(), &arc.duration, 0.01f, 1.0f, "%.3f", readonly);
            modified = (prev_value != arc.duration);
        }
        ImGui::End();
    }
}

//------------------------------------------------------------------------------
void Editor::inspector()
{
    static bool modified = false;
    static bool compiled = false;

    // Places/Steps panel
    drawPlacesPanel(net(), simulation(), modified, m_states.show_place_captions);

    // Transitions panel
    drawTransitionsPanel(net(), simulation(), modified, compiled, m_states.show_transition_captions);

    // GRAFCET-specific panels
    if (net().type() == TypeOfNet::GRAFCET)
    {
        drawGrafcetInputsPanel(simulation(), modified);
        drawGrafcetOutputsPanel(net());
        drawGrafcetActionsPanel(net(), simulation(), modified);
    }

    // Arc durations panel (for timed nets)
    drawArcsPanel(net(), simulation(), modified);

    // Update net state
    simulation().compiled = compiled;
    net().modified |= modified;
    modified = false;
}

} // namespace tpne
