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

#include "Imports.hpp"
#include "TimedPetriNetEditor/PetriNet.hpp"
#include "Utils/Utils.hpp"

#include <fstream>
#include <cstring>

namespace tpne {

//------------------------------------------------------------------------------
std::string importFromTimedEventGraph(Net& net, std::string const& filename)
{
    std::stringstream error;
    size_t initial_transition, final_transition, tokens;
    float duration;
    // windows screen.
    // FIXME: get the exact dimension Editor::viewSize()
    // FIXME: initial frame iteration: the screen size is not at its final size
    const size_t w = 700u; const size_t h = 700u;
    const size_t margin = 50u; const size_t nodes_by_line = 4u; 

    net.reset(TypeOfNet::TimedPetriNet);//TimedEventGraph);

    // Check if file exists
    std::ifstream file(filename);
    if (!file)
    {
        error << "Failed opening '" << filename << "'. Reason was '"
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

    // Since the file does not give position, we place them as square
    size_t dx = (w - 2u * margin) / (nodes_by_line - 1u);
    size_t dy = (h - 2u * margin) / (nodes_by_line - 1u);
    size_t x = margin, y = margin;
    for (size_t id = 0u; id < transitions; ++id)
    {
        net.addTransition(id, Transition::to_str(id), float(x), float(y), 0);
        x += dx;
        if (x > w - margin) { x = margin; y += dy; }
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
