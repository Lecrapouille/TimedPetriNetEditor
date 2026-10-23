// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "PetriNet/Exports/Exports.hpp"
#include "PetriNet/PetriNet.hpp"
#include "MaxPlus/Algorithms.hpp"
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
              << "'. Reason: 'the net is not an event graph'"
              << std::endl;
        return error.str();
    }

    std::ofstream file(filename);
    if (!file)
    {
        error << "Failed to export the Petri net to '" << filename
              << "'. Reason: " << strerror(errno) << std::endl;
        return error.str();
    }

    file << "TimedEventGraph " << net.transitions().size() << " "
         << net.places().size() << std::endl << std::endl;

    for (auto const& p: net.places())
    {
        Arc const& in = net.arcsIn(p)[0];
        file << in.from.id << " " << net.arcsOut(p)[0].to.id
             << ":   " << in.duration << "    " << p.tokens
             << std::endl;
    }

    return {};
}

} // namespace tpne