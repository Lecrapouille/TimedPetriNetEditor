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

#ifndef GRAFCET_ENGINE_HPP
#  define GRAFCET_ENGINE_HPP

#  include "PetriNet/Grafcet.hpp"

#  include <functional>
#  include <map>
#  include <set>
#  include <string>
#  include <vector>

namespace tpne {

class Net;
class Place;
class Transition;

// *****************************************************************************
//! \brief Runs a GRAFCET made of one or several graphs (partial GRAFCETs, as
//! the three graphs of a GEMMA: safety, production, control), following the
//! evolution rules of IEC 60848:
//!   - a transition can fire when all its upstream steps are active and its
//!     receptivity is true; all the transitions that can fire fire together;
//!   - a step activated and deactivated at the same time stays active;
//!   - after a change of inputs the graphs evolve until they are stable
//!     (no transition can fire) before actions are computed: the
//!     intermediate situations are not seen by the actions;
//!   - forcing orders of the active steps have priority: a forced graph takes
//!     the forced situation and does not evolve during the cycle.
//!
//! The engine does not own the graphs: they are nets of type GRAFCET owned by
//! the caller (an editor document, a robot cell controller ...), which must
//! not change their structure while the engine runs. It does not depend on
//! any GUI. Inputs are written by the caller (setInput) before each cycle and
//! outputs (actions) are read after.
// *****************************************************************************
class GrafcetEngine : private Receptivity::Resolver
{
public:

    //! \brief Runtime state of one action of one step.
    struct ActionState
    {
        //! \brief The action runs (its output is true).
        bool active = false;
        //! \brief Time used by the D, L, SD, DS, SL qualifiers.
        float timer = 0.0f;
        //! \brief SD and SL: the delay runs after the step activation.
        bool pending = false;
    };

    //! \brief A transition fired during the last cycle.
    struct Firing
    {
        //! \brief Index of the graph in graphs().
        size_t graph;
        //! \brief Transition identifier in its net.
        size_t transition;
    };

    //--------------------------------------------------------------------------
    //! \brief Set the graphs to run. Stops the engine if running. The inputs
    //! declared by the nets (Net::inputs) and the ones read by receptivities
    //! are listed in inputs(), with their initial values.
    //--------------------------------------------------------------------------
    void graphs(std::vector<Net*> const& nets);
    std::vector<Net*> const& graphs() const { return m_nets; }

    //--------------------------------------------------------------------------
    //! \brief Compile receptivities and action conditions, remember the
    //! initial situation. Return false on errors (see errors()).
    //--------------------------------------------------------------------------
    bool start();

    //--------------------------------------------------------------------------
    //! \brief Stop and restore the initial situation of all graphs.
    //--------------------------------------------------------------------------
    void stop();

    //--------------------------------------------------------------------------
    //! \brief Restart from the initial situation, keeping the compiled code.
    //--------------------------------------------------------------------------
    void reset();

    bool isRunning() const { return m_running; }

    //--------------------------------------------------------------------------
    //! \brief Run one cycle: let dt seconds pass, apply forcing orders, fire
    //! transitions until the graphs are stable, compute actions.
    //--------------------------------------------------------------------------
    void step(float dt);

    //--------------------------------------------------------------------------
    //! \brief Errors of the last start() (syntax errors ...).
    //--------------------------------------------------------------------------
    std::vector<std::string> const& errors() const { return m_errors; }

    //--------------------------------------------------------------------------
    //! \brief Inputs read by the graphs, with their current values.
    //--------------------------------------------------------------------------
    std::map<std::string, int> const& inputs() const { return m_inputs; }
    void setInput(std::string const& name, int value);
    int inputValue(std::string const& name) const;

    //--------------------------------------------------------------------------
    //! \brief Output of each action name: true while one action of this name
    //! runs (continuous actions, stored actions, pulses).
    //--------------------------------------------------------------------------
    std::map<std::string, bool> const& outputs() const { return m_outputs; }
    bool output(std::string const& name) const;

    //--------------------------------------------------------------------------
    //! \brief Transitions fired during the last cycle.
    //--------------------------------------------------------------------------
    std::vector<Firing> const& firings() const { return m_firings; }

    //--------------------------------------------------------------------------
    //! \brief Time since start(), in seconds.
    //--------------------------------------------------------------------------
    float time() const { return m_time; }

    //--------------------------------------------------------------------------
    //! \brief Number of transitions fired since start().
    //--------------------------------------------------------------------------
    size_t firingCount() const { return m_firing_count; }

    //--------------------------------------------------------------------------
    //! \brief Per graph information (null net or unknown index: defaults).
    //--------------------------------------------------------------------------
    ActionState const& actionState(Net const& net, size_t place_id, size_t action) const;
    bool isFrozen(Net const& net) const;
    bool isForced(Net const& net) const;
    std::vector<size_t> const& initialMarking(Net const& net) const;
    Receptivity const* receptivity(Net const& net, size_t transition_id) const;
    float transitionTimer(Net const& net, size_t transition_id) const;
    //! \brief Time since the step became active (0 if inactive).
    float stepTime(Net const& net, size_t place_id) const;

    //--------------------------------------------------------------------------
    //! \brief Called with warnings found while running (unknown forcing
    //! target, unstable graphs ...). Can be empty.
    //--------------------------------------------------------------------------
    std::function<void(std::string const&)> onWarning;

private: // Receptivity::Resolver

    int const* input(std::string const& name) override;
    Place const* step(std::string const& graph, size_t id) override;

private:

    //! \brief Runtime data of one graph.
    struct Graph
    {
        Net* net = nullptr;
        //! \brief Receptivities by transition index in Net::transitions().
        std::vector<Receptivity> receptivities;
        //! \brief Delay timers (Transition::delay) by transition index.
        std::vector<float> transition_timers;
        //! \brief Conditions of the actions, by place index then action index.
        std::vector<std::vector<Receptivity>> conditions;
        //! \brief States of the actions, by place index then action index.
        std::vector<std::vector<ActionState>> actions;
        //! \brief Initial number of tokens, by place index.
        std::vector<size_t> initial;
        //! \brief Step active at the end of the previous cycle, by place index.
        std::vector<bool> was_active;
        //! \brief Time since the step is active, by place index.
        std::vector<float> step_time;
        //! \brief Forced during the current cycle (does not evolve).
        bool forced = false;
        //! \brief Frozen by a forcing order {*}.
        bool frozen = false;
    };

    Graph const* findGraph(Net const& net) const;
    Graph* findGraph(std::string const& name);
    void refreshInputs();
    void applyForcings();
    bool evolve();
    void computeActions(float dt);
    void warn(std::string const& message) const;

private:

    std::vector<Net*> m_nets;
    std::vector<Graph> m_graphs;
    std::map<std::string, int> m_inputs;
    std::map<std::string, bool> m_outputs;
    //! \brief Stored actions (S, R ...) by action name.
    std::map<std::string, bool> m_stored;
    std::vector<Firing> m_firings;
    std::vector<std::string> m_errors;
    //! \brief Warnings already given since start() (given once).
    mutable std::set<std::string> m_warned;
    //! \brief Graph whose receptivities are being compiled (for "Xn").
    Net* m_compiling = nullptr;
    float m_time = 0.0f;
    size_t m_firing_count = 0u;
    bool m_running = false;
};

} // namespace tpne

#endif
