// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Runtime/Runtime.hpp"
#include "PetriNet/Random.hpp"

#include <algorithm>
#include <chrono>
#include <limits>

namespace tpne {

static constexpr double NEVER = std::numeric_limits<double>::infinity();

//==============================================================================
// Instance
//==============================================================================

//------------------------------------------------------------------------------
size_t Context::tokens(std::string const& place, Color const color) const
{
    for (auto const& p : net.places())
    {
        if ((p.caption == place) || (p.key == place))
            return marking.count(p.id, color);
    }
    return 0u;
}

//------------------------------------------------------------------------------
Instance::Instance(std::string const& id, size_t const places, size_t const transitions)
    : m_id(id), m_marking(places), m_incoming(places, 0u), m_running(transitions, 0u), m_injected(transitions, 0u),
      m_ready_since(transitions, NEVER), m_enabled(transitions, 0)
{}

//------------------------------------------------------------------------------
void Instance::post(Reducer reducer)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_reducers.push_back(std::move(reducer));
    }
    m_condition.notify_all();
}

//------------------------------------------------------------------------------
bool Instance::drain()
{
    std::deque<Reducer> reducers;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        reducers.swap(m_reducers);
    }
    for (auto& reducer : reducers)
        reducer(*this);
    return !reducers.empty();
}

//==============================================================================
// Runtime
//==============================================================================

//------------------------------------------------------------------------------
Runtime::Runtime(Net const& net)
    : Runtime(net, std::make_shared<InlineExecutor>(), Options())
{}

//------------------------------------------------------------------------------
Runtime::Runtime(Net const& net, std::shared_ptr<Executor> executor)
    : Runtime(net, std::move(executor), Options())
{}

//------------------------------------------------------------------------------
Runtime::Runtime(Net const& net, std::shared_ptr<Executor> executor, Options const& options)
    : m_net(net), m_executor(std::move(executor)), m_options(options)
{
    size_t const transitions = net.transitions().size();
    m_callbacks.resize(transitions);
    m_guards.resize(transitions);
    m_sources.resize(transitions, false);
    m_neighbors.resize(net.places().size());
    for (size_t t = 0u; t < transitions; ++t)
    {
        Transition const& transition = net.transitions()[t];
        m_sources[t] = net.arcsIn(transition).empty();
        m_has_sources = m_has_sources || m_sources[t];
        if (m_sources[t])
            m_source_list.push_back(t);
        for (auto const& a : net.arcsIn(transition))
            m_neighbors[a.from.id].push_back(t);
        for (auto const& a : net.arcsOut(transition))
            m_neighbors[a.to.id].push_back(t);
    }
    for (auto& around : m_neighbors)
    {
        std::sort(around.begin(), around.end());
        around.erase(std::unique(around.begin(), around.end()), around.end());
    }
}

//------------------------------------------------------------------------------
std::vector<size_t> Runtime::find(std::string const& transition) const
{
    std::vector<size_t> found;
    for (auto const& t : m_net.transitions())
    {
        if ((t.caption == transition) || (t.key == transition))
            found.push_back(t.id);
    }
    return found;
}

//------------------------------------------------------------------------------
size_t Runtime::transitionOf(std::string const& name) const
{
    std::vector<size_t> const found = find(name);
    return found.empty() ? size_t(-1) : found.front();
}

//------------------------------------------------------------------------------
std::string const& Runtime::nameOf(size_t const transition) const
{
    Transition const& t = m_net.transitions()[transition];
    return t.caption.empty() ? t.key : t.caption;
}

//------------------------------------------------------------------------------
size_t Runtime::placeOf(std::string const& place) const
{
    for (auto const& p : m_net.places())
    {
        if ((p.caption == place) || (p.key == place))
            return p.id;
    }
    return size_t(-1);
}

//------------------------------------------------------------------------------
bool Runtime::on(std::string const& transition, Callback const& callback)
{
    std::vector<size_t> const found = find(transition);
    for (size_t const t : found)
        m_callbacks[t] = std::make_unique<Callback>(callback);
    return !found.empty();
}

