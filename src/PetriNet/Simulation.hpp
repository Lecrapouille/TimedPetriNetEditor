// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef SIMULATION_NET_HPP
#  define SIMULATION_NET_HPP

#  include "PetriNet/Signal.hpp"
#  include "PetriNet/Engine.hpp"
#  include "PetriNet/GrafcetEngine.hpp"

#  include <atomic>
#  include <memory>
#  include <string>
#  include <vector>
#  include <map>

namespace tpne {

class Net;
class Transition;
class Receptivity;

// *****************************************************************************
//! \brief Simulation of a net as the editor runs it: started and stopped by the
//! user, stepped at each frame, its marking shown in the places of the net and
//! restored at the end. Petri nets and timed nets run on an Engine (discrete
//! events), GRAFCET on a GrafcetEngine. While it runs, the user can arm the
//! transitions of a Petri net (Transition::receptivity) and change the tokens
//! of the places.
// *****************************************************************************
class Simulation
{
public:

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
    //! \brief How the next simulations solve conflicts (policy, seed). The
    //! mode (manual, timed) comes from the type of net.
    //--------------------------------------------------------------------------
    void options(Engine::Options const& options) { m_options = options; }
    Engine::Options const& options() const { return m_options; }

    //--------------------------------------------------------------------------
    //! \brief Seed used by the last simulation, to replay it (0: none yet).
    //--------------------------------------------------------------------------
    uint64_t seed() const;

    //--------------------------------------------------------------------------
    //! \brief Time the tokens of an untimed Petri net spend on arcs, so that
    //! they are seen moving (seconds). 0: they arrive at once.
    //--------------------------------------------------------------------------
    double untimed_duration = 0.2;

    //--------------------------------------------------------------------------
    //! \brief Tokens on their way along arcs (Petri nets and timed nets).
    //--------------------------------------------------------------------------
    std::vector<Engine::Flight> const& flights() const;

    //--------------------------------------------------------------------------
    //! \brief GRAFCET: compiled receptivity of a transition while running, or
    //! null.
    //--------------------------------------------------------------------------
    Receptivity const* receptivity(size_t transition_id) const;

    //--------------------------------------------------------------------------
    //! \brief Get action state for a specific action.
    //! \param[in] place_id The place ID (0-based, consecutive).
    //! \param[in] action_index The action index within the place.
    //--------------------------------------------------------------------------
    ActionState const& actionState(size_t place_id, size_t action_index) const;

    //--------------------------------------------------------------------------
    //! \brief Time since the transition can fire, compared to its delay.
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
    //! \brief Tokens of the places before the simulation: while it runs, the
    //! places of the net hold the tokens of the moment (they are given back
    //! when it stops).
    //--------------------------------------------------------------------------
    std::vector<size_t> initialMarking() const;

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
    //! \brief Current simulation time since start().
    //--------------------------------------------------------------------------
    inline double simulationTime() const { return m_sim_time; }

    //--------------------------------------------------------------------------
    //! \brief Firing dates of a transition since start(). y[k] is the time at
    //! which the transition fired for the (k+1)-th time, analogous to the
    //! flowshop_simu outputs y(k) of ScicosLab/MaxPlus.jl. Kept after the end
    //! of the simulation.
    //--------------------------------------------------------------------------
    std::vector<double> const& firingDates(size_t transition_id) const;

    //--------------------------------------------------------------------------
    //! \brief Total number of recorded firings across all transitions.
    //--------------------------------------------------------------------------
    size_t totalFirings() const;

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
    //! \brief Signal emitted for each firing of a transition of a Petri net
    //! or a timed net: its identifier, the date (simulation time) and the
    //! firing number k (1 for the first). The journal of the editor shows
    //! them (GRAFCET: see GrafcetEngine::firings()).
    Signal<size_t, double, size_t> onFired;

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
    //! \brief Give the engine what the user changed in the net (tokens,
    //! armed transitions).
    void readUserChanges();
    //! \brief Show the marking and the armed transitions of the engine in the
    //! net.
    void showMarking();

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
    //! \brief Engine running Petri nets and timed nets. Kept after the end of
    //! the simulation for its firing dates.
    std::unique_ptr<Engine> m_petri;
    //! \brief Settings of the next engine.
    Engine::Options m_options;
    //! \brief Marking shown in the places at the last step: a difference is a
    //! change made by the user.
    Marking m_shown;
    //! \brief Initial marking stored at simulation start.
    std::vector<size_t> m_initial_tokens;
    //! \brief Simulation time since start().
    double m_sim_time = 0.0;
    //! \brief True if there are receptivity parsing errors.
    bool m_has_receptivity_errors = false;
};

} // namespace tpne

#endif
