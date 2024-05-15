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

#ifndef IMPORTS_HPP
#  define IMPORTS_HPP

#include <string>
#include <vector>

namespace tpne {

class Net;

//! \brief JSON is the main format used for saving Petri by this editor.
std::string importFromJSON(Net& net, std::string const& filename);
//! \brief Import http://www.cmap.polytechnique.fr/~gaubert/HOWARD2.html
std::string importFromTimedEventGraph(Net& net, std::string const& filename);
//! \brief Import https://gitlab.com/porky11/pn-editor
std::string importFromPNML(Net& net, std::string const& filename);
//! \brief Import
std::string importFlowshop(Net& net, std::string const& filename);

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
};

//! \brief Container of file formats we can import a Petri net from.
std::vector<Importer> const& importers();

Importer const* getImporter(std::string const& ext);

} // namespace tpne

#endif
