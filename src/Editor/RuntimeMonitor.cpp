// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Editor/RuntimeMonitor.hpp"
#include "Editor/Theme.hpp"

#include "nlohmann/json.hpp"

#include <algorithm>

#if TPNE_USE_ZEROMQ
#  include <zmq.h>
#endif

namespace tpne {

//! \brief Source of the marks of the nodes.
static char const* const MARKS = "runtime";

//------------------------------------------------------------------------------
bool RuntimeMonitor::connect(std::string const& endpoint)
{
    disconnect();
    m_error.clear();
#if TPNE_USE_ZEROMQ
    m_context = zmq_ctx_new();
    m_socket = (m_context != nullptr) ? zmq_socket(m_context, ZMQ_SUB) : nullptr;
    if ((m_socket == nullptr) || (zmq_connect(m_socket, endpoint.c_str()) != 0) ||
        (zmq_setsockopt(m_socket, ZMQ_SUBSCRIBE, "", 0) != 0))
    {
        m_error = "Cannot connect to " + endpoint + ": " + zmq_strerror(zmq_errno());
        disconnect();
        return false;
    }
    int linger = 0;
    zmq_setsockopt(m_socket, ZMQ_LINGER, &linger, sizeof(linger));
    m_endpoint = endpoint;
    return true;
#else
    m_error = "The editor was built without ZeroMQ (TPNE_ZEROMQ=0): cannot connect to " + endpoint;
    return false;
#endif
}

//------------------------------------------------------------------------------
void RuntimeMonitor::disconnect()
{
#if TPNE_USE_ZEROMQ
    if (m_socket != nullptr)
        zmq_close(m_socket);
    if (m_context != nullptr)
        zmq_ctx_term(m_context);
#endif
    m_socket = nullptr;
    m_context = nullptr;
    m_endpoint.clear();
    m_io.clear();
    m_manifests.clear();
    m_manifest = Manifest{};
}

//------------------------------------------------------------------------------
void RuntimeMonitor::mark(Net const& net, Manifest const& manifest, NodeMarks& marks)
{
    std::vector<NodeMark> nodes;
    for (auto const& t : net.transitions())
    {
        std::string const& name = t.caption.empty() ? t.key : t.caption;
        if (std::find(manifest.unregistered.begin(), manifest.unregistered.end(), name) !=
            manifest.unregistered.end())
            nodes.push_back({ t.nodeId(), palette().enabled, "no function" });
    }
    marks.set(net, MARKS, std::move(nodes));
}

//------------------------------------------------------------------------------
void RuntimeMonitor::handle(std::string const& message, Net& net, JournalView& journal,
                            NodeMarks& marks)
{
    nlohmann::json const json = nlohmann::json::parse(message, nullptr, false);
    if (json.is_discarded() || !json.is_object())
        return;

    std::string const type = json.value("type", "");
    if (type.empty())
    {
        // A line of the journal.
        LogEntry entry;
        if (fromJsonLine(message, net.colors, entry).empty())
            journal.append(entry);
    }
    else if (type == "manifest")
    {
        if (fromManifest(message, m_manifest).empty())
        {
            m_manifests[m_manifest.net] = m_manifest;
            mark(net, manifest(net.name), marks);
        }
    }
    else if (type == "marking")
    {
        std::string instance;
        double date = 0.0;
        ColoredMarking marking;
        if (fromMarkingMessage(message, net.colors, instance, date, marking).empty())
            journal.snapshot(instance, date, marking);
    }
    else if (type == "io")
    {
        // A GRAFCET: the values of its inputs and outputs.
        m_io.clear();
        for (char const* group : { "inputs", "outputs" })
        {
            if (!json.contains(group) || !json[group].is_object())
                continue;
            for (auto const& it : json[group].items())
                m_io.push_back(std::string(group[0] == 'i' ? "in  " : "out ") + it.key() + " = " +
                               it.value().dump());
        }
    }
}

//------------------------------------------------------------------------------
void RuntimeMonitor::poll(Net& net, JournalView& journal, NodeMarks& marks)
{
#if TPNE_USE_ZEROMQ
    if (m_socket == nullptr)
        return;

    // At most a few hundred messages per frame: the editor stays responsive.
    for (int i = 0; i < 500; ++i)
    {
        zmq_msg_t msg;
        zmq_msg_init(&msg);
        if (zmq_msg_recv(&msg, m_socket, ZMQ_DONTWAIT) < 0)
        {
            zmq_msg_close(&msg);
            break;
        }
        std::string const text(static_cast<char const*>(zmq_msg_data(&msg)), zmq_msg_size(&msg));
        zmq_msg_close(&msg);
        handle(text, net, journal, marks);
    }
#else
    (void) net;
    (void) journal;
    (void) marks;
#endif
}

} // namespace tpne
