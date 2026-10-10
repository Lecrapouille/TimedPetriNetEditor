// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef MESSAGES_HPP
#  define MESSAGES_HPP

#  include <vector>
#  include <string>
#  include <ctime>
#  include <cassert>

namespace tpne {

// *****************************************************************************
//! \brief Message logging system. Stores timestamped messages with severity.
// *****************************************************************************
class Messages
{
public:

    //! \brief Message severity level
    enum class Level { Info, Warning, Error };

    //! \brief A message with timestamp and severity
    struct TimedMessage
    {
        Level level;
        std::string time;
        std::string message;
    };

    //--------------------------------------------------------------------------
    //! \brief Log an informational message.
    //--------------------------------------------------------------------------
    void setInfo(std::string const& message) { add(Level::Info, message); }

    //--------------------------------------------------------------------------
    //! \brief Log a warning message.
    //--------------------------------------------------------------------------
    void setWarning(std::string const& message) { add(Level::Warning, message); }

    //--------------------------------------------------------------------------
    //! \brief Log an error message.
    //--------------------------------------------------------------------------
    void setError(std::string const& message) { add(Level::Error, message); }

    //--------------------------------------------------------------------------
    //! \brief Append text to the last message.
    //--------------------------------------------------------------------------
    Messages& append(std::string const& text)
    {
        if (!m_messages.empty())
            m_messages.back().message += text;
        return *this;
    }

    //--------------------------------------------------------------------------
    //! \brief Get the most recent message.
    //--------------------------------------------------------------------------
    TimedMessage getMessage() const
    {
        assert(!m_messages.empty() && "No messages available");
        return m_messages.back();
    }

    //--------------------------------------------------------------------------
    //! \brief Get all messages.
    //--------------------------------------------------------------------------
    std::vector<TimedMessage> const& getMessages() const { return m_messages; }

    //--------------------------------------------------------------------------
    //! \brief Check if there are any messages.
    //--------------------------------------------------------------------------
    bool empty() const { return m_messages.empty(); }

    //--------------------------------------------------------------------------
    //! \brief Clear all messages.
    //--------------------------------------------------------------------------
    void clear() { m_messages.clear(); }

private:

    void add(Level level, std::string const& message)
    {
        m_messages.push_back({level, currentTime(), message});
    }

    static std::string currentTime()
    {
        char buffer[32];
        time_t now = ::time(nullptr);
        strftime(buffer, sizeof(buffer), "[%H:%M:%S] ", localtime(&now));
        return buffer;
    }

    std::vector<TimedMessage> m_messages;
};

} // namespace tpne

#endif // MESSAGES_HPP
