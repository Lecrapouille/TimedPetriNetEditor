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

#ifndef BEZIER_HPP
#  define BEZIER_HPP

#  include <SFML/Graphics/RectangleShape.hpp>
#  include "utils/Utils.hpp"
#  include <algorithm>

// *****************************************************************************
//! \brief Class allowing to draw an arrow. Arrows are needed for drawing Petri
//! arcs. This class implements https://github.com/dragonman225/curved-arrows
//! described in https://dragonman225.js.org/curved-arrows.html by Alexander Wang
//! (MIT license).
// *****************************************************************************
class Bezier final : public sf::Drawable // FIXME sf::CircleShape
{
public:

    Bezier(sf::Vector2f const& start, sf::Vector2f const& end, const uint8_t alpha)
    {
        init(start, end);
    }

    Bezier(float const& sx, float const& sy, float const& ex, float const& ey, const uint8_t alpha)
    {
        init(sx, sy, 0.0f, 0.0f, ex, ey, 0.0f, 0.0f);
    }

    void init(sf::Vector2f const& start, sf::Vector2f const& end)
    {
        init(start.x, start.y, 0.0f, 0.0f, end.x, end.y, 0.0f, 0.0f);
    }

private:

    void init(float const x0, float const y0, float const w0, float const h0,
              float const x1, float const y1, float const w1, float const h1)
    {
        // options
        float const keepOutZone = 25.0f;
        const float padStart = 7.0f;
        const float padEnd = 7.0f;

        const sf::Rect<float> startBox(x0, y0, w0, h0);
        const sf::Vector2f startAtTop(x0 + w0 / 2.0f, y0 - 2.0f * padStart);
        const sf::Vector2f startAtBottom(x0 + w0 / 2.0f, y0 + h0 + 2.0f * padStart);
        const sf::Vector2f startAtLeft(x0 - 2.0f * padStart, y0 + h0 / 2.0f);
        const sf::Vector2f startAtRight(x0 + w0 + 2.0f * padStart, y0 + h0 / 2.0f);
        const std::vector<sf::Vector2f> startPoints = { startAtTop, startAtRight, startAtBottom, startAtLeft };

        const sf::Rect<float> endBox(x1, y1, w1, h1);
        const sf::Vector2f endAtTop(x1 + w1 / 2.0f, y1 - 2.0f * padEnd);
        const sf::Vector2f endAtBottom(x1 + w1 / 2, y1 + h1 + 2.0f * padEnd);
        const sf::Vector2f endAtLeft(x1 - 2 * padEnd, y1 + h1 / 2.0f);
        const sf::Vector2f endAtRight(x1 + w1 + 2 * padEnd, y1 + h1 / 2.0f);
        const std::vector<sf::Vector2f> endPoints = { endAtTop, endAtRight, endAtBottom, endAtLeft };

        float shortestDistance = 1.0f / 0.0f;
        sf::Vector2f bestStartPoint = startAtTop;
        sf::Vector2f bestEndPoint = endAtTop;
        size_t bestStartSide = 0u;
        size_t bestEndSide = 0u;

        for (size_t startSideId = 0u; startSideId < startPoints.size(); startSideId++)
        {
            sf::Vector2f const startPoint = startPoints[startSideId];
            if (isPointInBox(startPoint, growBox(endBox, keepOutZone)))
                continue ;

            for (size_t endSideId = 0; endSideId < endPoints.size(); endSideId++)
            {
                sf::Vector2f const endPoint = endPoints[endSideId];

                // If the start point is in the rectangle of end, or the end point
                // is in the rectangle of start, this combination is abandoned.
                if (isPointInBox(endPoint, growBox(startBox, keepOutZone)))
                    continue ;

                float const d = distance(startPoint, endPoint);
                if (d < shortestDistance)
                {
                    shortestDistance = d;
                    bestStartPoint = startPoint;
                    bestEndPoint = endPoint;
                    bestStartSide = startSideId;
                    bestEndSide = endSideId;
                }
            }
        }

        m_start = bestStartPoint;
        m_end = bestEndPoint;
        m_controls[0] = controlPointOf(bestStartPoint, bestEndPoint, bestStartSide);
        m_controls[1] = controlPointOf(bestEndPoint, bestStartPoint, bestEndSide);
        m_angles[0] = angleOf(bestStartSide);
        m_angles[1] = angleOf(bestEndSide);
        m_init = true;
    }

    inline bool isPointInBox(sf::Vector2f const& p, sf::Rect<float> const& r) const
    {
        return r.contains(p);
    }

