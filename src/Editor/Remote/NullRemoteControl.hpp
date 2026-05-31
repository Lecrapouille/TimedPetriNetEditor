//=============================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2026 Quentin Quadrat <lecrapouille@gmail.com>
//=============================================================================

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
