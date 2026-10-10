// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "NullRemoteControl.hpp"

namespace tpne {

bool NullRemoteControl::start(std::string const& endpoint)
{
    m_endpoint = endpoint;
    m_error = "Remote control disabled (build with TPNE_ZEROMQ=1 to enable ZeroMQ)";
    return false;
}

void NullRemoteControl::stop()
{
    m_endpoint.clear();
    m_error.clear();
}

void NullRemoteControl::poll() {}

} // namespace tpne
