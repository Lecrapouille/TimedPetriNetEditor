// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef TPNE_RUNTIME_HPP
#  define TPNE_RUNTIME_HPP

#  include "Runtime/Callback.hpp"
#  include "Runtime/Executor.hpp"
#  include "PetriNet/Engine.hpp"
#  include "PetriNet/PetriNet.hpp"

#  include <condition_variable>
#  include <deque>
#  include <functional>
#  include <map>
#  include <memory>
#  include <mutex>
#  include <random>
#  include <string>
#  include <vector>

namespace tpne {

// *****************************************************************************
//! \brief Tokens of each color in each place.
// *****************************************************************************
class ColoredMarking
{
public:

    explicit ColoredMarking(size_t const places = 0u) : m_counts(places) {}

    //! \brief Number of places.
    size_t places() const { return m_counts.size(); }

    //! \brief Tokens of the color in the place (AnyColor: of all colors).
    size_t count(size_t const place, Color const color = AnyColor) const;

    //! \brief Add tokens of the color in the place.
    void add(size_t const place, Color const color, size_t const tokens = 1u);

    //! \brief Remove tokens of the color (AnyColor: the first colors first).
    //! Return false, removing nothing, when there are not enough.
    bool take(size_t const place, Color const color, size_t const tokens);

    //! \brief Remove all the tokens of the color (AnyColor: all).
    void clear(size_t const place, Color const color = AnyColor);

    //! \brief (color, count) of the tokens of the place.
    std::vector<std::pair<Color, size_t>> tokens(size_t const place) const;

    //! \brief Number of tokens of each place, colors not considered.
    Marking counts() const;

    bool operator==(ColoredMarking const& other) const { return m_counts == other.m_counts; }

private:

    //! \brief Tokens by place, then by color.
    std::vector<std::vector<size_t>> m_counts;
};

// *****************************************************************************
//! \brief Tokens in a place, by its name: to give the initial and the goal
//! markings of an instance ({ "Ready", Success }).
// *****************************************************************************
struct PlaceTokens
{
    std::string place;
    Color color = Success;
    size_t tokens = 1u;
};

// *****************************************************************************
//! \brief One line of the journal of an instance.
// *****************************************************************************
struct LogEntry
{
    enum class Event
    {
        //! \brief The transition fired: it took its tokens.
        Scheduled,
        //! \brief Its function started.
        Started,
        //! \brief Its function ended with the color result: tokens given.
        Completed,
    };

    std::string instance;
    std::string transition;
    Event event;
    //! \brief Result of the function (Completed).
    Color result = Success;
    double date = 0.0;
};

// *****************************************************************************
//! \brief What a guard sees.
// *****************************************************************************
struct Context
{
    Net const& net;
    std::string const& instance;
    ColoredMarking const& marking;
    double now;

    //! \brief Tokens in the place of this name (caption or key).
    size_t tokens(std::string const& place, Color const color = AnyColor) const;
};

class Runtime;

// *****************************************************************************
//! \brief A run of a net: its marking, its journal. Several instances of a
//! runtime share its net and its functions. Only the thread calling run() or
//! step() changes it; the other threads post requests (inject, cancel, end of
//! the functions).
// *****************************************************************************
class Instance : public std::enable_shared_from_this<Instance>
{
public:

    std::string const& id() const { return m_id; }
    //! \brief Not ended yet (running or paused).
    bool isRunning() const { return m_result == AnyColor; }
    bool isPaused() const { return m_paused; }
    //! \brief How the instance ended: Success (goal reached), Deadlocked
    //! (nothing can happen any more), Canceled, Failed (a Failed token left
    //! when nothing can happen any more, or a cycle without duration). AnyColor
    //! while it runs.
    Color result() const { return m_result; }
    //! \brief Why the instance failed (cycle without duration ...).
    std::string const& error() const { return m_error; }
    ColoredMarking const& marking() const { return m_marking; }
    std::vector<LogEntry> const& log() const { return m_log; }
    double time() const { return m_time; }

private:

    friend class Runtime;

    //! \brief Tokens on their way to a place (arc with a duration).
    struct Flight
    {
        size_t place;
        Color color;
        size_t tokens;
        double end;
    };

    //! \brief A request of another thread, run by the owner of the marking.
    using Reducer = std::function<void(Instance&)>;

    Instance(std::string const& id, size_t places, size_t transitions);
    void post(Reducer reducer);
    bool drain();

    std::string m_id;
    Color m_result = AnyColor;
    bool m_paused = false;
    std::string m_error;
    ColoredMarking m_marking;
    std::vector<PlaceTokens> m_goal;
    std::vector<LogEntry> m_log;
    double m_time = 0.0;
    //! \brief Date given to the first step(): the time of the instance
    //! starts there.
    double m_origin = 0.0;
    bool m_started = false;
    std::vector<Flight> m_flights;
    //! \brief Functions running, by transition, and their callbacks.
    std::vector<size_t> m_running;
    size_t m_running_total = 0u;
    //! \brief Firings asked by inject(), by transition.
    std::vector<size_t> m_injected;
    //! \brief Date since which each transition can fire (its delay).
    std::vector<double> m_ready_since;
    std::mt19937_64 m_random;
    //! \brief Requests of the other threads.
    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::deque<Reducer> m_reducers;
};

// *****************************************************************************
//! \brief Runs a net by calling functions: a transition is a function. When a
//! transition can fire, the runtime takes its input tokens, calls its function
//! and, when it ends, gives the output tokens, colored by the result of the
//! function (Success, Failed or a color of the user) unless the arc gives its
//! own color. The colors asked by the arcs decide which transition takes the
//! tokens: this is how the result of a function chooses what comes next.
//!
//!   - A transition without function fires at once.
//!   - A transition without input place fires only when inject() asks.
//!   - Arcs with a duration (timed Petri nets) give their tokens later.
//!   - Transition::delay: enabled during this time before firing.
//!   - Conflicts: Transition::priority (or the policy of the options).
//!
//! The marking of an instance is changed only by the thread calling run() or
//! step(): functions running on other threads post their end to it (Redux or
//! actor pattern), so the marking needs no lock.
// *****************************************************************************
class Runtime
{
public:

