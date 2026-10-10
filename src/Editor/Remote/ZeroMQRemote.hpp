// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef ZEROMQ_REMOTE_HPP
#  define ZEROMQ_REMOTE_HPP

#  include "IRemoteControl.hpp"
#  include <nlohmann/json.hpp>

namespace tpne {

class Editor;

// ****************************************************************************
//! \brief ZeroMQ implementation of remote control using REQ/REP pattern.
//! Commands are received as JSON, executed, and responses are sent back.
// ****************************************************************************
class ZeroMQRemote final : public IRemoteControl
{
public:
    //--------------------------------------------------------------------------
    //! \brief Constructor.
    //! \param[in] editor Reference to the editor to control.
    //--------------------------------------------------------------------------
    explicit ZeroMQRemote(Editor& editor);

    //--------------------------------------------------------------------------
    //! \brief Destructor. Stops the server if running.
    //--------------------------------------------------------------------------
    ~ZeroMQRemote() override;

    // IRemoteControl interface
    bool start(std::string const& endpoint) override;
    void stop() override;
    void poll() override;
    bool isRunning() const override { return m_running; }
    std::string error() const override { return m_error; }
    std::string endpoint() const override { return m_endpoint; }

private:
    //--------------------------------------------------------------------------
    //! \brief Process a JSON command and return the response.
    //--------------------------------------------------------------------------
    std::string processCommand(std::string const& json);

    // Command handlers
    std::string cmdLoad(nlohmann::json const& params);
    std::string cmdStart(nlohmann::json const& params);
    std::string cmdStop(nlohmann::json const& params);
    std::string cmdFire(nlohmann::json const& params);
    std::string cmdState(nlohmann::json const& params);
    std::string cmdSetTokens(nlohmann::json const& params);
    std::string cmdClear(nlohmann::json const& params);

private:
    Editor& m_editor;                //!< Reference to editor
    char m_buffer[65536];            //!< Buffer for received commands
    void* m_context = nullptr;       //!< ZeroMQ context
    void* m_socket = nullptr;        //!< ZeroMQ REP socket
    bool m_running = false;          //!< Server running flag
    std::string m_error;             //!< Last error message
    std::string m_endpoint;          //!< Current endpoint
};

} // namespace tpne

#endif // ZEROMQ_REMOTE_HPP
