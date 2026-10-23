// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef PETRI_NET_ALGORITHMS_HPP
#  define PETRI_NET_ALGORITHMS_HPP

#  include <cstddef>
#  include <sstream>
#  include <string>
#  include <vector>

#  include "PetriNet/PetriNet.hpp"

namespace tpne {

class Net;
class MaxPlus;
class Arc;
template<typename T> class SparseMatrix;

//--------------------------------------------------------------------------
//! \brief Pre and Post incidence matrices (places x transitions) of the
//! normal arcs: Pre(p, t) tokens taken from p, Post(p, t) tokens given to p.
//! Inhibitor and reset arcs are not linear: they are not in the matrices.
//--------------------------------------------------------------------------
void incidenceMatrices(Net const& net, SparseMatrix<double>& pre, SparseMatrix<double>& post);

//--------------------------------------------------------------------------
//! \brief Make the exporters needing the (max,+) algebra (Julia) known by
//! saveToFile() and exporters(). Called once by the programs linking this
//! library (the editor, the Julia binding).
//--------------------------------------------------------------------------
void registerMaxPlusExporters();

//--------------------------------------------------------------------------
//! \brief Export a timed event graph as Julia code for MaxPlus.jl.
//--------------------------------------------------------------------------
std::string exportToJulia(Net const& net, std::string const& filename);

//--------------------------------------------------------------------------
//! \brief Return the event graph as implicit dynamic linear (max, +) system.
//! X(n) = D X(n) ⨁ A X(n-1) ⨁ B U(n)
//! Y(n) = C X(n)
//! \note This will work only if isEventGraph() returned true.
//! \return true if the Petri net was an graph event, else return false and
//! erroneous arcs can be get by \c markedArcs().
//--------------------------------------------------------------------------
bool toSysLin(Net const& net, SparseMatrix<MaxPlus>& D,
    SparseMatrix<MaxPlus>& A, SparseMatrix<MaxPlus>& B,
    SparseMatrix<MaxPlus>& C);

//--------------------------------------------------------------------------
//! \brief Inner method for the entry point toSysLin() method.
//! FIXME a mettre private
//--------------------------------------------------------------------------
void toSysLin(Net const& net,
              SparseMatrix<MaxPlus>& D, SparseMatrix<MaxPlus>& A,
              SparseMatrix<MaxPlus>& B, SparseMatrix<MaxPlus>& C,
              std::vector<size_t> const& indices, size_t const nb_inputs,
              size_t const nb_states, size_t const nb_outputs);

//--------------------------------------------------------------------------
//! \brief Transform the Event Graph to canonical form
//! \param[out] net: Petri net.
//! \param[out] canonic: resulting Petri net in canonical mode.
//! \note This will work only if isEventGraph() has returned true so make
//! the caller of this method be aware of what is he doing.
//--------------------------------------------------------------------------
void toCanonicalForm(Net const& net, Net& canonic);

//--------------------------------------------------------------------------
//! \brief Return the event graph as 2 adjacency matrices.
//! \param[out] tokens the adjacency matrix of tokens.
//! \param[out] durations the adjacency matrix of durations.
//! \note This will work only if isEventGraph() returned true.
//! \return false if the Petri net is not an event graph.
//--------------------------------------------------------------------------
bool toAdjacencyMatrices(Net const& net, SparseMatrix<MaxPlus>& tokens,
    SparseMatrix<MaxPlus>& durations);

//--------------------------------------------------------------------------
//! \brief Returned by findCriticalCycle()
//--------------------------------------------------------------------------
struct CriticalCycleResult
{
    //! \brief Has the algorithm found an optimal policy ? In case of failure
    //! all results are cleared.
    bool success = false;
    //! \brief In case of failure, holds the reason of the failure.
    //! In case of success hold all result in human readable format.
    std::stringstream message;
    //! \brief Number of connected components of the optimal policy.
    size_t cycles = 0u;
    //! \brief Indices in Net::arcs() of the arcs defining the optimal policy.
    std::vector<size_t> arcs; // TODO std::vector<std::vector<size_t>> where size of vector == cycles
    //! \brief Eigenvector: Bias vectors.
    std::vector<double> eigenvector;
    //! \brief Eigenvalues: Durations of the cycle starting for each transitions.
    std::vector<double> durations;
};

//--------------------------------------------------------------------------
//! \brief Show to the critical circuit of the net (where the cycle takes
//! the most of time).
//! \param[inout] result container of arcs storing the cycle.
//! The container is cleared before reserving its memory.
//! \return the struct CriticalCycleResult holding all information (success,
//! message, marked arcs for the cycle, eigenvector ...).
//--------------------------------------------------------------------------
CriticalCycleResult findCriticalCycle(Net const& net);

//--------------------------------------------------------------------------
//! \brief Return the timed event graph as (min,+) system. For example
//! T0(t) = min(2 + T2(t - 5)); where t - 5 is delay implied by duration on
//! arcs and min(2 + implied by tokens from incoming places.
//!
//! \param[in] comment string used as comment (when exported to a langage)
//! \param[in] use_caption if set to true use transition captions else use
//!   transition keys.
//! \param[in] minplus_notation if set to true use the (min-plus) algebra
//!   symbols (⨁ and no symbol for ⨂) else show min() symbol.
//!
//! \return the string depicting the timed Petri net as (min,+) system if
//! this net is a timed graph event. Else return empty string.
//--------------------------------------------------------------------------
std::stringstream showCounterEquation(Net const& net, std::string const& comment,
    bool use_caption, bool minplus_notation);

//--------------------------------------------------------------------------
//! \brief Return the timed event graph as (max,+) system. For example
//! T0(n) = max(5 + T2(n - 2)); where n - 2 is delay implied by tokens from
//! incoming places and max(5 + implied by duration from the incoming arc.
//!
//! \param[in] comment string used as comment (when exported to a langage)
//! \param[in] use_caption if set to true use transition captions else use
//!   transition keys.
//! \param[in] maxplus_notation if set to true use the (max-plus) algebra
//!   symbols (⨁ and no symbol for ⨂) else show max() symbol.
//!
//! \return the string depicting the timed Petri net as (max,+) system if
//! this net is a timed graph event. Else return empty string.
//--------------------------------------------------------------------------
std::stringstream showDaterEquation(Net const& net, std::string const& comment,
    bool use_caption, bool maxplus_notation);

} // namespace tpne

#endif
