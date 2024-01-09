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
Simulation::Simulation(Net& net, Messages& messages)
    : m_net(net), m_messages(messages)
{
    m_timed_tokens.reserve(128u);
}

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
        stateStarting();
        break;
    case Simulation::State::Simulating:
        stateSimulating(dt);
        break;
    case Simulation::State::Halting:
        stateHalting();
        break;
    default:
        std::cerr << "Odd state in the state machine doing the "
                  << "animation of the Petri net." << std::endl;
        exit(1);
        break;
    }
}

//------------------------------------------------------------------------------
void Simulation::stateStarting()
{
    // The user has requested to start the simulation ?
    if (!running)
        return;

    // Dummy Petri net: nothing to simulate
    if (m_net.isEmpty())
    {
        m_messages.setWarning("Starting simulation request ignored because the net is empty");
        running = false;
        return ;
    }

    // Reset states of the simulator
    m_net.generateArcsInArcsOut();
    m_initial_tokens = m_net.tokens();
    shuffle_transitions(true);
    m_timed_tokens.clear();
    for (auto& a: m_net.arcs())
    {
        a.count = 0u;
    }
    m_net.resetReceptivies();

    // Check for GRAFCET if boolean expressions in transitivities have
    // not syntaxical errors.
    if (m_net.type() == TypeOfNet::GRAFCET)
    {
        Sensors::instance().clear();
        m_receptivities.clear();
        m_receptivities.resize(m_net.transitions().size());
        for (auto const& it: m_net.transitions())
        {
            std::string error = m_receptivities[it.id].compile(it.caption, m_net);
            if (!error.empty())
            {
                m_messages.setWarning(error);
                running = false;
                return ;
            }
        }
    }

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
void Simulation::stateHalting()
{
    m_messages.setInfo("Simulation has ended!");
    std::cout << current_time() << "Simulation has ended!"
                << std::endl << std::endl;

    // Restore burnt tokens from the simulation
    m_net.tokens(m_initial_tokens);
    m_net.resetReceptivies();
    m_receptivities.clear();
    m_timed_tokens.clear();
    Sensors::instance().clear();
    m_state = Simulation::State::Idle;
}

//------------------------------------------------------------------------------
void Simulation::stateSimulating(float const dt)
{
    bool burnt = false;
    size_t burning = 0u;

    // The user has requested to halt the simulation ?
    if (!running)
    {
        m_state = Simulation::State::Halting;
        return ;
    }

    // Interpret the code of receptivities.
    if (m_net.type() == TypeOfNet::GRAFCET)
    {
        // TODO if (Sensors::modified) {
        for (auto& t: m_net.transitions())
            t.receptivity = m_receptivities[t.id].evaluate();
        // }  Sensors::modified = false;
    }

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

        burning = 0u;
        for (auto& trans: transitions)
        {
            // The theory would burn the maximum possibe of tokens that
            // we can in a single action but we can also try to burn tokens
            // one by one and randomize the transitions.
            size_t tokens = 0u;
            if (trans->isFireable())
            {
                tokens = trans->countBurnableTokens();
            }

            if (tokens > 0u)
            {
                assert(tokens <= Net::Settings::maxTokens);
                //TODO trans->fading.restart();

                burning = tokens; // keep iterating on this loop
                burnt = true; // At least one place has been fired

                // Transition source
                if (trans->isInput())
                {
                    burning = 0u;
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
    } while (burning != 0u);

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
                m_timed_tokens.emplace_back(a, a.count, m_net.type());
                //TODO a.fading.restart();
                a.count = 0u;
            }
        }
    }

    // Tokens Transition --> Places are transitioning.
    if (m_timed_tokens.size() > 0u)
    {
        size_t i = m_timed_tokens.size();
        while (i--)
        {
            TimedToken& an = m_timed_tokens[i];
            if (an.update(dt))
            {
                // Animated token reached its ddestination: Place
                std::cout << current_time()
                            << "Place " << an.arc->to.caption
                            << " got " << an.tokens << " token"
                            << (an.tokens == 1u ? "" : "s")
                            << std::endl;

                // Drop the number of tokens it was carrying.
                an.arc->tokensOut() += an.tokens;

                // Transition source. In Petri net we keep using the mouse to
                // fire source transition to generate a single token by mouse
                // click while in other mode the transition fires once the
                // animation ends.
                if (m_net.type() != TypeOfNet::PetriNet)
                {
                    Transition& t = reinterpret_cast<Transition&>(an.arc->from);
                    if (t.isInput())
                    {
                        t.receptivity = true;
                    }
                }

                // Remove it
                m_timed_tokens[i] = m_timed_tokens[m_timed_tokens.size() - 1u];
                m_timed_tokens.pop_back();
            }
        }
    }
    else if ((m_net.type() != TypeOfNet::PetriNet) && (m_net.type() != TypeOfNet::GRAFCET))
    {
        std::cout << current_time() << "The simulation cannot burn tokens."
                    << std::endl;
        running = false;
        m_state = Simulation::State::Halting;
    }
}

} // namespace tpne