    inline sf::Rect<float> growBox(sf::Rect<float> const& box, float const size) const
    {
        return { box.left - size, box.top - size, box.width + 2.0f * size, box.height + 2.0f * size };
    }

    inline float angleOf(size_t const& side) const
    {
        static const float angles[4u] = { 90.0f, 180.0f, 270.0f, 0.0f }; // top, right, bottom, left
        return angles[side];
    }

    sf::Vector2f controlPointOf(sf::Vector2f const& target, sf::Vector2f const& another, size_t const sideOfTarget)
    {
        // The distance a control point must be far away from the target in the
        // direction of leaving the target.
        float minDistanceToTarget = 25.0f;

        if (sideOfTarget == 0u) // top
            return { target.x, std::min((target.y + another.y) / 2.0f, target.y - minDistanceToTarget) };
        if (sideOfTarget == 1u) // right
            return { std::max((target.x + another.x) / 2.0f, target.x + minDistanceToTarget), target.y };
        if (sideOfTarget == 2u) // bottom
            return { target.x, std::max((target.y + another.y) / 2.0f, target.y + minDistanceToTarget) };
        // left
        return { std::min((target.x + another.x) / 2.0f, target.x - minDistanceToTarget), target.y };
    }

private: // Override sf::Drawable

    // FIXME Cache lines
    virtual void draw(sf::RenderTarget& target, sf::RenderStates /*states*/) const override final
    {
        const size_t SEGMENT_COUNT = 16;
        assert(m_init && "Bezier curbe not init");

        sf::Vector2f p0 = calculateBezierPoint(0.0f, m_start, m_controls[0], m_controls[1], m_end);
        for(size_t i = 1u; i <= SEGMENT_COUNT; i++)
        {
            const float t = float(i) / float(SEGMENT_COUNT);
            sf::Vector2f p1 = calculateBezierPoint(t, m_start, m_controls[0], m_controls[1], m_end);
            drawLine(target, p0, p1);
            p0 = p1;
        }

        // Head of the arrow
        const sf::Vector2f arrowHeadSize{ 10.0f, 10.0f };
        sf::ConvexShape head(3);
        head.setPoint(0, { 0.0f, 0.0f });
        head.setPoint(1, { arrowHeadSize.x, arrowHeadSize.y / 2.0f });
        head.setPoint(2, { 0.0f, arrowHeadSize.y });
        head.setOrigin(arrowHeadSize.x, arrowHeadSize.y / 2.0f);
        head.setPosition(sf::Vector2f(m_end));
        head.setRotation(m_angles[1]);
        head.setFillColor(OUTLINE_COLOR);
        target.draw(head);
    }

    void drawLine(sf::RenderTarget& target, sf::Vector2f const& p0, sf::Vector2f const& p1) const
    {
        const float depth = 2.0f;

        sf::RectangleShape tail(sf::Vector2f(distance(p0, p1), depth));
        tail.setPosition(p0);
        tail.setOrigin(0.0f, 0.5f * depth);
        tail.setRotation(orientation(p0, p1));
        tail.setFillColor(OUTLINE_COLOR);
        target.draw(tail);
    }

    float orientation(sf::Vector2f const& p0, sf::Vector2f const& p1) const
    {
        const float teta = (p1.y - p0.y) / (p1.x - p0.x);
        float arrowAngle = std::atan(teta) * 180.0f / 3.1415f; // rad -> deg
        if (p1.x < p0.x)
            arrowAngle += 180.0f;
        else if (p1.y < p0.y)
            arrowAngle += 360.0f;
        return arrowAngle;
    }

    sf::Vector2f
    calculateBezierPoint(const float t, sf::Vector2f const& p0,
                         sf::Vector2f const& p1, sf::Vector2f const& p2,
                         sf::Vector2f const& p3) const
    {
        assert(((t >= 0.0f) && (t <= 1.0f)) && "Out of range t = [0 .. 1]");
        const float t2 = t * t;
        const float t3 = t2 * t;
        const float u = 1.0f - t;
        const float u2 = u * u;
        const float u3 = u2 * u;

        sf::Vector2f p = u3 * p0; // First term
        p += 3.0f * u2 * t * p1;  // Second term
        p += 3.0f * u * t2 * p2;  // Third term
        p += t3 * p3;             // Fourth term

        return p;
    }

private:

    bool m_init = false; // FIXME cached m_lines ::size() != 0u
    sf::Vector2f m_start;
    sf::Vector2f m_end;
    sf::Vector2f m_controls[2];
    float m_angles[2]; // degrees
};

#endif