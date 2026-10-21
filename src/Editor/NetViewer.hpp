//=============================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2026 Quentin Quadrat <lecrapouille@gmail.com>
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
