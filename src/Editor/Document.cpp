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

#include "Editor/Document.hpp"
#include "PetriNet/Simulation.hpp"
#include "PetriNet/Grafcet.hpp"

#include <algorithm>
#include <set>
#include <cmath>
#include <cstdio>
#include <stdexcept>

namespace tpne {

//------------------------------------------------------------------------------
Document::NetEntry::NetEntry(TypeOfNet type, std::string const& name, Messages& messages)
    : net(type)
{
    net.name = name;
    simulation = std::make_unique<Simulation>(net);
    // Connect simulation signals to messages
    simulation->onInfo.connect([&messages](std::string const& msg) {
        messages.setInfo(msg);
    });
    simulation->onWarning.connect([&messages](std::string const& msg) {
        messages.setWarning(msg);
    });
    simulation->onError.connect([&messages](std::string const& msg) {
        messages.setError(msg);
    });
    simulation->onTrace.connect([&messages](std::string const& msg) {
        messages.setTrace(msg);
    });
}

//------------------------------------------------------------------------------
Document::NetEntry::~NetEntry() = default;

//------------------------------------------------------------------------------
Document::NetEntry::NetEntry(NetEntry&& other) noexcept
    : net(std::move(other.net)),
      simulation(std::move(other.simulation)),
      visible(other.visible),
      view_state(other.view_state),
      checked_revision(other.checked_revision)
{}

//------------------------------------------------------------------------------
Document::NetEntry& Document::NetEntry::operator=(NetEntry&& other) noexcept
{
    if (this != &other) {
        net = std::move(other.net);
        simulation = std::move(other.simulation);
        visible = other.visible;
        view_state = other.view_state;
        checked_revision = other.checked_revision;
    }
    return *this;
}

//------------------------------------------------------------------------------
Document::Document(Messages& messages)
    : m_messages(messages)
{
    m_grafcet.onWarning = [this](std::string const& msg) {
        m_messages.setWarning(msg);
    };
}

//------------------------------------------------------------------------------
std::string Document::title() const
{
    if (m_filepath.empty())
    {
        return "Untitled";
    }

    // Extract filename from path
    size_t pos = m_filepath.find_last_of("/\\");
    if (pos != std::string::npos)
    {
        return m_filepath.substr(pos + 1);
    }
    return m_filepath;
}

//------------------------------------------------------------------------------
Document::NetEntry& Document::addNet(TypeOfNet type, std::string const& name)
{
    m_nets.push_back(std::make_unique<NetEntry>(type, name, m_messages));
    registerNets();
    m_modified = true;
    return *m_nets.back();
}

//------------------------------------------------------------------------------
bool Document::removeNet(size_t index)
{
    if (index >= m_nets.size())
    {
        return false;
    }

    m_nets.erase(m_nets.begin() + static_cast<ptrdiff_t>(index));
    registerNets();

    if (m_active_net_index >= m_nets.size() && !m_nets.empty())
    {
        m_active_net_index = m_nets.size() - 1;
    }

    m_modified = true;
    return true;
}

//------------------------------------------------------------------------------
Document::NetEntry& Document::getNet(size_t index)
{
    if (index >= m_nets.size())
    {
        throw std::out_of_range("Net index out of range");
    }
    return *m_nets[index];
}

//------------------------------------------------------------------------------
Document::NetEntry const& Document::getNet(size_t index) const
{
    if (index >= m_nets.size())
    {
        throw std::out_of_range("Net index out of range");
    }
    return *m_nets[index];
}

//------------------------------------------------------------------------------
Document::NetEntry* Document::findNet(std::string const& name)
{
    for (auto const& entry : m_nets)
    {
        if (entry->net.name == name)
        {
            return entry.get();
        }
    }
    return nullptr;
}

//------------------------------------------------------------------------------
void Document::setActiveNetIndex(size_t index)
{
    if (index < m_nets.size())
    {
        m_active_net_index = index;
    }
}

//------------------------------------------------------------------------------
Document::NetEntry& Document::activeNet()
{
    if (m_nets.empty())
    {
        throw std::runtime_error("No nets in document");
    }
    return *m_nets[m_active_net_index];
}

//------------------------------------------------------------------------------
Document::NetEntry const& Document::activeNet() const
{
    if (m_nets.empty())
    {
        throw std::runtime_error("No nets in document");
    }
    return *m_nets[m_active_net_index];
}

//------------------------------------------------------------------------------
void Document::startAllSimulations()
{
    registerNets();
    for (auto const& entry : m_nets)
    {
        entry->simulation->start();
    }
}

//------------------------------------------------------------------------------
void Document::stopAllSimulations() const
{
    for (auto const& entry : m_nets)
    {
        entry->simulation->stop();
    }
}

//------------------------------------------------------------------------------
bool Document::isAnySimulationRunning() const
{
    for (auto const& entry : m_nets)
    {
        if (entry->simulation->isRunning())
        {
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
void Document::stepAllSimulations(float dt)
{
    for (auto const& entry : m_nets)
    {
        entry->simulation->step(dt);
    }

    // The GRAFCET graphs evolve together, once per cycle.
    if (m_grafcet.isRunning())
    {
        stepInputs(dt);
        m_grafcet.step(dt);
        traceGrafcet();
        record();

        // Pulses last one cycle.
        for (auto const& name : m_pulses)
            m_grafcet.setInput(name, 0);
        m_pulses.clear();
    }
    else
    {
        m_pulses.clear();
        m_driven.clear();
    }
}

//------------------------------------------------------------------------------
GrafcetInput& Document::describedInput(std::string const& name)
{
    if (GrafcetInput* input = findInput(m_info, name))
        return *input;
    GrafcetInput input;
    input.name = name;
    m_info.inputs.push_back(input);
    return m_info.inputs.back();
}

//------------------------------------------------------------------------------
GrafcetOutput& Document::describedOutput(std::string const& name)
{
    if (GrafcetOutput* output = findOutput(m_info, name))
        return *output;
    GrafcetOutput output;
    output.name = name;
    m_info.outputs.push_back(output);
    return m_info.outputs.back();
}

//------------------------------------------------------------------------------
void Document::pruneInfo()
{
    std::set<std::string> inputs;
    std::set<std::string> outputs;
    for (auto const& entry : m_nets)
    {
        if (entry->net.type() != TypeOfNet::GRAFCET)
            continue;
        for (auto const& transition : entry->net.transitions())
        {
            for (auto const& name : Receptivity::inputs(transition.caption))
                inputs.insert(name);
        }
        for (auto const& place : entry->net.places())
        {
            for (auto const& action : place.actions)
            {
                outputs.insert(action.name);
                for (auto const& name : Receptivity::inputs(action.condition))
                    inputs.insert(name);
            }
        }
    }
    auto& in = m_info.inputs;
    in.erase(std::remove_if(in.begin(), in.end(), [&inputs](GrafcetInput const& input) {
        return !inputs.count(input.name) && (input.kind == GrafcetInput::Kind::Switch) &&
               input.description.empty() && input.color.empty() && (input.initial == 0);
    }), in.end());
    auto& out = m_info.outputs;
    out.erase(std::remove_if(out.begin(), out.end(), [&outputs](GrafcetOutput const& output) {
        return !outputs.count(output.name) && (output.kind == GrafcetOutput::Kind::Light) &&
               output.description.empty();
    }), out.end());
    // Initial values of inputs nobody reads any more, and still 0.
    for (auto const& entry : m_nets)
    {
        auto& values = entry->net.inputs;
        for (auto it = values.begin(); it != values.end();)
        {
            if (!inputs.count(it->first) && (it->second == 0) && (findInput(m_info, it->first) == nullptr))
                it = values.erase(it);
            else
                ++it;
        }
    }
}

//------------------------------------------------------------------------------
void Document::setInput(std::string const& name, int const value)
{
    if (m_grafcet.inputValue(name) == value)
        return;
    m_grafcet.setInput(name, value);
    if (m_grafcet.isRunning())
    {
        char date[32];
        std::snprintf(date, sizeof(date), "t = %.2f  ", double(m_grafcet.time()));
        m_messages.setTrace(std::string(date) + "input " + name + " = " + std::to_string(value));
    }
}

//------------------------------------------------------------------------------
void Document::pulseInput(std::string const& name)
{
    setInput(name, 1);
    if (std::find(m_pulses.begin(), m_pulses.end(), name) == m_pulses.end())
        m_pulses.push_back(name);
}

//------------------------------------------------------------------------------
void Document::stepInputs(float const dt)
{
    float const time = m_grafcet.time();
    for (GrafcetInput const& input : m_info.inputs)
    {
        if ((input.kind == GrafcetInput::Kind::Clock) && (input.period > 0.0f))
        {
            // 1 during the first half of each period.
            float const phase = std::fmod(time + dt, input.period);
            m_grafcet.setInput(input.name, (phase < input.period * 0.5f) ? 1 : 0);
        }
        else if ((input.kind == GrafcetInput::Kind::Value) && !input.driven_by.empty())
        {
            // The value follows the output (heating, filling ...); the exact
            // value is kept here, the engine reads the integer part.
            int const current = m_grafcet.inputValue(input.name);
            auto it = m_driven.find(input.name);
            if ((it == m_driven.end()) || (int(it->second) != current))
                it = m_driven.insert_or_assign(input.name, float(current)).first;
            float& value = it->second;
            value += (m_grafcet.output(input.driven_by) ? input.rise : -input.fall) * dt;
            value = std::clamp(value, float(input.min), float(input.max));
            m_grafcet.setInput(input.name, int(value));
        }
    }
}

//------------------------------------------------------------------------------
void Document::sample(std::string const& name, Track::Kind const kind, size_t const graph,
                      int const value, float const time)
{
    auto it = m_track_index.find(name);
    if (it == m_track_index.end())
    {
        it = m_track_index.emplace(name, m_tracks.size()).first;
        m_tracks.push_back({ name, kind, graph, {}, false });
    }
    Track& track = m_tracks[it->second];
    if (track.changes.empty() || (track.changes.back().second != value))
    {
        // Bounded: the oldest half goes when full.
        static constexpr size_t MAX_CHANGES = 20000u;
        if (track.changes.size() >= MAX_CHANGES)
            track.changes.erase(track.changes.begin(), track.changes.begin() + MAX_CHANGES / 2u);
        track.changes.emplace_back(time, value);
    }
    track.used = track.used || (value != 0);
}

//------------------------------------------------------------------------------
void Document::record()
{
    // A new simulation (its time starts again): a new recording.
    float const time = m_grafcet.time();
    if (time < m_recorded_time)
    {
        m_tracks.clear();
        m_track_index.clear();
    }
    m_recorded_time = time;

    auto const& graphs = m_grafcet.graphs();
    for (size_t g = 0u; g < graphs.size(); ++g)
    {
        for (auto const& place : graphs[g]->places())
        {
            sample(graphs[g]->name + ":X" + std::to_string(place.id), Track::Kind::Step, g,
                   (place.tokens > 0u) ? 1 : 0, time);
        }
    }
    for (auto const& it : m_grafcet.inputs())
        sample(it.first, Track::Kind::Input, 0u, it.second, time);
    for (auto const& it : m_grafcet.outputs())
        sample(it.first, Track::Kind::Output, 0u, it.second ? 1 : 0, time);
}

//------------------------------------------------------------------------------
void Document::traceGrafcet()
{
    auto const& graphs = m_grafcet.graphs();
    for (auto const& firing : m_grafcet.firings())
    {
        if (firing.graph >= graphs.size())
            continue;
        Net const& net = *graphs[firing.graph];
        Transition const* transition = nullptr;
        for (auto const& t : net.transitions())
        {
            if (t.id == firing.transition)
                transition = &t;
        }
        if (transition == nullptr)
            continue;

        // "t = 1.20  Production: T1 [30s/X1]  X1 -> X2"
        std::string from, to;
        for (Arc const* arc : transition->arcsIn)
            from += (from.empty() ? "X" : ", X") + std::to_string(arc->from.id);
        for (Arc const* arc : transition->arcsOut)
            to += (to.empty() ? "X" : ", X") + std::to_string(arc->to.id);
        char date[32];
        std::snprintf(date, sizeof(date), "t = %.2f  ", double(m_grafcet.time()));
        m_messages.setTrace(std::string(date) + net.name + ": " + transition->key + " [" +
                            transition->caption + "]  " + from + " -> " + to);
    }
}

//------------------------------------------------------------------------------
void Document::registerNets()
{
    if (m_grafcet.isRunning())
        return;

    std::vector<Net*> graphs;
    for (auto const& entry : m_nets)
    {
        bool const grafcet = (entry->net.type() == TypeOfNet::GRAFCET);
        entry->simulation->grafcet(grafcet ? &m_grafcet : nullptr);
        if (grafcet)
            graphs.push_back(&entry->net);
    }
    m_grafcet.graphs(graphs);
}

//------------------------------------------------------------------------------
void Document::unregisterNets()
{
    m_grafcet.graphs({});
    for (auto const& entry : m_nets)
    {
        entry->simulation->grafcet(nullptr);
    }
}

} // namespace tpne
