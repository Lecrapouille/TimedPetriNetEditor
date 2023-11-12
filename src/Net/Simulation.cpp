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

#  include "Net/Simulation.hpp"

namespace tpne {

//------------------------------------------------------------------------------
static const char* current_time() // FIXME defined several times
{
    static char buffer[32];

    time_t current_time = ::time(nullptr);
    strftime(buffer, sizeof (buffer), "[%H:%M:%S] ", localtime(&current_time));
    return buffer;
}

//------------------------------------------------------------------------------
Simulation::Simulation(Net& net, std::atomic<bool>& simulating, Messages& messages)
    : m_net(net), m_simulating(simulating), m_messages(messages)
{}

//------------------------------------------------------------------------------
std::vector<Transition*> const& Simulation::shuffle_transitions(bool const reset)
{
    static std::random_device rd;
    static std::mt19937 g(rd());

    if (reset)
    {
        // Avoid useless copy at each iteration of the simulation. Do it
        // once at the begining of the simulation.
        m_shuffled_transitions.clear();
        m_shuffled_transitions.reserve(m_net.transitions().size());
        for (auto& trans: m_net.transitions())
            m_shuffled_transitions.push_back(&trans);
    }
    std::shuffle(m_shuffled_transitions.begin(),
                 m_shuffled_transitions.end(), g);
    return m_shuffled_transitions;
}

//------------------------------------------------------------------------------
void Simulation::step(float const dt)
{
    switch (m_state)
    {
    case Simulation::State::Idle:
        starting();
        break;
    case Simulation::State::Simulating:
        simulating(dt);
        break;
    case Simulation::State::Halting:
        halting();
        break;
    default:
        std::cerr << "Odd state in the state machine doing the "
                  << "animation of the Petri net." << std::endl;
        exit(1);
        break;
    }
}

//------------------------------------------------------------------------------
void Simulation::starting()
{
    // The user has requested to start the simulation ?
    if (!m_simulating)
        return;

    // Dummy Petri net: nothing to simulate
    if (m_net.isEmpty())
    {
        m_messages.setWarning("Starting simulation request ignored because the net is empty");
        m_simulating = false;
        return ;
    }

#if 0 // TODO
    // Check for GRAFCET if boolean expressions in transitivities have
    // not syntaxical errors.
    if (m_net.type() == TypeOfNet::GRAFCET)
    {
        if (!m_net.hasValidTransitivities())
        {
            m_messages.setWarning("transitivites have syntax error");
            m_simulating = false;
            return ;
        }
    }
#endif

    // Reset states of the simulator
    m_initial_tokens = m_net.tokens();
    m_animated_tokens.clear();

    //
    std::cout << current_time() << "Simulation has started!" << std::endl;
    if (m_net.type() == TypeOfNet::PetriNet)
    {
        m_messages.setInfo(
            "Simulation has started!\n"
            "  Click on transitions for firing!\n"
            "  Press the key '+' on Places for adding tokens\n"
            "  Press the key '-' on Places for removing tokens");
    }
    else
    {
        m_messages.setInfo("Simulation has started!");
    }

    m_state = Simulation::State::Simulating;
}

//------------------------------------------------------------------------------
void Simulation::halting()
{
    m_messages.setInfo("Simulation has ended!");
    std::cout << current_time() << "Simulation has ended!"
                << std::endl << std::endl;

    // Restore burnt tokens from the simulation
    m_net.tokens(m_initial_tokens);
    m_animated_tokens.clear();
    m_state = Simulation::State::Idle;
}

//------------------------------------------------------------------------------
void Simulation::simulating(float const dt)
{
    bool burnt = false;
    bool burning = false;

    // The user has requested to halt the simulation ?
    if (!m_simulating)
    {
        m_state = Simulation::State::Halting;
        return ;
    }

#if 0 // TODO
    // Interpret the code of receptivities
    if (m_net.type() == TypeOfNet::GRAFCET)
    {
        for (auto& t: m_net.transitions())
            t.evaluate(m_net.m_sensors);
    }
#endif

    // For each transition check if it is activated (all incoming Places
    // have at least one token to burn. Note: since here we care Petri but
    // not Grafcet the transitivity is always true). If yes, we will burn
    // the maximum possible of tokens in a single step for the animation.
    // But in the aim to divide tokens the most kindly over the maximum
    // transitions possible we have to iterate and burn tokens one by one.
    do
    {
        // Randomize the order of fired transition.
        // TODO: filter the list to speed up ?
        auto& transitions = shuffle_transitions();

        burning = false;
        for (auto& trans: transitions)
        {
            // The theory would burn the maximum possibe of tokens that
            // we can in a single action but we can also try to burn tokens
            // one by one and randomize the transitions.
            size_t tokens = (Net::Settings::firing == Net::Settings::Fire::OneByOne)
                            ? size_t(trans->canFire()) // [0 .. 1] tokens
                            : trans->howManyTokensCanBurnt(); // [0 .. N] tokens

            if (tokens > 0u)
            {
                assert(tokens <= Net::Settings::maxTokens);
                //TODO trans->fading.restart();

                burning = true; // keep iterating on this loop
                burnt = true; // At least one place has been fired

                // Transition source
                if (trans->isInput())
                {
                    burning = false;
                    trans->receptivity = false;
                }
                else
                {
                    // Burn tokens on each predecessor Places
                    for (auto& a: trans->arcsIn)
                    {
                        // Burn tokens
                        size_t& tks = a->tokensIn();
                        assert(tks >= tokens);
                        tks = std::min(Net::Settings::maxTokens, tks - tokens);

                        // Invalidate transitions of previous places
                        if (m_net.type() == TypeOfNet::PetriNet)
                        {
                            Transition& tr = reinterpret_cast<Transition&>(a->to);
                            tr.receptivity = false;
                        }
                        //TODO a->fading.restart();
                    }
                }

                // Count the number of tokens for the animation
                for (auto& a: trans->arcsOut)
                {
                    a->count = std::min(Net::Settings::maxTokens, a->count + tokens);
                }
            }
        }
    } while (burning);

    // Create animated tokens with the correct number of tokens they are
    // carrying.
    if (burnt)
    {
        for (auto& a: m_net.arcs()) // FIXME: speedup: trans->arcsOut
        {
            if (a.count > 0u) // number of tokens carried by a single animation
            {
                std::cout << current_time()
                            << "Transition " << a.from.caption << " burnt "
                            << a.count << " token"
                            << (a.count == 1u ? "" : "s")
                            << std::endl;
                m_animated_tokens.push_back(TimedToken(a, a.count, m_net.type()));
                //TODO a.fading.restart();
                a.count = 0u;
            }
        }
    }

    // Tokens Transition --> Places are transitioning.
    if (m_animated_tokens.size() > 0u)
    {
        size_t i = m_animated_tokens.size();
        while (i--)
        {
            TimedToken& an = m_animated_tokens[i];
            if (an.update(dt))
            {
                // Animated token reached its ddestination: Place
                std::cout << current_time()
                            << "Place " << an.arc.to.caption
                            << " got " << an.tokens << " token"
                            << (an.tokens == 1u ? "" : "s")
                            << std::endl;

                // Drop the number of tokens it was carrying.
                an.arc.tokensOut() += an.tokens;

                // Transition source. In Petri net we keep using the mouse to
                // fire source transition to generate a single token by mouse
                // click while in other mode the transition fires once the
                // animation ends.
                if (m_net.type() != TypeOfNet::PetriNet)
                {
                    Transition& t = reinterpret_cast<Transition&>(an.arc.from);
                    if (t.isInput())
                    {
                        t.receptivity = true;
                    }
                }

                // Remove it
                m_animated_tokens[i] = m_animated_tokens[m_animated_tokens.size() - 1u];
                m_animated_tokens.pop_back();
            }
        }
    }
    else if ((m_net.type() != TypeOfNet::PetriNet) &&
                (m_net.type() != TypeOfNet::GRAFCET))
    {
        std::cout << current_time() << "The simulation cannot burn tokens."
                    << std::endl;
        m_simulating = false;
        m_state = Simulation::State::Halting;
    }
}

} // namespace tpne