//------------------------------------------------------------------------------
bool Runtime::guard(std::string const& transition, std::function<bool(Context const&)> guard)
{
    std::vector<size_t> const found = find(transition);
    for (size_t const t : found)
        m_guards[t] = guard;
    return !found.empty();
}

//------------------------------------------------------------------------------
std::vector<std::string> Runtime::unregisteredTransitions() const
{
    std::vector<std::string> names;
    for (size_t t = 0u; t < m_callbacks.size(); ++t)
    {
        if ((m_callbacks[t] == nullptr) && !m_sources[t])
            names.push_back(nameOf(t));
    }
    return names;
}

//------------------------------------------------------------------------------
std::shared_ptr<Instance> Runtime::instantiate(std::string const& id,
                                               std::vector<PlaceTokens> const& initial,
                                               std::vector<PlaceTokens> const& goal)
{
    std::shared_ptr<Instance> instance(new Instance(id, m_net.places().size(),
                                                    m_net.transitions().size()));
    // Memory for the first firings, not taken again at each one.
    if (m_options.log)
        instance->m_log.reserve(64u);
    instance->m_touched.reserve(16u);
    if (initial.empty())
        instance->m_marking = initialMarking(m_net);
    for (auto const& tokens : initial)
    {
        size_t const place = placeOf(tokens.place);
        if (place < m_net.places().size())
            instance->m_marking.add(place, tokens.color, tokens.tokens);
    }
    instance->m_goal = goal;
    uint64_t const seed = (m_options.seed != 0u) ? m_options.seed : uint64_t(std::random_device{}());
    instance->m_random.seed(seed);
    return instance;
}

//------------------------------------------------------------------------------
std::shared_ptr<Instance> Runtime::restore(std::string const& id, std::vector<LogEntry> const& journal,
                                           Recovery const recovery,
                                           std::vector<PlaceTokens> const& initial,
                                           std::vector<PlaceTokens> const& goal)
{
    std::shared_ptr<Instance> instance = instantiate(id, initial, goal);

    // Replay: the tokens taken by the firings, the tokens given by the ends
    // of the functions (their durations are over).
    std::vector<size_t> running(m_net.transitions().size(), 0u);
    for (auto const& entry : journal)
    {
        size_t const t = transitionOf(entry.transition);
        if (t == size_t(-1))
            continue;
        if (entry.event == LogEntry::Event::Scheduled)
        {
            tpne::consume(m_net, instance->m_marking, t);
            ++running[t];
        }
        else if (entry.event == LogEntry::Event::Completed)
        {
            tpne::produce(m_net, instance->m_marking, t, entry.result);
            running[t] -= std::min(running[t], size_t(1u));
        }
        if (m_options.log)
            instance->m_log.push_back(entry);
    }

    // The functions that were running.
    for (size_t t = 0u; t < running.size(); ++t)
    {
        for (size_t k = 0u; k < running[t]; ++k)
        {
            if (recovery == Recovery::Restart)
            {
                instance->post([this, t](Instance& i) { start(i.shared_from_this(), t); });
            }
            else
            {
                instance->post([this, t](Instance& i) { complete(i, t, Failed); });
            }
        }
    }
    return instance;
}

//------------------------------------------------------------------------------
void Runtime::inject(Instance& instance, std::string const& transition)
{
    std::vector<size_t> const found = find(transition);
    instance.post([found](Instance& i) noexcept {
        for (size_t const t : found)
            ++i.m_injected[t];
    });
}

//------------------------------------------------------------------------------
void Runtime::cancel(Instance& instance)
{
    instance.post([this](Instance& i) { cancelNow(i); });
}

//------------------------------------------------------------------------------
void Runtime::pause(Instance& instance)
{
    instance.post([this](Instance& i) { pauseNow(i, true); });
}

//------------------------------------------------------------------------------
void Runtime::resume(Instance& instance)
{
    instance.post([this](Instance& i) { pauseNow(i, false); });
}

//------------------------------------------------------------------------------
void Runtime::cancelNow(Instance& instance)
{
    if (!instance.isRunning())
        return;
    instance.m_result = Canceled;
    for (size_t t = 0u; t < instance.m_running.size(); ++t)
    {
        if ((instance.m_running[t] > 0u) && (m_callbacks[t] != nullptr))
            m_callbacks[t]->cancel();
    }
}

