// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef PETRI_FILE_EXPORTERS_HPP
#  define PETRI_FILE_EXPORTERS_HPP

#include <string>
#include <vector>

namespace tpne {

class Net;

//! \brief JSON is the main format used for saving Petri by this editor.
std::string exportToJSON(Net const& net, std::string const& filename);
//! \brief Export multiple nets to a JSON file (for multi-net documents)
std::string exportAllNetsToJSON(std::vector<Net> const& nets, std::string const& filename);
//! \brief Import http://www.cmap.polytechnique.fr/~gaubert/HOWARD2.html
std::string exportToTimedEventGraph(Net const& net, std::string const& filename);
//! \brief
std::string exportToSymfony(Net const& net, std::string const& filename);
//! \brief Import https://gitlab.com/porky11/pn-editor
std::string exportToPNEditor(Net const& net, std::string const& filename);
std::string exportToDrawIO(Net const& net, std::string const& filename);
std::string exportToGraphviz(Net const& net, std::string const& filename);
std::string exportToPetriLaTeX(Net const& net, std::string const& filename);
std::string exportToGrafcetLaTeX(Net const& net, std::string const& filename);
std::string exportToJulia(Net const& net, std::string const& filename);
std::string exportToGrafcetCpp(Net const& net, std::string const& filename);
std::string exportToPNML(Net const& net, std::string const& filename);

//! \brief Interface for exporting a Petri file.
//! \param[in] net the net we are exporting.
//! \param[in] filepath the path of the file to import.
//! \return a dummy string in case of success, else return the error message.
typedef std::string (*ExportFunc)(Net const&, std::string const&);

//! \brief
struct Exporter
{
    //! \brief Name of the file format (i.e. "JSON")
    std::string format;
    //! \brief file extension with the dot (i.e. ".json")
    std::string extensions;
    //! \brief the pointer function for importing (i.e. importFromJSON)
    ExportFunc exportFct;
};

//! \brief Container of file formats we can export the net to (LaTeX, Symfony, Dot ...).
std::vector<Exporter> const& exporters();

Exporter const* getExporter(std::string const& extension);

} // namespace tpne

#endif
