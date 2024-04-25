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
#include "TimedPetriNetEditor/Algorithms.hpp"
#include <fstream>
#include <cstring>

namespace tpne {

//------------------------------------------------------------------------------
std::string exportToTimedEventGraph(Net const& net, std::string const& filename)
{
    std::stringstream error;

    if (!isEventGraph(net))
    {
        error << "Failed to export the Petri net to '" << filename
              << "'. Reason was 'the net is not an event graph'"
              << std::endl;
        return error.str();
    }

    std::ofstream file(filename);
    if (!file)
    {
        error << "Failed to export the Petri net to '" << filename
              << "'. Reason was " << strerror(errno) << std::endl;
        return error.str();
    }

    file << "TimedEventGraph " << net.transitions().size() << " "
         << net.places().size() << std::endl << std::endl;

    for (auto const& p: net.places())
    {
        file << p.arcsIn[0]->from.id << " " << p.arcsOut[0]->to.id
             << ":   " << p.arcsIn[0]->duration << "    " << p.tokens
             << std::endl;
    }

    return {};
}

} // namespace tpne