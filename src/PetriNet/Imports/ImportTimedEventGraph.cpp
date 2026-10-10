// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Imports.hpp"

#include "PetriNet/PetriNet.hpp"

#include <cmath>
#include <fstream>
#include <cstring>

namespace tpne {

//------------------------------------------------------------------------------
std::string importFromTimedEventGraph(Net& net, std::string const& filename)
{
    std::stringstream error;
    size_t initial_transition, final_transition, tokens;
    float duration;

    net.reset(TypeOfNet::TimedPetriNet);

    // Check if file exists
    std::ifstream file(filename);
    if (!file)
    {
        error << "Failed opening '" << filename << "'. Reason: '"
            << strerror(errno) << "'" << std::endl;
        return error.str();
    }

    // Extract number of transitions and number of lines
    size_t transitions, lines;
    char separator;
    std::string type;

    if (!(file >> type >> transitions >> lines))
    {
        error << "Malformed header. Needed 'TimedEventGraph number_transitions number_lines'"
            << std::endl;
        return error.str();
    }
    if (type != "TimedEventGraph")
    {
        error << "Malformed token. Expected to extract token 'TimedEventGraph'"
            << std::endl;
        return error.str();
    }

    // File does not provide positions: place on circle as initial layout.
    // ForceDirected (springify) will refine when importer.springify is true.
    const float cx = 350.0f, cy = 350.0f, radius = 250.0f;
    const float two_pi = 6.283185307f;
    for (size_t id = 0u; id < transitions; ++id)
    {
        float angle = (transitions > 1u) ? two_pi * float(id) / float(transitions) : 0.0f;
        float x = cx + radius * std::cos(angle);
        float y = cy + radius * std::sin(angle);
        net.addTransition(id, Transition::to_str(id), x, y);
    }

    // Create arcs between created transitions. Places are automatically created
    // when the arc is created.
    for (size_t i = 0u; i < lines; ++i)
    {
        if (!(file >> initial_transition >> final_transition >> separator >> duration >> tokens))
        {
            error << "Malformed line. Expected 4 values: 'initial_transition"
                << " final_transition: duration tokens'" << std::endl;
            return error.str();
        }
        if ((initial_transition >= transitions) || (final_transition >= transitions))
        {
            error << "Malformed line. Invalid transition ID" << std::endl;
            return error.str();
        }
        if (separator != ':')
        {
            error << "Malformed line. Missing ':' separator" << std::endl;
            return error.str();
        }

        Transition* t0 = net.findTransition(initial_transition);
        Transition* t1 = net.findTransition(final_transition);
        assert((t0 != nullptr) && (t1 != nullptr));
        net.addArc(*t0, *t1, tokens, duration);
    }

    return {};
}

} // namespace tpne
