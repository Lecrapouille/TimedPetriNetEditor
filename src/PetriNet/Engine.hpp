// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef TPNE_ENGINE_HPP
#  define TPNE_ENGINE_HPP

#  include "PetriNet/ColoredMarking.hpp"
#  include "PetriNet/Firing.hpp"

#  include <cstdint>
#  include <functional>
#  include <limits>
#  include <random>
#  include <string>
#  include <vector>

namespace tpne {

class Net;

//------------------------------------------------------------------------------
//! \brief Clock giving the current date (seconds or any time unit): the real
//! time (realTimeSource()) or a simulated one owned by the caller.
//------------------------------------------------------------------------------
using TimeSource = std::function<double()>;

//------------------------------------------------------------------------------
//! \brief Clock of the real time: seconds elapsed since its creation.
//------------------------------------------------------------------------------
TimeSource realTimeSource();

// *****************************************************************************
//! \brief Runs a Petri net or a timed Petri net as a discrete event simulation.
//!
//! The engine owns the marking and the dates; the net only gives the structure
//! and is not modified. A firing takes the tokens of the input places at once
//! and puts them on the output arcs: they reach their places after the
//! duration of the arc. Time goes from event to event (arrival of tokens, end
//! of an enabling delay): the dates do not depend on the size of the steps the
//! caller gives to advance().
//!
//! At each date the engine fires, one at a time, the transitions that can
//! fire until none can (a conflict is solved by the Policy), then waits for
//! the next event. A transition can fire when it is enabled (Firing.hpp), when
//! it is armed and when it has been so during its delay (Transition::delay).
//!
//! Two modes:
//!   - automatic (timed nets): transitions are armed. A transition without
//!     input place (source) fires again when the tokens of its previous firing
//!     have arrived;
//!   - manual (Petri nets): a transition fires once each time it is armed
//!     (the user clicks on it); it is disarmed after its firing.
// *****************************************************************************
class Engine
{
public:

    //! \brief Which transition fires first when several can fire at a date.
    enum class Policy
    {
        //! \brief One of them at random (see Options::seed).
        Random,
        //! \brief The one of highest Transition::priority, then the first
        //! declared one.
        Priority,
        //! \brief The one of lowest identifier.
        FirstDeclared,
    };

    //! \brief Result of a firing when onResult is not set: the color of the
    //! tokens given by the output arcs without color. There is no function
    //! to give it, as in the runtime.
    enum class Results
    {
        //! \brief Always Success.
        AlwaysSuccess,
        //! \brief One at random (Options::seed) among the colors asked by
        //! the arcs leaving its output places; Success when none is asked.
        Asked,
    };

    //! \brief Settings of the engine.
    struct Options
    {
        Policy policy = Policy::Random;
        Results results = Results::Asked;
        //! \brief Seed of the Random policy. 0: a random seed is drawn; seed()
        //! gives it to replay the same simulation.
        uint64_t seed = 0u;
        //! \brief Manual mode (see the class description).
        bool manual = false;
        //! \brief Durations of the arcs: their duration (timed nets), else this
        //! one for all of them (0: tokens arrive at once).
        bool timed = true;
        double untimed_duration = 0.0;
        //! \brief Firings at one date before the engine stops: a cycle of
        //! transitions without duration would fire for ever (Zeno behavior).
        size_t max_firings_per_date = 10000u;
    };

    //! \brief Tokens on their way along an arc after a firing.
    struct Flight
    {
        //! \brief Index of the arc in Net::arcs() (transition -> place).
        size_t arc;
        //! \brief Number of tokens (weight of the arc).
        size_t tokens;
        //! \brief Color of the tokens (tokenColor() of the arc).
        Color color;
        //! \brief Date of the firing and date of arrival in the place.
        double start;
        double end;
    };

    //--------------------------------------------------------------------------
    //! \brief The net shall not change its structure while the engine lives.
    //! reset() shall be called before running.
    //--------------------------------------------------------------------------
    explicit Engine(Net const& net);
    Engine(Net const& net, Options const& options);

    //--------------------------------------------------------------------------
    //! \brief Start again at date 0 from the marking of the net
    //! (initialMarking()), or the given one (a Marking: its tokens Success). Draws the seed when Options::seed is
    //! 0.
    //--------------------------------------------------------------------------
    void reset();
    void reset(Marking const& marking);
    void reset(ColoredMarking const& marking);

    //--------------------------------------------------------------------------
    //! \brief Let time go until the given date (or during dt): tokens arrive,
    //! transitions fire. Does nothing for a date in the past.
    //--------------------------------------------------------------------------
    void advanceTo(double const date);
    void advance(double const dt) { advanceTo(m_time + dt); }
    void update(TimeSource const& clock) { advanceTo(clock()); }

    //--------------------------------------------------------------------------
    //! \brief Fire now the transitions that can fire, without letting time
    //! go (after arming a transition or changing tokens).
    //--------------------------------------------------------------------------
    void settle();

