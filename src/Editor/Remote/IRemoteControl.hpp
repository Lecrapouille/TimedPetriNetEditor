// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

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
