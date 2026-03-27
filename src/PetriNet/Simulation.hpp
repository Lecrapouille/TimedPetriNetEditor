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

#  include <atomic>
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

    // *************************************************************************
    //! \brief Animated token moving along an arc from Transition to Place.
    //! Stores pointers to origin and destination nodes for position calculation,
    //! eliminating the need to store TypeOfNet.
    // *************************************************************************
    struct AnimatedToken
    {
        //! \brief Constructor.
        //! \param[in] origin_ The origin node (Transition).
        //! \param[in] destination_ The visual destination node (Place or next Transition for TEG).
        //! \param[in] target_place_ The actual Place where tokens will be deposited.
        //! \param[in] tokens_ Number of tokens being animated.
        //! \param[in] duration Arc duration for speed calculation.
        //! \param[in] default_duration Default duration if arc duration is 0.
        AnimatedToken(Node& origin_, Node& destination_, Place& target_place_,
                      size_t tokens_, float duration, float default_duration);

        //! \brief Update position on the screen.
        //! \param[in] dt Delta time in seconds.
        //! \return true when arriving at destination, false otherwise.
        bool update(float dt);

        Node* origin;           //!< Origin node (Transition)
        Node* destination;      //!< Visual destination (Place or next Transition for TEG)
        Place* targetPlace;     //!< Actual Place where tokens are deposited
        float x;                //!< Current X position for rendering
        float y;                //!< Current Y position for rendering
        size_t tokens;          //!< Number of tokens being carried
        float magnitude;        //!< Length of the path
        float speed;            //!< Animation speed
        float offset = 0.0f;    //!< Progress along the path (0.0 to 1.0)
    };

    using AnimatedTokens = std::vector<AnimatedToken>;
    using Receptivities = std::map<size_t, Receptivity>;

    // *************************************************************************
    //! \brief Runtime state for GRAFCET actions (indexed by place_id, action_index)
    // *************************************************************************
    struct ActionState
    {
        bool active = false;
        float timer = 0.0f;
    };

    explicit Simulation(Net& net);

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
    inline AnimatedTokens const& animatedTokens() const { return m_animated_tokens; }

    //--------------------------------------------------------------------------
    //! \brief Get the receptivities map for inspection.
    //--------------------------------------------------------------------------
    inline Receptivities const& receptivities() const { return m_receptivities; }

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
    //! \brief Check if a place was active in the previous cycle (for edge detection).
    //--------------------------------------------------------------------------
    inline bool wasPlaceActive(size_t place_id) const
    {
        return place_id < m_place_was_active.size() && m_place_was_active[place_id];
    }

    //--------------------------------------------------------------------------
    //! \brief Get the delay timer for a transition.
    //--------------------------------------------------------------------------
    inline float transitionDelayTimer(size_t transition_id) const
    {
        return transition_id < m_transition_delay_timers.size()
            ? m_transition_delay_timers[transition_id] : 0.0f;
    }

    //--------------------------------------------------------------------------
    //! \brief Check if the net is frozen by a forcing command.
    //--------------------------------------------------------------------------
    inline bool isFrozen() const { return m_frozen; }

    //--------------------------------------------------------------------------
    //! \brief Check if a place is an initial step (had tokens at simulation start).
    //! \param[in] place_id The place ID (0-based, consecutive).
    //--------------------------------------------------------------------------
    inline bool isInitialStep(size_t place_id) const
    {
        return place_id < m_initial_tokens.size() && m_initial_tokens[place_id] > 0;
    }

    //--------------------------------------------------------------------------
    //! \brief Check if initial marking has been stored.
    //--------------------------------------------------------------------------
    inline bool hasInitialMarking() const { return !m_initial_tokens.empty(); }

    //--------------------------------------------------------------------------
    //! \brief Check if there are any receptivity parsing errors (GRAFCET).
    //--------------------------------------------------------------------------
    inline bool hasReceptivityErrors() const { return m_has_receptivity_errors; }

    //--------------------------------------------------------------------------
    //! \brief Validate receptivities without starting simulation.
    //! \return true if all receptivities are valid, false otherwise.
    //--------------------------------------------------------------------------
    bool validateReceptivities();

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
    bool compileReceptivities();
    bool compileReceptivity(Transition const& transition);

    //--------------------------------------------------------------------------
    // Simulation logic
    //--------------------------------------------------------------------------
    void evaluateReceptivities();
    void updateTransitionDelayTimers(float const dt);
    void fireTransitions();
    void animateTokens(float const dt);
    std::vector<Transition*> const& shuffleTransitions(bool const reset = false);

    //--------------------------------------------------------------------------
    // GRAFCET specific
    //--------------------------------------------------------------------------
    void updateGrafcetActions(float const dt);
    void resetStoredAction(std::string const& name);
    void applyGrafcetForcings();

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
    //! \brief GRAFCET frozen state (no evolution allowed).
    bool m_frozen = false;

    //--------------------------------------------------------------------------
    // Runtime state vectors (indexed by node IDs which are consecutive from 0)
    //--------------------------------------------------------------------------

    //! \brief Previous activation state of places (for rising/falling edge detection).
    //! Indexed by Place::id.
    std::vector<bool> m_place_was_active;
    //! \brief Delay timers for transitions with temporization.
    //! Indexed by Transition::id.
    std::vector<float> m_transition_delay_timers;
    //! \brief Token counts for arcs during animation.
    //! Indexed by arc position in Net::arcs().
    std::vector<size_t> m_arc_token_counts;
    //! \brief Runtime state for GRAFCET actions.
    //! Key: (place_id, action_index), Value: ActionState.
    std::map<std::pair<size_t, size_t>, ActionState> m_action_states;

    //--------------------------------------------------------------------------
    // Other simulation data
    //--------------------------------------------------------------------------

    //! \brief List of shuffled Transitions for random firing order.
    std::vector<Transition*> m_shuffled_transitions;
    //! \brief Animated tokens transitioning from Transitions to Places.
    AnimatedTokens m_animated_tokens;
    //! \brief Initial marking stored at simulation start.
    std::vector<size_t> m_initial_tokens;
    //! \brief Compiled GRAFCET receptivities (boolean expressions).
    Receptivities m_receptivities;
    //! \brief True if there are receptivity parsing errors.
    bool m_has_receptivity_errors = false;
    //! \brief Dummy action state returned for invalid queries.
    static ActionState s_dummy_action_state;
};

} // namespace tpne

#endif