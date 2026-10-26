// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef TPNE_RUNTIME_JOURNAL_PUBLISHER_HPP
#  define TPNE_RUNTIME_JOURNAL_PUBLISHER_HPP

#  include "Runtime/Journal.hpp"

#  include <zmq.h>

#  include <chrono>
#  include <mutex>
#  include <string>

// *****************************************************************************
//! \file Show a running program in the editor: the runtime publishes its
//! journal on a ZeroMQ socket (PUB), the editor connects to it (Debug >
//! Connect to a runtime) and draws the marking of the instances as they go.
//! Only the programs including this header link libzmq (-lzmq): the runtime
//! does not depend on it.
//!
//! \code
//! JournalPublisher publisher(net);                 // tcp://*:5556
//! Runtime::Options options;
//! options.journal = [&publisher](LogEntry const& e) { publisher.publish(e); };
//! Runtime runtime(net, executor, options);
//! runtime.on("grasp", grasp);
//! publisher.manifest(runtime);                     // transitions without function
//! \endcode
//!
//! Each message is one JSON object: the lines of the journal
//! (Runtime/Journal.hpp), {"type":"manifest",...} and {"type":"marking",...}.
//! A subscriber connected late misses the first messages: marking() gives it
//! the tokens of an instance. The manifest is sent again every second.
// *****************************************************************************

namespace tpne {

class JournalPublisher
{
public:

    //--------------------------------------------------------------------------
    //! \brief Bind the socket. The net names the colors of the results.
    //--------------------------------------------------------------------------
    explicit JournalPublisher(Net const& net, std::string const& endpoint = "tcp://*:5556")
        : m_net(net)
    {
        m_context = zmq_ctx_new();
        m_socket = (m_context != nullptr) ? zmq_socket(m_context, ZMQ_PUB) : nullptr;
        if ((m_socket == nullptr) || (zmq_bind(m_socket, endpoint.c_str()) != 0))
        {
            m_error = "Cannot publish on " + endpoint + ": " + zmq_strerror(zmq_errno());
            close();
            return;
        }
        int linger = 0;
        zmq_setsockopt(m_socket, ZMQ_LINGER, &linger, sizeof(linger));
    }

    ~JournalPublisher() { close(); }

    JournalPublisher(JournalPublisher const&) = delete;
    JournalPublisher& operator=(JournalPublisher const&) = delete;

    //! \brief Empty when the socket is bound.
    std::string const& error() const { return m_error; }

    //--------------------------------------------------------------------------
    //! \brief Publish a line of the journal (any thread).
    //--------------------------------------------------------------------------
    void publish(LogEntry const& entry)
    {
        send(toJsonLine(entry, m_net.colors));
    }

    //--------------------------------------------------------------------------
    //! \brief Publish the manifest of the runtime (and again every second
    //! with the next lines, for the editors connected later).
    //--------------------------------------------------------------------------
    void manifest(Runtime const& runtime)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_manifest = toManifest(runtime);
        m_sent = std::chrono::steady_clock::time_point{};
    }

    //--------------------------------------------------------------------------
    //! \brief Publish the tokens of the places of the instance, with their
    //! colors.
    //--------------------------------------------------------------------------
    void marking(Instance const& instance)
    {
        send(toMarkingMessage(instance.id(), instance.time(), instance.marking(), m_net.colors));
    }

    //--------------------------------------------------------------------------
    //! \brief Publish any message (a GRAFCET sends its steps this way).
    //--------------------------------------------------------------------------
    void send(std::string const& message)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_socket == nullptr)
            return;
        auto const now = std::chrono::steady_clock::now();
        if (!m_manifest.empty() && (now - m_sent > std::chrono::seconds(1)))
        {
            zmq_send(m_socket, m_manifest.data(), m_manifest.size(), ZMQ_DONTWAIT);
            m_sent = now;
        }
        zmq_send(m_socket, message.data(), message.size(), ZMQ_DONTWAIT);
    }

private:

    void close()
    {
        if (m_socket != nullptr)
            zmq_close(m_socket);
        if (m_context != nullptr)
            zmq_ctx_term(m_context);
        m_socket = nullptr;
        m_context = nullptr;
    }

    Net const& m_net;
    void* m_context = nullptr;
    void* m_socket = nullptr;
    std::string m_error;
    std::string m_manifest;
    std::chrono::steady_clock::time_point m_sent;
    std::mutex m_mutex;
};

} // namespace tpne

#endif
