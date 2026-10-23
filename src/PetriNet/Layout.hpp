// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef TPNE_LAYOUT_HPP
#  define TPNE_LAYOUT_HPP

#  include <cstddef>
#  include <vector>

namespace tpne {

// *****************************************************************************
//! \brief Position of a node on the drawing, in net units.
// *****************************************************************************
struct Point
{
    float x = 0.0f;
    float y = 0.0f;
};

// *****************************************************************************
//! \brief Positions of the nodes of a net, by node identifier. Places and
//! transitions only hold their structure (tokens, captions ...): the drawing
//! is apart, so the engines and the algorithms never depend on it. The net
//! keeps its layout in line with its nodes (added, removed, renumbered).
// *****************************************************************************
class Layout
{
public:

    //! \brief Position of the place or of the transition of the identifier.
    Point& place(size_t const id) { return at(m_places, id); }
    Point const& place(size_t const id) const { return at(m_places, id); }
    Point& transition(size_t const id) { return at(m_transitions, id); }
    Point const& transition(size_t const id) const { return at(m_transitions, id); }

    //! \brief Number of positions known.
    size_t places() const { return m_places.size(); }
    size_t transitions() const { return m_transitions.size(); }

    //! \brief Same number of positions as of nodes (new ones at 0, 0).
    void resize(size_t const places, size_t const transitions)
    {
        m_places.resize(places);
        m_transitions.resize(transitions);
    }

    //! \brief The last position takes the place of the removed one (as the
    //! last node of a net takes the identifier of a removed one).
    void removePlace(size_t const id) { remove(m_places, id); }
    void removeTransition(size_t const id) { remove(m_transitions, id); }

    void clear()
    {
        m_places.clear();
        m_transitions.clear();
    }

private:

    static Point& at(std::vector<Point>& points, size_t const id)
    {
        if (id >= points.size())
            points.resize(id + 1u);
        return points[id];
    }

    static Point const& at(std::vector<Point> const& points, size_t const id)
    {
        static Point const origin;
        return (id < points.size()) ? points[id] : origin;
    }

    static void remove(std::vector<Point>& points, size_t const id)
    {
        if (id >= points.size())
            return;
        points[id] = points.back();
        points.pop_back();
    }

private:

    std::vector<Point> m_places;
    std::vector<Point> m_transitions;
};

} // namespace tpne

#endif
