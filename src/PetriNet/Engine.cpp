// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "PetriNet/Engine.hpp"
#include "PetriNet/PetriNet.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace tpne {

//------------------------------------------------------------------------------
TimeSource realTimeSource()
{
    auto const origin = std::chrono::steady_clock::now();
    return [origin]() {
        std::chrono::duration<double> const elapsed = std::chrono::steady_clock::now() - origin;
        return elapsed.count();
    };
}

//------------------------------------------------------------------------------
Engine::Engine(Net const& net)
    : Engine(net, Options())
{}

//------------------------------------------------------------------------------
Engine::Engine(Net const& net, Options const& options)
    : m_net(net), m_options(options)
{}

//------------------------------------------------------------------------------
void Engine::reset()
{
    reset(m_net.tokens());
}

//------------------------------------------------------------------------------
void Engine::reset(Marking const& marking)
{
    size_t const transitions = m_net.transitions().size();
    m_seed = (m_options.seed != 0u) ? m_options.seed
                                    : ((uint64_t(std::random_device{}()) << 32) | std::random_device{}());
    m_random.seed(m_seed);
    m_time = 0.0;
    m_marking = marking;
    m_incoming.assign(m_marking.size(), 0u);
    m_flights.clear();
    m_flying.assign(transitions, 0u);
    m_armed.assign(transitions, !m_options.manual);
    m_ready_since.assign(transitions, NEVER);
    m_firing_dates.assign(transitions, {});
    m_zeno = false;

    // An isolated transition would fire for ever.
    if (!m_options.manual)
    {
        for (size_t t = 0u; t < transitions; ++t)
        {
            Transition const& transition = m_net.transitions()[t];
            if (m_net.arcsIn(transition).empty() && m_net.arcsOut(transition).empty())
                m_armed[t] = false;
        }
    }
    updateEnabling();
}

//------------------------------------------------------------------------------
void Engine::setTokens(size_t const place, size_t const tokens)
{
    m_marking[place] = tokens;
    updateEnabling();
}

//------------------------------------------------------------------------------
void Engine::arm(size_t const transition, bool const armed)
{
    m_armed[transition] = armed;
    updateEnabling();
}

//------------------------------------------------------------------------------
bool Engine::isReady(size_t const transition) const
{
    return m_armed[transition] && isEnabled(m_net, m_marking, transition, &m_incoming);
}

//------------------------------------------------------------------------------
bool Engine::canFire(size_t const transition) const
{
    if (!isReady(transition))
        return false;
    double const delay = double(m_net.transitions()[transition].delay);
    return (delay <= 0.0) || (m_time >= m_ready_since[transition] + delay);
}

//------------------------------------------------------------------------------
double Engine::enabledFor(size_t const transition) const
{
    return (m_ready_since[transition] == NEVER) ? 0.0 : (m_time - m_ready_since[transition]);
}

//------------------------------------------------------------------------------
void Engine::updateEnabling()
{
    for (size_t t = 0u; t < m_ready_since.size(); ++t)
    {
        if (!isReady(t))
            m_ready_since[t] = NEVER;
        else if (m_ready_since[t] == NEVER)
            m_ready_since[t] = m_time;
    }
}

//------------------------------------------------------------------------------
double Engine::nextEventDate() const
{
    double next = NEVER;
    for (auto const& flight: m_flights)
        next = std::min(next, flight.end);
    for (size_t t = 0u; t < m_ready_since.size(); ++t)
    {
        double const delay = double(m_net.transitions()[t].delay);
        if ((delay > 0.0) && (m_ready_since[t] != NEVER) && (m_ready_since[t] + delay > m_time))
            next = std::min(next, m_ready_since[t] + delay);
    }
    return next;
}

//------------------------------------------------------------------------------
bool Engine::isDeadlocked() const
{
    if (m_zeno)
        return true;
    if (!m_flights.empty() || (nextEventDate() != NEVER))
        return false;
    for (size_t t = 0u; t < m_armed.size(); ++t)
    {
        if (canFire(t))
            return false;
    }
    return true;
}