    //--------------------------------------------------------------------------
    //! \brief Date of the next event (arrival of tokens, end of a delay), or
    //! +infinity when nothing will happen by itself.
    //--------------------------------------------------------------------------
    double nextEventDate() const;

    //--------------------------------------------------------------------------
    //! \brief Nothing can happen any more: no token on its way, no transition
    //! can fire or will be able to fire after its delay.
    //--------------------------------------------------------------------------
    bool isDeadlocked() const;

    //--------------------------------------------------------------------------
    //! \brief A cycle without duration made the engine stop (see
    //! Options::max_firings_per_date).
    //--------------------------------------------------------------------------
    bool isZeno() const { return m_zeno; }

    //--------------------------------------------------------------------------
    //! \brief Current date, marking and tokens on their way. The engine
    //! follows the colors of the tokens as the runtime does (Firing.hpp);
    //! marking() gives their number by place, colors not considered.
    //--------------------------------------------------------------------------
    double time() const { return m_time; }
    Marking const& marking() const { return m_marking; }
    ColoredMarking const& coloredMarking() const { return m_colored; }
    std::vector<Flight> const& flights() const { return m_flights; }

    //--------------------------------------------------------------------------
    //! \brief Change the tokens of a place (the user adds a token ...): the
    //! tokens added are Success, the ones removed of any color.
    //--------------------------------------------------------------------------
    void setTokens(size_t const place, size_t const tokens);
    //! \brief Same with the (color, number) of its tokens.
    void setTokens(size_t const place, std::vector<std::pair<Color, size_t>> const& tokens);

    //--------------------------------------------------------------------------
    //! \brief Arm or disarm a transition (manual mode: a click).
    //--------------------------------------------------------------------------
    void arm(size_t const transition, bool const armed = true);
    bool isArmed(size_t const transition) const { return m_armed[transition]; }

    //--------------------------------------------------------------------------
    //! \brief Can the transition fire now (enabled, armed, delay elapsed).
    //--------------------------------------------------------------------------
    bool canFire(size_t const transition) const;

    //--------------------------------------------------------------------------
    //! \brief Time since the transition is enabled and armed (0 if it is not):
    //! compared to its delay.
    //--------------------------------------------------------------------------
    double enabledFor(size_t const transition) const;

    //--------------------------------------------------------------------------
    //! \brief Seed used by the Random policy.
    //--------------------------------------------------------------------------
    uint64_t seed() const { return m_seed; }
    Options const& options() const { return m_options; }

    //--------------------------------------------------------------------------
    //! \brief Dates of the firings of each transition since reset().
    //--------------------------------------------------------------------------
    std::vector<std::vector<double>> const& firingDates() const { return m_firing_dates; }

    //! \brief Called at each firing: transition, date, number k of the firing
    //! of this transition (1 for the first one).
    std::function<void(size_t, double, size_t)> onFire;
    //! \brief Result of the firing of the transition: the color of the
    //! tokens given by its output arcs without color. Not set:
    //! Transition::result, else Options::results.
    std::function<Color(size_t)> onResult;
    //! \brief Called when tokens reach their place.
    std::function<void(Flight const&)> onArrival;
    //! \brief Called with warnings (Zeno behavior ...).
    std::function<void(std::string const&)> onWarning;

private:

    //! \brief Fire the transitions that can fire at the current date.
    void fireAll();
    //! \brief Fire the transition at the current date.
    void fire(size_t const transition);
    //! \brief Tokens arrived at the current date reach their places.
    void deliver();
    //! \brief Follow the dates since which transitions can fire.
    void updateEnabling();
    //! \brief Transitions able to fire, by the policy: the one to fire first,
    //! or npos.
    size_t pick();
    //! \brief Enabled and armed, delay not considered.
    bool isReady(size_t const transition) const;
    //! \brief Result of the firing: onResult, else Transition::result, else
    //! Options::results.
    Color result(size_t const transition);
    //! \brief m_marking follows the places of m_colored that changed.
    void count();

private:

    static constexpr double NEVER = std::numeric_limits<double>::infinity();

    Net const& m_net;
    Options m_options;
    uint64_t m_seed = 0u;
    std::mt19937_64 m_random;
    double m_time = 0.0;
    ColoredMarking m_colored;
    //! \brief Tokens of m_colored by place.
    Marking m_marking;
    std::vector<size_t> m_touched;
    //! \brief Tokens on their way to each place (capacities).
    Marking m_incoming;
    std::vector<Flight> m_flights;
    //! \brief Flights of each transition not yet arrived.
    std::vector<size_t> m_flying;
    std::vector<bool> m_armed;
    //! \brief Date since which each transition is ready, or NEVER.
    std::vector<double> m_ready_since;
    std::vector<std::vector<double>> m_firing_dates;
    //! \brief results() of each transition (Results::Asked).
    std::vector<std::vector<Color>> m_asked;
    bool m_zeno = false;
};

} // namespace tpne

#endif
