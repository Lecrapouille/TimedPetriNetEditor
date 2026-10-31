// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef PETRI_VIEW_HPP
#  define PETRI_VIEW_HPP

#  include "Document.hpp"
#  include "PetriNet/PetriNet.hpp"
#  include "PetriNet/Simulation.hpp"
#  include "imgui.h"

#  include <memory>
#  include <string>
#  include <vector>

namespace tpne {

class Editor;
class NetModificationAction;

// ****************************************************************************
//! \brief The canvas of a net: drawing and editing with the mouse only, no
//! mode to choose. What a button does depends on what is under the mouse and
//! on whether the mouse moves before the button is released:
//!
//! | Under the mouse | Click (release without moving) | Drag                |
//! |-----------------|--------------------------------|---------------------|
//! | nothing, left   | add a place (clears the selection first if any) | select in a rectangle |
//! | nothing, right  | add a transition               | move the view       |
//! | node, left      | select it (Ctrl: add/remove)   | draw an arc from it |
//! | node, right     | menu of the node               | move it (and the selection) |
//! | arc, left       | select it                      | select in a rectangle |
//! | arc, right      | menu of the arc                | move the view       |
//! | middle button   |                                | move the view       |
//!
//! The left button draws (places, arcs, selection), the right button moves
//! (nodes, view).
//!
//! Double click: rename a node, edit the duration of an arc. Wheel: zoom
//! around the mouse. The status bar shows what each button would do now
//! (hint()).
// ****************************************************************************
class PetriView
{
public:

    // ************************************************************************
    //! \brief Grid of the canvas.
    // ************************************************************************
    struct GridLayout
    {
        float step = 50.0f;   //!< Grid cell size in net units
        bool  show = true;    //!< Whether to display the grid
        bool  menu = true;    //!< Unused, kept for the saved settings
        bool  snap = true;    //!< Snap nodes to the grid while moving them

        float snapValue(float v) const
        {
            return snap ? roundf(v / step) * step : v;
        }
    };

    explicit PetriView(Editor& editor);
    ~PetriView();

    //! \brief Take the space left in the current window.
    void reshape();
    //! \brief Mouse and keyboard on the canvas of the active net.
    void onHandleInput(Net& net, Simulation& simulation);
    //! \brief Draw the net and the overlays.
    void drawPetriNet(Net& net, Simulation& simulation, bool interactive = true,
                      bool show_hover = true);

    inline ImVec2 const& origin() const { return m_canvas.origin; }
    inline ImVec2 const& size() const { return m_canvas.size; }
    inline float zoom() const { return m_canvas.zoom; }
    //! \brief Mouse position in net units.
    inline ImVec2 const& mouse() const { return m_mouse.position; }

    bool screenshotRect(int& x, int& y, int& width, int& height) const;
    void loadViewState(Document::ViewState const& state);
    void saveViewState(Document::ViewState& state) const;
    void clearAllSelections();
    //! \brief Fit the whole net in the canvas.
    void centerOnNet(Net const& net);
    //! \brief Zoom by a factor around the center of the canvas.
    void zoomBy(float factor);
    //! \brief What the mouse buttons would do now (for the status bar).
    std::string const& hint() const { return m_hint; }
    //! \brief Number of selected nodes and arcs.
    size_t selectionSize() const { return m_mouse.selected_nodes.size() + m_mouse.selected_arcs.size(); }
    //! \brief True once if the user moved or zoomed the view since the last
    //! call (the net is then no longer fitted when its window changes size).
    bool consumeUserMoved() { bool const moved = m_user_moved; m_user_moved = false; return moved; }
    //! \brief Keep the point of the net at the center of the canvas when its
    //! size changes from the old one.
    void keepCenter(ImVec2 const& old_size);

    GridLayout grid;

private:

    // ************************************************************************
    //! \brief Placement of the net in the canvas.
    // ************************************************************************
    struct Canvas
    {
        ImVec2 corners[2] = {{0.0f, 0.0f}, {0.0f, 0.0f}};
        ImVec2 size{800.0f, 600.0f};
        ImVec2 origin{0.0f, 0.0f};
        ImVec2 scrolling{0.0f, 0.0f};
        float zoom = 1.0f;
        static constexpr float ZOOM_MIN = 0.05f;
        static constexpr float ZOOM_MAX = 6.0f;
        ImDrawList* draw_list = nullptr;

        ImVec2 getMousePosition() const;
        void reshape();
        void push();
        void pop();
    };

