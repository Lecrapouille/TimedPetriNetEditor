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

#include "Net/Exports/Exports.hpp"
#include <sstream>

namespace tpne {

//------------------------------------------------------------------------------
std::vector<Exporter> const& exporters()
{
    static const std::vector<Exporter> s_exporters = {
        { "JSON", ".json", exportToJSON },
        { "Grafcet C++", ".hpp,.h,.hh,.h++", exportToGrafcetCpp },
        { "Symfony", ".yaml", exportToSymfony },
        { "Julia", ".jl", exportToJulia },
        { "Draw.io", ".drawio.xml", exportToDrawIO },
        { "Graphviz", ".gv,.dot", exportToGraphviz },
        { "PN-Editor", ".pns,.pnl,.pnk,.pnkp", exportToPNEditor },
        { "Petri-LaTeX", ".tex", exportToPetriLaTeX },
        { "Petri Net Markup Language", ".pnml", exportToPNML },
        //{ "Codesys", ".codesys.xml", exportToCodesys },
        //{ "Grafcet-LaTeX", ".tex", exportToGrafcetLaTeX },
    };

    return s_exporters;
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
