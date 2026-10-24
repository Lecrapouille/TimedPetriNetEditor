// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef TPNE_IO_HPP
#  define TPNE_IO_HPP

#  include "IO/Exports/Exports.hpp"
#  include "IO/Imports/Imports.hpp"

#  include <string>

// *****************************************************************************
//! \file Load and save nets in the file formats known by importers() and
//! exporters(), chosen by the extension of the file. The library of the
//! formats is apart from the nets: a program running nets (runtime, Arduino)
//! does not link it.
// *****************************************************************************

namespace tpne {

class Net;

//-----------------------------------------------------------------------------
//! \brief Load the Petri net from a a file. The current net is cleared
//! before the loading. If the loading failed (missing file or invalid
//! syntax) the net is set dummy.
//! \param[in] filepath: the file path in where a Petri net has been
//! saved. Its extension chooses the importer.
//! \param[out] springify: the importer gives no positions: the nodes should
//! be placed by a force directed layout.
//! \return error message in case of failure, else return dummy string.
//-----------------------------------------------------------------------------
std::string loadFromFile(Net& net, std::string const& filepath, bool& springify);

//-----------------------------------------------------------------------------
//! \brief Same without caring of the layout.
//-----------------------------------------------------------------------------
std::string loadFromFile(Net& net, std::string const& filepath);

//-----------------------------------------------------------------------------
//! \brief Save the Petri net in a a file.
//! \param[in] filepath: the file path in where to save the Petri net. Its
//! extension chooses the exporter (the longest one matching: "net.drawio.xml"
//! is a Draw.io file).
//! \return error message in case of failure, else return dummy string.
//-----------------------------------------------------------------------------
std::string saveToFile(Net const& net, std::string const& filepath);

//-----------------------------------------------------------------------------
//! \brief Extension of the file, lower case, with its dot (".json"); a
//! trailing ~ (backup files) is ignored.
//-----------------------------------------------------------------------------
std::string fileExtension(std::string const& path);

} // namespace tpne

#endif
