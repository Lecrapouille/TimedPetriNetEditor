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

#ifndef SIMULATION_NET_HPP
#  define SIMULATION_NET_HPP

#  include "PetriNet/Signal.hpp"
#  include "PetriNet/TimedTokens.hpp"
#  include "PetriNet/GrafcetEngine.hpp"

#  include <atomic>
#  include <memory>
#  include <string>
#  include <vector>
#  include <map>

namespace tpne {

class Net;
class Arc;
class Node;
class Place;
class Transition;
class Receptivity;

// *****************************************************************************
//! \brief Petri net simulation engine.
//! Handles token animation, GRAFCET actions, temporizations and forcings.
// *****************************************************************************
class Simulation
{
public:

    using TimedTokens = std::vector<TimedToken>;
    using ActionState = GrafcetEngine::ActionState;

    explicit Simulation(Net& net);
    ~Simulation();

    //--------------------------------------------------------------------------
    //! \brief GRAFCET: run this net with the given engine, shared with the
    //! other graphs of a document (cross-graph receptivities and forcings).
    //! The owner of the shared engine steps it. Null: the simulation uses its
    //! own engine running this net alone.
    //--------------------------------------------------------------------------
    void grafcet(GrafcetEngine* shared);

    //--------------------------------------------------------------------------
    //! \brief GRAFCET engine used by this simulation.
    //--------------------------------------------------------------------------
    GrafcetEngine& grafcet() { return *m_engine; }
    GrafcetEngine const& grafcet() const { return *m_engine; }

    //--------------------------------------------------------------------------
    //! \brief Start the simulation. Emits onStarted signal on success.
    //--------------------------------------------------------------------------
    void start();

    //--------------------------------------------------------------------------
    //! \brief Stop the simulation. Emits onStopped signal.
    //--------------------------------------------------------------------------
    void stop();

    //--------------------------------------------------------------------------
    //! \brief Check if the simulation is currently running.
    //--------------------------------------------------------------------------
    inline bool isRunning() const { return m_running; }

    //--------------------------------------------------------------------------
    //! \brief Advance the simulation by dt seconds. Called by the editor.
    //--------------------------------------------------------------------------
    void step(float const dt);

    //--------------------------------------------------------------------------
    //! \brief Get the animated tokens for rendering.
    //--------------------------------------------------------------------------
    inline TimedTokens const& animatedTokens() const { return m_timed_tokens; }

    //--------------------------------------------------------------------------
    //! \brief GRAFCET: compiled receptivity of a transition while running, or
    //! null.
    //--------------------------------------------------------------------------
    Receptivity const* receptivity(size_t transition_id) const;

    //--------------------------------------------------------------------------
    //! \brief Get the arc token count for animation (indexed by arc position).
    //--------------------------------------------------------------------------
    inline size_t arcTokenCount(size_t arc_index) const
    {
        return arc_index < m_arc_token_counts.size() ? m_arc_token_counts[arc_index] : 0;
    }

    //--------------------------------------------------------------------------
    //! \brief Set the arc token count (used during firing).
    //--------------------------------------------------------------------------
    inline void setArcTokenCount(size_t arc_index, size_t count)
    {
        if (arc_index < m_arc_token_counts.size())
            m_arc_token_counts[arc_index] = count;
    }

    //--------------------------------------------------------------------------
    //! \brief Get action state for a specific action.
    //! \param[in] place_id The place ID (0-based, consecutive).
    //! \param[in] action_index The action index within the place.
    //--------------------------------------------------------------------------
    ActionState const& actionState(size_t place_id, size_t action_index) const;

    //--------------------------------------------------------------------------
    //! \brief Get the delay timer for a transition (GRAFCET).
    //--------------------------------------------------------------------------
    float transitionDelayTimer(size_t transition_id) const;

    //--------------------------------------------------------------------------
    //! \brief Check if the net is frozen by a forcing command (GRAFCET).
    //--------------------------------------------------------------------------
    bool isFrozen() const;

    //--------------------------------------------------------------------------
    //! \brief Check if a place is an initial step (had tokens at simulation start).
    //! \param[in] place_id The place ID (0-based, consecutive).
    //--------------------------------------------------------------------------
    bool isInitialStep(size_t place_id) const;

    //--------------------------------------------------------------------------
    //! \brief Check if initial marking has been stored.
    //--------------------------------------------------------------------------
    bool hasInitialMarking() const;

    //--------------------------------------------------------------------------
    //! \brief Check if there are any receptivity parsing errors (GRAFCET).
    //--------------------------------------------------------------------------
    bool hasReceptivityErrors() const;

    //--------------------------------------------------------------------------
    //! \brief Validate receptivities without starting simulation.
    //! \return true if all receptivities are valid, false otherwise.
    //--------------------------------------------------------------------------
    bool validateReceptivities();