//------------------------------------------------------------------------------
void Runtime::pauseNow(Instance& instance, bool const paused)
{
    if (!instance.isRunning() || (instance.m_paused == paused))
        return;
    instance.m_paused = paused;
    for (size_t t = 0u; t < instance.m_running.size(); ++t)
    {
        if ((instance.m_running[t] > 0u) && (m_callbacks[t] != nullptr))
        {
            if (paused)
                m_callbacks[t]->pause();
            else
                m_callbacks[t]->resume();
        }
    }
}

//------------------------------------------------------------------------------
void Runtime::log(Instance& instance, size_t const transition, LogEntry::Event const event,
                  Color const result)
{
    LogEntry const entry{ instance.m_id, nameOf(transition), event, result, instance.m_time };
    if (m_options.journal)
        m_options.journal(entry);
    if (m_options.log)
        instance.m_log.push_back(entry);
}

//------------------------------------------------------------------------------
bool Runtime::isEnabled(Instance const& instance, size_t const transition) const
{
    if (!tpne::isEnabled(m_net, instance.m_marking, transition, &instance.m_incoming))
        return false;

    // Sources fire when asked.
    if (m_sources[transition] && (instance.m_injected[transition] == 0u))
        return false;
    return true;
}

//------------------------------------------------------------------------------
void Runtime::refresh(Instance& instance) const
{
    std::vector<size_t>& touched = instance.m_touched;
    instance.m_marking.takeTouched(touched);
    if (!instance.m_enabled_valid)
    {
        for (size_t t = 0u; t < instance.m_enabled.size(); ++t)
            instance.m_enabled[t] = isEnabled(instance, t);
        instance.m_enabled_valid = true;
        return;
    }
    for (size_t const p : touched)
    {
        for (size_t const t : m_neighbors[p])
            instance.m_enabled[t] = isEnabled(instance, t);
    }
    // The sources depend on inject().
    for (size_t const t : m_source_list)
        instance.m_enabled[t] = isEnabled(instance, t);
}

//------------------------------------------------------------------------------
bool Runtime::canFire(Instance const& instance, size_t const transition) const
{
    // Instance::m_enabled is up to date (refresh()).
    if (!instance.m_enabled[transition])
        return false;
    if (m_guards[transition])
    {
        Context const context{ m_net, instance.m_id, instance.m_marking, instance.m_time };
        if (!m_guards[transition](context))
            return false;
    }
    double const delay = double(m_net.transitions()[transition].delay);
    return (delay <= 0.0) || ((instance.m_ready_since[transition] != NEVER) &&
                              (instance.m_time >= instance.m_ready_since[transition] + delay));
}

//------------------------------------------------------------------------------
bool Runtime::waitsForGuard(Instance const& instance) const
{
    // Instance::m_enabled is up to date (refresh()).
    for (size_t t = 0u; t < m_guards.size(); ++t)
    {
        if (m_guards[t] && instance.m_enabled[t])
            return true;
    }
    return false;
}

//------------------------------------------------------------------------------
void Runtime::updateEnabling(Instance& instance)
{
    refresh(instance);
    for (size_t t = 0u; t < instance.m_ready_since.size(); ++t)
    {
        if (m_net.transitions()[t].delay <= 0.0f)
            continue;
        if (!instance.m_enabled[t])
            instance.m_ready_since[t] = NEVER;
        else if (instance.m_ready_since[t] == NEVER)
            instance.m_ready_since[t] = instance.m_time;
    }
}

//------------------------------------------------------------------------------
size_t Runtime::pick(Instance& instance)
{
    refresh(instance);
    size_t const none = size_t(-1);
    switch (m_options.policy)
    {
    case Engine::Policy::FirstDeclared:
        for (size_t t = 0u; t < m_callbacks.size(); ++t)
        {
            if (canFire(instance, t))
                return t;
        }
        return none;
    case Engine::Policy::Random:
    {
        std::vector<size_t>& candidates = instance.m_candidates;
        candidates.clear();
        for (size_t t = 0u; t < m_callbacks.size(); ++t)
        {
            if (canFire(instance, t))
                candidates.push_back(t);
        }
        if (candidates.empty())
            return none;
        return candidates[randomIndex(instance.m_random, candidates.size())];
    }
    case Engine::Policy::Priority:
    default:
    {
        // The highest priority, the first declared among equals.
        size_t best = none;
        for (size_t t = 0u; t < m_callbacks.size(); ++t)
        {
            if (((best == none) || (m_net.transitions()[t].priority > m_net.transitions()[best].priority))
                && canFire(instance, t))
                best = t;
        }
        return best;
    }
    }
}

