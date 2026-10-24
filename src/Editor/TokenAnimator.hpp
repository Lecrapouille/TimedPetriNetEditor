// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef TPNE_TOKEN_ANIMATOR_HPP
#  define TPNE_TOKEN_ANIMATOR_HPP

#  include "PetriNet/Engine.hpp"

#  include <vector>

namespace tpne {

// *****************************************************************************
//! \brief Tokens drawn on their way along an arc: the engine only knows when
//! they left and when they arrive, the animation places them in between.
// *****************************************************************************
struct AnimatedToken
{
    //! \brief Position in net units.
    float x;
    float y;
    //! \brief Tokens of the same firing on the same arc, drawn as one dot.
    size_t tokens;
};

//------------------------------------------------------------------------------
//! \brief Positions of the tokens on their way at the given date, along the
//! line of their arc as drawn (bent or not). For timed event graphs, they go
//! from the transition to where the tokens of the hidden place wait, near
//! the next transition.
//------------------------------------------------------------------------------
std::vector<AnimatedToken> animateTokens(Net const& net, std::vector<Engine::Flight> const& flights,
                                         double date);

} // namespace tpne

#endif
