// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef IMPORTS_HPP
#  define IMPORTS_HPP

#include <string>
#include <vector>

namespace tpne {

class Net;
struct DocumentInfo;

//! \brief JSON is the main format used for saving Petri by this editor.
std::string importFromJSON(Net& net, std::string const& filename);
//! \brief Import all nets from a JSON file (for multi-net documents). When
//! info is not null, it receives the description of the document and of its
//! GRAFCET inputs and outputs.
std::string importAllNetsFromJSON(std::vector<Net>& nets, std::string const& filename,
                                  DocumentInfo* info = nullptr);
//! \brief Import http://www.cmap.polytechnique.fr/~gaubert/HOWARD2.html
std::string importFromTimedEventGraph(Net& net, std::string const& filename);
//! \brief Import the Petri Net Markup Language (standard, Symmetri).
std::string importFromPNML(Net& net, std::string const& filename);

//! \brief Interface for importing a Petri file.
//! \param[inout] net the net we are importing. Better to call net.clear()
//! before importing the file.
//! \param[in] filepath the path of the file to import. We do not check
//! if the file format is the expected one for the importer.
//! \return a dummy string in case of success, else return the error message.
//! \note the net is not cleared in case of error.
typedef std::string (*ImportFunc)(Net& net, std::string const& filepath);

//! \brief
struct Importer
{
    //! \brief Name of the file format (i.e. "JSON")
    std::string format;
    //! \brief file extension with the dot (i.e. ".json")
    std::string extensions;
    //! \brief the pointer function for importing (i.e. importFromJSON)
    ImportFunc importFct;
    //! \brief Shall apply spring force on the net ?
    bool springify;
};

//! \brief Container of file formats we can import a Petri net from.
std::vector<Importer> const& importers();

//! \brief Add a file format to importers() (formats of other libraries, as the
//! flowshops of the (max,+) library). Ignored if the format is known.
void addImporter(Importer const& importer);

//! \brief The importer of the extension (".json"), or null.
Importer const* getImporter(std::string const& ext);

//! \brief The importer of the file, by the longest of its extensions it ends
//! with, or null.
Importer const* findImporter(std::string const& filepath);

} // namespace tpne

#endif
