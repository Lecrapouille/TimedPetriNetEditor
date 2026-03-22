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

#ifndef IREMOTE_CONTROL_HPP
#  define IREMOTE_CONTROL_HPP

#  include <string>

namespace tpne {

class Editor;

// ****************************************************************************
//! \brief Abstract interface for remote control backends.
//! Allows controlling the editor from external applications (debuggers, etc.)
// ****************************************************************************
class IRemoteControl
{
public:
    virtual ~IRemoteControl() = default;

    //--------------------------------------------------------------------------
    //! \brief Start the remote control server.
    //! \param[in] endpoint The endpoint to bind to (e.g., "tcp://*:5555").
    //! \return True if server started successfully.
    //--------------------------------------------------------------------------
    virtual bool start(std::string const& endpoint) = 0;

    //--------------------------------------------------------------------------
    //! \brief Stop the remote control server.
    //--------------------------------------------------------------------------
    virtual void stop() = 0;

    //--------------------------------------------------------------------------
    //! \brief Poll for incoming commands. Must be called regularly.
    //! This is non-blocking; processes one command if available.
    //--------------------------------------------------------------------------
    virtual void poll() = 0;

    //--------------------------------------------------------------------------
    //! \brief Check if the server is running.
    //--------------------------------------------------------------------------
    virtual bool isRunning() const = 0;

    //--------------------------------------------------------------------------
    //! \brief Get the last error message.
    //--------------------------------------------------------------------------
    virtual std::string error() const = 0;

    //--------------------------------------------------------------------------
    //! \brief Get the current endpoint.
    //--------------------------------------------------------------------------
    virtual std::string endpoint() const = 0;
};

} // namespace tpne

#endif // IREMOTE_CONTROL_HPP
