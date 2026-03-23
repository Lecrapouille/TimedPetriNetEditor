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
class ZeroMQRemote : public IRemoteControl
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
    void* m_context = nullptr;       //!< ZeroMQ context
    void* m_socket = nullptr;        //!< ZeroMQ REP socket
    bool m_running = false;          //!< Server running flag
    std::string m_error;             //!< Last error message
    std::string m_endpoint;          //!< Current endpoint
};

} // namespace tpne

#endif // ZEROMQ_REMOTE_HPP
