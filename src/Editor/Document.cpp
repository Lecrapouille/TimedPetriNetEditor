// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

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
      visible(other.visible)
{}

//------------------------------------------------------------------------------
Document::NetEntry& Document::NetEntry::operator=(NetEntry&& other) noexcept
{
    if (this != &other) {
        net = std::move(other.net);
        simulation = std::move(other.simulation);
        visible = other.visible;
    }
    return *this;
}

//------------------------------------------------------------------------------
Document::Document(Messages& messages)
    : m_messages(messages)
{
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
    NetRegistry::instance().registerNet(name, &m_nets.back()->net);
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

    NetRegistry::instance().unregisterNet(m_nets[index]->net.name);

    m_nets.erase(m_nets.begin() + static_cast<ptrdiff_t>(index));

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
void Document::startAllSimulations() const
{
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
void Document::stepAllSimulations(float dt) const
{
    for (auto const& entry : m_nets)
    {
        entry->simulation->step(dt);
    }
}

//------------------------------------------------------------------------------
void Document::registerNets() const
{
    for (auto const& entry : m_nets)
    {
        NetRegistry::instance().registerNet(entry->net.name, &entry->net);
    }
}

//------------------------------------------------------------------------------
void Document::unregisterNets() const
{
    for (auto const& entry : m_nets)
    {
        NetRegistry::instance().unregisterNet(entry->net.name);
    }
}

} // namespace tpne
