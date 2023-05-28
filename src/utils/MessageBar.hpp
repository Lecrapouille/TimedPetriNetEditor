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

#ifndef MESSAGEBAR_HPP
#define MESSAGEBAR_HPP

static size_t tictac = 0u;

// *****************************************************************************
//! \brief A text inside a rectangle
// *****************************************************************************
class MessageBar
{
public:

    struct TimedMessage
    {
        std::string time;
        std::string txt;
    };

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    MessageBar(sf::Font& font)
    {
    }

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    inline void setText(const std::string& alert, const std::string& message)
    {
        m_message.time = std::to_string(tictac++);
        m_message.txt = alert + ": " + message;
        m_buffer.push_back(m_message);
    }

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    inline void setInfo(const std::string& message)
    {
        setText("Info", message);
    }

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    inline void setWarning(const std::string& message)
    {
        setText("Warning", message);
    }

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    inline void setError(const std::string& message)
    {
        setText("Error", message);
    }

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    void setSize(sf::Vector2u const& dimensions)
    {
    }

    //--------------------------------------------------------------------------
    //! \brief Append the displayed messge. The color is not modified.
    //--------------------------------------------------------------------------
    MessageBar& append(std::string const& message)
    {
        m_message.txt += message;
        m_buffer.back().txt += message;
        return *this;
    }

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    std::string const& getText() const
    {
        return m_message.txt;
    }

    void clear() { m_buffer.clear(); }

    std::vector<TimedMessage> const& getBuffer() const
    {
        return m_buffer;
    }

private:

    //! \brief String returned when the entry is activated
    TimedMessage m_message;
    std::vector<TimedMessage> m_buffer;
};

#endif
