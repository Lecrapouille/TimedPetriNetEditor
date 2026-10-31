// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef TPNE_EDITOR_NODE_MARKS_HPP
#  define TPNE_EDITOR_NODE_MARKS_HPP

#  include "PetriNet/PetriNet.hpp"
#  include "imgui.h"

#  include <map>
#  include <string>
#  include <vector>

namespace tpne {

// *****************************************************************************
//! \brief A node drawn with a halo of a color and a short label: transitions
//! running in a replayed journal or in a connected runtime, transitions
//! without function, dead transitions and unbounded places of the analysis.
// *****************************************************************************
struct NodeMark
{
    NodeId node;
    ImU32 color;
    std::string label;
};

// *****************************************************************************
//! \brief Marks of the nodes of nets, by source ("journal", "analysis" ...):
//! each source replaces its own marks. The marks of a net are dropped when
//! its number of nodes changes (they would be on other nodes).
// *****************************************************************************
class NodeMarks
{
public:

    void set(Net const& net, std::string const& source, std::vector<NodeMark> marks)
    {
        m_entries[source] = { &net, net.places().size(), net.transitions().size(), std::move(marks) };
    }

    void clear(std::string const& source) { m_entries.erase(source); }

    //! \brief Call f(mark) for each mark of the net.
    template<class F>
    void forEach(Net const& net, F const& f) const
    {
        for (auto const& it : m_entries)
        {
            Entry const& e = it.second;
            if ((e.net != &net) || (e.places != net.places().size()) ||
                (e.transitions != net.transitions().size()))
                continue;
            for (auto const& mark : e.marks)
                f(mark);
        }
    }

private:

    struct Entry
    {
        Net const* net = nullptr;
        size_t places = 0u;
        size_t transitions = 0u;
        std::vector<NodeMark> marks;
    };

    std::map<std::string, Entry> m_entries;
};

} // namespace tpne

#endif