//------------------------------------------------------------------------------
void Runtime::fire(std::shared_ptr<Instance> const& instance, size_t const transition)
{
    Instance& i = *instance;
    tpne::consume(m_net, i.m_marking, transition);
    if (m_sources[transition])
        --i.m_injected[transition];
    i.m_ready_since[transition] = NEVER;
    log(i, transition, LogEntry::Event::Scheduled);
    start(instance, transition);
}

//------------------------------------------------------------------------------
void Runtime::start(std::shared_ptr<Instance> const& instance, size_t const transition)
{
    Instance& i = *instance;
    Callback const* callback = m_callbacks[transition].get();
    if (callback == nullptr)
    {
        // No function: the tokens go on at once.
        complete(i, transition, Success);
        return;
    }

    log(i, transition, LogEntry::Event::Started);
    if (callback->isSynchronous())
    {
        complete(i, transition, callback->fire());
        return;
    }

    // The executor runs the function; its end is posted to the instance.
    ++i.m_running[transition];
    ++i.m_running_total;
    Callback const function = *callback;
    m_executor->post([this, instance, transition, function]() {
        Color const result = function.fire();
        instance->post([this, transition, result](Instance& self) {
            --self.m_running[transition];
            --self.m_running_total;
            complete(self, transition, result);
        });
    });
}

//------------------------------------------------------------------------------
void Runtime::complete(Instance& instance, size_t const transition, Color const result)
{
    if (!instance.isRunning())
        return;
    log(instance, transition, LogEntry::Event::Completed, result);
    produce(instance, transition, result);
}

//------------------------------------------------------------------------------
void Runtime::produce(Instance& instance, size_t const transition, Color const result) const
{
    bool const timed = (m_net.type() == TypeOfNet::TimedPetriNet);
    for (auto const& a : m_net.arcsOut(m_net.transitions()[transition]))
    {
        Color const color = tokenColor(a, result);
        double const duration = timed ? double(a.durationOrZero()) : 0.0;
        if (duration > 0.0)
        {
            // Tokens on their way count for the capacity of the place.
            instance.m_flights.push_back({ a.to.id, color, a.weight, instance.m_time + duration });
            instance.m_incoming[a.to.id] += a.weight;
            instance.m_marking.touch(a.to.id);
        }
        else
            instance.m_marking.add(a.to.id, color, a.weight);
    }
}

//------------------------------------------------------------------------------
void Runtime::deliver(Instance& instance)
{
    auto& flights = instance.m_flights;
    std::stable_sort(flights.begin(), flights.end(), [](Instance::Flight const& a, Instance::Flight const& b) {
        return a.end < b.end;
    });
    size_t arrived = 0u;
    while ((arrived < flights.size()) && (flights[arrived].end <= instance.m_time))
    {
        instance.m_marking.add(flights[arrived].place, flights[arrived].color, flights[arrived].tokens);
        instance.m_incoming[flights[arrived].place] -= flights[arrived].tokens;
        ++arrived;
    }
    flights.erase(flights.begin(), flights.begin() + std::ptrdiff_t(arrived));
}

//------------------------------------------------------------------------------
double Runtime::nextEventDate(Instance const& instance) const
{
    double next = NEVER;
    for (auto const& flight : instance.m_flights)
        next = std::min(next, flight.end);
    for (size_t t = 0u; t < instance.m_ready_since.size(); ++t)
    {
        double const delay = double(m_net.transitions()[t].delay);
        if ((delay > 0.0) && (instance.m_ready_since[t] != NEVER) &&
            (instance.m_ready_since[t] + delay > instance.m_time))
            next = std::min(next, instance.m_ready_since[t] + delay);
    }
    return next;
}

