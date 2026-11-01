// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef TPNE_COLOR_HPP
#  define TPNE_COLOR_HPP

#  include <cstdint>
#  include <optional>
#  include <string>
#  include <vector>

namespace tpne {

//------------------------------------------------------------------------------
//! \brief Color of a token: the result given by the function of the transition
//! that produced it (Success, Failed ...) or a color of the user. It is the
//! index of its name in the ColorTable of the net. Colors decide which
//! transition takes a token: an arc from a place asks for a color.
//------------------------------------------------------------------------------
using Color = uint8_t;

//! \brief On an arc from a place: a token of any color. On an arc to a place:
//! the color given by the transition (its result).
constexpr Color AnyColor = 0u;
//! \brief Results of a transition: done, failed, canceled. Also the results of
//! an instance of a runtime, with Deadlocked (nothing can happen any more).
constexpr Color Success = 1u;
constexpr Color Failed = 2u;
constexpr Color Canceled = 3u;
constexpr Color Deadlocked = 4u;

// *****************************************************************************
//! \brief Names of the colors of a net: the built-in ones ("any", "Success",
//! "Failed", "Canceled", "Deadlocked") then the ones of the user. A color may
//! also say how it is drawn (rgb()): a product drawn with the color of its
//! parts, for example. Without it the editor picks a hue from its number.
// *****************************************************************************
class ColorTable
{
public:

    ColorTable() : m_names({ "any", "Success", "Failed", "Canceled", "Deadlocked" }) {}

    //--------------------------------------------------------------------------
    //! \brief The color of this name, added if unknown. AnyColor when the
    //! table is full (256 colors) or for an empty name.
    //--------------------------------------------------------------------------
    Color add(std::string const& name)
    {
        if (name.empty())
            return AnyColor;
        if (std::optional<Color> const color = find(name))
            return *color;
        if (m_names.size() > 255u)
            return AnyColor;
        m_names.push_back(name);
        m_rgb.resize(m_names.size());
        return Color(m_names.size() - 1u);
    }

    //--------------------------------------------------------------------------
    //! \brief Draws the color with this RGB value (0xRRGGBB). Ignored for an
    //! unknown color.
    //--------------------------------------------------------------------------
    void rgb(Color const color, std::uint32_t const value)
    {
        if (color < m_names.size())
        {
            m_rgb.resize(m_names.size());
            m_rgb[color] = value & 0xFFFFFFu;
        }
    }

    //--------------------------------------------------------------------------
    //! \brief The RGB value of the color (0xRRGGBB), if it has one.
    //--------------------------------------------------------------------------
    std::optional<std::uint32_t> rgb(Color const color) const
    {
        return (color < m_rgb.size()) ? m_rgb[color] : std::nullopt;
    }

    //--------------------------------------------------------------------------
    //! \brief The color of this name, if known.
    //--------------------------------------------------------------------------
    std::optional<Color> find(std::string const& name) const
    {
        for (size_t i = 0u; i < m_names.size(); ++i)
        {
            if (m_names[i] == name)
                return Color(i);
        }
        return std::nullopt;
    }

    //--------------------------------------------------------------------------
    //! \brief The name of the color ("Color42" if unknown).
    //--------------------------------------------------------------------------
    std::string name(Color const color) const
    {
        return (color < m_names.size()) ? m_names[color] : "Color" + std::to_string(color);
    }

    //--------------------------------------------------------------------------
    //! \brief All the names, by color, and the names of the user only.
    //--------------------------------------------------------------------------
    std::vector<std::string> const& names() const { return m_names; }
    std::vector<std::string> user() const
    {
        return std::vector<std::string>(m_names.begin() + BUILT_IN, m_names.end());
    }

    //! \brief Number of built-in colors.
    static constexpr std::ptrdiff_t BUILT_IN = 5;

private:

    std::vector<std::string> m_names;
    //! \brief RGB value of each color, if it has one.
    std::vector<std::optional<std::uint32_t>> m_rgb;
};

//------------------------------------------------------------------------------
//! \brief "#e61a1a" for 0xE61A1A.
//------------------------------------------------------------------------------
std::string toHex(std::uint32_t const rgb);

//------------------------------------------------------------------------------
//! \brief 0xE61A1A for "#e61a1a" or "#E61A1A"; nothing for another text.
//------------------------------------------------------------------------------
std::optional<std::uint32_t> fromHex(std::string const& text);

} // namespace tpne

#endif
