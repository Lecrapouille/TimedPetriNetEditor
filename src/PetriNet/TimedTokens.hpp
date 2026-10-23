// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef TIMED_TOKENS_HPP
#  define TIMED_TOKENS_HPP

#  include <cstddef>

namespace tpne {

class Net;
enum class TypeOfNet;

// *****************************************************************************
//! \brief Tokens are systems resources. Places indicate how many tokens they
//! have but in this project, when simulation is run, we want to render them
//! moving along arcs Transitions -> Places (note there is no animation for arcs
//! Places -> Transitions: they are teleported). For the rendering, instead of
//! showing many tokens (dots) at the same position, we "group" them as a dot
//! with the number of tokens carried as caption. Since we are working on timed
//! petri nets arcs have a duration which is also constrain their velocity.
// *****************************************************************************
struct TimedToken
{
    //--------------------------------------------------------------------------
    //! \brief Constructor. The positions of the ends of the arc are read now.
    //! \param[in] net: the net owning the arc.
    //! \param[in] arc_: index in Net::arcs() of the arc the tokens are moving
    //! along. Shall be an arc Transition -> Place.
    //! \param[in] tokens_: the number of tokens it shall carry.
    //! \param[in] type_: Type of the net (Petri, timed Petri, GRAFCET ...)
    //--------------------------------------------------------------------------
    TimedToken(Net const& net, size_t const arc_, size_t const tokens_,
               TypeOfNet const type_);

    //--------------------------------------------------------------------------
    //! \brief Update position on the screen.
    //! \param[in] dt: the delta time (in seconds) from the previous call.
    //! \return true when arriving to the destination node (Place) else false.
    //--------------------------------------------------------------------------
    bool update(float const dt);

    //! \brief Index in Net::arcs() of the arc the token is moving along.
    size_t arc;
    //! \brief Identifier of the destination place.
    size_t place;
    //! \brief X-axis coordinate in the window used for the display.
    float x;
    //! \brief Y-axis coordinate in the window used for the display.
    float y;
    //! \brief Number of carried tokens.
    size_t tokens;
    //! \brief
    TypeOfNet type;
    //! \brief Origin and destination positions of the animation (for timed
    //! event graphs, the destination is the transition after the place).
    float x0, y0, x1, y1;
    //! \brief The length of the arc.
    float magnitude;
    //! \brief Time to travel the arc (seconds).
    float travel_time = 1.0f;
    //! \brief The speed of the token moving along the arc.
    float speed;
    //! \brief What ratio the token has transitioned over the arc (0%: origin
    //! position, 100%: destination position).
    float offset = 0.0f;
};

} // namespace tpne

#endif
