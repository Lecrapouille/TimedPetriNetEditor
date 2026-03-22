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

#ifndef PETRI_VIEW_HPP
#  define PETRI_VIEW_HPP

#  include "Document.hpp"
#  include "TimedPetriNetEditor/PetriNet.hpp"
#  include "Net/Simulation.hpp"
#  include "imgui/imgui.h"
#  include <vector>

namespace tpne {

class Editor;

// ****************************************************************************
//! \brief Graphical representation of the Petri net and user interaction.
//! Handles canvas drawing, mouse/keyboard input, and selection management.
// ****************************************************************************
class PetriView
{
public:

    // ************************************************************************
    //! \brief Grid layout configuration for the canvas.
    // ************************************************************************
    struct GridLayout
    {
        float step = 50.0f;   //!< Grid cell size in pixels
        bool  show = true;    //!< Whether to display the grid
        bool  menu = true;    //!< Whether menu is shown (affects pan threshold)
        bool  snap = true;    //!< Snap nodes to grid when moving

        //! \brief Snap a value to the nearest grid line.
        float snapValue(float v) const
        {
            return snap ? roundf(v / step) * step : v;
        }
    };

    //--------------------------------------------------------------------------
    //! \brief Constructor.
    //! \param[in] editor Reference to the parent editor.
    //--------------------------------------------------------------------------
    PetriView(Editor& editor);

    //--------------------------------------------------------------------------
    //! \brief Reshape the canvas to fit the available window space.
    //! \return The new canvas size.
    //--------------------------------------------------------------------------
    ImVec2 reshape();

    //--------------------------------------------------------------------------
    //! \brief Handle mouse and keyboard input for the Petri net view.
    //! \param[in,out] net The Petri net being edited.
    //! \param[in,out] simulation The simulation state.
    //--------------------------------------------------------------------------
    void onHandleInput(Net& net, Simulation& simulation);

    //--------------------------------------------------------------------------
    //! \brief Draw the Petri net on the canvas.
    //! \param[in] net The Petri net to draw.
    //! \param[in] simulation The simulation state.
    //! \param[in] interactive Whether to allow user interaction.
    //! \param[in] show_hover Whether to show hover effect (e.g. in simulation mode).
    //--------------------------------------------------------------------------
    void drawPetriNet(Net& net, Simulation& simulation, bool interactive = true,
                      bool show_hover = true);

    //--------------------------------------------------------------------------
    //! \brief Get the canvas origin in screen coordinates.
    //--------------------------------------------------------------------------
    inline ImVec2 const& origin() const { return m_canvas.origin; }

    //--------------------------------------------------------------------------
    //! \brief Get the canvas size.
    //--------------------------------------------------------------------------
    inline ImVec2 const& size() const { return m_canvas.size; }

    //--------------------------------------------------------------------------
    //! \brief Load view state from document.
    //! \param[in] state The view state to load.
    //--------------------------------------------------------------------------
    void loadViewState(Document::ViewState const& state);

    //--------------------------------------------------------------------------
    //! \brief Save view state to document.
    //! \param[out] state The view state to save to.
    //--------------------------------------------------------------------------
    void saveViewState(Document::ViewState& state) const;

    //--------------------------------------------------------------------------
    //! \brief Clear all node and arc selections.
    //--------------------------------------------------------------------------
    void clearAllSelections();

    //! \brief Grid layout configuration (public for menu access)
    GridLayout grid;

private:

    // ************************************************************************
    //! \brief Canvas state for viewport management.
    // ************************************************************************
    struct Canvas
    {
        ImVec2 corners[2] = {{0.0f, 0.0f}, {0.0f, 0.0f}};
        ImVec2 size{800.0f, 600.0f};
        ImVec2 origin{0.0f, 0.0f};
        ImVec2 scrolling{0.0f, 0.0f};
        float zoom = 1.0f;
        static constexpr float ZOOM_MIN = 0.1f;
        static constexpr float ZOOM_MAX = 5.0f;
        ImDrawList* draw_list = nullptr;

        ImVec2 getMousePosition();
        ImVec2 reshape();
        void push();
        void pop();
    };

    // ************************************************************************
    //! \brief Mouse state for tracking user interactions.
    // ************************************************************************
    struct MouseState
    {
        ImVec2 position;                     //!< Current mouse position in world coords
        ImVec2 clicked_at;                   //!< Position where mouse was clicked
        bool is_dragging_view = false;       //!< Is user dragging the view?
        Node* from = nullptr;                //!< Arc origin node
        bool handling_arc = false;           //!< Is user creating an arc?
        Node* to = nullptr;                  //!< Arc destination node
        std::vector<Node*> selection;        //!< Legacy node movement selection
        std::vector<Node*> selected_nodes;   //!< Persistent node selection
        std::vector<Arc*> selected_arcs;     //!< Persistent arc selection
        bool is_rubber_band = false;         //!< Is rubber band selection active?
        ImVec2 rubber_band_start;            //!< Rubber band start position
        bool is_dragging_nodes = false;      //!< Is user dragging selected nodes?
        std::vector<ImVec2> drag_offsets;    //!< Offsets for dragging nodes
        Node* editing_node = nullptr;        //!< Node being renamed
        char edit_buffer[256] = {0};     //!< Buffer for name editing
        bool edit_focus_requested = false;   //!< Request focus for edit widget
        Node* hovered_node = nullptr;        //!< Node under mouse cursor
        Arc* hovered_arc = nullptr;          //!< Arc under mouse cursor
        Node* context_menu_node = nullptr;   //!< Node for context menu
        Arc* context_menu_arc = nullptr;     //!< Arc for context menu
    };

    // Input handling helpers
    bool isMouseClicked(ImGuiMouseButton& key);
    bool isMouseReleased(ImGuiMouseButton& key);
    bool isMouseDraggingView(ImGuiMouseButton const& key);
    void handleAddNode(ImGuiMouseButton button);
    void handleArcOrigin();
    void handleMoveNode();
    void handleArcDestination();
    void handleSelection(ImGuiMouseButton button);

    // Drawing helpers
    void drawGrid(ImDrawList* draw_list, bool const running);
    void drawRubberBand(ImDrawList* draw_list);

    // Selection management
    bool isNodeSelected(Node* node) const;
    void selectNode(Node* node, bool add_to_selection);
    void deselectNode(Node* node);
    void clearSelection();
    void selectNodesInRect(ImVec2 const& min, ImVec2 const& max);
    Arc* getArc(ImVec2 const& position);
    bool isArcSelected(Arc* arc) const;
    void selectArc(Arc* arc, bool add_to_selection);
    void deselectArc(Arc* arc);
    void clearArcSelection();

    // Clipboard operations
    void copySelection();
    void pasteClipboard();

private:

    Editor& m_editor;                        //!< Reference to parent editor
    Net* m_current_net = nullptr;            //!< Current net being edited
    Simulation* m_current_simulation = nullptr; //!< Current simulation
    Canvas m_canvas;                         //!< Canvas state
    MouseState m_mouse;                      //!< Mouse state
};

} // namespace tpne

#endif // PETRI_VIEW_HPP
