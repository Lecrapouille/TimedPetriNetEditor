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

#  include "PetriNet/TimedTokens.hpp"
#  include "PetriNet/Signal.hpp"

#  include <atomic>
#  include <string>
#  include <vector>

namespace tpne {

class Net;
class Transition;
class Receptivity;

// *****************************************************************************
//! \brief
// *****************************************************************************
class Simulation
{
public:

    using TimedTokens = std::vector<TimedToken>;
    using Receptivities = std::map<size_t, Receptivity>;

    // *************************************************************************
    //! \brief State machine for the Petri net simulation.
    // *************************************************************************
    enum State
    {
        Idle,       //! Waiting the user request to start the simulation.
        Simulating, //! Simulation on-going: animate tokens.
        Halting,    //! Restore states after the simulation.
    };

    explicit Simulation(Net& net);
    void step(float const dt);
    bool generateSensors();
    bool generateSensor(Transition const& transition);
    inline TimedTokens const& timedTokens() const { return m_timed_tokens; }
    inline Receptivities const& receptivities() const { return m_receptivities; }
    //! \brief Store current marking as initial marking (for simulation reset)
    void storeInitialMarking();
    //! \brief Restore marking to the stored initial marking
    void restoreInitialMarking();
    //! \brief Check if a place is an initial step (had tokens at simulation start)
    //! \param[in] place_index Index of the place in the places vector
    //! \return true if the place had tokens when simulation started
    inline bool isInitialStep(size_t place_index) const
    {
        return place_index < m_initial_tokens.size() && m_initial_tokens[place_index] > 0;
    }
    //! \brief Check if initial marking has been stored
    inline bool hasInitialMarking() const { return !m_initial_tokens.empty(); }

private:

    std::vector<Transition*> const& shuffle_transitions(bool const reset = false);
    void stateStarting();
    void stateSimulating(float const dt);
    void stateHalting();
    //! \brief Update GRAFCET action states based on qualifiers (N, S, R, D, L, etc.)
    void updateActions(float const dt);
    //! \brief Reset a stored action by name (used by R qualifier)
    void resetStoredAction(std::string const& name);

public:

    //! \brief Set true for starting the simulation the Petri net and to
    //! maintain the simulation running. Set false to halt the simulation.
    std::atomic<bool> running{false};

    //! \brief When set to true then receptivities shall be recompiled.
    std::atomic<bool> compiled{false};

    //! \brief Signal emitted for informational messages.
    Signal<std::string const&> onInfo;
    //! \brief Signal emitted for warning messages.
    Signal<std::string const&> onWarning;
    //! \brief Signal emitted for error messages.
    Signal<std::string const&> onError;

private:

    //! \brief The single Petri net we are simulating
    Net& m_net;
    //! \brief List of shuffled Transitions.
    std::vector<Transition*> m_shuffled_transitions;
    //! \brief Animation of tokens when transitioning from Transitions to Places.
    TimedTokens m_timed_tokens;
    //! \brief Memorize initial number of tokens in places.
    std::vector<size_t> m_initial_tokens;
    //! \brief For GRAFCET boolean expressions in transitions.
    Receptivities m_receptivities;
    //! \brief State machine for the simulation.
    Simulation::State m_state{Simulation::State::Idle};
};

} // namespace tpne

#endif