    //--------------------------------------------------------------------------
    //! \brief Current simulation time, accumulated since start().
    //--------------------------------------------------------------------------
    inline float simulationTime() const { return m_sim_time; }

    //--------------------------------------------------------------------------
    //! \brief Firing dates of a transition since start(). y[k] is the time at
    //! which the transition fired for the (k+1)-th time, analogous to the
    //! flowshop_simu outputs y(k) of ScicosLab/MaxPlus.jl.
    //--------------------------------------------------------------------------
    inline std::vector<float> const& firingDates(size_t transition_id) const
    {
        static std::vector<float> const empty;
        return transition_id < m_firing_dates.size()
            ? m_firing_dates[transition_id] : empty;
    }

    //--------------------------------------------------------------------------
    //! \brief Total number of recorded firings across all transitions.
    //--------------------------------------------------------------------------
    size_t totalFirings() const;

    //--------------------------------------------------------------------------
    //! \brief (max,+) eigenvalue (cycle time λ) of the Timed Event Graph,
    //! cached once at simulation start. Returns 0 if not a TEG or if the
    //! Howard algorithm failed.
    //--------------------------------------------------------------------------
    inline double eigenvalue() const { return m_eigenvalue; }

    //! \brief Signal emitted when simulation starts successfully.
    Signal<> onStarted;
    //! \brief Signal emitted when simulation stops.
    Signal<> onStopped;
    //! \brief Signal emitted for informational messages.
    Signal<std::string const&> onInfo;
    //! \brief Signal emitted for warning messages.
    Signal<std::string const&> onWarning;
    //! \brief Signal emitted for error messages.
    Signal<std::string const&> onError;

private:

    // *************************************************************************
    //! \brief State machine for the Petri net simulation.
    // *************************************************************************
    enum class State
    {
        Idle,       //!< Waiting the user request to start the simulation.
        Simulating, //!< Simulation on-going: animate tokens.
        Halting,    //!< Restore states after the simulation.
    };

    //--------------------------------------------------------------------------
    // State machine handlers
    //--------------------------------------------------------------------------
    void handleIdleState();
    void handleSimulatingState(float const dt);
    void handleHaltingState();

    //--------------------------------------------------------------------------
    // Initialization
    //--------------------------------------------------------------------------
    void initializeRuntimeState();

    //--------------------------------------------------------------------------
    // Simulation logic
    //--------------------------------------------------------------------------
    void updateTransitionDelayTimers(float const dt);
    void fireTransitions();
    void animateTokens(float const dt);
    std::vector<Transition*> const& shuffleTransitions(bool const reset = false);

    //--------------------------------------------------------------------------
    // Marking management
    //--------------------------------------------------------------------------
    void storeInitialMarking();
    void restoreInitialMarking();

private:

    //! \brief The Petri net being simulated.
    Net& m_net;
    //! \brief Current state of the simulation state machine.
    State m_state{State::Idle};
    //! \brief True if the simulation is running.
    std::atomic<bool> m_running{false};
    //! \brief GRAFCET engine running this net: m_own_engine or an engine
    //! shared with the other graphs of a document.
    GrafcetEngine* m_engine;
    //! \brief Engine used when no engine is shared.
    std::unique_ptr<GrafcetEngine> m_own_engine;

    //--------------------------------------------------------------------------
    // Runtime state vectors (indexed by node IDs which are consecutive from 0)
    //--------------------------------------------------------------------------

    //! \brief Delay timers for transitions with temporization.
    //! Indexed by Transition::id.
    std::vector<float> m_transition_delay_timers;
    //! \brief Token counts for arcs during animation.
    //! Indexed by arc position in Net::arcs().
    std::vector<size_t> m_arc_token_counts;

    //--------------------------------------------------------------------------
    // Other simulation data
    //--------------------------------------------------------------------------

    //! \brief List of shuffled Transitions for random firing order.
    std::vector<Transition*> m_shuffled_transitions;
    //! \brief Animated tokens transitioning from Transitions to Places.
    TimedTokens m_timed_tokens;
    //! \brief Initial marking stored at simulation start.
    std::vector<size_t> m_initial_tokens;
    //! \brief Accumulated simulation time since start().
    float m_sim_time = 0.0f;
    //! \brief Firing dates per transition (indexed by Transition::id).
    //! Only filled when the net is a Timed Event Graph.
    std::vector<std::vector<float>> m_firing_dates;
    //! \brief (max,+) cycle time λ, computed once at simulation start when
    //! the net is a Timed Event Graph. Zero otherwise.
    double m_eigenvalue = 0.0;
    //! \brief True if there are receptivity parsing errors.
    bool m_has_receptivity_errors = false;
};

} // namespace tpne

#endif