//------------------------------------------------------------------------------
size_t Engine::pick()
{
    std::vector<size_t> candidates;
    for (size_t t = 0u; t < m_armed.size(); ++t)
    {
        if (canFire(t))
            candidates.push_back(t);
    }
    if (candidates.empty())
        return size_t(-1);

    switch (m_options.policy)
    {
    case Policy::FirstDeclared:
        return candidates.front();
    case Policy::Priority:
        return *std::max_element(candidates.begin(), candidates.end(), [this](size_t a, size_t b) {
            // Highest priority, then lowest identifier.
            int const pa = m_net.transitions()[a].priority;
            int const pb = m_net.transitions()[b].priority;
            return (pa < pb) || ((pa == pb) && (a > b));
        });
    case Policy::Random:
    default:
        return candidates[std::uniform_int_distribution<size_t>(0u, candidates.size() - 1u)(m_random)];
    }
}

//------------------------------------------------------------------------------
void Engine::fire(size_t const transition)
{
    Transition const& t = m_net.transitions()[transition];
    consume(m_net, m_marking, transition);
    m_firing_dates[transition].push_back(m_time);

    // Tokens leave on the output arcs.
    for (size_t const arc: m_net.arcsOut(t).indices())
    {
        Arc const& a = m_net.arcs()[arc];
        double duration = m_options.untimed_duration;
        if (m_options.timed)
            duration = double(a.durationOrZero());
        m_flights.push_back({ arc, a.weight, m_time, m_time + duration });
        m_incoming[a.to.id] += a.weight;
        ++m_flying[transition];
    }

    // Manual mode: armed for one firing. A source transition waits for its
    // tokens. The delay starts again.
    if (m_options.manual || (m_net.arcsIn(t).empty() && (m_flying[transition] > 0u)))
        m_armed[transition] = false;
    m_ready_since[transition] = NEVER;

    if (onFire)
        onFire(transition, m_time, m_firing_dates[transition].size());
}

//------------------------------------------------------------------------------
void Engine::deliver()
{
    // In the order of their arrival, then of their departure.
    std::stable_sort(m_flights.begin(), m_flights.end(), [](Flight const& a, Flight const& b) {
        return a.end < b.end;
    });
    size_t arrived = 0u;
    while ((arrived < m_flights.size()) && (m_flights[arrived].end <= m_time))
        ++arrived;
    if (arrived == 0u)
        return;

    std::vector<Flight> const flights(m_flights.begin(), m_flights.begin() + std::ptrdiff_t(arrived));
    m_flights.erase(m_flights.begin(), m_flights.begin() + std::ptrdiff_t(arrived));
    for (auto const& flight: flights)
    {
        Arc const& arc = m_net.arcs()[flight.arc];
        size_t const capacity = m_net.places()[arc.to.id].capacity;
        size_t& tokens = m_marking[arc.to.id];
        tokens = (flight.tokens > capacity - std::min(capacity, tokens)) ? capacity : tokens + flight.tokens;
        m_incoming[arc.to.id] -= flight.tokens;

        // Automatic mode: a source transition fires again once its tokens
        // have arrived.
        size_t const transition = arc.from.id;
        if ((--m_flying[transition] == 0u) && !m_options.manual &&
            m_net.arcsIn(m_net.transitions()[transition]).empty())
        {
            m_armed[transition] = true;
        }
        if (onArrival)
            onArrival(flight);
    }
}

//------------------------------------------------------------------------------
void Engine::fireAll()
{
    if (m_zeno)
        return;

    deliver();
    updateEnabling();
    size_t firings = 0u;
    for (size_t t = pick(); t != size_t(-1); t = pick())
    {
        if (++firings > m_options.max_firings_per_date)
        {
            m_zeno = true;
            if (onWarning)
            {
                onWarning("Transitions keep firing at the date " + std::to_string(m_time) +
                          ": a cycle without duration. The simulation stops.");
            }
            return;
        }
        fire(t);
        deliver();
        updateEnabling();
    }
}

//------------------------------------------------------------------------------
void Engine::settle()
{
    fireAll();
}

//------------------------------------------------------------------------------
void Engine::advanceTo(double const date)
{
    if (date < m_time)
        return;

    fireAll();
    for (double next = nextEventDate(); !m_zeno && (next <= date); next = nextEventDate())
    {
        m_time = next;
        fireAll();
    }
    m_time = date;
    updateEnabling();
}

} // namespace tpne
