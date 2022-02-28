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

#ifndef ANIMATION_HPP
#  define ANIMATION_HPP

#  include "PetriNet.hpp"
#  include "utils/Utils.hpp"

// *****************************************************************************
//! \brief Tokens are systems resources. Places indicate how many tokens they
//! have but in this project, when simulation is run, we want to render them
//! moving along arcs Transitions -> Places (note there is no animation for arcs
//! Places -> Transitions: they are teleported). For the rendering, instead of
//! showing many tokens (dots) at the same position, we "group" them as a dot
//! with the number of tokens carried as caption. Since we are working on timed
//! petri nets arcs have a duration which is also constrain their velocity.
// *****************************************************************************
struct AnimatedToken
{
    //--------------------------------------------------------------------------
    //! \brief Constructor.
    //! \param[in] arc: to which arc token are moving along. Shall be an arc
    //! Transition -> Place. No check is performed here.
    //! \param[in] tokens: the number of tokens it shall carry.
    //--------------------------------------------------------------------------
    AnimatedToken(Arc& arc_, size_t tokens_)
        : x(arc_.from.x), y(arc_.from.y), tokens(tokens_), arc(arc_)
    {
        assert(arc.from.type == Node::Type::Transition);
        assert(arc.to.type == Node::Type::Place);

        // Note: we are supposing the norm and duration is never updated by
        // the user during the simulation.
        magnitude = norm(arc.from.x, arc.from.y, arc.to.x, arc.to.y);
        speed = magnitude / arc.duration;
    }

    //--------------------------------------------------------------------------
    //! \brief Hack needed because of references
    //--------------------------------------------------------------------------
    AnimatedToken& operator=(AnimatedToken const& other)
    {
        this->~AnimatedToken(); // destroy
        new (this) AnimatedToken(other); // copy construct in place
        return *this;
    }

    //--------------------------------------------------------------------------
    //! \brief Needed to remove compilation warnings
    //--------------------------------------------------------------------------
    AnimatedToken(AnimatedToken const& other)
        : AnimatedToken(other.arc, other.tokens)
    {}

    //--------------------------------------------------------------------------
    //! \brief Needed to remove compilation warnings
    //--------------------------------------------------------------------------
    AnimatedToken(AnimatedToken&& other)
        : AnimatedToken(other.arc, other.tokens)
    {}

    //--------------------------------------------------------------------------
    //! \brief Needed to remove compilation warnings
    //--------------------------------------------------------------------------
    AnimatedToken& operator=(AnimatedToken&& other)
    {
        this->~AnimatedToken(); // destroy
        new (this) AnimatedToken(other); // copy construct in place
        return *this;
    }

    //--------------------------------------------------------------------------
    //! \brief Update position on the screen.
    //! \param[in] dt: the delta time (in seconds) from the previous call.
    //! \return true when arriving to the destination node (Place) else false.
    //--------------------------------------------------------------------------
    bool update(float const dt)
    {
        offset += dt * speed / magnitude;
        x = arc.from.x + (arc.to.x - arc.from.x) * offset;
        y = arc.from.y + (arc.to.y - arc.from.y) * offset;

        return (offset >= 1.0);
    }

    //--------------------------------------------------------------------------
    //! \brief Return the reference of the destination node casted as a Place.
    //! \note Since Tokens are animated from Transition to Places there is no
    //! confusion possible in the type of destination node.
    //--------------------------------------------------------------------------
    inline Place& toPlace()
    {
        return *reinterpret_cast<Place*>(&(arc.to));
    }

    //! \brief X-axis coordinate in the window used for the display.
    float x;
    //! \brief Y-axis coordinate in the window used for the display.
    float y;
    //! \brief Number of carried tokens.
    size_t tokens;
    //! \brief In which arc the token is moving along.
    Arc& arc;
    //! \brief The length of the arc.
    float magnitude;
    //! \brief The speed of the token moving along the arc.
    float speed;
    //! \brief What ratio the token has transitioned over the arc (0%: origin
    //! position, 100%: destination position).
    float offset = 0.0f;
};

#endif
