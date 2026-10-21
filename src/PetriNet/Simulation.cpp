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
#include "PetriNet/Algorithms.hpp"
#include "PetriNet/Flowshop.hpp"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <random>

namespace tpne {

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
    : m_net(net), m_own_engine(std::make_unique<GrafcetEngine>())
{
    m_engine = m_own_engine.get();
    m_timed_tokens.reserve(128u);
}

//------------------------------------------------------------------------------
Simulation::~Simulation() = default;

//------------------------------------------------------------------------------
void Simulation::grafcet(GrafcetEngine* shared)
{
    m_engine = (shared != nullptr) ? shared : m_own_engine.get();
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
Simulation::ActionState const& Simulation::actionState(size_t place_id, size_t action_index) const
{
    return m_engine->actionState(m_net, place_id, action_index);
}

//------------------------------------------------------------------------------
Receptivity const* Simulation::receptivity(size_t transition_id) const
{
    return m_engine->receptivity(m_net, transition_id);
}

//------------------------------------------------------------------------------
float Simulation::transitionDelayTimer(size_t transition_id) const
{
    if (m_net.type() == TypeOfNet::GRAFCET)
        return m_engine->transitionTimer(m_net, transition_id);
    return transition_id < m_transition_delay_timers.size()
        ? m_transition_delay_timers[transition_id] : 0.0f;
}

//------------------------------------------------------------------------------
bool Simulation::isFrozen() const
{
    return m_engine->isFrozen(m_net);
}

//------------------------------------------------------------------------------
bool Simulation::isInitialStep(size_t place_id) const
{
    std::vector<size_t> const& initial = m_engine->isRunning()
        ? m_engine->initialMarking(m_net) : m_initial_tokens;
    return place_id < initial.size() && initial[place_id] > 0;
}

//------------------------------------------------------------------------------
bool Simulation::hasInitialMarking() const
{
    return !m_initial_tokens.empty() || !m_engine->initialMarking(m_net).empty();
}

//------------------------------------------------------------------------------
void Simulation::initializeRuntimeState()
{
    // Generate arcsIn and arcsOut for each transition
    m_net.generateArcsInArcsOut();

    // Memorize initial marking for restoring after the simulation
    storeInitialMarking();

    // Shuffle transitions for random firing order
    shuffleTransitions(true);

    // Clear animated tokens
    m_timed_tokens.clear();

    // Initialize transition delay timers
    m_transition_delay_timers.clear();
    m_transition_delay_timers.resize(m_net.transitions().size(), 0.0f);

    // Initialize arc token counts
    m_arc_token_counts.clear();
    m_arc_token_counts.resize(m_net.arcs().size(), 0u);

    m_sim_time = 0.0f;
    m_firing_dates.assign(m_net.transitions().size(), {});

    // Pre-compute the (max,+) eigenvalue (cycle time λ) once for TEGs.
    // Used by the real-time output plot to subtract the cyclic drift λ·k.
    m_eigenvalue = 0.0;
    if ((m_net.type() == TypeOfNet::TimedEventGraph) ||
        ((m_net.type() == TypeOfNet::TimedPetriNet) && isEventGraph(m_net)))
    {
        CriticalCycleResult const cc = findCriticalCycle(m_net);
        if (cc.success && !cc.durations.empty())
            m_eigenvalue = cc.durations.front();
    }
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
    initializeRuntimeState();

    // GRAFCET: compile receptivities and start the engine (shared with the
    // other graphs of the document, it may already run).
    m_net.resetReceptivies();
    bool first_graph = true;
    if (m_net.type() == TypeOfNet::GRAFCET)
    {
        first_graph = !m_engine->isRunning();
        if (m_engine == m_own_engine.get())
        {
            m_engine->graphs({ &m_net });
            m_engine->onWarning = [this](std::string const& msg) { onWarning.emit(msg); };
        }
        if (!m_engine->isRunning() && !m_engine->start())
        {
            for (auto const& error : m_engine->errors())
                onWarning.emit(error);
            m_has_receptivity_errors = true;
            m_running = false;
            return;
        }
        m_has_receptivity_errors = false;
    }

    std::cout << current_time() << "Simulation has started!" << std::endl;

    if ((m_net.type() == TypeOfNet::PetriNet) || (m_net.type() == TypeOfNet::TimedPetriNet))
    {
        onInfo.emit(
            "Simulation has started!\n"
            "  Left click on a transition to fire it\n"
            "  Keys + and - on a place add or remove tokens\n"
            "  Space stops the simulation");
    }
    else if (m_net.type() == TypeOfNet::GRAFCET)
    {
        // The graphs of a document share the engine: said once.
        if (first_graph)
            onInfo.emit("GRAFCET simulation has started: act on the inputs of the Control panel");
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
    if ((m_net.type() != TypeOfNet::GRAFCET) || m_engine->isRunning())
        onInfo.emit("Simulation has ended!");
    std::cout << current_time() << "Simulation has ended!" << std::endl << std::endl;

    if ((m_net.type() == TypeOfNet::GRAFCET) && m_engine->isRunning())
    {
        m_engine->stop();
    }
    restoreInitialMarking();
    m_net.resetReceptivies();
    m_timed_tokens.clear();

    m_state = State::Idle;
    onStopped.emit();
}

//------------------------------------------------------------------------------
size_t Simulation::totalFirings() const
{
    size_t n = 0u;
    for (auto const& dates : m_firing_dates)
        n += dates.size();
    return n;
}

//------------------------------------------------------------------------------
void Simulation::handleSimulatingState(float const dt)
{
    if (!m_running)
    {
        m_state = State::Halting;
        return;
    }

    m_sim_time += dt;

    // GRAFCET: the engine evolves the graphs (instantaneous firing, no token
    // animation). A shared engine is stepped once per cycle by its owner.
    if (m_net.type() == TypeOfNet::GRAFCET)
    {
        if (m_engine == m_own_engine.get())
        {
            m_engine->step(dt);
        }
        if (!m_engine->isRunning())
        {
            m_running = false;
        }
        return;
    }

    updateTransitionDelayTimers(dt);
    fireTransitions();
    animateTokens(dt);
}

//------------------------------------------------------------------------------
bool Simulation::validateReceptivities()
{
    if (m_net.type() != TypeOfNet::GRAFCET)
    {
        m_has_receptivity_errors = false;
        return true;
    }

    bool all_ok = true;
    for (auto const& trans : m_net.transitions())
    {
        all_ok &= Receptivity::check(trans.caption).empty();
    }
    for (auto const& place : m_net.places())
    {
        for (auto const& action : place.actions)
            all_ok &= Receptivity::check(action.condition).empty();
    }

    m_has_receptivity_errors = !all_ok;
    return all_ok;
}

//------------------------------------------------------------------------------
bool Simulation::hasReceptivityErrors() const
{
    return m_net.type() == TypeOfNet::GRAFCET && m_has_receptivity_errors;
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

                if (trans->id < m_firing_dates.size())
                {
                    for (size_t k = 0u; k < tokens; ++k)
                        m_firing_dates[trans->id].push_back(m_sim_time);
                    trace(*trans, tokens);
                }

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
void Simulation::trace(Transition const& transition, size_t const tokens)
{
    if (onTrace.connectionCount() == 0u)
        return;

    // Same words as the Gantt chart: an operation "<machine>_<piece>" of a
    // flowshop starts on its machine; k counts the firings of the transition.
    float duration = 0.0f;
    for (Arc const* arc : transition.arcsOut)
    {
        if (!std::isnan(arc->duration))
            duration = std::max(duration, arc->duration);
    }
    std::string const name = transition.caption.empty() ? transition.key : transition.caption;
    std::string machine, piece;
    operationOf(name, machine, piece);
    size_t const k = m_firing_dates[transition.id].size();

    char date[64];
    std::snprintf(date, sizeof(date), "t = %.2f  ", double(m_sim_time));
    std::string text = date;
    if (machine != piece)
        text += machine + " starts " + piece;
    else
        text += name + ((name != transition.key) ? " (" + transition.key + ")" : "") + " fires";
    text += " (k = " + std::to_string(k) + (tokens > 1u ? ", " + std::to_string(tokens) + " tokens" : "") + ")";
    if ((duration > 0.0f) && (m_net.type() != TypeOfNet::PetriNet))
    {
        std::snprintf(date, sizeof(date), ", until %.2f", double(m_sim_time + duration));
        text += date;
    }
    onTrace.emit(text);
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

} // namespace tpne
