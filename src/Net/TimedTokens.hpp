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

#ifndef TIMED_TOKENS_HPP
#  define TIMED_TOKENS_HPP

namespace tpne {

class Arc;
class Place;
enum TypeOfNet;

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
    //! \brief Constructor.
    //! \param[in] arc_: to which arc token are moving along. Shall be an arc
    //! Transition -> Place. No check is performed here.
    //! \param[in] tokens_: the number of tokens it shall carry.
    //! \param[in] type_: Type of the net (Petri, timed Petri, GRAFCET ...)
    //--------------------------------------------------------------------------
    TimedToken(Arc& arc_, size_t const tokens_, TypeOfNet const type_);

    // I dunno why the code in the #else branch seems to make buggy animations
    // with tokens that disapear. Cannot catch it by unit tests.
    // https://github.com/Lecrapouille/TimedPetriNetEditor/issues/2
# if 1

    //--------------------------------------------------------------------------
    //! \brief Hack needed because of references
    //--------------------------------------------------------------------------
    TimedToken& operator=(const TimedToken& obj)
    {
        this->~TimedToken(); // destroy
        new (this) TimedToken(obj); // copy construct in place
        return *this;
    }

    TimedToken(const TimedToken&) = default;
    TimedToken(TimedToken&&) = default;
    TimedToken& operator=(TimedToken&&) = default;

#else

    //--------------------------------------------------------------------------
    //! \brief Hack needed because of references
    //--------------------------------------------------------------------------
    TimedToken& operator=(TimedToken const& other)
    {
        this->~TimedToken(); // destroy
        new (this) TimedToken(other); // copy construct in place
        return *this;
    }

    //--------------------------------------------------------------------------
    //! \brief Needed to remove compilation warnings
    //--------------------------------------------------------------------------
    TimedToken(TimedToken const& other)
        : TimedToken(other.arc, other.tokens)
    {}

    //--------------------------------------------------------------------------
    //! \brief Needed to remove compilation warnings
    //--------------------------------------------------------------------------
    TimedToken(TimedToken&& other)
        : TimedToken(other.arc, other.tokens)
    {}

    //--------------------------------------------------------------------------
    //! \brief Needed to remove compilation warnings
    //--------------------------------------------------------------------------
    TimedToken& operator=(TimedToken&& other)
    {
        this->~TimedToken(); // destroy
        new (this) TimedToken(other); // copy construct in place
        return *this;
    }

#endif

    //--------------------------------------------------------------------------
    //! \brief Update position on the screen.
    //! \param[in] dt: the delta time (in seconds) from the previous call.
    //! \return true when arriving to the destination node (Place) else false.
    //--------------------------------------------------------------------------
    bool update(float const dt);

    //--------------------------------------------------------------------------
    //! \brief Return the reference of the destination node casted as a Place.
    //! \note Since Tokens are animated from Transition to Places there is no
    //! confusion possible in the type of destination node.
    //--------------------------------------------------------------------------
    inline Place& toPlace()
    {
        return *reinterpret_cast<Place*>(&(arc.to));
    }

    //! \brief In which arc the token is moving along.
    Arc& arc;
    //! \brief X-axis coordinate in the window used for the display.
    float x;
    //! \brief Y-axis coordinate in the window used for the display.
    float y;
    //! \brief Number of carried tokens.
    size_t tokens;
    //! \brief
    TypeOfNet type;
    //! \brief The length of the arc.
    float magnitude;
    //! \brief The speed of the token moving along the arc.
    float speed;
    //! \brief What ratio the token has transitioned over the arc (0%: origin
    //! position, 100%: destination position).
    float offset = 0.0f;
};

} // namespace tpne

#endif
