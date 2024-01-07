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

#ifndef PETRI_NET_ALGORITHMS_HPP
#  define PETRI_NET_ALGORITHMS_HPP

namespace tpne {

class Net;
template<typename T> class SparseMatrix; // FIXME add (max,+) struct

//--------------------------------------------------------------------------
//! \brief Chech if the Petri net is a graph event meaning that each places
//! have exactly one input arc and one output arc.
//! \param[inout] error human readable error message.
//! \param[inout] erroneous_arcs store detected erroneous arcs.
//!
//! \return true if the Petri net is a graph event and \c erroneous_arcs is
//! empty. Return false if the Petri net is not a graph event and \c
//! erroneous_arcs will contain defectuous arcs information and call message()
//! to get the real reason.
//!
//! \note call generateArcsInArcsOut(/*arcs: true*/); before calling this
//! method.
//--------------------------------------------------------------------------
bool isEventGraph(Net const& net, std::string& error, std::vector<Arc*>& erroneous_arcs);

//--------------------------------------------------------------------------
//! \brief Conpact form of isEventGraph(Net const&, std::string&,
//! std::vector<Arc*>&) but do not care of why the net is not an event graph.
//--------------------------------------------------------------------------
bool isEventGraph(Net const& net);

//--------------------------------------------------------------------------
//! \brief Return the event graph as implicit dynamic linear (max, +) system.
//! X(n) = D X(n) ⨁ A X(n-1) ⨁ B U(n)
//! Y(n) = C X(n)
//! \note This will work only if isEventGraph() returned true.
//! \return true if the Petri net was an graph event, else return false and
//! erroneous arcs can be get by \c markedArcs().
//--------------------------------------------------------------------------
bool toSysLin(Net const& net, SparseMatrix<double>& D, SparseMatrix<double>& A, SparseMatrix<double>& B, SparseMatrix<double>& C);

//--------------------------------------------------------------------------
//! \brief Inner method for the entry point toSysLin() method.
//! FIXME a mettre private
//--------------------------------------------------------------------------
void toSysLin(Net const& net,
              SparseMatrix<double>& D, SparseMatrix<double>& A,
              SparseMatrix<double>& B, SparseMatrix<double>& C,
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
bool toAdjacencyMatrices(Net const& net, SparseMatrix<double>& tokens, SparseMatrix<double>& durations);

//--------------------------------------------------------------------------
//! \brief Returned by findCriticalCycle()
//--------------------------------------------------------------------------
struct CriticalCycleResult
{
    //! \brief In case of failure only the field message can be used indicating
    //! the reason of the failure.
    bool success = false;
    //! \brief
    std::vector<double> eigenvector;
    //! \brief Sum of durations divided by the number of tokens.
    std::vector<double> cycle_time;
    //! \brief
    std::vector<int> optimal_policy;
    //! \brief List of selected arcs defining the critical cycle (to be used for
    //! the display).
    std::vector<Arc*> arcs;
    //! \brief In case of failure, holds the reason of the failure. In case of
    //! success hold all result in human readable format.
    std::stringstream message;
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
