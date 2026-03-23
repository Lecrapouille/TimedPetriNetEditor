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

#include "ZeroMQRemote.hpp"
#include "Editor/Editor.hpp"
#include "PetriNet/Imports/Imports.hpp"
#include <zmq.h>
#include <fstream>

namespace tpne {

//------------------------------------------------------------------------------
ZeroMQRemote::ZeroMQRemote(Editor& editor)
    : m_editor(editor)
{}

//------------------------------------------------------------------------------
ZeroMQRemote::~ZeroMQRemote()
{
    stop();
}

//------------------------------------------------------------------------------
bool ZeroMQRemote::start(std::string const& endpoint)
{
    if (m_running)
    {
        m_error = "Server already running";
        return false;
    }

    // Create ZeroMQ context
    m_context = zmq_ctx_new();
    if (m_context == nullptr)
    {
        m_error = "Failed to create ZeroMQ context";
        return false;
    }

    // Create REP socket
    m_socket = zmq_socket(m_context, ZMQ_REP);
    if (m_socket == nullptr)
    {
        m_error = "Failed to create ZeroMQ socket";
        zmq_ctx_destroy(m_context);
        m_context = nullptr;
        return false;
    }

    // Set socket to non-blocking
    int timeout = 0;
    zmq_setsockopt(m_socket, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));

    // Bind to endpoint
    if (zmq_bind(m_socket, endpoint.c_str()) != 0)
    {
        m_error = "Failed to bind to " + endpoint + ": " + zmq_strerror(errno);
        zmq_close(m_socket);
        zmq_ctx_destroy(m_context);
        m_socket = nullptr;
        m_context = nullptr;
        return false;
    }

    m_endpoint = endpoint;
    m_running = true;
    m_error.clear();
    return true;
}

//------------------------------------------------------------------------------
void ZeroMQRemote::stop()
{
    if (m_socket != nullptr)
    {
        zmq_close(m_socket);
        m_socket = nullptr;
    }
    if (m_context != nullptr)
    {
        zmq_ctx_destroy(m_context);
        m_context = nullptr;
    }
    m_running = false;
    m_endpoint.clear();
}

//------------------------------------------------------------------------------
void ZeroMQRemote::poll()
{
    if (!m_running || m_socket == nullptr)
        return;

    // Try to receive a message (non-blocking)
    char buffer[65536];
    int size = zmq_recv(m_socket, buffer, sizeof(buffer) - 1, ZMQ_DONTWAIT);

    if (size < 0)
    {
        // No message available or error
        if (errno != EAGAIN)
        {
            m_error = "Receive error: " + std::string(zmq_strerror(errno));
        }
        return;
    }

    buffer[size] = '\0';

    // Process command and get response
    std::string response = processCommand(buffer);

    // Send response
    zmq_send(m_socket, response.c_str(), response.length(), 0);
}

//------------------------------------------------------------------------------
std::string ZeroMQRemote::processCommand(std::string const& json_str)
{
    nlohmann::json response;

    try
    {
        nlohmann::json cmd = nlohmann::json::parse(json_str);

        if (!cmd.contains("command"))
        {
            response["status"] = "error";
            response["message"] = "Missing 'command' field";
            return response.dump();
        }

        std::string command = cmd["command"];
        nlohmann::json params = cmd.value("params", nlohmann::json::object());

        // Dispatch command
        if (command == "load")
            return cmdLoad(params);
        else if (command == "start")
            return cmdStart(params);
        else if (command == "stop")
            return cmdStop(params);
        else if (command == "fire")
            return cmdFire(params);
        else if (command == "state")
            return cmdState(params);
        else if (command == "set_tokens")
            return cmdSetTokens(params);
        else if (command == "clear")
            return cmdClear(params);
        else
        {
            response["status"] = "error";
            response["message"] = "Unknown command: " + command;
        }
    }
    catch (nlohmann::json::exception const& e)
    {
        response["status"] = "error";
        response["message"] = std::string("JSON parse error: ") + e.what();
    }

    return response.dump();
}

//------------------------------------------------------------------------------
std::string ZeroMQRemote::cmdLoad(nlohmann::json const& params)
{
    nlohmann::json response;

    if (m_editor.simulation().running)
    {
        response["status"] = "error";
        response["message"] = "Cannot load while simulation is running";
        return response.dump();
    }

    try
    {
        // Params can be either a JSON net definition or a file path
        if (params.contains("net"))
        {
            // Write to temporary file and load
            std::string path = "/tmp/petri_remote.json";
            std::ofstream file(path);
            file << params["net"].dump();
            file.close();

            bool shall_springify;
            std::string error = loadFromFile(m_editor.net(), path, shall_springify);
            if (error.empty())
            {
                response["status"] = "ok";
                response["message"] = "Net loaded successfully";
            }
            else
            {
                response["status"] = "error";
                response["message"] = error;
            }
        }
        else if (params.contains("path"))
        {
            bool shall_springify;
            std::string error = loadFromFile(m_editor.net(), params["path"], shall_springify);
            if (error.empty())
            {
                response["status"] = "ok";
                response["message"] = "Net loaded from " + params["path"].get<std::string>();
            }
            else
            {
                response["status"] = "error";
                response["message"] = error;
            }
        }
        else
        {
            response["status"] = "error";
            response["message"] = "Missing 'net' or 'path' parameter";
        }
    }
    catch (std::exception const& e)
    {
        response["status"] = "error";
        response["message"] = e.what();
    }

    return response.dump();
}

