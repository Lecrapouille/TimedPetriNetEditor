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

#ifndef GRAFCET_HELP_HPP
#  define GRAFCET_HELP_HPP

#  include "PetriNet/Grafcet.hpp"

namespace tpne {

//------------------------------------------------------------------------------
//! \brief What an action qualifier does, in one sentence.
//------------------------------------------------------------------------------
char const* qualifierMeaning(Action::Qualifier qualifier);

//------------------------------------------------------------------------------
//! \brief Timing diagram of an action qualifier drawn in the current window:
//! the step (and the step resetting a stored action), then the output of the
//! action, with the duration d of the timed qualifiers.
//! \param[in] width width of the diagram in pixels.
//------------------------------------------------------------------------------
void qualifierChronogram(Action::Qualifier qualifier, float width);

//------------------------------------------------------------------------------
//! \brief Help of the action qualifiers: one row per qualifier with its
//! meaning and its timing diagram.
//------------------------------------------------------------------------------
void qualifiersHelp();

//------------------------------------------------------------------------------
//! \brief Help of the syntax of the receptivities: a table of what to write.
//------------------------------------------------------------------------------
void receptivitiesHelp();

//------------------------------------------------------------------------------
//! \brief Help of the kinds of inputs and outputs of the control panel.
//------------------------------------------------------------------------------
void controlPanelHelp();

//------------------------------------------------------------------------------
//! \brief What a kind of input is, in one sentence.
//------------------------------------------------------------------------------
char const* inputKindMeaning(GrafcetInput::Kind kind);

//------------------------------------------------------------------------------
//! \brief What a kind of output is, in one sentence.
//------------------------------------------------------------------------------
char const* outputKindMeaning(GrafcetOutput::Kind kind);

} // namespace tpne

#endif
