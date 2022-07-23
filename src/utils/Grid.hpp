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

#  include <SFML/Graphics.hpp>
#  include "utils/Theme.hpp"
#  include <vector>
#  include <cassert>

class Grid: public sf::Drawable
{
public:

    Grid(sf::Rect<float> const& bounds)
    {
        resize(bounds);
    }

    void resize(sf::Rect<float> const& bounds)
    {
        resize(bounds, sf::Vector2f(bounds.width / TRANS_WIDTH, bounds.height / TRANS_WIDTH));
    }

private:

    void resize(sf::Rect<float> const& bounds, sf::Vector2f const& dimensions)
    {
        const float x = bounds.left;
        const float y = bounds.top;
        const float w = bounds.width;
        const float h = bounds.height;

        const float dx = bounds.width / dimensions.x;
        const float dy = bounds.height / dimensions.y;

        assert(dimensions.x != 0u);
        assert(dimensions.y != 0u);

        // Vertical lines
        for (uint32_t u = 0u; u <= uint32_t(dimensions.x); ++u)
        {
            m_lines.push_back(sf::Vertex(sf::Vector2f(x + dx * float(u), y), color));
            m_lines.push_back(sf::Vertex(sf::Vector2f(x + dx * float(u), y + h), color));
        }

        // Horizontal lines
        for (uint32_t u = 0u; u <= uint32_t(dimensions.y); ++u)
        {
            m_lines.push_back(sf::Vertex(sf::Vector2f(x, y + dy * float(u)), color));
            m_lines.push_back(sf::Vertex(sf::Vector2f(x + w, y + dy * float(u)), color));
        }
    }

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override final
    {
        target.draw(m_lines.data(), m_lines.size(), sf::Lines, states);
    }

public:

    sf::Color color = sf::Color::Black;
    bool show = false;

private:

    std::vector<sf::Vertex> m_lines;
};
