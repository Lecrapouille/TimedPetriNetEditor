// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef MESSAGES_HPP
#  define MESSAGES_HPP

#  include <array>
#  include <cassert>
#  include <chrono>
#  include <ctime>
#  include <functional>
#  include <string>
#  include <vector>

namespace tpne {

class Net;

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
    //! \brief A transition of the net fired at the date (simulation time):
    //! given to the journal of the editor.
    //--------------------------------------------------------------------------
    void fired(Net const& net, size_t const transition, double const date) const
    {
        if (onFired)
            onFired(net, transition, date);
    }

    //! \brief Receives fired() (the journal of the editor).
    std::function<void(Net const&, size_t, double)> onFired;

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
