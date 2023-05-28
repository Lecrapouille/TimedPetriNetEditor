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

#ifndef ENTRY_BOX_HPP
#  define ENTRY_BOX_HPP

#  include "utils/Theme.hpp"
#  include "PetriNet.hpp"

// *****************************************************************************
//! \brief Allow to modify the caption of places and transitions.
// *****************************************************************************
class EntryBox : public sf::Drawable
{
private:

    class Cursor
    {
    public:

        void blink()
        {
            float s = timer.getElapsedTime().asSeconds();
            if (s >= FADING_PERIOD)
                timer.restart();
            color.a = sf::Uint8(255.0f - (255.0f * s / FADING_PERIOD));
            shape.setFillColor(color);
        }

        //! \brief Draw the cursor for editing.
        sf::RectangleShape shape;
        //! \brief Blinking cursor.
        sf::Clock timer;
        //! \brief index of the cursor on the text.
        size_t index = 0;
        //! \brief Color
        sf::Color color = sf::Color::Black;
    };

public:

    EntryBox(sf::Font const& font)
    {
        m_text.setFont(font);
        m_text.setCharacterSize(CAPTION_FONT_SIZE);
        m_text.setFillColor(sf::Color::Black);

        m_box.setFillColor({255,255,255,0});
        m_box.setOutlineThickness(1.0f);
        m_box.setOutlineColor(OUTLINE_COLOR);
    }

    //--------------------------------------------------------------------------
    inline bool hasFocus() const { return m_caption != nullptr; }
    inline void unfocus() { m_caption = nullptr; }

    //--------------------------------------------------------------------------
    bool canFocusOn(std::string& caption, float* fvalue, float const caption_x,
                    float const caption_y, sf::Vector2f const& mouse)
    {
        // Since node does not use directly sf::Text we have to compute it back
        m_text.setString(caption);
        const float x = caption_x - m_text.getLocalBounds().width / 2.0f;
        const float y = caption_y - m_text.getLocalBounds().height -
                        PLACE_RADIUS - CAPTION_FONT_SIZE / 2.0f - 2.0f;
        m_text.setPosition(x, y);

        // Mouse cursor inside the caption of the node ?
        if (!m_text.getGlobalBounds().contains(mouse))
        {
            return false;
        }

        // Update the box size to draw it.
        sf::FloatRect textBounds = m_text.getGlobalBounds();
        m_box.setPosition(textBounds.left, textBounds.top);
        m_box.setSize({textBounds.width, textBounds.height});

        // Place cursor after the character under the mouse
        m_cursor.index = findCursorPosition(mouse.x);
        m_cursor.shape.setPosition(m_text.findCharacterPos(m_cursor.index) +
                                   sf::Vector2f(0.f, 2.0f));
        m_cursor.shape.setSize({2.0f, textBounds.height});
        m_cursor.timer.restart();

        // Already focused on this caption ? Memorize text for its restoration
        if (m_caption != &caption)
        {
            m_backup = caption;
        }

        m_caption = &caption;
        m_caption_x = caption_x;
        m_caption_y = caption_y;
        m_fvalue = fvalue;
        return true;
    }

    //--------------------------------------------------------------------------
    void refresh()
    {
        // Since node does not use directly sf::Text we have to compute it back
        m_text.setString(*m_caption);
        const float x = m_caption_x - m_text.getLocalBounds().width / 2.0f;
        const float y = m_caption_y - m_text.getLocalBounds().height -
                        PLACE_RADIUS - CAPTION_FONT_SIZE / 2.0f - 2.0f;
        m_text.setPosition(x, y);

        // Update the box size to draw it.
        sf::FloatRect textBounds = m_text.getGlobalBounds();
        m_box.setPosition(textBounds.left, textBounds.top);
        m_box.setSize({ std::max(2.0f, textBounds.width), textBounds.height });

        // Place cursor after the character under the mouse
        m_cursor.shape.setPosition(m_text.findCharacterPos(m_cursor.index) +
                                   sf::Vector2f(0.f, 2.0f));
        m_cursor.shape.setSize({2.0f, textBounds.height});
        m_cursor.timer.restart();
    }

    //--------------------------------------------------------------------------
    void onMousePressed(sf::Vector2f const& mouse)
    {
        m_cursor.index = findCursorPosition(mouse.x);
        refresh();
    }

    //--------------------------------------------------------------------------
    void onKeyPressed(const sf::Event::KeyEvent& key, bool& modified)
    {
        if (m_caption == nullptr)
            return ;

        switch (key.code)
        {
        case sf::Keyboard::Left:
            if (m_cursor.index > 0)
            {
                m_cursor.index = m_cursor.index - 1u;
                refresh();
            }
            break;

        case sf::Keyboard::Right:
            if (m_cursor.index < m_caption->size())
            {
                m_cursor.index = m_cursor.index + 1u;
                refresh();
            }
            break;

        case sf::Keyboard::BackSpace:
            if (m_cursor.index > 0)
            {
                m_cursor.index = m_cursor.index - 1u;
                m_caption->erase(m_cursor.index, 1u);
                refresh();
            }
            break;
        case sf::Keyboard::Delete:
            if (m_cursor.index < m_caption->size())
            {
                m_caption->erase(m_cursor.index, 1u);
                refresh();
            }
            break;

        case sf::Keyboard::Home:
            m_cursor.index = 0u;
            refresh();
            break;

        case sf::Keyboard::End:
            m_cursor.index = m_caption->size();
            refresh();
            break;

        case sf::Keyboard::Escape:
            *m_caption = m_backup;
            m_caption = nullptr;
            break;

        case sf::Keyboard::Return:
            if (m_caption->empty())
            {
                *m_caption = m_backup;
            }
            else
            {
                if (m_fvalue != nullptr)
                {
                    *m_fvalue = float(std::atof(m_caption->c_str()));
                }
                modified = true;
            }
            m_caption = nullptr;
            break;

        default:
            break;
        }
    }

    //--------------------------------------------------------------------------
    void onTextEntered(sf::Uint32 unicode)
    {
        if (m_caption == nullptr)
            return ;

        if (unicode > 30 && (unicode < 127 || unicode > 159))
        {
            std::string c(1u, char(unicode));
            m_caption->insert(m_cursor.index, c);
            m_cursor.index = m_cursor.index + 1u;
            refresh();
        }
    }

private:

    //--------------------------------------------------------------------------
    size_t findCursorPosition(float const x)
    {
        size_t i = m_text.getString().getSize();
        while (i--)
        {
            if (m_text.findCharacterPos(i).x <= x)
            {
                return i + 1u;
            }
        }

        return 0u;
    }

    //--------------------------------------------------------------------------
    virtual void draw(sf::RenderTarget& target,
                      sf::RenderStates /*states*/) const override final
    {
        if (m_caption == nullptr)
            return ;

        m_cursor.blink();
        target.draw(m_box);
        target.draw(m_cursor.shape);
    }

private:

    //! \brief Reference the caption of the selected Petri node.
    std::string* m_caption = nullptr;
    float m_caption_x; float m_caption_y; float* m_fvalue = nullptr;
    //! \brief Memorize the initial caption when we need to restore it.
    std::string m_backup;
    //! \brief Since Node does not use sf::Text we have to get this information
    //! back
    sf::Text m_text;
    //! \brief Draw a box around the focused text.
    sf::RectangleShape m_box;
    //! \brief Blinking cursor for editing the text.
    mutable Cursor m_cursor;
};

#endif
