//=============================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2026 Quentin Quadrat <lecrapouille@gmail.com>
//=============================================================================

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
