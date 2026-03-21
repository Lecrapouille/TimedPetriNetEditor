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

#ifndef DOCUMENT_HPP
#define DOCUMENT_HPP

#include "TimedPetriNetEditor/PetriNet.hpp"
#include "Net/Simulation.hpp"
#include "Net/Receptivities.hpp"
#include "Utils/Messages.hpp"
#include <string>
#include <vector>
#include <memory>

namespace tpne {

// *****************************************************************************
//! \brief A Document represents a single file that can contain multiple Petri
//! nets (useful for GRAFCET GEMMA with Security, Production, Control graphs).
//! Each net has its own simulation but they share the same Sensors (inputs).
// *****************************************************************************
class Document
{
public:

    //--------------------------------------------------------------------------
    //! \brief A single net entry within a document, bundling the net with its
    //! simulation instance.
    //--------------------------------------------------------------------------
    struct ViewState
    {
        float scrolling_x = 0.0f;
        float scrolling_y = 0.0f;
        float zoom = 1.0f;
    };

    struct NetEntry
    {
        Net net;
        std::unique_ptr<Simulation> simulation;
        bool visible = true;
        ViewState view_state;

        NetEntry(TypeOfNet type, std::string const& name, Messages& messages)
            : net(type)
        {
            net.name = name;
            simulation = std::make_unique<Simulation>(net, messages);
        }

        // Move constructor needed for vector
        NetEntry(NetEntry&& other) noexcept
            : net(std::move(other.net)),
              simulation(std::move(other.simulation)),
              visible(other.visible)
        {}

        NetEntry& operator=(NetEntry&& other) noexcept
        {
            if (this != &other) {
                net = std::move(other.net);
                simulation = std::move(other.simulation);
                visible = other.visible;
            }
            return *this;
        }

        // Disable copy
        NetEntry(const NetEntry&) = delete;
        NetEntry& operator=(const NetEntry&) = delete;
    };

    //--------------------------------------------------------------------------
    //! \brief Constructor for a new empty document.
    //--------------------------------------------------------------------------
    Document(Messages& messages);

    //--------------------------------------------------------------------------
    //! \brief Check if document has unsaved modifications.
    //--------------------------------------------------------------------------
    bool isModified() const { return m_modified; }

    //--------------------------------------------------------------------------
    //! \brief Mark document as modified or saved.
    //--------------------------------------------------------------------------
    void setModified(bool modified) { m_modified = modified; }

    //--------------------------------------------------------------------------
    //! \brief Get the file path associated with this document.
    //--------------------------------------------------------------------------
    std::string const& filepath() const { return m_filepath; }

    //--------------------------------------------------------------------------
    //! \brief Set the file path for this document.
    //--------------------------------------------------------------------------
    void setFilepath(std::string const& path) { m_filepath = path; }

    //--------------------------------------------------------------------------
    //! \brief Get the document title (filename or "Untitled").
    //--------------------------------------------------------------------------
    std::string title() const;

    //--------------------------------------------------------------------------
    //! \brief Get access to all net entries in this document.
    //--------------------------------------------------------------------------
    std::vector<std::unique_ptr<NetEntry>>& nets() { return m_nets; }
    std::vector<std::unique_ptr<NetEntry>> const& nets() const { return m_nets; }

    //--------------------------------------------------------------------------
    //! \brief Get the number of nets in this document.
    //--------------------------------------------------------------------------
    size_t netCount() const { return m_nets.size(); }

    //--------------------------------------------------------------------------
    //! \brief Add a new net to the document.
    //! \param[in] type The type of Petri net (GRAFCET, TimedPetri, etc.)
    //! \param[in] name The name of the net.
    //! \return Reference to the newly created NetEntry.
    //--------------------------------------------------------------------------
    NetEntry& addNet(TypeOfNet type, std::string const& name);

    //--------------------------------------------------------------------------
    //! \brief Remove a net from the document.
    //! \param[in] index The index of the net to remove.
    //! \return True if removed successfully.
    //--------------------------------------------------------------------------
    bool removeNet(size_t index);

    //--------------------------------------------------------------------------
    //! \brief Get a specific net entry by index.
    //--------------------------------------------------------------------------
    NetEntry& getNet(size_t index);
    NetEntry const& getNet(size_t index) const;

    //--------------------------------------------------------------------------
    //! \brief Find a net by name.
    //! \return Pointer to the NetEntry or nullptr if not found.
    //--------------------------------------------------------------------------
    NetEntry* findNet(std::string const& name);

    //--------------------------------------------------------------------------
    //! \brief Get/set the active net index for this document.
    //--------------------------------------------------------------------------
    size_t activeNetIndex() const { return m_active_net_index; }
    void setActiveNetIndex(size_t index);

    //--------------------------------------------------------------------------
    //! \brief Get the currently active net entry.
    //--------------------------------------------------------------------------
    NetEntry& activeNet();
    NetEntry const& activeNet() const;

    //--------------------------------------------------------------------------
    //! \brief Start simulation for all nets in this document.
    //--------------------------------------------------------------------------
    void startAllSimulations();

    //--------------------------------------------------------------------------
    //! \brief Stop simulation for all nets in this document.
    //--------------------------------------------------------------------------
    void stopAllSimulations();

    //--------------------------------------------------------------------------
    //! \brief Check if any simulation is running.
    //--------------------------------------------------------------------------
    bool isAnySimulationRunning() const;

    //--------------------------------------------------------------------------
    //! \brief Step all simulations forward by dt seconds.
    //--------------------------------------------------------------------------
    void stepAllSimulations(float dt);

    //--------------------------------------------------------------------------
    //! \brief Register all nets in this document to the global NetRegistry.
    //! This enables cross-graph references in receptivities.
    //--------------------------------------------------------------------------
    void registerNets();

    //--------------------------------------------------------------------------
    //! \brief Unregister all nets from the global NetRegistry.
    //--------------------------------------------------------------------------
    void unregisterNets();

    //--------------------------------------------------------------------------
    //! \brief Whether the document is open (for tab close handling).
    //--------------------------------------------------------------------------
    bool open = true;

private:

    Messages& m_messages;
    std::string m_filepath;
    std::vector<std::unique_ptr<NetEntry>> m_nets;
    size_t m_active_net_index = 0;
    bool m_modified = false;
};

} // namespace tpne

#endif // DOCUMENT_HPP
