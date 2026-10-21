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

#include "PetriNet/GrafcetEngine.hpp"
#include "PetriNet/PetriNet.hpp"

namespace tpne {

//! \brief Maximum number of evolutions in one cycle before declaring that the
//! graphs never become stable (a loop of transitions always true).
static constexpr size_t MAX_EVOLUTIONS = 256u;

//! \brief Default values returned for unknown graphs.
static GrafcetEngine::ActionState const NO_ACTION_STATE;
static std::vector<size_t> const NO_MARKING;

//------------------------------------------------------------------------------
//! \brief True if all the steps upstream of the transition are active.
static bool isEnabled(Transition const& transition)
{
    if (transition.arcsIn.empty())
        return false;
    for (Arc const* arc : transition.arcsIn)
    {
        if (static_cast<Place const&>(arc->from).tokens == 0u)
            return false;
    }
    return true;
}

//------------------------------------------------------------------------------
void GrafcetEngine::warn(std::string const& message) const
{
    if (onWarning && m_warned.insert(message).second)
        onWarning(message);
}

//------------------------------------------------------------------------------
void GrafcetEngine::graphs(std::vector<Net*> const& nets)
{
    if (m_running)
        stop();
    m_nets = nets;
    refreshInputs(true);
}

//------------------------------------------------------------------------------
void GrafcetEngine::refreshInputs(bool const keep_values)
{
    // With keep_values, values given by the caller are kept; else, and for new
    // inputs, the initial value declared by the net, else 0.
    std::map<std::string, int> inputs;
    for (Net const* net : m_nets)
    {
        for (auto const& it : net->inputs)
            inputs.emplace(it.first, it.second);
        for (auto const& transition : net->transitions())
        {
            for (auto const& name : Receptivity::inputs(transition.caption))
                inputs.emplace(name, 0);
        }
        for (auto const& place : net->places())
        {
            for (auto const& action : place.actions)
            {
                for (auto const& name : Receptivity::inputs(action.condition))
                    inputs.emplace(name, 0);
            }
        }
    }
    if (keep_values)
    {
        for (auto& it : inputs)
        {
            auto const old = m_inputs.find(it.first);
            if (old != m_inputs.end())
                it.second = old->second;
        }
    }
    m_inputs = std::move(inputs);
}

//------------------------------------------------------------------------------
int const* GrafcetEngine::input(std::string const& name)
{
    // std::map keeps the address of its values on insertion.
    return &m_inputs.emplace(name, 0).first->second;
}

//------------------------------------------------------------------------------
Place const* GrafcetEngine::step(std::string const& graph, size_t const id)
{
    Net* net = m_compiling;
    if (!graph.empty())
    {
        net = nullptr;
        for (Net* n : m_nets)
        {
            if (n->name == graph)
                net = n;
        }
        if (net == nullptr)
        {
            m_errors.push_back("Unknown graph '" + graph + "' in '" + graph + ":X" +
                               std::to_string(id) + "'");
            return nullptr;
        }
    }
    Place const* place = (net == nullptr) ? nullptr : net->findPlace(id);
    if (place == nullptr)
    {
        m_errors.push_back("Unknown step X" + std::to_string(id) +
                           (graph.empty() ? "" : " in graph '" + graph + "'"));
    }
    return place;
}

//------------------------------------------------------------------------------
bool GrafcetEngine::start()
{
    if (m_running)
        stop();

    m_errors.clear();
    m_warned.clear();
    m_graphs.clear();
    m_graphs.resize(m_nets.size());
    // A run starts from the initial values of the inputs: edges are seen
    // from there (an input set to 1 by the caller after start() rises).
    refreshInputs(false);

    for (size_t g = 0u; g < m_nets.size(); ++g)
    {
        Net& net = *m_nets[g];
        Graph& graph = m_graphs[g];
        graph.net = &net;
        net.generateArcsInArcsOut();
        m_compiling = &net;

        auto& transitions = net.transitions();
        graph.receptivities.resize(transitions.size());
        graph.transition_timers.assign(transitions.size(), 0.0f);
        for (size_t t = 0u; t < transitions.size(); ++t)
        {
            size_t const errors = m_errors.size();
            if (!graph.receptivities[t].compile(transitions[t].caption, *this))
                m_errors.push_back(graph.receptivities[t].error());
            for (size_t e = errors; e < m_errors.size(); ++e)
                m_errors[e] = net.name + ": " + transitions[t].key + ": " + m_errors[e];
            if (transitions[t].arcsIn.empty())
            {
                m_errors.push_back(net.name + ": " + transitions[t].key +
                                   " has no upstream step");
            }
        }

        auto const& places = net.places();
        graph.conditions.resize(places.size());
        graph.actions.resize(places.size());
        graph.initial.resize(places.size());
        graph.was_active.assign(places.size(), false);
        graph.step_time.assign(places.size(), 0.0f);
        for (size_t p = 0u; p < places.size(); ++p)
        {
            graph.initial[p] = places[p].tokens;
            graph.actions[p].assign(places[p].actions.size(), ActionState{});
            graph.conditions[p].resize(places[p].actions.size());
            for (size_t a = 0u; a < places[p].actions.size(); ++a)
            {
                size_t const errors = m_errors.size();
                if (!graph.conditions[p][a].compile(places[p].actions[a].condition, *this))
                    m_errors.push_back(graph.conditions[p][a].error());
                for (size_t e = errors; e < m_errors.size(); ++e)
                {
                    m_errors[e] = net.name + ": " + places[p].key + ": action " +
                        places[p].actions[a].name + ": " + m_errors[e];
                }
            }
        }
    }
    m_compiling = nullptr;

    if (!m_errors.empty())
    {
        m_graphs.clear();
        return false;
    }

    m_outputs.clear();
    m_stored.clear();
    for (Net const* net : m_nets)
    {
        for (auto const& place : net->places())
        {
            for (auto const& action : place.actions)
            {
                if (action.forcings.empty())
                    m_outputs[action.name] = false;
            }
        }
    }
    m_firings.clear();
    m_time = 0.0f;
    m_firing_count = 0u;
    m_running = true;
    return true;
}

//------------------------------------------------------------------------------
void GrafcetEngine::stop()
{
    for (Graph& graph : m_graphs)
    {
        auto& places = graph.net->places();
        for (size_t p = 0u; (p < places.size()) && (p < graph.initial.size()); ++p)
            places[p].tokens = graph.initial[p];
        graph.net->resetReceptivies();
    }
    m_graphs.clear();
    m_firings.clear();
    for (auto& it : m_outputs)
        it.second = false;
    m_stored.clear();
    m_running = false;
}

//------------------------------------------------------------------------------
void GrafcetEngine::reset()
{
    stop();
    start();
}

//------------------------------------------------------------------------------
void GrafcetEngine::setInput(std::string const& name, int const value)
{
    m_inputs[name] = value;
}

//------------------------------------------------------------------------------
int GrafcetEngine::inputValue(std::string const& name) const
{
    auto const it = m_inputs.find(name);
    return (it == m_inputs.end()) ? 0 : it->second;
}

//------------------------------------------------------------------------------
bool GrafcetEngine::output(std::string const& name) const
{
    auto const it = m_outputs.find(name);
    return (it != m_outputs.end()) && it->second;
}

//------------------------------------------------------------------------------
void GrafcetEngine::step(float const dt)
{
    if (!m_running)
        return;

    m_time += dt;
    m_firings.clear();

    // Let time pass for the time operators (3s/X2) and the delays of
    // transitions.
    for (Graph& graph : m_graphs)
    {
        auto& transitions = graph.net->transitions();
        for (size_t t = 0u; t < transitions.size(); ++t)
        {
            graph.receptivities[t].tick(dt);
            if (transitions[t].delay > 0.0f)
            {
                bool const ready = isEnabled(transitions[t]) && graph.receptivities[t].evaluate();
                graph.transition_timers[t] = ready ? graph.transition_timers[t] + dt : 0.0f;
            }
        }
        for (auto& conditions : graph.conditions)
        {
            for (auto& condition : conditions)
                condition.tick(dt);
        }
    }

    applyForcings();
    if (!evolve())
    {
        warn("The GRAFCET does not reach a stable situation after " +
             std::to_string(MAX_EVOLUTIONS) + " evolutions: is there a loop of "
             "transitions always true?");
    }
    computeActions(dt);
}

//------------------------------------------------------------------------------
void GrafcetEngine::applyForcings()
{
    for (Graph& graph : m_graphs)
    {
        graph.forced = false;
        graph.frozen = false;
    }

    // Forcing orders are continuous actions of the active steps. They are
    // read from the situation at the beginning of the cycle.
    for (Graph& source : m_graphs)
    {
        auto const& places = source.net->places();
        for (size_t p = 0u; p < places.size(); ++p)
        {
            if (places[p].tokens == 0u)
                continue;
            for (size_t a = 0u; a < places[p].actions.size(); ++a)
            {
                Action const& action = places[p].actions[a];
                if (action.forcings.empty() || !source.conditions[p][a].evaluate())
                    continue;
                for (Forcing const& forcing : action.forcings)
                {
                    Graph* target = findGraph(forcing.targetNet);
                    if (target == nullptr)
                    {
                        warn("Forcing target '" + forcing.targetNet + "' not found");
                        continue;
                    }
                    target->forced = true;
                    auto& target_places = target->net->places();
                    switch (forcing.type)
                    {
                    case Forcing::Type::Init:
                        for (size_t i = 0u; i < target_places.size(); ++i)
                            target_places[i].tokens = target->initial[i];
                        break;
                    case Forcing::Type::Freeze:
                        target->frozen = true;
                        break;
                    case Forcing::Type::Empty:
                        for (auto& place : target_places)
                            place.tokens = 0u;
                        break;
                    case Forcing::Type::Steps:
                        for (auto& place : target_places)
                            place.tokens = 0u;
                        for (size_t id : forcing.steps)
                        {
                            Place* place = target->net->findPlace(id);
                            if (place != nullptr)
                                place->tokens = 1u;
                            else
                                warn("Forcing step X" + std::to_string(id) +
                                     " not found in " + forcing.targetNet);
                        }
                        break;
                    }
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
bool GrafcetEngine::evolve()
{
    struct Fire { size_t graph; size_t transition; };
    std::vector<Fire> fire;

    for (size_t evolution = 0u; evolution < MAX_EVOLUTIONS; ++evolution)
    {
        // 1. Transitions that can fire, all graphs at once.
        fire.clear();
        for (size_t g = 0u; g < m_graphs.size(); ++g)
        {
            Graph& graph = m_graphs[g];
            auto& transitions = graph.net->transitions();
            for (size_t t = 0u; t < transitions.size(); ++t)
            {
                Transition& transition = transitions[t];
                transition.receptivity = graph.receptivities[t].evaluate();
                if (graph.forced || !transition.receptivity || !isEnabled(transition))
                    continue;
                if ((transition.delay > 0.0f) &&
                    (graph.transition_timers[t] < transition.delay))
                    continue;
                fire.push_back({ g, t });
            }
        }

        // 2. The instant is over: edges of inputs and steps are consumed.
        for (Graph& graph : m_graphs)
        {
            for (auto& receptivity : graph.receptivities)
                receptivity.endOfInstant();
        }

        if (fire.empty())
            return true;

        // 3. Fire together: deactivate all upstream steps, then activate all
        // downstream steps (a step deactivated and activated stays active).
        for (Fire const& f : fire)
        {
            Transition& transition = m_graphs[f.graph].net->transitions()[f.transition];
            for (Arc* arc : transition.arcsIn)
                static_cast<Place&>(arc->from).tokens = 0u;
        }
        for (Fire const& f : fire)
        {
            Graph& graph = m_graphs[f.graph];
            Transition& transition = graph.net->transitions()[f.transition];
            for (Arc* arc : transition.arcsOut)
                static_cast<Place&>(arc->to).tokens = 1u;
            graph.transition_timers[f.transition] = 0.0f;
            m_firings.push_back({ f.graph, transition.id });
            ++m_firing_count;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
void GrafcetEngine::computeActions(float const dt)
{
    for (auto& it : m_outputs)
        it.second = false;

    for (Graph& graph : m_graphs)
    {
        auto const& places = graph.net->places();
        for (size_t p = 0u; p < places.size(); ++p)
        {
            bool const active = (places[p].tokens > 0u);
            bool const was_active = graph.was_active[p];
            bool const rising = active && !was_active;
            bool const falling = !active && was_active;
            graph.step_time[p] = (active && was_active) ? (graph.step_time[p] + dt) : 0.0f;
            float const step_time = graph.step_time[p];

            for (size_t a = 0u; a < places[p].actions.size(); ++a)
            {
                Action const& action = places[p].actions[a];
                ActionState& state = graph.actions[p][a];
                bool const condition = graph.conditions[p][a].evaluate();

                switch (action.qualifier)
                {
                case Action::Qualifier::N:
                    // Continuous action: runs while the step is active (and its
                    // condition is true).
                    state.active = active && condition;
                    break;

                case Action::Qualifier::S:
                    // Stored action on activation: stays on until reset.
                    if (rising && condition)
                        m_stored[action.name] = true;
                    state.active = active && m_stored[action.name];
                    break;

                case Action::Qualifier::R:
                    // Reset of a stored action.
                    if (active && condition)
                        m_stored[action.name] = false;
                    state.active = false;
                    break;

                case Action::Qualifier::D:
                    // Delayed: runs once the step has been active for duration.
                    state.timer = active ? step_time : 0.0f;
                    state.active = active && condition && (state.timer >= action.duration);
                    break;

                case Action::Qualifier::L:
                    // Limited: runs during the first duration of the step.
                    state.timer = active ? step_time : 0.0f;
                    state.active = active && condition && (state.timer < action.duration);
                    break;

                case Action::Qualifier::DS:
                    // Delayed then stored: stored if the step is still active
                    // after duration.
                    state.timer = active ? step_time : 0.0f;
                    if (active && condition && (state.timer >= action.duration))
                        m_stored[action.name] = true;
                    state.active = m_stored[action.name];
                    break;

                case Action::Qualifier::SD:
                    // Stored then delayed: the delay starts at activation and
                    // goes on even if the step is deactivated.
                    if (rising && condition)
                    {
                        state.timer = 0.0f;
                        state.pending = true;
                    }
                    else if (state.pending)
                    {
                        state.timer += dt;
                    }
                    if (state.pending && (state.timer >= action.duration))
                    {
                        m_stored[action.name] = true;
                        state.pending = false;
                    }
                    state.active = m_stored[action.name];
                    break;

                case Action::Qualifier::SL:
                    // Stored and limited: on at activation for duration, even
                    // if the step is deactivated.
                    if (rising && condition)
                    {
                        state.timer = 0.0f;
                        state.pending = true;
                    }
                    else if (state.pending)
                    {
                        state.timer += dt;
                    }
                    if (state.pending && (state.timer >= action.duration))
                        state.pending = false;
                    state.active = state.pending;
                    break;

                case Action::Qualifier::P:
                    // Pulse on activation (P1).
                    state.active = rising && condition;
                    break;

                case Action::Qualifier::P0:
                    // Pulse on deactivation.
                    state.active = falling && condition;
                    break;
                }

                if (state.active && action.forcings.empty() &&
                    (action.qualifier != Action::Qualifier::R))
                {
                    m_outputs[action.name] = true;
                }
            }
            graph.was_active[p] = active;
        }

        for (auto& conditions : graph.conditions)
        {
            for (auto& condition : conditions)
            {
                condition.endOfInstant();
                condition.endOfCycle();
            }
        }
        for (auto& receptivity : graph.receptivities)
            receptivity.endOfCycle();
    }

    for (auto const& it : m_stored)
    {
        if (it.second)
            m_outputs[it.first] = true;
    }
}

//------------------------------------------------------------------------------
GrafcetEngine::Graph const* GrafcetEngine::findGraph(Net const& net) const
{
    for (Graph const& graph : m_graphs)
    {
        if (graph.net == &net)
            return &graph;
    }
    return nullptr;
}

//------------------------------------------------------------------------------
GrafcetEngine::Graph* GrafcetEngine::findGraph(std::string const& name)
{
    for (Graph& graph : m_graphs)
    {
        if (graph.net->name == name)
            return &graph;
    }
    return nullptr;
}

//------------------------------------------------------------------------------
//! \brief Index of a place in its net, or the size of the net if unknown.
static size_t placeIndex(Net const& net, size_t const place_id)
{
    auto const& places = net.places();
    for (size_t i = 0u; i < places.size(); ++i)
    {
        if (places[i].id == place_id)
            return i;
    }
    return places.size();
}

//------------------------------------------------------------------------------
GrafcetEngine::ActionState const&
GrafcetEngine::actionState(Net const& net, size_t const place_id, size_t const action) const
{
    Graph const* graph = findGraph(net);
    if (graph == nullptr)
        return NO_ACTION_STATE;
    size_t const p = placeIndex(net, place_id);
    if ((p >= graph->actions.size()) || (action >= graph->actions[p].size()))
        return NO_ACTION_STATE;
    return graph->actions[p][action];
}

//------------------------------------------------------------------------------
bool GrafcetEngine::isFrozen(Net const& net) const
{
    Graph const* graph = findGraph(net);
    return (graph != nullptr) && graph->frozen;
}

//------------------------------------------------------------------------------
bool GrafcetEngine::isForced(Net const& net) const
{
    Graph const* graph = findGraph(net);
    return (graph != nullptr) && graph->forced;
}

//------------------------------------------------------------------------------
std::vector<size_t> const& GrafcetEngine::initialMarking(Net const& net) const
{
    Graph const* graph = findGraph(net);
    return (graph == nullptr) ? NO_MARKING : graph->initial;
}

//------------------------------------------------------------------------------
Receptivity const* GrafcetEngine::receptivity(Net const& net, size_t const transition_id) const
{
    Graph const* graph = findGraph(net);
    if (graph == nullptr)
        return nullptr;
    auto const& transitions = net.transitions();
    for (size_t t = 0u; t < transitions.size(); ++t)
    {
        if (transitions[t].id == transition_id)
            return &graph->receptivities[t];
    }
    return nullptr;
}

//------------------------------------------------------------------------------
float GrafcetEngine::transitionTimer(Net const& net, size_t const transition_id) const
{
    Graph const* graph = findGraph(net);
    if (graph == nullptr)
        return 0.0f;
    auto const& transitions = net.transitions();
    for (size_t t = 0u; t < transitions.size(); ++t)
    {
        if (transitions[t].id == transition_id)
            return graph->transition_timers[t];
    }
    return 0.0f;
}

//------------------------------------------------------------------------------
float GrafcetEngine::stepTime(Net const& net, size_t const place_id) const
{
    Graph const* graph = findGraph(net);
    if (graph == nullptr)
        return 0.0f;
    size_t const p = placeIndex(net, place_id);
    return (p < graph->step_time.size()) ? graph->step_time[p] : 0.0f;
}

} // namespace tpne
