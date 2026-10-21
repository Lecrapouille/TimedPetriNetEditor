// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

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
