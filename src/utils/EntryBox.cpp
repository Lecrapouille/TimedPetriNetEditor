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

#include "EntryBox.hpp"

#define BLINK_PERIOD 1.0f

struct Theme
{
    static constexpr float PADDING = 2.0f;
    static constexpr float borderSize = 1.0f;
    static constexpr float textSize = 11.0f;

    static float getLineSpacing(sf::Font const& font)
    {
        return font.getLineSpacing(textSize);
    }

    static float getBoxHeight(sf::Font const& font)
    {
        return getLineSpacing(font) + borderSize * 2.0f + PADDING * 2.0f;
    }
};

//------------------------------------------------------------------------------
EntryBox::EntryBox(sf::Font& font, float width)
{
    m_box.setSize({width, Theme::getBoxHeight(font)});

    float offset = Theme::borderSize + Theme::PADDING;
    m_text.setFont(font);
    m_text.setPosition(offset, offset);
    m_text.setFillColor(sf::Color::Black);
    m_text.setCharacterSize(Theme::textSize);

    // Cursor
    m_cursor.setPosition(offset, offset);
    m_cursor.setSize(sf::Vector2f(1.f, Theme::getLineSpacing(font)));
    m_cursor.setFillColor(sf::Color::Black);
    setCursor(0);
}

//------------------------------------------------------------------------------
void EntryBox::setCursor(size_t index)
{
    if (index > m_text.getString().getSize())
        return ;

    m_cursor_position = index;

    float padding = Theme::borderSize + Theme::PADDING;
    m_cursor.setPosition(m_text.findCharacterPos(index).x, padding);
    m_timer.restart();

    if (m_cursor.getPosition().x > m_box.getSize().x - padding)
    {
        // Shift text on left
        float diff = m_cursor.getPosition().x - m_box.getSize().x + padding;
        m_text.move(-diff, 0);
        m_cursor.move(-diff, 0);
    }
    else if (m_cursor.getPosition().x < padding)
    {
        // Shift text on right
        float diff = padding - m_cursor.getPosition().x;
        m_text.move(diff, 0);
        m_cursor.move(diff, 0);
    }

    float text_width = m_text.getLocalBounds().width;
    if ((m_text.getPosition().x < padding) &&
        (m_text.getPosition().x + text_width < m_box.getSize().x - padding))
    {
        float diff = (m_box.getSize().x - padding) -
                     (m_text.getPosition().x + text_width);
        m_text.move(diff, 0);
        m_cursor.move(diff, 0);
        // If text is smaller than the textbox, force align on left
        if (text_width < (m_box.getSize().x - padding * 2))
        {
            diff = padding - m_text.getPosition().x;
            m_text.move(diff, 0);
            m_cursor.move(diff, 0);
        }
    }
}

//------------------------------------------------------------------------------
void EntryBox::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    target.draw(m_box, states);

    // Crop the text with Scissor
    //glEnable(GL_SCISSOR_TEST);
    //sf::Vector2f pos = getAbsolutePosition();
    //glScissor(pos.x + Theme::borderSize, target.getSize().y - (pos.y + getSize().y), getSize().x, getSize().y);
    target.draw(m_text, states);

    //glDisable(GL_SCISSOR_TEST);

    // Show cursor if focused
    if (m_bind != nullptr)
    {
        // Make it blink
        float timer = m_timer.getElapsedTime().asSeconds();
        if (timer >= BLINK_PERIOD)
            m_timer.restart();

        // Updating in the drawing method, deal with it
        sf::Color color = sf::Color::Black;
        color.a = sf::Uint8(255.0f - (255.0f * timer / BLINK_PERIOD));
        m_cursor.setFillColor(color);

        target.draw(m_cursor, states);
    }
}
