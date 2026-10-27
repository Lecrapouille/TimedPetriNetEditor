// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef TPNE_FIRING_HPP
#  define TPNE_FIRING_HPP

#  include "PetriNet/Color.hpp"

#  include <cstddef>
#  include <vector>

// *****************************************************************************
//! \file The firing rule of Petri nets, apart from the net: the net gives the
//! structure (arcs, weights, kinds, capacities), a Marking gives the tokens.
//! These functions do not modify the net, so several markings (simulations,
//! runtime instances, states of a reachability graph) can share one net.
// *****************************************************************************

namespace tpne {

class Net;
class Arc;
class Place;
class Transition;

//------------------------------------------------------------------------------
//! \brief Number of tokens of each place, by place identifier.
//------------------------------------------------------------------------------
using Marking = std::vector<size_t>;

class ColoredMarking;

//------------------------------------------------------------------------------
//! \brief Is the transition enabled for the marking:
//!   - normal input arcs: the place holds at least the weight;
//!   - inhibitor arcs: the place holds less than the weight;
//!   - reset arcs: no condition;
//!   - output places: they do not exceed their capacity after the firing,
//!     counting the tokens already on their way to them (\c incoming, by
//!     place identifier, can be null).
//! A transition without input arc is enabled (its guard decides).
//------------------------------------------------------------------------------
bool isEnabled(Net const& net, Marking const& marking, size_t const transition,
               Marking const* incoming = nullptr);

//------------------------------------------------------------------------------
//! \brief Same rule on colored tokens: a normal or inhibitor arc counts the
//! tokens of its color only (all of them without color), a reset arc empties
//! the tokens of its color; capacities count the tokens of all colors. This is
//! the rule of the runtime.
//------------------------------------------------------------------------------
bool isEnabled(Net const& net, ColoredMarking const& marking, size_t const transition,
               Marking const* incoming = nullptr);

//------------------------------------------------------------------------------
//! \brief Tokens of the color held by the place (AnyColor: all): the ones of
//! Place::colors, at most Place::tokens, the others Success.
//------------------------------------------------------------------------------
size_t placeTokens(Place const& place, Color const color);

//------------------------------------------------------------------------------
//! \brief (color, number) of the tokens held by the place, by color, Success
//! included: as ColoredMarking::tokens().
//------------------------------------------------------------------------------
std::vector<std::pair<Color, size_t>> placeColors(Place const& place);

//------------------------------------------------------------------------------
//! \brief Same rule on the tokens held by the places of the net, with their
//! colors (placeTokens()).
//------------------------------------------------------------------------------
bool isEnabled(Net const& net, Transition const& transition);

//------------------------------------------------------------------------------
//! \brief Transitions enabled for the marking, by increasing identifier.
//------------------------------------------------------------------------------
std::vector<size_t> enabledTransitions(Net const& net, Marking const& marking);

//------------------------------------------------------------------------------
//! \brief Start of a firing: take the tokens of the input places (normal
//! arcs), empty the places of reset arcs. The transition shall be enabled.
//------------------------------------------------------------------------------
void consume(Net const& net, Marking& marking, size_t const transition);

//------------------------------------------------------------------------------
//! \brief End of a firing: give the tokens to the output places (bounded by
//! their capacity).
//------------------------------------------------------------------------------
void produce(Net const& net, Marking& marking, size_t const transition);

//------------------------------------------------------------------------------
//! \brief Color of the tokens given by an output arc: its own color, else the
//! result of the transition.
//------------------------------------------------------------------------------
Color tokenColor(Arc const& arc, Color const result);

//------------------------------------------------------------------------------
//! \brief Results the firing of the transition can give, without the function
//! that gives them (simulation, analysis): the colors asked by the arcs leaving
//! the places of its output arcs without color, sorted; Success when none is
//! asked or when all its output arcs have a color (the result changes nothing).
//------------------------------------------------------------------------------
std::vector<Color> results(Net const& net, size_t const transition);

//------------------------------------------------------------------------------
//! \brief Colored start of a firing: take the tokens of the color of each
//! normal input arc, empty the tokens of the color of each reset arc.
//------------------------------------------------------------------------------
void consume(Net const& net, ColoredMarking& marking, size_t const transition);

//------------------------------------------------------------------------------
//! \brief Colored end of a firing, without durations: give the tokens of
//! tokenColor() to the output places.
//------------------------------------------------------------------------------
void produce(Net const& net, ColoredMarking& marking, size_t const transition, Color const result);

//------------------------------------------------------------------------------
//! \brief Marking after firing the enabled transition (consume then produce,
//! durations are not considered).
//------------------------------------------------------------------------------
Marking fire(Net const& net, Marking const& marking, size_t const transition);

} // namespace tpne

#endif