    //! \brief What the mouse is doing between press and release.
    enum class Gesture { None, Pending, MoveNodes, RubberBand, Arc, Pan };

    // ************************************************************************
    //! \brief State of the mouse.
    // ************************************************************************
    struct MouseState
    {
        ImVec2 position;                     //!< Mouse position in net units
        Gesture gesture = Gesture::None;
        ImGuiMouseButton button = ImGuiMouseButton_Left; //!< Button of the gesture
        ImVec2 pressed_at;                   //!< Net position of the press
        ImVec2 pressed_screen;               //!< Screen position of the press
        Node* pressed_node = nullptr;        //!< Node under the press
        Arc* pressed_arc = nullptr;          //!< Arc under the press
        bool pressed_ctrl = false;
        std::vector<Node*> selected_nodes;   //!< Persistent node selection
        std::vector<Arc*> selected_arcs;     //!< Persistent arc selection
        std::vector<ImVec2> drag_offsets;    //!< Offsets of the moved nodes
        Node* editing_node = nullptr;        //!< Node being renamed
        char edit_buffer[256] = {0};         //!< Buffer for name editing
        bool edit_focus_requested = false;   //!< Request focus for edit widget
        Arc* editing_arc = nullptr;          //!< Arc whose values are edited
        float edit_duration = 0.0f;          //!< Duration (timed nets)
        int edit_weight = 1;                 //!< Weight (if the net allows it)
        int edit_tokens = 0;                 //!< Tokens of the place (event graph)
        Node* hovered_node = nullptr;        //!< Node under mouse cursor
        Arc* hovered_arc = nullptr;          //!< Arc under mouse cursor
        Node* context_menu_node = nullptr;   //!< Node for context menu
        Arc* context_menu_arc = nullptr;     //!< Arc for context menu
    };

    // Input
    void pressed(ImGuiMouseButton button, Net& net);
    void dragged(Net& net);
    void released(Net& net, Simulation& simulation);
    void clicked(Net& net, Simulation& simulation);
    void finishArc(Net& net);
    void handleWheel();
    void handleKeyboard(Net& net, Simulation& simulation);
    void deleteSelection();
    void sanitize(Net& net);
    void nudgeSelection(Net& net, float dx, float dy);
    void updateHint(Net const& net, Simulation const& simulation);
    void updateCursor() const;

    // Drawing
    void drawGrid(ImDrawList* draw_list, bool const running, bool const has_error = false);
    void drawNetElements(Net& net, Simulation& simulation, float zoom);
    void drawOverlays(Net& net, Simulation& simulation, float zoom);
    //! \brief GRAFCET: state of the timers of the transitions.
    void drawTimers(Net const& net, Simulation const& simulation, float zoom);
    void drawEditWidgets(Net& net);
    //! \brief Open the widget editing the duration, the weight (and the
    //! tokens of the place of a timed event graph) of the arc.
    void editArc(Net const& net, Arc& arc);
    void drawContextMenus(Net& net);
    void drawBadges(Net const& net, Simulation const& simulation);

    // Tooltips
    void showPlaceTooltip(Net const& net, Place const& place, TypeOfNet type);
    void showTransitionTooltip(Net const& net, Transition const& trans);
    void showArcTooltip(Net const& net, Arc const& arc);

    // Selection
    bool isNodeSelected(Node const* node) const;
    void selectNode(Node* node, bool add_to_selection);
    void deselectNode(Node* node);
    void clearSelection();
    void selectNodesInRect(ImVec2 const& min, ImVec2 const& max);
    Node* getNode(ImVec2 const& position) const;
    Arc* getArc(ImVec2 const& position) const;
    bool isArcSelected(Arc const* arc) const;
    void selectArc(Arc* arc, bool add_to_selection);
    void deselectArc(Arc* arc);
    void clearArcSelection();

    // Clipboard
    void copySelection();
    void pasteClipboard();

private:

    Editor& m_editor;
    Net* m_current_net = nullptr;
    Simulation* m_current_simulation = nullptr;
    Canvas m_canvas;
    MouseState m_mouse;
    std::string m_hint;
    //! \brief The user moved or zoomed the view (see consumeUserMoved()).
    bool m_user_moved = false;
    //! \brief Net before the nodes are moved (undo of a move).
    std::unique_ptr<NetModificationAction> m_move_action;
};

} // namespace tpne

#endif // PETRI_VIEW_HPP