    struct Options
    {
        //! \brief Which transition fires first when several can.
        Engine::Policy policy = Engine::Policy::Priority;
        //! \brief Seed of the Random policy (0: drawn).
        uint64_t seed = 0u;
        //! \brief Keep the journal of the instances.
        bool log = true;
        //! \brief Firings at one date before the instance fails (a cycle
        //! of transitions without function and without duration).
        size_t max_firings_per_date = 10000u;
        //! \brief Clock of run() (seconds) and of step() without date.
        TimeSource clock = realTimeSource();
    };

    //--------------------------------------------------------------------------
    //! \brief The net shall not change while the runtime lives. Functions of
    //! the transitions are run by the executor (InlineExecutor by default:
    //! in the thread of run() or step()).
    //--------------------------------------------------------------------------
    explicit Runtime(Net const& net);
    Runtime(Net const& net, std::shared_ptr<Executor> executor);
    Runtime(Net const& net, std::shared_ptr<Executor> executor, Options const& options);

    //--------------------------------------------------------------------------
    //! \brief Function of the transitions of this name (caption or key).
    //! Return false when no transition has this name.
    //--------------------------------------------------------------------------
    bool on(std::string const& transition, Callback const& callback);

    //--------------------------------------------------------------------------
    //! \brief Extra condition of the transitions of this name: they fire only
    //! while it returns true.
    //--------------------------------------------------------------------------
    bool guard(std::string const& transition, std::function<bool(Context const&)> guard);

    //--------------------------------------------------------------------------
    //! \brief Names of the transitions with an input place and no function
    //! (they fire at once): to check the functions given at startup.
    //--------------------------------------------------------------------------
    std::vector<std::string> unregisteredTransitions() const;

    //--------------------------------------------------------------------------
    //! \brief New instance. Initial tokens: the given ones, else the tokens of
    //! the places of the net (Success). Goal: when its tokens are present the
    //! instance ends with Success (no goal: it ends when nothing more can
    //! happen).
    //--------------------------------------------------------------------------
    std::shared_ptr<Instance> instantiate(std::string const& id,
                                          std::vector<PlaceTokens> const& initial = {},
                                          std::vector<PlaceTokens> const& goal = {});

    //--------------------------------------------------------------------------
    //! \brief Ask one firing of the transition of this name (a transition
    //! without input place: a button, a message). Any thread.
    //--------------------------------------------------------------------------
    void inject(Instance& instance, std::string const& transition);

    //--------------------------------------------------------------------------
    //! \brief Stop the instance (Canceled) and cancel its running functions.
    //! Any thread.
    //--------------------------------------------------------------------------
    void cancel(Instance& instance);

    //--------------------------------------------------------------------------
    //! \brief Pause and resume the instance and its running functions.
    //--------------------------------------------------------------------------
    void pause(Instance& instance);
    void resume(Instance& instance);

    //--------------------------------------------------------------------------
    //! \brief Run until the instance ends: its result. Waits for the
    //! functions and the durations with the clock of the options (real time).
    //--------------------------------------------------------------------------
    Color run(Instance& instance);

    //--------------------------------------------------------------------------
    //! \brief Let the instance evolve until the given date (seconds since its
    //! start, or the clock of the options), without waiting: to call from the
    //! loop of a program (ImGui, ROS timer, Arduino loop()). Return false
    //! when the instance has ended.
    //--------------------------------------------------------------------------
    bool step(Instance& instance, double const now);
    bool step(Instance& instance) { return step(instance, m_options.clock()); }

    Net const& net() const { return m_net; }
    Options const& options() const { return m_options; }

private:

    //! \brief Transitions of this name.
    std::vector<size_t> find(std::string const& transition) const;
    //! \brief Name of the transition in the journal and the callbacks.
    std::string const& nameOf(size_t const transition) const;
    size_t placeOf(std::string const& place) const;
    //! \brief Rules of the colored firing.
    bool isEnabled(Instance const& instance, size_t const transition) const;
    bool canFire(Instance const& instance, size_t const transition) const;
    //! \brief Fire one transition: take the tokens, start its function.
    void fire(std::shared_ptr<Instance> const& instance, size_t const transition);
    //! \brief End of the function: give the output tokens.
    void complete(Instance& instance, size_t const transition, Color const result);
    void deliver(Instance& instance);
    void updateEnabling(Instance& instance);
    size_t pick(Instance& instance);
    void fireAll(std::shared_ptr<Instance> const& instance);
    double nextEventDate(Instance const& instance) const;
    void checkEnd(Instance& instance);
    void log(Instance& instance, size_t const transition, LogEntry::Event const event,
             Color const result = Success);
    //! \brief Requests: cancel, pause, resume (in the owner thread).
    void cancelNow(Instance& instance);
    void pauseNow(Instance& instance, bool const paused);

private:

    Net const& m_net;
    std::shared_ptr<Executor> m_executor;
    Options m_options;
    std::vector<std::unique_ptr<Callback>> m_callbacks;
    std::vector<std::function<bool(Context const&)>> m_guards;
    //! \brief Transitions without input place (fired by inject()).
    std::vector<bool> m_sources;
    bool m_has_sources = false;
};

} // namespace tpne

#endif
