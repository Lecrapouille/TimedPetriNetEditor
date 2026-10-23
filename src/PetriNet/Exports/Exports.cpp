// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "PetriNet/Exports/Exports.hpp"
#include <sstream>

namespace tpne {

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

} // namespace tpne
