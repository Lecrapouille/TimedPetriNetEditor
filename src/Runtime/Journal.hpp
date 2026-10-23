// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef TPNE_RUNTIME_JOURNAL_HPP
#  define TPNE_RUNTIME_JOURNAL_HPP

#  include "Runtime/Runtime.hpp"

#  include <string>
#  include <vector>

namespace tpne {

//------------------------------------------------------------------------------
//! \brief One line of JSON (JSON lines format) for an entry of the journal:
//! {"instance":"order_42","transition":"grasp","event":"Completed",
//! "result":"Failed","date":1.5}. Colors are written by their name.
//------------------------------------------------------------------------------
std::string toJsonLine(LogEntry const& entry, ColorTable const& colors);

//------------------------------------------------------------------------------
//! \brief The journal, one line of JSON per entry.
//------------------------------------------------------------------------------
std::string toJsonLines(std::vector<LogEntry> const& journal, ColorTable const& colors);

//------------------------------------------------------------------------------
//! \brief Read a journal written by toJsonLines() (empty lines are skipped).
//! \return the error, or empty.
//------------------------------------------------------------------------------
std::string fromJsonLines(std::string const& text, ColorTable const& colors,
                          std::vector<LogEntry>& journal);

//------------------------------------------------------------------------------
//! \brief Name of the event ("Scheduled", "Started", "Completed").
//------------------------------------------------------------------------------
std::string to_str(LogEntry::Event const event);

} // namespace tpne

#endif
