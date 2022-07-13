//=====================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2022 Quentin Quadrat <lecrapouille@gmail.com>
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
//=====================================================================

#ifndef ENTRYBOX_HPP
#  define ENTRYBOX_HPP

#  include <SFML/Graphics.hpp>

// *****************************************************************************
//! \brief A text inside a rectangle
// *****************************************************************************
class EntryBox : public sf::Drawable
{
public:

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    EntryBox(sf::Font& font, float width);

    //--------------------------------------------------------------------------
    //! \brief Set textbox content
    //--------------------------------------------------------------------------
    void seText(const std::string& string)
    {
        // Trim current text if needed
        if (string.size() > m_max_length)
        {
            m_text.setString(string.substr(0, m_max_length));
        }
        else
        {
            m_text.setString(string);
        }
        setCursor(getText().getSize());
    }

    //--------------------------------------------------------------------------
    //! \brief Get textbox content.
    //--------------------------------------------------------------------------
    const sf::String& getText() const
    {
        return m_text.getString();
    }

    //--------------------------------------------------------------------------
    //! \brief Define max length of textbox content (default is 256 characters)
    //--------------------------------------------------------------------------
    void setMaxLength(size_t max_length)
    {
        m_max_length = max_length;
        if (m_text.getString().getSize() > m_max_length)
        {
            m_text.setString(m_text.getString().substring(0, m_max_length));
            setCursor(m_max_length);
        }
    }

    //--------------------------------------------------------------------------
    //! \brief Set the cursor position
    //--------------------------------------------------------------------------
    void setCursor(size_t index);

    //--------------------------------------------------------------------------
    //! \brief Get the cursor position
    //--------------------------------------------------------------------------
    size_t getCursor() const
    {
        return m_cursor_position;
    }

    void bind(std::string* caption = nullptr)
    {
        m_bind = caption;
    }

    bool bound() const { return m_bind != nullptr; }

private:

    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:

    sf::Text m_text;
    sf::RectangleShape m_box;
    mutable sf::RectangleShape m_cursor;
    mutable sf::Clock m_timer;
    size_t m_cursor_position = 0u;
    size_t m_max_length = 256u;
    std::string* m_bind = nullptr;
};

#endif
