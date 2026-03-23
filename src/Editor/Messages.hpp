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
