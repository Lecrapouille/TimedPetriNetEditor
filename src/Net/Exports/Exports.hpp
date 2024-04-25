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

#ifndef PETRI_FILE_EXPORTERS_HPP
#  define PETRI_FILE_EXPORTERS_HPP

#include <string>
#include <vector>

namespace tpne {

class Net;

//! \brief JSON is the main format used for saving Petri by this editor.
std::string exportToJSON(Net const& net, std::string const& filename);
//! \brief Import http://www.cmap.polytechnique.fr/~gaubert/HOWARD2.html
std::string exportToTimedEventGraph(Net const& net, std::string const& filename);
//! \brief
std::string exportToSymfony(Net const& net, std::string const& filename);
//! \brief Import https://gitlab.com/porky11/pn-editor
std::string exportToPNEditor(Net const& net, std::string const& filename);
std::string exportToDrawIO(Net const& net, std::string const& filename);
std::string exportToGraphviz(Net const& net, std::string const& filename);
std::string exportToPetriLaTeX(Net const& net, std::string const& filename);
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
