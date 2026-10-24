// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "PetriNet/Simulation.hpp"
#include "PetriNet/PetriNet.hpp"
#include "PetriNet/Grafcet.hpp"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <cstdio>
#include <iostream>

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
uint64_t Simulation::seed() const
{
    return (m_petri != nullptr) ? m_petri->seed() : 0u;
}

//------------------------------------------------------------------------------
std::vector<Engine::Flight> const& Simulation::flights() const
{
    static std::vector<Engine::Flight> const none;
    return (m_running && (m_petri != nullptr)) ? m_petri->flights() : none;
}

//------------------------------------------------------------------------------
std::vector<double> const& Simulation::firingDates(size_t transition_id) const
{
    static std::vector<double> const empty;
    if ((m_petri == nullptr) || (transition_id >= m_petri->firingDates().size()))
        return empty;
    return m_petri->firingDates()[transition_id];
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
    if ((m_petri == nullptr) || (transition_id >= m_net.transitions().size()))
        return 0.0f;
    return float(m_petri->enabledFor(transition_id));
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
    // Memorize initial marking for restoring after the simulation
    storeInitialMarking();
    m_sim_time = 0.0;

    // Petri nets: the user fires transitions by clicks, tokens move during a
    // short visual time. Timed nets: transitions fire by themselves, tokens
    // move during the duration of their arc.
    m_petri.reset();
    if (m_net.type() != TypeOfNet::GRAFCET)
    {
        Engine::Options options = m_options;
        options.manual = (m_net.type() == TypeOfNet::PetriNet);
        options.timed = (m_net.type() != TypeOfNet::PetriNet);
        options.untimed_duration = untimed_duration;
        m_petri = std::make_unique<Engine>(m_net, options);
        m_petri->onFire = [this](size_t const t, double const date, size_t const k) {
            trace(m_net.transitions()[t], date, k);
        };
        m_petri->onWarning = [this](std::string const& msg) { onWarning.emit(msg); };
        m_petri->reset(m_initial_tokens);
        m_shown = m_initial_tokens;
    }
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

    // GRAFCET: compile receptivities and start the engine (shared with the
    // other graphs of the document, it may already run).
    m_net.resetReceptivies();

    // Initialize simulation state
    initializeRuntimeState();

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

    // The seed replays the same choices between conflicting transitions.
    std::string const seed = ((m_petri != nullptr) && (m_options.policy == Engine::Policy::Random))
        ? " (seed " + std::to_string(m_petri->seed()) + ")" : "";
    if (m_net.type() == TypeOfNet::PetriNet)
    {
        onInfo.emit(
            "Simulation has started" + seed + "!\n"
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
        onInfo.emit(
            "Simulation has started" + seed + "!\n"
            "  Keys + and - on a place add or remove tokens\n"
            "  Space stops the simulation");
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

    m_state = State::Idle;
    onStopped.emit();
}

//------------------------------------------------------------------------------
size_t Simulation::totalFirings() const
{
    size_t n = 0u;
    if (m_petri != nullptr)
    {
        for (auto const& dates : m_petri->firingDates())
            n += dates.size();
    }
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

    m_sim_time += double(dt);

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

    readUserChanges();
    m_petri->advanceTo(m_sim_time);
    showMarking();

    // Timed nets stop when nothing can happen any more. Petri nets wait for
    // the clicks of the user.
    if ((m_net.type() != TypeOfNet::PetriNet) && m_petri->isDeadlocked())
    {
        std::cout << current_time() << "The simulation cannot consume tokens." << std::endl;
        m_running = false;
        m_state = State::Halting;
    }
}

//------------------------------------------------------------------------------
void Simulation::readUserChanges()
{
    // Tokens added or removed by the user (keys, remote control).
    auto const& places = m_net.places();
    for (size_t p = 0u; (p < places.size()) && (p < m_shown.size()); ++p)
    {
        if (places[p].tokens != m_shown[p])
            m_petri->setTokens(p, places[p].tokens);
    }

    // Petri nets: transitions armed by a click (the receptivity).
    if (m_net.type() == TypeOfNet::PetriNet)
    {
        auto const& transitions = m_net.transitions();
        for (size_t t = 0u; t < transitions.size(); ++t)
        {
            if (transitions[t].receptivity != m_petri->isArmed(t))
                m_petri->arm(t, transitions[t].receptivity);
        }
    }
}

//------------------------------------------------------------------------------
void Simulation::showMarking()
{
    m_shown = m_petri->marking();
    m_net.tokens(m_shown);
    auto& transitions = m_net.transitions();
    for (size_t t = 0u; t < transitions.size(); ++t)
        transitions[t].receptivity = m_petri->isArmed(t);
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
void Simulation::trace(Transition const& transition, double const date, size_t const k)
{
    std::cout << current_time() << "Transition " << transition.caption << " fired" << std::endl;
    if (onTrace.connectionCount() == 0u)
        return;

    // k counts the firings of the transition.
    float duration = 0.0f;
    for (Arc const& arc : m_net.arcsOut(transition))
    {
        duration = std::max(duration, arc.durationOrZero());
    }
    std::string const name = transition.caption.empty() ? transition.key : transition.caption;

    char text_date[64];
    std::snprintf(text_date, sizeof(text_date), "t = %.2f  ", date);
    std::string text = text_date;
    text += name + ((name != transition.key) ? " (" + transition.key + ")" : "") + " fires";
    text += " (k = " + std::to_string(k) + ")";
    if ((duration > 0.0f) && (m_net.type() != TypeOfNet::PetriNet))
    {
        std::snprintf(text_date, sizeof(text_date), ", until %.2f", date + double(duration));
        text += text_date;
    }
    onTrace.emit(text);
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
