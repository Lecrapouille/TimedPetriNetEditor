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

#include "Net/Imports/Imports.hpp"
#include <sstream>
#include <iostream>
namespace tpne {

std::vector<Importer> const& importers()
{
    static const std::vector<Importer> s_importers = {
        { "JSON", ".json", importFromJSON },
        { "Petri Net Markup Language", ".pnml", importFromPNML },
        // FIXME add a filter to eliminate it in the case the net is not event graph
        { "Timed Event Graph", ".teg", importFromTimedEventGraph }
    };

    return s_importers;
}

Importer const* getImporter(std::string const& extension)
{
    for (auto const& it: importers())
    {
        // split all extensions
        std::stringstream ss(it.extensions);
        std::string ext;
        while (!ss.eof())
        {
            std::getline(ss, ext, ',');
            if (extension == ext)
            {
                return &it;
            }
        }
    }
    return nullptr;
}

} // namespace tpne