//=====================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 Quentin Quadrat <lecrapouille@gmail.com>
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

#ifndef ARROW_HPP
#  define ARROW_HPP

#  include <SFML/Graphics.hpp>
#  include "utils/Utils.hpp"
#  include "Settings.hpp"

// *****************************************************************************
//! \brief Class allowing to draw an arrow. Arrows are needed for drawing Petri
//! arcs.
// *****************************************************************************
class Arrow : public sf::Drawable
{
public:

    Arrow(const float xa, const float ya, const float xb, const float yb,
          const uint8_t alpha)
    {
        // Arc magnitude
        const float arrowLength = norm(xa, ya, xb, yb);

        // Orientation
        const float teta = (yb - ya) / (xb - xa);
        float arrowAngle = std::atan(teta) * 180.0f / 3.1415f; // rad -> deg
        if (xb < xa)
            arrowAngle += 180.f;
        else if (yb < ya)
            arrowAngle += 360.f;

        // Reduce the arrow magnitude to avoid entering in the place and having
        // a mush of pixels when multiple arrows are pointing on the same
        // position. To get full scaled arrow comment this block of code and
        // uncomment xa, xb, ya, yb and tailSize.
        float r = arrowLength - PLACE_RADIUS;
        float dx = ((xb - xa) * r) / arrowLength;
        float dy = ((yb - ya) * r) / arrowLength;
        float a1 = xb - dx;
        float b1 = yb - dy;
        float a2 = xa + dx;
        float b2 = ya + dy;

        // Head of the arrow
        const sf::Vector2f arrowHeadSize{ 14.f, 14.f };
        m_head = sf::ConvexShape{ 3 };
        m_head.setPoint(0, { 0.f, 0.f });
        m_head.setPoint(1, { arrowHeadSize.x, arrowHeadSize.y / 2.f });
        m_head.setPoint(2, { 0.f, arrowHeadSize.y });
        m_head.setOrigin(arrowHeadSize.x, arrowHeadSize.y / 2.f);
        m_head.setPosition(sf::Vector2f(a2, b2 /*xb, yb*/));
        m_head.setRotation(arrowAngle);

        // Tail of the arrow.
        //const sf::Vector2f tailSize{ arrowLength - arrowHeadSize.x, 2.f };
        const sf::Vector2f tailSize{ r - arrowHeadSize.x - 15, 2.f };
        m_tail = sf::RectangleShape{ tailSize };
        m_tail.setOrigin(0.f, tailSize.y / 2.f);
        m_tail.setPosition(sf::Vector2f(a1, b1 /*xa, ya*/));
        m_tail.setRotation(arrowAngle);

        if (alpha > 0u)
        {
            m_head.setFillColor(FILL_COLOR(alpha));
            m_tail.setFillColor(FILL_COLOR(alpha));
        }
        else if (alpha == 0u)
        {
            m_head.setFillColor(OUTLINE_COLOR);
            m_tail.setFillColor(OUTLINE_COLOR);
        }
        else
        {
            // hack to show critical cycles
            m_head.setFillColor(CRITICAL_COLOR);
            m_tail.setFillColor(CRITICAL_COLOR);
        }
    }

    Arrow(sf::Vector2f const& startPoint, sf::Vector2f const& endPoint, const uint8_t alpha)
        : Arrow(startPoint.x, startPoint.y, endPoint.x, endPoint.y, alpha)
    {}

private:

    void draw(sf::RenderTarget& target, sf::RenderStates /*states*/) const override final
    {
        target.draw(m_tail);
        target.draw(m_head);
    }

private:

    sf::RectangleShape m_tail;
    sf::ConvexShape m_head;
};

#endif
