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
#include "TimedPetriNetEditor/PetriNet.hpp"
#include <fstream>
#include <cstring>

namespace tpne {

const float TRANS_WIDTH = 25.0f;  // Rectangle width for rendering Transitions
const float TRANS_HEIGHT = TRANS_WIDTH / 2.0f;  // Rectangle height for rendering Transitions
const float PLACE_RADIUS = TRANS_WIDTH / 2.0f; // Circle radius for rendering Places

//------------------------------------------------------------------------------
std::string exportToDrawIO(Net const& net, std::string const& filename)
{
    const float scale = 2.0f;

    // Update arcs in/out for all transitions to be sure to generate the correct
    // net.
    //generateArcsInArcsOut();

    std::ofstream file(filename);
    if (!file)
    {
        std::stringstream error;
        error << "Failed to export the Petri net to '" << filename
              << "'. Reason was " << strerror(errno) << std::endl;
        return error.str();
    }

    file << R"PN(<mxfile host="Electron" modified="2023-04-18T17:28:34.713Z" agent="Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) draw.io/21.1.2 Chrome/106.0.5249.199 Electron/21.4.3 Safari/537.36" etag="XHyy8f1ZfD_TdxfTxzzU" version="21.1.2" type="device">
  <diagram name="Page-1" id="28U-fHV5pnyzOUwH80FG">
    <mxGraphModel dx="1362" dy="843" grid="1" gridSize="10" guides="1" tooltips="1" connect="1" arrows="1" fold="1" page="1" pageScale="1" pageWidth="1169" pageHeight="827" math="0" shadow="0">
      <root>
        <mxCell id="0" />
        <mxCell id="1" parent="0" />
)PN";

    // Places
    for (auto const& p: net.places())
    {
        file << "        <mxCell id=\"" << p.key << "\" value=\"" << p.caption
             << "\" style=\"ellipse;whiteSpace=wrap;html=1;aspect=fixed;\" vertex=\"1\" parent=\"1\">\n"
             << "          <mxGeometry x=\"" << p.x << "\" y=\"" << p.y
             << "\" width=\"" << (PLACE_RADIUS * scale) << "\" height=\"" << (PLACE_RADIUS * scale)
             << "\" as=\"geometry\" />\n        </mxCell>" << std::endl;
    }

    // Transitions
    for (auto const& t: net.transitions())
    {
        std::string color = (t.canFire() ? "green" : "red");

        file << "        <mxCell id=\"" << t.key << "\" value=\"" << t.caption
             << "\" style=\"whiteSpace=wrap;html=1;aspect=fixed;\" vertex=\"1\" parent=\"1\">\n"
             << "          <mxGeometry x=\"" << t.x << "\" y=\"" << t.y
             << "\" width=\"" << (TRANS_WIDTH * scale) << "\" height=\"" << (TRANS_HEIGHT * scale)
             << "\" as=\"geometry\" />\n        </mxCell>" << std::endl;
    }

    // Arcs
    for (auto const& a: net.arcs())
    {
        file << "        <mxCell id=\"" << a.from.key << a.to.key << "\" value=\"\" "
             << "style=\"endArrow=classic;html=1;rounded=0;exitX=0.5;exitY=1;exitDx=0;exitDy=0;entryX=0.5;entryY=0;entryDx=0;entryDy=0;\" "
             << "edge=\"1\" parent=\"1\" source=\"" << a.from.key << "\" target=\"" << a.to.key << "\">\n"
             << "          <mxGeometry width=\"50\" height=\"50\" relative=\"1\" as=\"geometry\">\n"
             << "            <mxPoint x=\"" << a.from.x << "\" y=\"" << a.from.y << "\" as=\"sourcePoint\" />\n"
             << "            <mxPoint x=\"" << a.to.x << "\" y=\"" << a.to.y << "\" as=\"targetPoint\" />\n"
             << "          </mxGeometry>\n        </mxCell>" << std::endl;
    }

    file << R"PN(      </root>
    </mxGraphModel>
  </diagram>
</mxfile>
)PN";

    return {};
}

} // namespace tpne
