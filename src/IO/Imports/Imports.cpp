// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "IO/Imports/Imports.hpp"
#include <algorithm>
#include <sstream>
namespace tpne {

//------------------------------------------------------------------------------
//! \brief The format of the list whose extensions (".a,.b") the file ends with,
//! the longest one winning (case is ignored).
//------------------------------------------------------------------------------
template<class Format>
static Format const* findFormat(std::vector<Format> const& formats, std::string const& filepath,
                                std::string Format::*extensions)
{
    std::string path(filepath);
    if (!path.empty() && (path.back() == '~'))
        path.pop_back();
    std::transform(path.begin(), path.end(), path.begin(), ::tolower);

    Format const* best = nullptr;
    size_t best_size = 0u;
    for (auto const& it: formats)
    {
        std::stringstream ss(it.*extensions);
        std::string ext;
        while (std::getline(ss, ext, ','))
        {
            if ((ext.size() > best_size) && (path.size() >= ext.size()) &&
                (path.compare(path.size() - ext.size(), ext.size(), ext) == 0))
            {
                best = &it;
                best_size = ext.size();
            }
        }
    }
    return best;
}

//------------------------------------------------------------------------------
static std::vector<Importer>& registry()
{
    static std::vector<Importer> s_importers = {
        { "JSON", ".json", importFromJSON, false },
        { "Petri Net Markup Language", ".pnml", importFromPNML, false },
        { "GreatSPN GRML", ".grml", importFromGRML, false },
        { "Timed Event Graph", ".teg", importFromTimedEventGraph, true }
    };

    return s_importers;
}

//------------------------------------------------------------------------------
std::vector<Importer> const& importers()
{
    return registry();
}

//------------------------------------------------------------------------------
void addImporter(Importer const& importer)
{
    for (auto const& it: registry())
    {
        if (it.format == importer.format)
            return;
    }
    registry().push_back(importer);
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
Importer const* findImporter(std::string const& filepath)
{
    return findFormat(importers(), filepath, &Importer::extensions);
}

} // namespace tpne