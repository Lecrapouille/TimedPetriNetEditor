// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef TPNE_EDITOR_JOURNAL_VIEW_HPP
#  define TPNE_EDITOR_JOURNAL_VIEW_HPP

#  include "Editor/NodeMarks.hpp"
#  include "Runtime/Runtime.hpp"

#  include <map>
#  include <string>
#  include <vector>

namespace tpne {

// *****************************************************************************
//! \brief The journal of the editor: the firings of the simulations, a journal
//! of the runtime opened from a file (JSON lines, Runtime/Journal.hpp) or
//! received from a connected runtime. Its window lists the entries and
//! replays them on the active net: a cursor on the dates gives the marking
//! (Runtime::restore()) and the transitions whose function runs.
// *****************************************************************************
class JournalView
{
public:

    //--------------------------------------------------------------------------
    //! \brief A firing of the simulation of the net: tokens taken and given at
    //! once (the tokens on arcs with a duration are in their place). A date
    //! before the last one of the net is a new run: its entries are dropped.
    //--------------------------------------------------------------------------
    void record(Net const& net, size_t const transition, double const date);

    //--------------------------------------------------------------------------
    //! \brief An entry received from a connected runtime.
    //--------------------------------------------------------------------------
    void append(LogEntry const& entry);

    //--------------------------------------------------------------------------
    //! \brief Follow a running program on the net: the entries are cleared, the
    //! replay shows the last date as the entries come (append(), snapshot()).
    //! An empty source stops it.
    //--------------------------------------------------------------------------
    void live(std::string const& source, Net& net);

    //--------------------------------------------------------------------------
    //! \brief Replace the entries by these firings of the net (a path found by
    //! the analysis), one per second, and replay them from the start.
    //--------------------------------------------------------------------------
    void showFirings(Net& net, std::string const& source, std::vector<size_t> const& transitions);

    //--------------------------------------------------------------------------
    //! \brief Tokens of each place of an instance at a date (a program
    //! connected late gives them): the replay of the instance starts there.
    //--------------------------------------------------------------------------
    void snapshot(std::string const& instance, double const date, Marking const& tokens);

    //--------------------------------------------------------------------------
    //! \brief Forget the entries (and stop the replay).
    //--------------------------------------------------------------------------
    void clear();

    //--------------------------------------------------------------------------
    //! \brief Read or write a journal (JSON lines), colors named by the net.
    //! \return the error, or empty.
    //--------------------------------------------------------------------------
    std::string load(std::string const& path, Net const& net);
    std::string save(std::string const& path, Net const& net) const;

    //--------------------------------------------------------------------------
    //! \brief The window. The replay changes the tokens of the places of the
    //! net (they are restored when it stops); marks: the running transitions.
    //! \param[in] busy the net is simulated (no replay).
    //--------------------------------------------------------------------------
    void draw(Net& net, bool const busy, NodeMarks& marks);

    //--------------------------------------------------------------------------
    //! \brief Stop the replay: the replayed net gets its tokens back. To call
    //! before the net is simulated, saved, closed.
    //--------------------------------------------------------------------------
    void stopReplay();

    bool isReplaying() const { return m_replay; }
    bool isLive() const { return !m_live.empty(); }
    std::vector<LogEntry> const& entries() const { return m_entries; }

    //! \brief Ask the window to open a file dialog (menu of the editor).
    bool request_open = false;
    bool request_save = false;

private:

    //! \brief Index k of the entry: its number among the firings of its
    //! transition in its instance.
    void numberLast();
    //! \brief Marking of the net at the cursor (instance selected).
    void replay(Net& net, NodeMarks& marks);
    void toolbar(Net& net, bool const busy, NodeMarks& marks);
    void table(Net& net, NodeMarks& marks);

private:

    std::vector<LogEntry> m_entries;
    std::vector<size_t> m_k;
    //! \brief Where the entries come from (simulation, file, runtime).
    std::string m_source;
    std::string m_filter;
    bool m_follow = true;
    std::string m_error;

    //! \brief Replay: on the net of the given revision, the tokens it had,
    //! the instance replayed and the date of the cursor.
    bool m_replay = false;
    Net* m_replay_net = nullptr;
    std::vector<size_t> m_saved_tokens;
    std::string m_instance;
    double m_cursor = 0.0;
    bool m_playing = false;
    float m_speed = 1.0f;
    bool m_dirty = true;
    //! \brief Entry selected in the table (its transition is marked).
    size_t m_selected = size_t(-1);
    //! \brief Endpoint of the program followed (empty: none).
    std::string m_live;
    //! \brief Tokens given by the program for an instance at a date.
    struct Snapshot
    {
        double date = 0.0;
        Marking tokens;
        //! \brief Number of entries received before it: they are in it.
        size_t entries = 0u;
    };
    std::map<std::string, Snapshot> m_snapshots;
};

} // namespace tpne

#endif
