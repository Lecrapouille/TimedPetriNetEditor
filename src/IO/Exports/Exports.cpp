// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "IO/Exports/Exports.hpp"
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
//------------------------------------------------------------------------------
static std::vector<Exporter>& registry()
{
    static std::vector<Exporter> s_exporters = {
        { "JSON", ".json", exportToJSON },
        { "Arduino GRAFCET", ".hpp,.h,.hh,.h++", exportToGrafcetCpp },
        { "Symfony", ".yaml", exportToSymfony },
        { "Draw.io", ".drawio.xml", exportToDrawIO },
        { "Graphviz", ".gv,.dot", exportToGraphviz },
        { "PN-Editor", ".pns,.pnl,.pnk,.pnkp", exportToPNEditor },
        { "Petri LaTeX", ".tex", exportToPetriLaTeX },
        { "Grafcet LaTeX", ".tex", exportToGrafcetLaTeX },
        { "Petri Net Markup Language", ".pnml", exportToPNML },
        { "Timed Event Graph", ".teg", exportToTimedEventGraph },
        //{ "Codesys", ".codesys.xml", exportToCodesys },
        //{ "Grafcet-LaTeX", ".tex", exportToGrafcetLaTeX },
    };

    return s_exporters;
}

//------------------------------------------------------------------------------
std::vector<Exporter> const& exporters()
{
    return registry();
}

//------------------------------------------------------------------------------
void addExporter(Exporter const& exporter)
{
    for (auto const& it: registry())
    {
        if (it.format == exporter.format)
            return;
    }
    registry().push_back(exporter);
}

//------------------------------------------------------------------------------
Exporter const* getExporter(std::string const& extension)
{
    for (auto const& it: exporters())
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
Exporter const* findExporter(std::string const& filepath)
{
    return findFormat(exporters(), filepath, &Exporter::extensions);
}

} // namespace tpne
