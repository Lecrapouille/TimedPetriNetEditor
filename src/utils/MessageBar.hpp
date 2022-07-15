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

#ifndef MESSAGEBAR_HPP
#define MESSAGEBAR_HPP


// *****************************************************************************
//! \brief A text inside a rectangle
// *****************************************************************************
class MessageBar : public sf::Drawable
{
public:

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    MessageBar(sf::Font& font)
    {
        m_text.setPosition(0, 0);
        m_text.setFont(font);
        m_text.setCharacterSize(20);
        m_text.setFillColor(sf::Color::Black);

        m_shape.setFillColor(sf::Color(100,100,100));
        m_shape.setOutlineThickness(-1);
        m_shape.setOutlineColor(sf::Color::Black);

        m_timer.restart();
    }

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    void setText(const std::string& message, sf::Color const& color)
    {
        m_message = message;
        m_shape.setFillColor(color);
        m_text.setString(m_message);
        m_timer.restart();
    }

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    inline void setInfo(const std::string& message)
    {
        setText(message, sf::Color::Green);
    }

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    inline void setWarning(const std::string& message)
    {
        setText(message, sf::Color(255, 126, 0));
        std::cerr << message << std::endl;
    }

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    inline void setError(const std::string& message)
    {
        setText(message, sf::Color::Red);
        std::cerr << message << std::endl;
    }

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    void setSize(sf::Vector2u const& dimensions)
    {
        m_shape.setSize(sf::Vector2f(float(dimensions.x), 25.0f));
    }

private:

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    virtual void draw(sf::RenderTarget& target, sf::RenderStates /*states*/) const override final
    {
        const float BLINK_PERIOD = 2.5f;

        float period = m_timer.getElapsedTime().asSeconds();
        if (period >= BLINK_PERIOD)
        {
            period = BLINK_PERIOD;
        }
        else
        {
            target.draw(m_shape);
            target.draw(m_text);
        }
    }

private:

    //! \brief Timer for removing the text
    sf::Clock m_timer;

    //! \brief Text displayed on the entry
    sf::Text m_text;

    //! \brief Handles appearance of the entry
    sf::RectangleShape m_shape;

    //! \brief String returned when the entry is activated
    std::string m_message;
};

#endif
