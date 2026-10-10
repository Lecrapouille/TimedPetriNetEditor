// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "PetriNet/Imports/Imports.hpp"
#include <sstream>
#include <iostream>
namespace tpne {

std::vector<Importer> const& importers()
{
    static const std::vector<Importer> s_importers = {
        { "JSON", ".json", importFromJSON, false },
        { "Petri Net Markup Language", ".pnml", importFromPNML, false },
        { "Flowshop", ".flowshop", importFlowshop, false },
        { "Timed Event Graph", ".teg", importFromTimedEventGraph, true }
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