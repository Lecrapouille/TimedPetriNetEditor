// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef TPNE_FIRING_HPP
#  define TPNE_FIRING_HPP

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
class Transition;
template<typename T> class SparseMatrix;

//------------------------------------------------------------------------------
//! \brief Number of tokens of each place, by place identifier.
//------------------------------------------------------------------------------
using Marking = std::vector<size_t>;

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
//! \brief Same rule on the tokens held by the places of the net.
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
//! \brief Marking after firing the enabled transition (consume then produce,
//! durations are not considered).
//------------------------------------------------------------------------------
Marking fire(Net const& net, Marking const& marking, size_t const transition);

//------------------------------------------------------------------------------
//! \brief Pre and Post incidence matrices (places x transitions) of the
//! normal arcs: Pre(p, t) tokens taken from p, Post(p, t) tokens given to p.
//! Inhibitor and reset arcs are not linear: they are not in the matrices.
//------------------------------------------------------------------------------
void incidenceMatrices(Net const& net, SparseMatrix<double>& pre, SparseMatrix<double>& post);

} // namespace tpne

#endif
