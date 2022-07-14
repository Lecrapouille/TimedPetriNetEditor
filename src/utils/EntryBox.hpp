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

#ifndef ENTRY_BOX_HPP
#  define ENTRY_BOX_HPP

#  include "utils/Settings.hpp"
#  include "PetriNet.hpp"

// *****************************************************************************
//! \brief Allow to modify the caption of places and transitions.
// *****************************************************************************
class EntryBox : public sf::Drawable
{
public:

    EntryBox(sf::Font const& font)
    {
        m_text.setFont(font);
        m_text.setCharacterSize(CAPTION_FONT_SIZE);
        m_text.setFillColor(sf::Color::Black);

        m_box.setFillColor({255,255,255,0});
        m_box.setOutlineThickness(1.0f);
        m_box.setOutlineColor(OUTLINE_COLOR);

        m_cursor.setFillColor(sf::Color::Black);
    }

    //------------------------------------------------------------------------------
    bool focus(Node& node, sf::Vector2f const& mouse)
    {
        //
        m_text.setString(node.caption);

        const float y = node.y - m_text.getLocalBounds().height -
            PLACE_RADIUS - CAPTION_FONT_SIZE / 2.0f - 2.0f;
        m_text.setPosition(node.x - m_text.getLocalBounds().width / 2.0f, y);

        //
        sf::FloatRect textBounds = m_text.getGlobalBounds();
        m_box.setPosition(textBounds.left, textBounds.top);
        m_box.setSize({textBounds.width, textBounds.height});

        //
        m_cursor_pos = 0;
        m_cursor.setPosition(m_text.findCharacterPos(m_cursor_pos));
        m_cursor.setSize({2.0f, textBounds.height});

        if (m_text.getGlobalBounds().contains(mouse))
        {
            m_backup = node.caption;
            m_caption = &node.caption;
            m_timer.restart();
            return true;
        }

        return false;
     }

    //------------------------------------------------------------------------------
    inline bool focus() const { return m_caption != nullptr; }
    inline void unfocus() { m_caption = nullptr; }

    //------------------------------------------------------------------------------
    void onKeyPressed(const sf::Event::KeyEvent& key)
    {
        if (m_caption == nullptr)
            return ;

        switch (key.code)
        {
        case sf::Keyboard::Left:
            if (m_cursor_pos > 0)
            {
                m_cursor_pos = m_cursor_pos - 1u;
                m_cursor.setPosition(m_text.findCharacterPos(m_cursor_pos));
            }
            break;

        case sf::Keyboard::Right:
            if (m_cursor_pos < m_caption->size())
            {
                m_cursor_pos = m_cursor_pos + 1u;
                m_cursor.setPosition(m_text.findCharacterPos(m_cursor_pos));
            }
            break;

        case sf::Keyboard::BackSpace:
            if (m_cursor_pos > 0)
            {
                m_cursor_pos =  m_cursor_pos - 1u;
                m_caption->erase(m_cursor_pos, 1u);
                m_text.setString(*m_caption);
                m_cursor.setPosition(m_text.findCharacterPos(m_cursor_pos));
            }
            break;

        case sf::Keyboard::Delete:
            if (m_cursor_pos < m_caption->size())
            {
                m_caption->erase(m_cursor_pos, 1u);
                m_text.setString(*m_caption);
                m_cursor.setPosition(m_text.findCharacterPos(m_cursor_pos));
            }
            break;

        case sf::Keyboard::Home:
            m_cursor_pos = 0u;
            m_cursor.setPosition(m_text.findCharacterPos(m_cursor_pos));
            break;

        case sf::Keyboard::End:
            m_cursor_pos = m_caption->size();
            m_cursor.setPosition(m_text.findCharacterPos(m_cursor_pos));
            break;

        case sf::Keyboard::Escape:
            m_text.setString(m_backup);
            m_caption = nullptr;
            break;

        case sf::Keyboard::Return:
            m_caption = nullptr;
            break;

#if 0
        case sf::Keyboard::V:
            if (key.control)
            {
                sf::String string = m_text.getString();
                sf::String clipboardString = sf::Clipboard::getString();
                // Trim clipboard content if needed
                if ((string.getSize() + clipboardString.getSize()) > m_maxLength)
                {
                    clipboardString = clipboardString.substring(0, m_maxLength - string.getSize());
                }
                // Insert string at cursor position
                string.insert(m_cursorPos, clipboardString);
                m_text.setString(string);
                setCursor(m_cursorPos + clipboardString.getSize());
            }
            break;
#endif
        default:
            break;
        }
    }

    //------------------------------------------------------------------------------
    void onTextEntered(sf::Uint32 unicode)
    {
        if (m_caption == nullptr)
            return ;

        if (unicode > 30 && (unicode < 127 || unicode > 159))
        {
            std::string s(1u, char(unicode));
            m_caption->insert(m_cursor_pos, s);
            m_text.setString(*m_caption);
            m_cursor_pos = m_cursor_pos + 1u;
            m_cursor.setPosition(m_text.findCharacterPos(m_cursor_pos));
        }
    }

private:

    //------------------------------------------------------------------------------
    virtual void draw(sf::RenderTarget& target, sf::RenderStates /*states*/) const override final
    {
        if (m_caption == nullptr)
            return ;

        // Make the cursor blink
        float timer = m_timer.getElapsedTime().asSeconds();
        if (timer >= FADING_PERIOD)
            m_timer.restart();

        sf::Color color = sf::Color::Black;
        color.a = sf::Uint8(255.0f - (255.0f * timer / FADING_PERIOD));
        m_cursor.setFillColor(color);

        target.draw(m_box);
        target.draw(m_cursor);
    }

private:

    sf::Text m_text;
    sf::RectangleShape m_box;
    mutable sf::RectangleShape m_cursor;
    mutable sf::Clock m_timer;
    std::string* m_caption = nullptr;
    size_t m_cursor_pos = 0u;
    std::string m_backup;
};

#endif
