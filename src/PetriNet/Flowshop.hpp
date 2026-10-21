//=============================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2026 Quentin Quadrat <lecrapouille@gmail.com>
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

#ifndef TPNE_FLOWSHOP_HPP
#  define TPNE_FLOWSHOP_HPP

#  include <string>
#  include <vector>

namespace tpne {

class Net;

// *****************************************************************************
//! \brief A flowshop: every piece visits the machines in the same order (rows
//! of PT), every machine works on the pieces in the same order (columns of
//! PT). Same data as the .flowshop files and the flowshop of ScicosLab and
//! MaxPlus.jl.
// *****************************************************************************
struct Flowshop
{
    size_t npieces = 0;
    size_t nmachines = 0;
    std::vector<std::string> pieceNames;
    std::vector<std::string> machineNames;
    //! \brief Tokens per piece (pieces of this kind in the shop at once).
    std::vector<size_t> np;
    //! \brief Tokens per machine (pieces a machine works on at once).
    std::vector<size_t> nm;
    //! \brief PT[machine][piece]: processing time, or flowshopNoOperation().
    std::vector<std::vector<float>> PT;
};

//! \brief Value of PT for "this piece does not visit this machine" ((max,+)
//! epsilon, -inf).
float flowshopNoOperation();

//! \brief True for flowshopNoOperation() (also NaN, an older convention).
bool flowshopIsNoOperation(float value);

//------------------------------------------------------------------------------
//! \brief Check the sizes of a flowshop. Return the error, or empty.
//------------------------------------------------------------------------------
std::string checkFlowshop(Flowshop const& flowshop);

//------------------------------------------------------------------------------
//! \brief Replace the net by the timed event graph of the flowshop: one
//! transition "<machine>_<piece>" per operation (it fires when the operation
//! starts), one input and one output transition per piece and per machine
//! ("<piece>", "out_<piece>", ...). The flowshop shall be checked.
//------------------------------------------------------------------------------
void buildFlowshop(Net& net, Flowshop const& flowshop);

//------------------------------------------------------------------------------
//! \brief Machine and piece of an operation: a transition named
//! "<machine>_<piece>" by buildFlowshop() (the Gantt chart draws it on the row
//! of the machine, colored by piece). Other names ("T3", "out_P1") are their
//! own machine and piece.
//------------------------------------------------------------------------------
void operationOf(std::string const& name, std::string& machine, std::string& piece);

//------------------------------------------------------------------------------
//! \brief Earliest firing dates of a timed event graph during its first
//! cycles, by the (max,+) dater equation x(k) = max(x(k - tokens) + duration)
//! over the input places of each transition. Initial tokens are available at
//! time 0.
//! \param[out] dates dates[t][k]: date of the k-th firing of the transition
//!   of index t in Net::transitions().
//! \return the error (not an event graph, a circuit without token), or empty.
//------------------------------------------------------------------------------
std::string earliestDates(Net const& net, size_t cycles,
                          std::vector<std::vector<double>>& dates);

} // namespace tpne

#endif
