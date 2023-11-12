//=============================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2023 Quentin Quadrat <lecrapouille@gmail.com>
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

#  include "TimedPetriNetEditor/PetriNet.hpp"
#  include "Net/TimedTokens.hpp"
#  include "Editor/Messages.hpp"

namespace tpne {

// *****************************************************************************
//! \brief
// *****************************************************************************
class Simulation
{
public:

    // *************************************************************************
    //! \brief State machine for the Petri net simulation.
    // *************************************************************************
    enum State
    {
        Idle,       //! Waiting the user request to start the simulation.
        Simulating, //! Simulation on-going: animate tokens.
        Halting,    //! Restore states after the simulation.
    };

    Simulation(Net& net, std::atomic<bool>& simulating, Messages& m_messages);
    void step(float const dt);

private:

    std::vector<Transition*> const& shuffle_transitions(bool const reset = false);
    void starting();
    void simulating(float const dt);
    void halting();

private:

    //! \brief The single Petri net we are simulating
    Net& m_net;
    //! \brief Set true for starting the simulation the Petri net and to
    //! maintain the simulation running. Set false to halt the simulation.
    std::atomic<bool>& m_simulating;
    //! \brief Used for error messages.
    Messages& m_messages;
    //! \brief List of shuffled Transitions.
    std::vector<Transition*> m_shuffled_transitions;
    //! \brief Animation of tokens when transitioning from Transitions to Places.
    std::vector<TimedToken> m_animated_tokens;
    //! \brief Memorize initial number of tokens in places.
    std::vector<size_t> m_initial_tokens;
    //! \brief State machine for the simulation.
    std::atomic<Simulation::State> m_state{Simulation::State::Idle};

    // FIXME: ajouter les receptivites ici ?????,
};

} // namespace tpne

#endif