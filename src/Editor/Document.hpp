// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef DOCUMENT_HPP
#define DOCUMENT_HPP

#include "PetriNet/PetriNet.hpp"
#include "Editor/Messages.hpp"

#include <map>
#include <string>
#include <vector>
#include <memory>

#  include "PetriNet/GrafcetEngine.hpp"

namespace tpne { class Simulation; }

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
        //! \brief Fit the net in its window during the next draws. Deferred
        //! because the size of the window is only known while drawing it, and
        //! the docking layout may still resize it during the first frames.
        int center_request = CENTER_FRAMES;
        static constexpr int CENTER_FRAMES = 4;
        //! \brief The net is fitted again each time its window changes size,
        //! until the user moves or zooms the view.
        bool auto_fit = true;
        //! \brief Size of the canvas at the previous frame (0: unknown).
        float canvas_width = 0.0f;
        float canvas_height = 0.0f;
        //! \brief Zoom factor asked by a menu or a key, applied when the net
        //! is drawn (1: none).
        float zoom_request = 1.0f;
    };

    struct NetEntry
    {
        Net net;
        std::unique_ptr<Simulation> simulation;
        bool visible = true;
        ViewState view_state;
        //! \brief Net revision (Net::modified.revision()) for which the GRAFCET
        //! receptivities have been checked. Avoids parsing them every frame.
        size_t checked_revision = size_t(-1);

        NetEntry(TypeOfNet type, std::string const& name, Messages& messages);
        ~NetEntry();

        // Move constructor needed for vector
        NetEntry(NetEntry&& other) noexcept;
        NetEntry& operator=(NetEntry&& other) noexcept;

        // Disable copy
        NetEntry(const NetEntry&) = delete;
        NetEntry& operator=(const NetEntry&) = delete;
    };

    //--------------------------------------------------------------------------
    //! \brief Constructor for a new empty document.
    //--------------------------------------------------------------------------
    explicit Document(Messages& messages);

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
    void stopAllSimulations() const;

    //--------------------------------------------------------------------------
    //! \brief Check if any simulation is running.
    //--------------------------------------------------------------------------
    bool isAnySimulationRunning() const;

    //--------------------------------------------------------------------------
    //! \brief Step all simulations forward by dt seconds.
    //--------------------------------------------------------------------------
    void stepAllSimulations(float dt);

    //--------------------------------------------------------------------------
    //! \brief Give the GRAFCET nets of this document to the GRAFCET engine
    //! shared by their simulations: they run together, which enables
    //! cross-graph receptivities (Safety:X3) and forcing orders. Call it after
    //! adding, removing or changing the type of nets.
    //--------------------------------------------------------------------------
    void registerNets();

    //--------------------------------------------------------------------------
    //! \brief Stop the GRAFCET engine and forget the nets (before clearing).
    //--------------------------------------------------------------------------
    void unregisterNets();

    //--------------------------------------------------------------------------
    //! \brief GRAFCET engine running all the GRAFCET nets of the document.
    //--------------------------------------------------------------------------
    GrafcetEngine& grafcet() { return m_grafcet; }
    GrafcetEngine const& grafcet() const { return m_grafcet; }

    //--------------------------------------------------------------------------
    //! \brief Description of the document and of its GRAFCET inputs and
    //! outputs (control panel), saved with the nets.
    //--------------------------------------------------------------------------
    DocumentInfo& info() { return m_info; }
    DocumentInfo const& info() const { return m_info; }

    //--------------------------------------------------------------------------
    //! \brief Description of the GRAFCET input of the given name, added (a
    //! switch) if the file did not describe it.
    //--------------------------------------------------------------------------
    GrafcetInput& describedInput(std::string const& name);

    //--------------------------------------------------------------------------
    //! \brief Description of the GRAFCET output of the given name, added (a
    //! light) if the file did not describe it.
    //--------------------------------------------------------------------------
    GrafcetOutput& describedOutput(std::string const& name);

    //--------------------------------------------------------------------------
    //! \brief Forget the GRAFCET inputs and outputs that no net uses any more
    //! and that say nothing more than the defaults (before saving).
    //--------------------------------------------------------------------------
    void pruneInfo();

    //--------------------------------------------------------------------------
    //! \brief GRAFCET input changed by the operator: the value is given to
    //! the engine and written in the console.
    //--------------------------------------------------------------------------
    void setInput(std::string const& name, int value);

    //--------------------------------------------------------------------------
    //! \brief GRAFCET input set to 1 during the next cycle of the engine only
    //! (pulse, or a push button released before the engine saw it).
    //--------------------------------------------------------------------------
    void pulseInput(std::string const& name);

    // *************************************************************************
    //! \brief One signal of the timing diagram of a GRAFCET: a step, an input
    //! or an output, and its changes during the simulation.
    // *************************************************************************
    struct Track
    {
        enum class Kind { Step, Input, Output };
        std::string name;
        Kind kind;
        //! \brief Index of the graph (steps), for the order of the rows.
        size_t graph = 0u;
        //! \brief (time, value) at each change, the first one at the start.
        std::vector<std::pair<float, int>> changes;
        //! \brief The value was not 0 at least once.
        bool used = false;
    };

    //--------------------------------------------------------------------------
    //! \brief Signals of the GRAFCET recorded since the start of the last
    //! simulation (kept after its end).
    //--------------------------------------------------------------------------
    std::vector<Track> const& tracks() const { return m_tracks; }

    //--------------------------------------------------------------------------
    //! \brief Time of the last recorded cycle.
    //--------------------------------------------------------------------------
    float recordedTime() const { return m_recorded_time; }

    //--------------------------------------------------------------------------
    //! \brief Whether the document is open (for tab close handling).
    //--------------------------------------------------------------------------
    bool open = true;

private:

    //! \brief Inputs that change by themselves: clocks, values driven by an
    //! output (the model of the machine).
    void stepInputs(float dt);
    //! \brief Write the firings of the last GRAFCET cycle in the console.
    void traceGrafcet();
    //! \brief Add the values of the last GRAFCET cycle to the tracks.
    void record();
    //! \brief Value of a track at the last cycle.
    void sample(std::string const& name, Track::Kind kind, size_t graph, int value, float time);

private:

    Messages& m_messages;
    //! \brief Description of the document, its inputs and outputs.
    DocumentInfo m_info;
    //! \brief Inputs to set back to 0 after the next cycle of the engine.
    std::vector<std::string> m_pulses;
    //! \brief Exact values of the inputs driven by an output (the engine
    //! only knows integers).
    std::map<std::string, float> m_driven;
    //! \brief Timing diagram.
    std::vector<Track> m_tracks;
    std::map<std::string, size_t> m_track_index;
    float m_recorded_time = -1.0f;
    //! \brief Engine shared by the GRAFCET nets of the document.
    GrafcetEngine m_grafcet;
    std::string m_filepath;
    std::vector<std::unique_ptr<NetEntry>> m_nets;
    size_t m_active_net_index = 0;
    bool m_modified = false;
};

} // namespace tpne

#endif // DOCUMENT_HPP
