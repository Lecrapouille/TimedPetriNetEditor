// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef TPNE_EDITOR_RUNTIME_MONITOR_HPP
#  define TPNE_EDITOR_RUNTIME_MONITOR_HPP

#  include "Editor/JournalView.hpp"
#  include "Editor/NodeMarks.hpp"
#  include "Runtime/Journal.hpp"

#  include <map>
#  include <string>
#  include <vector>

namespace tpne {

// *****************************************************************************
//! \brief The editor as the debugger of a running program: it subscribes to
//! the messages of a JournalPublisher (runtime) or of a GRAFCET generated
//! with its debug option, on ZeroMQ, and gives them to the journal (which
//! draws the marking as they come) and to the marks of the nodes (the
//! transitions without function). Without ZeroMQ (TPNE_USE_ZEROMQ=0) it
//! cannot connect.
// *****************************************************************************
class RuntimeMonitor
{
public:

    ~RuntimeMonitor() { disconnect(); }

    //--------------------------------------------------------------------------
    //! \brief Subscribe to the program publishing on the endpoint
    //! ("tcp://localhost:5556").
    //! \return false on error (see error()).
    //--------------------------------------------------------------------------
    bool connect(std::string const& endpoint);
    void disconnect();
    bool isConnected() const { return m_socket != nullptr; }
    std::string const& endpoint() const { return m_endpoint; }
    std::string const& error() const { return m_error; }

    //--------------------------------------------------------------------------
    //! \brief Read the messages received (without waiting) and give them to
    //! the journal (put it live on the net when connecting). Call it once
    //! per frame.
    //--------------------------------------------------------------------------
    void poll(Net& net, JournalView& journal, NodeMarks& marks);

    //--------------------------------------------------------------------------
    //! \brief Mark the transitions of the net without function in the
    //! manifest.
    //--------------------------------------------------------------------------
    static void mark(Net const& net, Manifest const& manifest, NodeMarks& marks);

    //! \brief Manifest of the net of this name (the graphs of a GRAFCET
    //! send one each), else the last one received.
    Manifest const& manifest(std::string const& net) const
    {
        auto const it = m_manifests.find(net);
        return (it != m_manifests.end()) ? it->second : m_manifest;
    }
    //! \brief Values of the inputs and outputs of a GRAFCET ("name=value").
    std::vector<std::string> const& io() const { return m_io; }

private:

    void handle(std::string const& message, Net& net, JournalView& journal, NodeMarks& marks);

private:

    void* m_context = nullptr;
    void* m_socket = nullptr;
    std::string m_endpoint;
    std::string m_error;
    Manifest m_manifest;
    std::map<std::string, Manifest> m_manifests;
    std::vector<std::string> m_io;
};

} // namespace tpne

#endif
