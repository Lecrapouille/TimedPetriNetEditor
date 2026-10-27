// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef TPNE_MARKINGS_GRAPH_HPP
#  define TPNE_MARKINGS_GRAPH_HPP

#  include "Analysis/Analysis.hpp"
#  include "imgui/imgui.h"

#  include <string>
#  include <vector>

namespace tpne {

// *****************************************************************************
//! \brief The graph of the markings of a net (reachability graph, or the
//! coverability graph when the markings are too many), drawn in a window: a
//! box per marking (the initial one bold, the deadlocks red), an arrow per
//! firing labeled by its transition. A spring layout places the boxes; the
//! mouse moves the view (drag), a box (drag) and zooms (wheel).
// *****************************************************************************
class MarkingsGraph
{
public:

    //! \brief Beyond, the graph is not drawn (only exported).
    static constexpr size_t MAX_DRAWN = 400u;

    //--------------------------------------------------------------------------
    //! \brief Take the graph of the markings of the net. dot is its Graphviz
    //! text (toDot()), for the export.
    //--------------------------------------------------------------------------
    void set(Net const& net, std::vector<Marking> const& states,
             std::vector<ReachabilityGraph::Edge> const& edges,
             std::vector<ColoredMarking> const& colored, std::string const& title,
             std::string dot);

    //--------------------------------------------------------------------------
    //! \brief Draw the graph in the rest of the current window.
    //--------------------------------------------------------------------------
    void draw();

    //! \brief Place the boxes again (the spring layout starts again).
    void relayout();

    std::string const& dot() const { return m_dot; }
    std::string const& title() const { return m_title; }
    size_t size() const { return m_nodes.size(); }

    //! \brief The window is shown.
    bool open = false;

private:

    //! \brief Some iterations of the spring layout (Fruchterman-Reingold),
    //! fewer moves as it cools down.
    void step();

private:

    struct Box
    {
        std::string label;
        ImVec2 at;
        ImVec2 size;
        bool initial = false;
        bool deadlock = false;
    };
    struct Link
    {
        size_t from;
        size_t to;
        std::string label;
    };

    std::vector<Box> m_nodes;
    std::vector<Link> m_links;
    std::string m_dot;
    std::string m_title;
    //! \brief Wanted distance between two linked boxes.
    float m_spring = 140.0f;
    //! \brief Max move of a box at the next iteration (cools down).
    float m_temperature = 0.0f;
    //! \brief View: screen = origin + zoom * position.
    ImVec2 m_offset = ImVec2(0.0f, 0.0f);
    float m_zoom = 1.0f;
    //! \brief Box dragged by the mouse (-1: none).
    int m_dragged = -1;
    bool m_fit = true;
};

} // namespace tpne

#endif
