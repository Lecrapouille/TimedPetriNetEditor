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

#include "PetriNet/Simulation.hpp"
#include "PetriNet/PetriNet.hpp"
#include "PetriNet/Grafcet.hpp"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <random>

namespace tpne {

// Static member initialization
Simulation::ActionState Simulation::s_dummy_action_state;

//------------------------------------------------------------------------------
static const char* current_time()
{
    static char buffer[32];
    time_t t = ::time(nullptr);
    strftime(buffer, sizeof(buffer), "[%H:%M:%S] ", localtime(&t));
    return buffer;
}

//------------------------------------------------------------------------------
Simulation::Simulation(Net& net)
    : m_net(net)
{
    m_timed_tokens.reserve(128u);
}

//------------------------------------------------------------------------------
void Simulation::start()
{
    m_running = true;
}

//------------------------------------------------------------------------------
void Simulation::stop()
{
    m_running = false;
}

//------------------------------------------------------------------------------
void Simulation::restart()
{
    stop();
    start();
}

//------------------------------------------------------------------------------
Simulation::ActionState const& Simulation::actionState(size_t place_id, size_t action_index) const
{
    auto key = std::make_pair(place_id, action_index);
    auto it = m_action_states.find(key);
    if (it != m_action_states.end())
        return it->second;
    return s_dummy_action_state;
}

//------------------------------------------------------------------------------
void Simulation::initializeRuntimeState()
{
    // Initialize place activation states
    m_place_was_active.clear();
    m_place_was_active.resize(m_net.places().size(), false);

    // Initialize transition delay timers
    m_transition_delay_timers.clear();
    m_transition_delay_timers.resize(m_net.transitions().size(), 0.0f);

    // Initialize arc token counts
    m_arc_token_counts.clear();
    m_arc_token_counts.resize(m_net.arcs().size(), 0u);

    // Initialize action states
    m_action_states.clear();
    for (auto const& place : m_net.places())
    {
        for (size_t i = 0; i < place.actions.size(); ++i)
        {
            m_action_states[std::make_pair(place.id, i)] = ActionState{};
        }
    }

    m_frozen = false;
}

//------------------------------------------------------------------------------
std::vector<Transition*> const& Simulation::shuffleTransitions(bool const reset)
{
    static std::random_device rd;
    static std::mt19937 g(rd());

    if (reset)
    {
        m_shuffled_transitions.clear();
        m_shuffled_transitions.reserve(m_net.transitions().size());
        for (auto& trans : m_net.transitions())
        {
            m_shuffled_transitions.push_back(&trans);
        }
    }
    std::shuffle(m_shuffled_transitions.begin(), m_shuffled_transitions.end(), g);
    return m_shuffled_transitions;
}

//------------------------------------------------------------------------------
void Simulation::step(float const dt)
{
    switch (m_state)
    {
    case State::Idle:
        handleIdleState();
        break;
    case State::Simulating:
        handleSimulatingState(dt);
        break;
    case State::Halting:
        handleHaltingState();
        break;
    }
}

//------------------------------------------------------------------------------
void Simulation::handleIdleState()
{
    if (!m_running)
        return;

    if (m_net.isEmpty())
    {
        onWarning.emit("Starting simulation request ignored because the net is empty");
        m_running = false;
        return;
    }

    // Initialize simulation state
    m_net.generateArcsInArcsOut();
    storeInitialMarking();
    initializeRuntimeState();
    shuffleTransitions(true);
    m_timed_tokens.clear();

    // Reset receptivities and compile for GRAFCET
    m_net.resetReceptivies();
    if (!compileReceptivities())
    {
        m_running = false;
        return;
    }

    std::cout << current_time() << "Simulation has started!" << std::endl;

    if ((m_net.type() == TypeOfNet::PetriNet) || (m_net.type() == TypeOfNet::TimedPetriNet))
    {
        onInfo.emit(
            "Simulation has started!\n"
            "  Click on transitions for firing!\n"
            "  Press the key '+' on Places for adding tokens\n"
            "  Press the key '-' on Places for removing tokens");
    }
    else if (m_net.type() == TypeOfNet::GRAFCET)
    {
        onInfo.emit("Simulation has started!\nGo to the inputs tab and click on inputs for firing transitions!");
    }
    else
    {
        onInfo.emit("Simulation has started!");
    }

    m_state = State::Simulating;
    onStarted.emit();
}

//------------------------------------------------------------------------------
void Simulation::handleHaltingState()
{
    onInfo.emit("Simulation has ended!");
    std::cout << current_time() << "Simulation has ended!" << std::endl << std::endl;

    restoreInitialMarking();
    m_net.resetReceptivies();
    m_receptivities.clear();
    m_timed_tokens.clear();
    m_action_states.clear();
    Sensors::instance().clear();

    m_state = State::Idle;
    onStopped.emit();
}

//------------------------------------------------------------------------------
void Simulation::handleSimulatingState(float const dt)
{
    if (!m_running)
    {
        m_state = State::Halting;
        return;
    }

    // Skip simulation if net is frozen by a forcing command
    if (m_frozen)
    {
        if (m_net.type() == TypeOfNet::GRAFCET)
        {
            updateGrafcetActions(dt);
        }
        return;
    }

    evaluateReceptivities();
    updateTransitionDelayTimers(dt);
    fireTransitions();
    animateTokens(dt);

    if (m_net.type() == TypeOfNet::GRAFCET)
    {
        updateGrafcetActions(dt);
    }
}

//------------------------------------------------------------------------------
bool Simulation::compileReceptivities()
{
    if (m_net.type() != TypeOfNet::GRAFCET)
        return true;

    // Save existing sensor values
    std::map<std::string, int, std::less<>> saved_values;
    for (const auto& [key, value] : Sensors::instance().database())
    {
        saved_values[key] = value;
    }

    m_receptivities.clear();

    bool all_ok = true;
    for (auto const& trans : m_net.transitions())
    {
        std::string error = m_receptivities[trans.id].compile(trans.caption, m_net);
        if (!error.empty())
        {
            onWarning.emit(trans.key + ": " + error);
            all_ok = false;
        }
    }

    // Restore saved sensor values
    for (const auto& [key, value] : saved_values)
    {
        auto& db = Sensors::instance().database();
        if (db.find(key) != db.end())
        {
            db[key] = value;
        }
    }

    return all_ok;
}

//------------------------------------------------------------------------------
bool Simulation::compileReceptivity(Transition const& transition)
{
    std::string error = m_receptivities[transition.id].compile(transition.caption, m_net);
    if (!error.empty())
    {
        onWarning.emit(error);
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------
void Simulation::evaluateReceptivities()
{
    if (m_net.type() != TypeOfNet::GRAFCET)
        return;

    for (auto& t : m_net.transitions())
    {
        t.receptivity = m_receptivities[t.id].evaluate();
    }
}

//------------------------------------------------------------------------------
void Simulation::updateTransitionDelayTimers(float const dt)
{
    for (auto& t : m_net.transitions())
    {
        if (t.delay > 0.0f)
        {
            if (t.isEnabled() && t.receptivity)
            {
                m_transition_delay_timers[t.id] += dt;
            }
            else
            {
                m_transition_delay_timers[t.id] = 0.0f;
            }
        }
    }
}

//------------------------------------------------------------------------------
void Simulation::fireTransitions()
{
    size_t consuming = 0u;

    // Reset arc token counts
    std::fill(m_arc_token_counts.begin(), m_arc_token_counts.end(), 0u);

    // Collect arcs that will carry animated tokens
    std::vector<std::pair<Arc*, size_t>> arcs_to_animate; // arc, arc_index
    arcs_to_animate.reserve(32u);

    do
    {
        auto& transitions = shuffleTransitions();
        consuming = 0u;

        for (auto* trans : transitions)
        {
            // Check delay timer for GRAFCET temporization
            if (trans->delay > 0.0f && m_transition_delay_timers[trans->id] < trans->delay)
                continue;

            const size_t tokens = trans->maxTokensToConsume();

            if (tokens > 0u)
            {
                assert(tokens <= Net::Settings::maxTokens);
                consuming = tokens;

                if (trans->isInput())
                {
                    consuming = 0u;
                    trans->receptivity = false;
                }
                else
                {
                    for (auto* a : trans->arcsIn)
                    {
                        size_t& tks = a->tokensIn();
                        assert(tks >= tokens);
                        tks = std::min(Net::Settings::maxTokens, tks - tokens);

                        if (m_net.type() == TypeOfNet::PetriNet)
                        {
                            Transition& tr = reinterpret_cast<Transition&>(a->to);
                            tr.receptivity = false;
                        }
                    }
                }

                // Count tokens for animation using arc indices
                for (auto* a : trans->arcsOut)
                {
                    // Find arc index
                    size_t arc_idx = 0;
                    for (auto& arc : m_net.arcs())
                    {
                        if (&arc == a)
                            break;
                        ++arc_idx;
                    }

                    if (m_arc_token_counts[arc_idx] == 0u)
                    {
                        arcs_to_animate.push_back({a, arc_idx});
                    }
                    m_arc_token_counts[arc_idx] = std::min(
                        Net::Settings::maxTokens,
                        m_arc_token_counts[arc_idx] + tokens);
                }
            }
        }
    } while (consuming != 0u);

    // Create animated tokens
    for (auto& [arc, arc_idx] : arcs_to_animate)
    {
        size_t count = m_arc_token_counts[arc_idx];
        if (count > 0u)
        {
            std::cout << current_time()
                      << "Transition " << arc->from.caption << " consumed "
                      << count << " token" << (count == 1u ? "" : "s")
                      << std::endl;
            m_timed_tokens.emplace_back(*arc, count, m_net.type());
            m_arc_token_counts[arc_idx] = 0u;
        }
    }
}

//------------------------------------------------------------------------------
void Simulation::animateTokens(float const dt)
{
    if (m_timed_tokens.empty())
    {
        if ((m_net.type() != TypeOfNet::PetriNet) && (m_net.type() != TypeOfNet::GRAFCET))
        {
            std::cout << current_time() << "The simulation cannot consume tokens." << std::endl;
            m_running = false;
            m_state = State::Halting;
        }
        return;
    }

    size_t i = m_timed_tokens.size();
    while (i--)
    {
        TimedToken& token = m_timed_tokens[i];
        if (token.update(dt))
        {
            std::cout << current_time()
                      << "Place " << token.arc->to.caption
                      << " got " << token.tokens << " token"
                      << (token.tokens == 1u ? "" : "s")
                      << std::endl;

            token.arc->tokensOut() += token.tokens;

            if (m_net.type() != TypeOfNet::PetriNet)
            {
                Transition& t = reinterpret_cast<Transition&>(token.arc->from);
                if (t.isInput())
                {
                    t.receptivity = true;
                }
            }

            // Remove by swapping with last
            m_timed_tokens[i] = m_timed_tokens.back();
            m_timed_tokens.pop_back();
        }
    }
}

//------------------------------------------------------------------------------
void Simulation::storeInitialMarking()
{
    m_initial_tokens = m_net.tokens();
}

//------------------------------------------------------------------------------
void Simulation::restoreInitialMarking()
{
    if (!m_initial_tokens.empty())
    {
        m_net.tokens(m_initial_tokens);
    }
}

//------------------------------------------------------------------------------
void Simulation::resetStoredAction(std::string const& name)
{
    for (auto const& place : m_net.places())
    {
        for (size_t i = 0; i < place.actions.size(); ++i)
        {
            auto const& action = place.actions[i];
            if (action.name == name &&
                (action.qualifier == Action::Qualifier::S ||
                 action.qualifier == Action::Qualifier::SD ||
                 action.qualifier == Action::Qualifier::DS ||
                 action.qualifier == Action::Qualifier::SL))
            {
                auto key = std::make_pair(place.id, i);
                m_action_states[key].active = false;
                m_action_states[key].timer = 0.0f;
            }
        }
    }
}

//------------------------------------------------------------------------------
void Simulation::updateGrafcetActions(float const dt)
{
    for (auto const& place : m_net.places())
    {
        bool step_active = (place.tokens > 0);
        bool was_active = m_place_was_active[place.id];
        bool rising_edge = step_active && !was_active;
        bool falling_edge = !step_active && was_active;

        for (size_t i = 0; i < place.actions.size(); ++i)
        {
            auto const& action = place.actions[i];
            auto key = std::make_pair(place.id, i);
            ActionState& state = m_action_states[key];

            switch (action.qualifier)
            {
            case Action::Qualifier::N:
                state.active = step_active;
                if (!step_active) state.timer = 0.0f;
                break;

            case Action::Qualifier::S:
                if (rising_edge)
                    state.active = true;
                break;

            case Action::Qualifier::R:
                if (step_active)
                    resetStoredAction(action.name);
                state.active = false;
                break;

            case Action::Qualifier::D:
                if (step_active)
                {
                    state.timer += dt;
                    state.active = (state.timer >= action.duration);
                }
                else
                {
                    state.timer = 0.0f;
                    state.active = false;
                }
                break;

            case Action::Qualifier::L:
                if (step_active)
                {
                    if (state.timer < action.duration)
                    {
                        state.timer += dt;
                        state.active = true;
                    }
                    else
                    {
                        state.active = false;
                    }
                }
                else
                {
                    state.timer = 0.0f;
                    state.active = false;
                }
                break;

            case Action::Qualifier::SD:
                if (step_active)
                {
                    state.timer += dt;
                    if (state.timer >= action.duration)
                        state.active = true;
                }
                else if (falling_edge)
                {
                    state.timer = 0.0f;
                }
                break;

            case Action::Qualifier::DS:
                if (step_active)
                {
                    state.timer += dt;
                    if (state.timer >= action.duration)
                        state.active = true;
                }
                else
                {
                    state.timer = 0.0f;
                }
                break;

            case Action::Qualifier::SL:
                if (rising_edge)
                {
                    state.active = true;
                    state.timer = 0.0f;
                }
                if (state.active)
                {
                    state.timer += dt;
                    if (state.timer >= action.duration)
                        state.active = false;
                }
                break;

            case Action::Qualifier::P:
                state.active = rising_edge;
                break;
            }
        }

        m_place_was_active[place.id] = step_active;
    }

    applyGrafcetForcings();
}

//------------------------------------------------------------------------------
void Simulation::applyGrafcetForcings()
{
    // First pass: clear frozen state on all nets
    m_frozen = false;

    // Second pass: apply forcings from active actions
    for (auto const& place : m_net.places())
    {
        bool step_active = (place.tokens > 0);
        if (!step_active) continue;

        for (size_t i = 0; i < place.actions.size(); ++i)
        {
            auto const& action = place.actions[i];
            auto key = std::make_pair(place.id, i);
            ActionState const& state = m_action_states[key];

            if (!state.active) continue;

            for (auto const& forcing : action.forcings)
            {
                // Check if forcing targets this net
                if (forcing.targetNet == m_net.name)
                {
                    switch (forcing.type)
                    {
                    case Forcing::Type::Init:
                        restoreInitialMarking();
                        break;

                    case Forcing::Type::Freeze:
                        m_frozen = true;
                        break;

                    case Forcing::Type::Empty:
                        for (auto& p : m_net.places())
                            p.tokens = 0;
                        onInfo.emit("Forcing " + forcing.targetNet + " to empty state");
                        break;

                    case Forcing::Type::Steps:
                        for (auto& p : m_net.places())
                            p.tokens = 0;
                        for (size_t step_id : forcing.steps)
                        {
                            Place* p = m_net.findPlace(step_id);
                            if (p)
                                p->tokens = 1;
                            else
                                onWarning.emit("Forcing step " + std::to_string(step_id) +
                                             " not found in " + forcing.targetNet);
                        }
                        break;
                    }
                }
                else
                {
                    // Cross-net forcing via registry
                    Net* target = NetRegistry::instance().findNet(forcing.targetNet);
                    if (target == nullptr)
                    {
                        onWarning.emit("Forcing target '" + forcing.targetNet + "' not found");
                        continue;
                    }

                    switch (forcing.type)
                    {
                    case Forcing::Type::Init:
                        // Cannot restore initial marking of other nets from here
                        onWarning.emit("Cross-net Init forcing not supported");
                        break;

                    case Forcing::Type::Freeze:
                        // Cannot freeze other nets from here
                        onWarning.emit("Cross-net Freeze forcing not supported");
                        break;

                    case Forcing::Type::Empty:
                        for (auto& p : target->places())
                            p.tokens = 0;
                        onInfo.emit("Forcing " + forcing.targetNet + " to empty state");
                        break;

                    case Forcing::Type::Steps:
                        for (auto& p : target->places())
                            p.tokens = 0;
                        for (size_t step_id : forcing.steps)
                        {
                            Place* p = target->findPlace(step_id);
                            if (p)
                                p->tokens = 1;
                            else
                                onWarning.emit("Forcing step " + std::to_string(step_id) +
                                             " not found in " + forcing.targetNet);
                        }
                        break;
                    }
                }
            }
        }
    }
}

} // namespace tpne
