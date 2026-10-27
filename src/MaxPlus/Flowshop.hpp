// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef TPNE_FLOWSHOP_HPP
#  define TPNE_FLOWSHOP_HPP

#  include <string>
#  include <vector>

namespace tpne {

class Net;

// *****************************************************************************
//! \brief A flowshop: a factory that produces parts. Every part visits the
//! machines in the same order (rows of PT), and every machine works on the
//! conveyors in the same order (columns of PT). On the timed event graph a
//! machine is a horizontal cycle and a conveyor a vertical one; nm and np are
//! how many of each resource the factory owns. Same data as the .flowshop
//! files and the flowshop of ScicosLab and MaxPlus.jl.
// *****************************************************************************
struct Flowshop
{
    size_t nconveyors = 0;
    size_t nmachines = 0;
    std::vector<std::string> conveyorNames;
    std::vector<std::string> machineNames;
    //! \brief Tokens per conveyor (how many conveyors of this column).
    std::vector<size_t> np;
    //! \brief Tokens per machine (how many machines of this row).
    std::vector<size_t> nm;
    //! \brief PT[machine][conveyor]: processing time, or flowshopNoOperation().
    std::vector<std::vector<float>> PT;
};

//! \brief Value of PT for "this conveyor does not visit this machine" ((max,+)
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
//! transition "<machine>_<conveyor>" per operation (it fires when the operation
//! starts), one input and one output transition per conveyor and per machine
//! ("<conveyor>", "out_<conveyor>", ...). The flowshop shall be checked.
//------------------------------------------------------------------------------
void buildFlowshop(Net& net, Flowshop const& flowshop);

//------------------------------------------------------------------------------
//! \brief Machine and conveyor of an operation: a transition named
//! "<machine>_<conveyor>" by buildFlowshop() (the Gantt chart draws it on the
//! row of the machine, colored by conveyor). Other names ("T3", "out_C1") are
//! their own machine and conveyor.
//------------------------------------------------------------------------------
void operationOf(std::string const& name, std::string& machine, std::string& conveyor);

//------------------------------------------------------------------------------
//! \brief Earliest firing dates of a timed event graph during its first
//! cycles, by the (max,+) dater equation x(k) = max(x(k - tokens) + duration)
//! over the input places of each transition. Initial tokens are available at
//! time 0.
//! \param[out] dates dates[t][k]: date of the k-th firing of the transition
//!   of index t in Net::transitions().
//! \return the error (not an event graph, a circuit without token), or empty.
//------------------------------------------------------------------------------
//! \brief Import a flowshop file (.flowshop) as a timed event graph. Known by
//! loadFromFile() after registerMaxPlusFormats().
std::string importFlowshop(Net& net, std::string const& filename);

std::string earliestDates(Net const& net, size_t cycles,
                          std::vector<std::vector<double>>& dates);

} // namespace tpne

#endif
