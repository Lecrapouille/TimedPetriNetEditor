// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef TPNE_ANALYSIS_HPP
#  define TPNE_ANALYSIS_HPP

#  include "PetriNet/ColoredMarking.hpp"
#  include "PetriNet/Firing.hpp"

#  include <cstdint>
#  include <limits>
#  include <string>
#  include <vector>

// *****************************************************************************
//! \file Properties of Petri nets, time not considered: reachable markings,
//! deadlocks, dead transitions, bounds, liveness, invariants. The analyses
//! that do not hold for some extensions of the net (inhibitor or reset arcs)
//! say so instead of giving a wrong answer.
// *****************************************************************************

namespace tpne {

class Net;

// *****************************************************************************
//! \brief Graph of the markings reachable from the initial one.
// *****************************************************************************
struct ReachabilityGraph
{
    struct Edge
    {
        size_t from;
        size_t transition;
        size_t to;
    };

    //! \brief Reachable markings; the first one is the initial marking.
    std::vector<Marking> states;
    //! \brief Nets with colors: the colored marking of each state (states
    //! holds their number of tokens: two states can have the same one).
    std::vector<ColoredMarking> colored;
    std::vector<Edge> edges;
    //! \brief False when the exploration stopped at the max number of
    //! states: the properties found are then only the ones of the explored
    //! part.
    bool complete = true;
    //! \brief States without enabled transition.
    std::vector<size_t> deadlocks;
};

//------------------------------------------------------------------------------
//! \brief Explore the markings reachable from the marking of the net (all the
//! rules of the firing: weights, inhibitor and reset arcs, capacities),
//! breadth first, until max_states. A net with colors is explored with its
//! colored markings (initialMarking()), as the runtime fires it: a firing
//! gives one edge for each of its results() (a function of the runtime
//! returning another color is not considered).
//------------------------------------------------------------------------------
ReachabilityGraph reachabilityGraph(Net const& net, size_t const max_states = 100000u);

//------------------------------------------------------------------------------
//! \brief Number of tokens standing for omega (as many as wanted) in a
//! coverability graph.
//------------------------------------------------------------------------------
constexpr size_t OMEGA = std::numeric_limits<size_t>::max();

// *****************************************************************************
//! \brief Coverability graph (Karp-Miller): finite even for unbounded nets,
//! a place that can grow without bound holds OMEGA.
// *****************************************************************************
struct CoverabilityGraph
{
    std::vector<Marking> states;
    std::vector<ReachabilityGraph::Edge> edges;
    //! \brief Empty when it holds, else why it does not (inhibitor or reset
    //! arcs: Karp-Miller does not apply).
    std::string error;
    bool complete = true;
};

//------------------------------------------------------------------------------
//! \brief Karp-Miller coverability graph. Not for nets with inhibitor or reset
//! arcs (error is given).
//------------------------------------------------------------------------------
CoverabilityGraph coverabilityGraph(Net const& net, size_t const max_states = 100000u);

// *****************************************************************************
//! \brief Properties of a net.
// *****************************************************************************
struct Properties
{
    //! \brief Number of markings explored, and whether all were.
    size_t states = 0u;
    bool complete = true;
    //! \brief Reachable markings without enabled transition (example: the
    //! first one, with the transitions firing to it from the initial marking).
    bool deadlock = false;
    Marking deadlock_marking;
    //! \brief Its colors (nets with colors, else empty).
    ColoredMarking deadlock_colored;
    std::vector<size_t> deadlock_path;
    //! \brief Transitions never enabled (quasi-liveness, L1).
    std::vector<size_t> dead_transitions;
    //! \brief Max tokens of each place (OMEGA: unbounded); bounded if none is.
    std::vector<size_t> bounds;
    bool bounded = true;
    //! \brief Every transition can always fire again (L4), when the graph is
    //! complete; transitions that are not live.
    bool live = false;
    std::vector<size_t> not_live;
    //! \brief Why some properties could not be computed (extensions of the
    //! net, max number of states).
    std::vector<std::string> notes;
};

//------------------------------------------------------------------------------
//! \brief Deadlocks, dead transitions, bounds, liveness of the net.
//------------------------------------------------------------------------------
Properties properties(Net const& net, size_t const max_states = 100000u);

//------------------------------------------------------------------------------
//! \brief Minimal semi-positive P-invariants (P-semiflows: y >= 0, y.C = 0:
//! the weighted sum of the tokens of their places never changes) and
//! T-invariants (T-semiflows: x >= 0, C.x = 0: firing these transitions
//! these numbers of times gives back the marking), by the algorithm of
//! Farkas. Only normal arcs: \c error says when the net has inhibitor or
//! reset arcs (the invariants of the normal arcs may not hold).
//------------------------------------------------------------------------------
std::vector<std::vector<int64_t>> pInvariants(Net const& net, std::string& error);
std::vector<std::vector<int64_t>> tInvariants(Net const& net, std::string& error);

//------------------------------------------------------------------------------
//! \brief A marking as text: the tokens of each place ("1 0 2", OMEGA as w),
//! with their colors when they are not all Success ("2{Phi0:1 Phi1:1}").
//------------------------------------------------------------------------------
std::string markingLabel(Net const& net, Marking const& marking, ColoredMarking const* colored = nullptr);

//------------------------------------------------------------------------------
//! \brief The graph of the markings as Graphviz (dot): a node per marking
//! ("1 0 2", OMEGA as w), deadlocks in red, the initial one in bold, an edge
//! per firing labeled by the transition. With the colored markings
//! (ReachabilityGraph::colored), a place whose tokens are not all Success
//! gives their colors: "2{Phi0:1 Phi1:1}".
//------------------------------------------------------------------------------
std::string toDot(Net const& net, std::vector<Marking> const& states,
                  std::vector<ReachabilityGraph::Edge> const& edges,
                  std::vector<ColoredMarking> const& colored = {});

//------------------------------------------------------------------------------
//! \brief Report of all the analyses in text (tpne-check, the editor).
//------------------------------------------------------------------------------
std::string report(Net const& net, size_t const max_states = 100000u);

} // namespace tpne

#endif