//------------------------------------------------------------------------------
std::string ZeroMQRemote::cmdStart(nlohmann::json const& /*params*/)
{
    nlohmann::json response;

    if (m_editor.net().type() == TypeOfNet::TimedEventGraph ||
        m_editor.net().type() == TypeOfNet::TimedPetriNet)
    {
        response["status"] = "error";
        response["message"] = "Convert to non-timed net before starting simulation";
        return response.dump();
    }

    if (!m_editor.simulation().running)
    {
        m_editor.toogleStartAllSimulations();
    }

    response["status"] = "ok";
    response["message"] = "Simulation started";
    return response.dump();
}

//------------------------------------------------------------------------------
std::string ZeroMQRemote::cmdStop(nlohmann::json const& /*params*/)
{
    nlohmann::json response;

    if (m_editor.simulation().running)
    {
        m_editor.toogleStartAllSimulations();
    }

    response["status"] = "ok";
    response["message"] = "Simulation stopped";
    return response.dump();
}

//------------------------------------------------------------------------------
std::string ZeroMQRemote::cmdFire(nlohmann::json const& params)
{
    nlohmann::json response;

    if (!m_editor.simulation().running)
    {
        response["status"] = "error";
        response["message"] = "Simulation is not running";
        return response.dump();
    }

    // Params can be transition ID(s) or a receptivity bitfield
    if (params.contains("transitions"))
    {
        auto& transitions = m_editor.net().transitions();
        for (auto& t_id : params["transitions"])
        {
            size_t id = t_id.get<size_t>();
            if (id < transitions.size())
            {
                transitions[id].receptivity = true;
            }
        }
        response["status"] = "ok";
        response["message"] = "Transitions fired";
    }
    else if (params.contains("bitfield"))
    {
        std::string bitfield = params["bitfield"];
        auto& transitions = m_editor.net().transitions();
        if (bitfield.length() == transitions.size())
        {
            for (size_t i = 0; i < bitfield.length(); ++i)
            {
                transitions[i].receptivity = (bitfield[i] != '0');
            }
            response["status"] = "ok";
            response["message"] = "Receptivities set";
        }
        else
        {
            response["status"] = "error";
            response["message"] = "Bitfield length doesn't match transition count";
        }
    }
    else
    {
        response["status"] = "error";
        response["message"] = "Missing 'transitions' or 'bitfield' parameter";
    }

    return response.dump();
}

//------------------------------------------------------------------------------
std::string ZeroMQRemote::cmdState(nlohmann::json const& /*params*/)
{
    nlohmann::json response;
    response["status"] = "ok";
    response["running"] = m_editor.simulation().running.load();

    // Places with token counts
    nlohmann::json places = nlohmann::json::array();
    for (auto const& p : m_editor.net().places())
    {
        nlohmann::json place;
        place["id"] = p.id;
        place["key"] = p.key;
        place["caption"] = p.caption;
        place["tokens"] = p.tokens;
        places.push_back(place);
    }
    response["places"] = places;

    // Transitions with receptivity state
    nlohmann::json transitions = nlohmann::json::array();
    for (auto const& t : m_editor.net().transitions())
    {
        nlohmann::json trans;
        trans["id"] = t.id;
        trans["key"] = t.key;
        trans["caption"] = t.caption;
        trans["receptivity"] = t.receptivity;
        transitions.push_back(trans);
    }
    response["transitions"] = transitions;

    return response.dump();
}

//------------------------------------------------------------------------------
std::string ZeroMQRemote::cmdSetTokens(nlohmann::json const& params)
{
    nlohmann::json response;

    if (!params.contains("place") || !params.contains("tokens"))
    {
        response["status"] = "error";
        response["message"] = "Missing 'place' or 'tokens' parameter";
        return response.dump();
    }

    size_t place_id = params["place"].get<size_t>();
    size_t tokens = params["tokens"].get<size_t>();

    auto& places = m_editor.net().places();
    if (place_id < places.size())
    {
        places[place_id].tokens = tokens;
        m_editor.net().modified = true;
        response["status"] = "ok";
        response["message"] = "Tokens set";
    }
    else
    {
        response["status"] = "error";
        response["message"] = "Invalid place ID";
    }

    return response.dump();
}

//------------------------------------------------------------------------------
std::string ZeroMQRemote::cmdClear(nlohmann::json const& /*params*/)
{
    nlohmann::json response;

    if (m_editor.simulation().running)
    {
        response["status"] = "error";
        response["message"] = "Cannot clear while simulation is running";
        return response.dump();
    }

    m_editor.clearNet();

    response["status"] = "ok";
    response["message"] = "Net cleared";
    return response.dump();
}

} // namespace tpne
