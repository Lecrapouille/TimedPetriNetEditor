// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef NULL_REMOTE_CONTROL_HPP
#  define NULL_REMOTE_CONTROL_HPP

#  include "IRemoteControl.hpp"

namespace tpne {

// ****************************************************************************
//! \brief No-op remote control when ZeroMQ is disabled at build time.
// ****************************************************************************
class NullRemoteControl final : public IRemoteControl
{
public:
    bool start(std::string const& endpoint) override;
    void stop() override;
    void poll() override;
    bool isRunning() const override { return false; }
    std::string error() const override { return m_error; }
    std::string endpoint() const override { return m_endpoint; }

private:
    std::string m_error;
    std::string m_endpoint;
};

} // namespace tpne

#endif
