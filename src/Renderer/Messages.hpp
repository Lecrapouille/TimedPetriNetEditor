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

#ifndef MESSAGES_HPP
#  define MESSAGES_HPP

#  include "Utils.hpp"
#  include <cassert>

// *****************************************************************************
//! \brief A text inside a rectangle
// *****************************************************************************
class Messages
{
public:

    enum Level { Info, Warning, Error };
    struct TimedMessage
    {
        Level level;
        std::string time;
        std::string message;
    };

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    inline void setInfo(const std::string& message)
    {
        add(Messages::Level::Info, message);
    }

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    inline void setWarning(const std::string& message)
    {
        add(Messages::Level::Warning, message);
    }

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    inline void setError(const std::string& message)
    {
        add(Messages::Level::Error, message);
    }

    //--------------------------------------------------------------------------
    //! \brief Append the displayed messge. The color is not modified.
    //--------------------------------------------------------------------------
    Messages& append(std::string const& message)
    {
        m_messages.back().message += message;
        return *this;
    }

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    TimedMessage getMessage() const
    {
        assert((m_messages.size() >= 1u) && "dummy message");
        return m_messages.back();
    }

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    std::vector<TimedMessage> const& getMessages() const
    {
        return m_messages;
    }

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    void clear() { m_messages.clear(); }

protected:

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    inline void add(Messages::Level const level, std::string const& message)
    {
        m_messages.push_back({level, current_time(), message});
    }

private:

    std::vector<TimedMessage> m_messages;
};

#endif
