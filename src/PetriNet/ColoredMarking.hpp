// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef TPNE_COLORED_MARKING_HPP
#  define TPNE_COLORED_MARKING_HPP

#  include "PetriNet/Color.hpp"
#  include "PetriNet/Firing.hpp"

#  include <cstddef>
#  include <utility>
#  include <vector>

namespace tpne {

// *****************************************************************************
//! \brief Tokens of each color in each place.
// *****************************************************************************
class ColoredMarking
{
public:

    explicit ColoredMarking(size_t const places = 0u) : m_counts(places), m_is_touched(places, 0) {}

    //! \brief Number of places.
    size_t places() const { return m_counts.size(); }

    //! \brief Tokens of the color in the place (AnyColor: of all colors).
    size_t count(size_t const place, Color const color = AnyColor) const;

    //! \brief Add tokens of the color in the place.
    void add(size_t const place, Color const color, size_t const tokens = 1u);

    //! \brief Remove tokens of the color (AnyColor: the first colors first).
    //! Return false, removing nothing, when there are not enough.
    bool take(size_t const place, Color const color, size_t const tokens);

    //! \brief Remove all the tokens of the color (AnyColor: all).
    void clear(size_t const place, Color const color = AnyColor);

    //! \brief (color, count) of the tokens of the place.
    std::vector<std::pair<Color, size_t>> tokens(size_t const place) const;

    //! \brief Number of tokens of each place, colors not considered.
    Marking counts() const;

    bool operator==(ColoredMarking const& other) const { return m_counts == other.m_counts; }

    //! \brief The tokens of the place change (add(), take(), clear() call it):
    //! the runtime checks again the transitions around it.
    void touch(size_t const place)
    {
        if ((place < m_is_touched.size()) && !m_is_touched[place])
        {
            m_is_touched[place] = 1;
            m_touched.push_back(place);
        }
    }

    //! \brief The places touched since the last call.
    std::vector<size_t> takeTouched()
    {
        std::vector<size_t> touched;
        takeTouched(touched);
        return touched;
    }

    //! \brief The same in a vector given (its memory is kept for the next
    //! ones: no allocation at each step).
    void takeTouched(std::vector<size_t>& touched)
    {
        for (size_t const p : m_touched)
            m_is_touched[p] = 0;
        touched.clear();
        touched.swap(m_touched);
    }

private:

    //! \brief Tokens by place, then by color.
    std::vector<std::vector<size_t>> m_counts;
    //! \brief Places whose tokens changed, not yet seen by the runtime.
    std::vector<size_t> m_touched;
    std::vector<char> m_is_touched;
};

} // namespace tpne

#endif