//------------------------------------------------------------------------------
void Runtime::fireAll(std::shared_ptr<Instance> const& instance)
{
    Instance& i = *instance;
    size_t firings = 0u;
    for (;;)
    {
        i.drain();
        if (!i.isRunning() || i.m_paused)
            return;
        deliver(i);
        updateEnabling(i);
        size_t const t = pick(i);
        if (t == size_t(-1))
            return;
        if (++firings > m_options.max_firings_per_date)
        {
            i.m_result = Failed;
            i.m_error = "Transitions keep firing at the date " + std::to_string(i.m_time) +
                        ": a cycle without function and without duration";
            return;
        }
        fire(instance, t);
    }
}

//------------------------------------------------------------------------------
void Runtime::checkEnd(Instance& instance)
{
    if (!instance.isRunning() || instance.m_paused)
        return;

    // Goal reached.
    if (!instance.m_goal.empty())
    {
        bool reached = true;
        for (auto const& goal : instance.m_goal)
        {
            size_t const place = placeOf(goal.place);
            reached = reached && (place < m_net.places().size()) &&
                      (instance.m_marking.count(place, goal.color) >= goal.tokens);
        }
        if (reached)
        {
            instance.m_result = Success;
            return;
        }
    }

    // Something can still happen: a function runs, tokens are on their way,
    // a delay runs, or an input transition may be injected.
    if ((instance.m_running_total > 0u) || !instance.m_flights.empty() ||
        (nextEventDate(instance) != NEVER) || m_has_sources)
        return;
    refresh(instance);
    for (size_t t = 0u; t < m_callbacks.size(); ++t)
    {
        if (canFire(instance, t))
            return;
    }
    if (waitsForGuard(instance))
        return;

    // A failure nobody took care of, or a dead end.
    bool failed = false;
    for (size_t p = 0u; p < instance.m_marking.places(); ++p)
        failed = failed || (instance.m_marking.count(p, Failed) > 0u);
    instance.m_result = failed ? Failed : Deadlocked;
}

//------------------------------------------------------------------------------
bool Runtime::step(Instance& instance, double const now)
{
    std::shared_ptr<Instance> const self = instance.shared_from_this();
    if (!instance.m_started)
    {
        instance.m_started = true;
        instance.m_origin = now;
    }
    double const date = now - instance.m_origin;

    // The events of the time up to now, in their order, then the requests
    // posted since the last step (ends of functions ...), dated now.
    for (double next = nextEventDate(instance);
         instance.isRunning() && !instance.m_paused && (next <= date);
         next = nextEventDate(instance))
    {
        instance.m_time = next;
        fireAll(self);
    }
    instance.m_time = std::max(instance.m_time, date);
    fireAll(self);
    checkEnd(instance);
    return instance.isRunning();
}

//------------------------------------------------------------------------------
Color Runtime::run(Instance& instance)
{
    while (step(instance))
    {
        // Sleep until a request (end of a function, inject, cancel) or the
        // next event of the time.
        double const next = instance.m_paused ? NEVER : nextEventDate(instance);
        std::unique_lock<std::mutex> lock(instance.m_mutex);
        auto const requested = [&instance]() { return !instance.m_reducers.empty(); };
        if ((next == NEVER) && waitsForGuard(instance))
        {
            // Nothing tells when a guard opens: look again soon.
            instance.m_condition.wait_for(lock, std::chrono::milliseconds(10), requested);
        }
        else if (next == NEVER)
        {
            instance.m_condition.wait(lock, requested);
        }
        else
        {
            double const wait = next - (m_options.clock() - instance.m_origin);
            if (wait > 0.0)
                instance.m_condition.wait_for(lock, std::chrono::duration<double>(wait), requested);
        }
    }
    // Wait for the functions still running before returning (they refer to
    // the instance and to this runtime).
    while (instance.m_running_total > 0u)
    {
        {
            std::unique_lock<std::mutex> lock(instance.m_mutex);
            instance.m_condition.wait(lock, [&instance]() { return !instance.m_reducers.empty(); });
        }
        instance.drain();
    }
    return instance.m_result;
}

} // namespace tpne
