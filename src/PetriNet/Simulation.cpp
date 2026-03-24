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

#  include "PetriNet/Simulation.hpp"

#  include "PetriNet/PetriNet.hpp"
#  include "PetriNet/Receptivities.hpp"

#  include <algorithm>
#  include <cstdlib>
#  include <ctime>
#  include <iostream>
#  include <random>

namespace tpne {

//------------------------------------------------------------------------------
static const char* current_time()
{
    static char buffer[32];

    time_t t = ::time(nullptr);
    strftime(buffer, sizeof (buffer), "[%H:%M:%S] ", localtime(&t));
    return buffer;
}

//------------------------------------------------------------------------------
Simulation::Simulation(Net& net)
    : m_net(net)
{
    m_timed_tokens.reserve(128u);
}

//------------------------------------------------------------------------------
std::vector<Transition*> const& Simulation::shuffle_transitions(bool const reset)
{
    static std::random_device rd;
    static std::mt19937 g(rd());

    if (reset)
    {
        // Avoid useless copy at each iteration of the simulation. Do it
        // once at the begining of the simulation.
        m_shuffled_transitions.clear();
        m_shuffled_transitions.reserve(m_net.transitions().size());
        for (auto& trans: m_net.transitions()) {
            m_shuffled_transitions.push_back(&trans);
        }
    }
    std::shuffle(m_shuffled_transitions.begin(),
                 m_shuffled_transitions.end(), g);
    return m_shuffled_transitions;
}

//------------------------------------------------------------------------------
void Simulation::step(float const dt)
{
    switch (m_state)
    {
    case Simulation::State::Idle:
        stateStarting();
        break;
    case Simulation::State::Simulating:
        stateSimulating(dt);
        break;
    case Simulation::State::Halting:
        stateHalting();
        break;
    default:
        std::cerr << "Odd state in the state machine doing the "
                  << "animation of the Petri net." << std::endl;
        exit(1);
        break;
    }
}

//------------------------------------------------------------------------------
void Simulation::stateStarting()
{
    // The user has requested to start the simulation ?
    if (!running)
        return;

    // Dummy Petri net: nothing to simulate
    if (m_net.isEmpty())
    {
        onWarning.emit("Starting simulation request ignored because the net is empty");
        running = false;
        return ;
    }

    // Reset states of the simulator
    m_net.generateArcsInArcsOut();
    m_initial_tokens = m_net.tokens();
    m_net.storeInitialMarking();  // Store for forcing restore
    shuffle_transitions(true);
    m_timed_tokens.clear();
    for (auto& a: m_net.arcs())
    {
        a.count = 0u;
    }

    // Reset values on transitivities and sensors for GRAFCET
    m_net.resetReceptivies();
    if (!generateSensors())
    {
        running = false;
    }

    //
    std::cout << current_time() << "Simulation has started!" << std::endl;
    if (m_net.type() == TypeOfNet::PetriNet)
    {
        onInfo.emit(
            "Simulation has started!\n"
            "  Click on transitions for firing!\n"
            "  Press the key '+' on Places for adding tokens\n"
            "  Press the key '-' on Places for removing tokens");
    }
    else
    {
        onInfo.emit("Simulation has started!");
    }

    m_state = Simulation::State::Simulating;
}

//------------------------------------------------------------------------------
// Generate sensors from transition captions, preserving existing values
bool Simulation::generateSensors()
{
    // Check for GRAFCET if boolean expressions in transitivities have
    // not syntaxical errors.
    if (m_net.type() == TypeOfNet::GRAFCET)
    {
        // Save existing sensor values before regenerating
        std::map<std::string, int, std::less<>> saved_values;
        for (const auto& [key, value] : Sensors::instance().database())
        {
            saved_values[key] = value;
        }

        // Don't clear - the compile() function will add new sensors as needed
        // Sensors::instance().clear();
        m_receptivities.clear();

        bool all_ok = true;
        for (auto const& it: m_net.transitions())
        {
            std::string error = m_receptivities[it.id].compile(it.caption, m_net);
            if (!error.empty())
            {
                onWarning.emit(it.key + ": " + error);
                all_ok = false;
            }
        }

        // Restore saved values for sensors that existed before
        for (const auto& [key, value] : saved_values)
        {
            auto& db = Sensors::instance().database();
            if (db.find(key) != db.end())
            {
                db[key] = value;
            }
        }
        if (!all_ok)
            return false;
    }
    this->compiled = true;
    return true;
}

//------------------------------------------------------------------------------
bool Simulation::generateSensor(Transition const& transition)
{
    std::string error = m_receptivities[transition.id].compile(transition.caption, m_net);
    if (!error.empty())
    {
        onWarning.emit(error);
        this->compiled = false;
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------
void Simulation::stateHalting()
{
    onInfo.emit("Simulation has ended!");
    std::cout << current_time() << "Simulation has ended!"
                << std::endl << std::endl;

    // Restore consumed tokens from the simulation
    m_net.tokens(m_initial_tokens);
    m_net.resetReceptivies();
    m_receptivities.clear();
    m_timed_tokens.clear();
    Sensors::instance().clear();
    m_state = Simulation::State::Idle;
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
void Simulation::stateSimulating(float const dt)
{
    size_t consuming = 0u;

    // The user has requested to halt the simulation ?
    if (!running)
    {
        m_state = Simulation::State::Halting;
        return ;
    }

    // Skip simulation if net is frozen by a forcing command
    if (m_net.frozen)
    {
        // Still update actions so forcings can be applied/removed
        if (m_net.type() == TypeOfNet::GRAFCET)
        {
            updateActions(dt);
        }
        return;
    }

    // Interpret the code of receptivities.
    if (m_net.type() == TypeOfNet::GRAFCET)
    {
        // TODO if (Sensors::modified) {
        for (auto& t: m_net.transitions())
            t.receptivity = m_receptivities[t.id].evaluate();
        // }  Sensors::modified = false;
    }

    // Update transition delay timers (GRAFCET temporization)
    for (auto& t: m_net.transitions())
    {
        if (t.delay > 0.0f)
        {
            // Check if transition is enabled (all input places have tokens)
            if (t.isEnabled() && t.receptivity)
            {
                t.delayTimer += dt;
            }
            else
            {
                // Reset timer if transition is no longer enabled
                t.delayTimer = 0.0f;
            }
        }
    }

    // For each transition check if it is activated (all incoming Places
    // have at least one token to consume). If yes, we will consume
    // the maximum possible of tokens in a single step for the animation.
    // But in the aim to divide tokens the most kindly over the maximum
    // transitions possible we have to iterate and consume tokens one by one.

    // Collect arcs that will carry animated tokens (optimization: avoid
    // iterating over all arcs at the end)
    std::vector<Arc*> arcs_to_animate;
    arcs_to_animate.reserve(32u);

    do
    {
        // Randomize the order of fired transitions (shuffle once per outer iteration)
        auto& transitions = shuffle_transitions();

        consuming = 0u;
        for (auto* trans: transitions)
        {
            // Check delay timer for GRAFCET temporization
            if (trans->delay > 0.0f && trans->delayTimer < trans->delay)
                continue;  // Delay not yet elapsed

            // maxTokensToConsume() returns 0 if transition cannot fire,
            // so no need to call canFire() separately (optimization)
            const size_t tokens = trans->maxTokensToConsume();

            if (tokens > 0u)
            {
                assert(tokens <= Net::Settings::maxTokens);

                consuming = tokens;

                // Input transition (source)
                if (trans->isInput())
                {
                    consuming = 0u;
                    trans->receptivity = false;
                }
                else
                {
                    // Consume tokens from each input place
                    for (auto* a: trans->arcsIn)
                    {
                        size_t& tks = a->tokensIn();
                        assert(tks >= tokens);
                        tks = std::min(Net::Settings::maxTokens, tks - tokens);

                        // Invalidate transitions of previous places
                        if (m_net.type() == TypeOfNet::PetriNet)
                        {
                            Transition& tr = reinterpret_cast<Transition&>(a->to);
                            tr.receptivity = false;
                        }
                    }
                }

                // Count the number of tokens for the animation and collect arcs
                for (auto* a: trans->arcsOut)
                {
                    if (a->count == 0u)
                    {
                        arcs_to_animate.push_back(a);
                    }
                    a->count = std::min(Net::Settings::maxTokens, a->count + tokens);
                }
            }
        }
    } while (consuming != 0u);

    // Create animated tokens from the collected arcs (optimization: O(k) instead of O(n))
    for (auto* a: arcs_to_animate)
    {
        if (a->count > 0u)
        {
            std::cout << current_time()
                      << "Transition " << a->from.caption << " consumed "
                      << a->count << " token"
                      << (a->count == 1u ? "" : "s")
                      << std::endl;
            m_timed_tokens.emplace_back(*a, a->count, m_net.type());
            a->count = 0u;
        }
    }

    // Tokens Transition --> Places are transitioning.
    if (m_timed_tokens.size() > 0u)
    {
        size_t i = m_timed_tokens.size();
        while (i--)
        {
            TimedToken& an = m_timed_tokens[i];
            if (an.update(dt))
            {
                // Animated token reached its ddestination: Place
                std::cout << current_time()
                            << "Place " << an.arc->to.caption
                            << " got " << an.tokens << " token"
                            << (an.tokens == 1u ? "" : "s")
                            << std::endl;

                // Drop the number of tokens it was carrying.
                an.arc->tokensOut() += an.tokens;

                // Transition source. In Petri net we keep using the mouse to
                // fire source transition to generate a single token by mouse
                // click while in other mode the transition fires once the
                // animation ends.
                if (m_net.type() != TypeOfNet::PetriNet)
                {
                    Transition& t = reinterpret_cast<Transition&>(an.arc->from);
                    if (t.isInput())
                    {
                        t.receptivity = true;
                    }
                }

                // Remove it
                m_timed_tokens[i] = m_timed_tokens[m_timed_tokens.size() - 1u];
                m_timed_tokens.pop_back();
            }
        }
    }
    else if ((m_net.type() != TypeOfNet::PetriNet) && (m_net.type() != TypeOfNet::GRAFCET))
    {
        std::cout << current_time() << "The simulation cannot consume tokens."
                  << std::endl;
        running = false;
        m_state = Simulation::State::Halting;
    }

    // Update GRAFCET action states based on qualifiers
    if (m_net.type() == TypeOfNet::GRAFCET)
    {
        updateActions(dt);
    }
}

//------------------------------------------------------------------------------
void Simulation::resetStoredAction(std::string const& name)
{
    // Find and reset all stored actions with the given name
    for (auto& place : m_net.places())
    {
        for (auto& action : place.actions)
        {
            if (action.name == name &&
                (action.qualifier == Action::Qualifier::S ||
                 action.qualifier == Action::Qualifier::SD ||
                 action.qualifier == Action::Qualifier::DS ||
                 action.qualifier == Action::Qualifier::SL))
            {
                action.active = false;
                action.timer = 0.0f;
            }
        }
    }
}

//------------------------------------------------------------------------------
void Simulation::updateActions(float const dt)
{
    for (auto& place : m_net.places())
    {
        bool step_active = (place.tokens > 0);
        bool rising_edge = step_active && !place.wasActive;
        bool falling_edge = !step_active && place.wasActive;

        for (auto& action : place.actions)
        {
            switch (action.qualifier)
            {
            case Action::Qualifier::N:
                // Normal: active while step is active
                action.active = step_active;
                if (!step_active) action.timer = 0.0f;
                break;

            case Action::Qualifier::S:
                // Set (Stored): latched ON at step activation
                if (rising_edge)
                    action.active = true;
                break;

            case Action::Qualifier::R:
                // Reset: resets stored actions with the same name
                if (step_active)
                {
                    resetStoredAction(action.name);
                }
                action.active = false;
                break;

            case Action::Qualifier::D:
                // Delayed: active after delay while step active
                if (step_active)
                {
                    action.timer += dt;
                    action.active = (action.timer >= action.duration);
                }
                else
                {
                    action.timer = 0.0f;
                    action.active = false;
                }
                break;

            case Action::Qualifier::L:
                // Limited: active for limited time while step active
                if (step_active)
                {
                    if (action.timer < action.duration)
                    {
                        action.timer += dt;
                        action.active = true;
                    }
                    else
                    {
                        action.active = false;
                    }
                }
                else
                {
                    action.timer = 0.0f;
                    action.active = false;
                }
                break;

            case Action::Qualifier::SD:
                // Stored & Delayed: set after delay (stays on even if step deactivates)
                if (step_active)
                {
                    action.timer += dt;
                    if (action.timer >= action.duration)
                        action.active = true;
                }
                else if (falling_edge)
                {
                    action.timer = 0.0f;
                    // Keep active state (stored)
                }
                break;

            case Action::Qualifier::DS:
                // Delayed & Stored: delayed then latched
                if (step_active)
                {
                    action.timer += dt;
                    if (action.timer >= action.duration)
                        action.active = true;
                }
                else
                {
                    action.timer = 0.0f;
                    // Keep active state (stored) - only reset by R
                }
                break;

            case Action::Qualifier::SL:
                // Stored & Limited: set for limited time (stays on even if step deactivates)
                if (rising_edge)
                {
                    action.active = true;
                    action.timer = 0.0f;
                }
                if (action.active)
                {
                    action.timer += dt;
                    if (action.timer >= action.duration)
                        action.active = false;
                }
                break;

            case Action::Qualifier::P:
                // Pulse: single pulse at step activation (one cycle only)
                action.active = rising_edge;
                break;
            }
        }

        // Update wasActive for next cycle
        place.wasActive = step_active;
    }

    // Apply forcings from active actions
    applyForcings();
}

//------------------------------------------------------------------------------
void Simulation::applyForcings()
{
    // First pass: clear frozen state on all nets (will be re-applied if needed)
    for (auto& [name, net] : NetRegistry::instance().getRegistry())
    {
        if (net) net->frozen = false;
    }

    // Second pass: apply forcings from active actions
    for (auto const& place : m_net.places())
    {
        bool step_active = (place.tokens > 0);
        if (!step_active) continue;

        for (auto const& action : place.actions)
        {
            for (auto const& forcing : action.forcings)
            {
                Net* target = NetRegistry::instance().findNet(forcing.targetNet);
                if (target == nullptr)
                {
                    onWarning.emit("Forcing target '" + forcing.targetNet + "' not found");
                    continue;
                }

                switch (forcing.type)
                {
                case Forcing::Type::Init:
                    // Force to initial state: restore initial marking
                    if (target->hasInitialMarking())
                    {
                        target->restoreInitialMarking();
                    }
                    else
                    {
                        onWarning.emit("No initial marking stored for " + forcing.targetNet);
                    }
                    break;

                case Forcing::Type::Freeze:
                    // Freeze: prevent any evolution
                    target->frozen = true;
                    break;

                case Forcing::Type::Empty:
                    // Empty: deactivate all steps
                    for (auto& p : target->places())
                    {
                        p.tokens = 0;
                    }
                    onInfo.emit("Forcing " + forcing.targetNet + " to empty state");
                    break;

                case Forcing::Type::Steps:
                    // Steps: activate only specific steps
                    for (auto& p : target->places())
                    {
                        p.tokens = 0;
                    }
                    for (size_t step_id : forcing.steps)
                    {
                        Place* p = target->findPlace(step_id);
                        if (p)
                        {
                            p->tokens = 1;
                        }
                        else
                        {
                            onWarning.emit("Forcing step " + std::to_string(step_id) +
                                         " not found in " + forcing.targetNet);
                        }
                    }
                    break;
                }
            }
        }
    }
}

} // namespace tpne