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

#  include <array>
#  include <cassert>
#  include <chrono>
#  include <ctime>
#  include <string>
#  include <vector>

namespace tpne {

// *****************************************************************************
//! \brief Events of the editor: what the user should know (info, warnings,
//! errors) and what the simulation does (traces). The same message given
//! several times in a row is kept once, with a counter, so that a repeated
//! error does not fill the console.
// *****************************************************************************
class Messages
{
public:

    //! \brief Message severity level. Trace: events of the simulation.
    enum class Level { Info, Warning, Error, Trace };
    static constexpr size_t LEVELS = 4u;

    //! \brief A message with timestamp and severity
    struct TimedMessage
    {
        Level level;
        //! \brief Wall clock time of the last occurrence "[12:03:54] ".
        std::string time;
        std::string message;
        //! \brief Occurrences in a row of the same message.
        size_t count = 1u;
        //! \brief Seconds (steady clock) of the last occurrence.
        double stamp = 0.0;
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
    //! \brief Log an event of the simulation.
    //--------------------------------------------------------------------------
    void setTrace(std::string const& message) { add(Level::Trace, message); }

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
    TimedMessage const& getMessage() const
    {
        assert(!m_messages.empty() && "No messages available");
        return m_messages.back();
    }

    //--------------------------------------------------------------------------
    //! \brief Get all messages, oldest first.
    //--------------------------------------------------------------------------
    std::vector<TimedMessage> const& getMessages() const { return m_messages; }

    //--------------------------------------------------------------------------
    //! \brief Number of messages of a level (repetitions counted once).
    //--------------------------------------------------------------------------
    size_t count(Level level) const { return m_counts[size_t(level)]; }

    //--------------------------------------------------------------------------
    //! \brief Check if there are any messages.
    //--------------------------------------------------------------------------
    bool empty() const { return m_messages.empty(); }

    //--------------------------------------------------------------------------
    //! \brief Clear all messages.
    //--------------------------------------------------------------------------
    void clear() { m_messages.clear(); m_counts.fill(0u); }

    //--------------------------------------------------------------------------
    //! \brief Seconds of a steady clock (to age the messages).
    //--------------------------------------------------------------------------
    static double now()
    {
        using namespace std::chrono;
        return duration<double>(steady_clock::now().time_since_epoch()).count();
    }

private:

    void add(Level level, std::string const& message)
    {
        // The same message again: count it instead of adding a line.
        if (!m_messages.empty() && (m_messages.back().level == level) &&
            (m_messages.back().message == message))
        {
            TimedMessage& last = m_messages.back();
            ++last.count;
            last.time = currentTime();
            last.stamp = now();
            return;
        }

        // Keep the log bounded: drop the oldest quarter when it is full.
        if (m_messages.size() >= MAX_MESSAGES)
        {
            for (auto it = m_messages.begin(); it != m_messages.begin() + MAX_MESSAGES / 4u; ++it)
                --m_counts[size_t(it->level)];
            m_messages.erase(m_messages.begin(),
                             m_messages.begin() + MAX_MESSAGES / 4u);
        }
        m_messages.push_back({level, currentTime(), message, 1u, now()});
        ++m_counts[size_t(level)];
    }

    static std::string currentTime()
    {
        char buffer[32];
        time_t now = ::time(nullptr);
        strftime(buffer, sizeof(buffer), "[%H:%M:%S] ", localtime(&now));
        return buffer;
    }

    //! \brief Maximum number of messages kept in memory.
    static constexpr size_t MAX_MESSAGES = 4000u;

    std::vector<TimedMessage> m_messages;
    std::array<size_t, LEVELS> m_counts{};
};

} // namespace tpne

#endif // MESSAGES_HPP
