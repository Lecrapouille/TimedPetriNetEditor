// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef TPNE_NET_VIEWER_HPP
#  define TPNE_NET_VIEWER_HPP

#  include "PetriNet/PetriNet.hpp"
#  include "imgui/imgui.h"

#  include <vector>

namespace tpne {

// *****************************************************************************
//! \brief Read-only view of a net, drawn inside the current Dear ImGui window
//! of any host application (no window, no OpenGL context, no file dialog of
//! its own). The user can pan (drag), zoom (wheel) and fit the net (double
//! click). The host owns the net and its marking: the viewer only reads them,
//! so it can show a net simulated somewhere else (for example the GRAFCET
//! controlling a robot cell).
// *****************************************************************************
class NetViewer
{
public:

    //! \brief What to show in addition to the net.
    struct Options
    {
        //! \brief Show captions (true) or identifiers like P0, T1 (false).
        bool captions = true;
        //! \brief Show the actions of GRAFCET steps.
        bool actions = true;
        //! \brief Arcs drawn highlighted (critical circuit ...). Can be null.
        std::vector<Arc const*> const* highlighted_arcs = nullptr;
        //! \brief Initial marking (one count per place) to draw GRAFCET initial
        //! steps with a double square. Can be null: then none is drawn so.
        std::vector<size_t> const* initial_marking = nullptr;
        //! \brief Tell if the action of a step is running (drawn highlighted).
        //! Can be null.
        bool (*is_action_active)(size_t place_id, size_t action, void* data) = nullptr;
        //! \brief Data given to is_action_active.
        void* data = nullptr;
    };

    //--------------------------------------------------------------------------
    //! \brief Draw the net in the space left in the current window.
    //! \param[in] id unique Dear ImGui identifier of the canvas.
    //--------------------------------------------------------------------------
    void draw(char const* id, Net const& net, Options const& options);
    void draw(char const* id, Net const& net) { draw(id, net, Options{}); }

    //--------------------------------------------------------------------------
    //! \brief Fit the net in the canvas during the next draws.
    //--------------------------------------------------------------------------
    void fit() { m_fit_frames = FIT_FRAMES; }

    //--------------------------------------------------------------------------
    //! \brief Node under the mouse at the last draw, or null.
    //--------------------------------------------------------------------------
    Node const* hoveredNode() const { return m_hovered; }

private:

    void fitNow(Net const& net, ImVec2 const& size);

private:

    //! \brief Number of frames during which the net is fitted after a change:
    //! the window may still be resized by its docking layout.
    static constexpr int FIT_FRAMES = 4;
    //! \brief Offset of the net origin in the canvas, in pixels.
    ImVec2 m_scrolling = ImVec2(0.0f, 0.0f);
    //! \brief Scale between net coordinates and pixels.
    float m_zoom = 1.0f;
    //! \brief Remaining frames to fit the net.
    int m_fit_frames = FIT_FRAMES;
    //! \brief Net drawn at the last frame: a new net is fitted.
    Net const* m_net = nullptr;
    //! \brief Node under the mouse.
    Node const* m_hovered = nullptr;
};

} // namespace tpne

#endif
