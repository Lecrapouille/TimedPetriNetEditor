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
        m_grafcet.step(dt);
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
