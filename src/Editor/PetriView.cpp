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

#include "Editor/PetriView.hpp"

#include "Editor/Editor.hpp"
#include "Editor/Drawable.hpp"
#include "Editor/KeyBindings.hpp"

#include <algorithm>
#include <limits>
#include <map>
#include <cmath>

namespace tpne {

//------------------------------------------------------------------------------
PetriView::PetriView(Editor& editor)
    : m_editor(editor)
{}

//------------------------------------------------------------------------------
void PetriView::Canvas::push()
{
    draw_list = ImGui::GetWindowDrawList();
    draw_list->ChannelsSplit(1);
    draw_list->PushClipRect(corners[0], corners[1], true);
}

//------------------------------------------------------------------------------
void PetriView::Canvas::pop()
{
    draw_list->PopClipRect();
}

//------------------------------------------------------------------------------
void PetriView::Canvas::reshape()
{
    // ImDrawList API uses screen coordinates
    corners[0] = ImGui::GetCursorScreenPos();

    // Resize canvas to what's available
    size = ImGui::GetContentRegionAvail();
    if (size.x < 50.0f) size.x = 50.0f;
    if (size.y < 50.0f) size.y = 50.0f;
    corners[1] = corners[0] + size;

    // Lock scrolled origin
    origin = corners[0] + scrolling;
}

//------------------------------------------------------------------------------
void PetriView::reshape()
{
    m_canvas.reshape();
}

//------------------------------------------------------------------------------
void PetriView::loadViewState(Document::ViewState const& state)
{
    m_canvas.scrolling.x = state.scrolling_x;
    m_canvas.scrolling.y = state.scrolling_y;
    m_canvas.zoom = state.zoom;
}

//------------------------------------------------------------------------------
void PetriView::saveViewState(Document::ViewState& state) const
{
    state.scrolling_x = m_canvas.scrolling.x;
    state.scrolling_y = m_canvas.scrolling.y;
    state.zoom = m_canvas.zoom;
}

//------------------------------------------------------------------------------
void PetriView::centerOnNet(Net const& net)
{
    // Calculate bounding box of all nodes
    if (net.places().empty() && net.transitions().empty())
        return;

    float min_x = std::numeric_limits<float>::max();
    float min_y = std::numeric_limits<float>::max();
    float max_x = std::numeric_limits<float>::lowest();
    float max_y = std::numeric_limits<float>::lowest();

    for (auto const& place : net.places())
    {
        min_x = std::min(min_x, place.x);
        min_y = std::min(min_y, place.y);
        max_x = std::max(max_x, place.x);
        max_y = std::max(max_y, place.y);
    }

    for (auto const& trans : net.transitions())
    {
        min_x = std::min(min_x, trans.x);
        min_y = std::min(min_y, trans.y);
        max_x = std::max(max_x, trans.x);
        max_y = std::max(max_y, trans.y);
    }

    // Add margin
    const float margin = 50.0f;
    min_x -= margin;
    min_y -= margin;
    max_x += margin;
    max_y += margin;

    float net_width = max_x - min_x;
    float net_height = max_y - min_y;

    // Calculate zoom to fit the net in the canvas
    float zoom_x = m_canvas.size.x / net_width;
    float zoom_y = m_canvas.size.y / net_height;
    float new_zoom = std::min(zoom_x, zoom_y);

    // Clamp zoom to reasonable bounds
    new_zoom = std::clamp(new_zoom, Canvas::ZOOM_MIN, Canvas::ZOOM_MAX);
    m_canvas.zoom = new_zoom;

    // Center the net in the canvas
    float center_x = (min_x + max_x) / 2.0f;
    float center_y = (min_y + max_y) / 2.0f;

    m_canvas.scrolling.x = m_canvas.size.x / 2.0f - center_x * new_zoom;
    m_canvas.scrolling.y = m_canvas.size.y / 2.0f - center_y * new_zoom;
    m_canvas.origin = m_canvas.corners[0] + m_canvas.scrolling;
}

//------------------------------------------------------------------------------
ImVec2 PetriView::Canvas::getMousePosition() const
{
    ImGuiIO const& io = ImGui::GetIO();
    ImVec2 pos = io.MousePos - origin;
    return ImVec2(pos.x / zoom, pos.y / zoom);
}

//------------------------------------------------------------------------------
bool PetriView::isMouseReleased(ImGuiMouseButton& key) const
{
    ImGuiIO const& io = ImGui::GetIO();
    if ((io.MousePos.x >= m_canvas.corners[0].x) &&
        (io.MousePos.x <= m_canvas.corners[1].x) &&
        (io.MousePos.y >= m_canvas.corners[0].y) &&
        (io.MousePos.y <= m_canvas.corners[1].y))
    {
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Middle))
        {
            key = ImGuiMouseButton_Middle;
            return true;
        }
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            key = ImGuiMouseButton_Left;
            return true;
        }
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
        {
            key = ImGuiMouseButton_Right;
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
bool PetriView::isMouseClicked(ImGuiMouseButton& key) const
{
    ImGuiIO const& io = ImGui::GetIO();
    if ((io.MousePos.x >= m_canvas.corners[0].x) &&
        (io.MousePos.x <= m_canvas.corners[1].x) &&
        (io.MousePos.y >= m_canvas.corners[0].y) &&
        (io.MousePos.y <= m_canvas.corners[1].y))
    {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
        {
            key = ImGuiMouseButton_Middle;
            return true;
        }
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            key = ImGuiMouseButton_Left;
            return true;
        }
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        {
            key = ImGuiMouseButton_Right;
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
bool PetriView::isMouseDraggingView(ImGuiMouseButton const& button) const
{
    (void) button;
    const float mouse_threshold_for_pan = grid.menu ? -1.0f : 0.0f;
    return ImGui::IsMouseDragging(MOUSE_BOUTON_DRAGGING_VIEW, mouse_threshold_for_pan);
}

//------------------------------------------------------------------------------
void PetriView::handleMoveNode()
{
    if (m_mouse.selection.empty())
    {
        Node* node = m_editor.getNode(m_mouse.position);
        if (node != nullptr)
        {
            m_mouse.selection.push_back(node);
            m_current_net->modified = true;
        }
    }
    else
    {
        m_mouse.selection.clear();
    }
}

//------------------------------------------------------------------------------
bool PetriView::isNodeSelected(Node const* node) const
{
    if (node == nullptr) return false;
    for (auto const* n : m_mouse.selected_nodes)
    {
        if (n == node) return true;
    }
    return false;
}

//------------------------------------------------------------------------------
void PetriView::selectNode(Node* node, bool add_to_selection)
{
    if (node == nullptr) return;
    if (!add_to_selection)
    {
        m_mouse.selected_nodes.clear();
    }
    if (!isNodeSelected(node))
    {
        m_mouse.selected_nodes.push_back(node);
    }
}

//------------------------------------------------------------------------------
void PetriView::deselectNode(Node* node)
{
    auto it = std::find(m_mouse.selected_nodes.begin(), m_mouse.selected_nodes.end(), node);
    if (it != m_mouse.selected_nodes.end())
    {
        m_mouse.selected_nodes.erase(it);
    }
}

//------------------------------------------------------------------------------
void PetriView::clearSelection()
{
    m_mouse.selected_nodes.clear();
}

//------------------------------------------------------------------------------
void PetriView::selectNodesInRect(ImVec2 const& min, ImVec2 const& max)
{
    float x_min = (min.x < max.x) ? min.x : max.x;
    float x_max = (min.x > max.x) ? min.x : max.x;
    float y_min = (min.y < max.y) ? min.y : max.y;
    float y_max = (min.y > max.y) ? min.y : max.y;

    for (auto& place : m_current_net->places())
    {
        if (place.x >= x_min && place.x <= x_max &&
            place.y >= y_min && place.y <= y_max)
        {
            if (!isNodeSelected(&place))
                m_mouse.selected_nodes.push_back(&place);
        }
    }
    for (auto& trans : m_current_net->transitions())
    {
        if (trans.x >= x_min && trans.x <= x_max &&
            trans.y >= y_min && trans.y <= y_max)
        {
            if (!isNodeSelected(&trans))
                m_mouse.selected_nodes.push_back(&trans);
        }
    }
}

//------------------------------------------------------------------------------
static float pointToSegmentDistance(ImVec2 const& p, ImVec2 const& a, ImVec2 const& b)
{
    ImVec2 ab(b.x - a.x, b.y - a.y);
    ImVec2 ap(p.x - a.x, p.y - a.y);
    float ab_len_sq = ab.x * ab.x + ab.y * ab.y;
    if (ab_len_sq < 0.0001f) return sqrtf(ap.x * ap.x + ap.y * ap.y);
    float t = (ap.x * ab.x + ap.y * ab.y) / ab_len_sq;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    ImVec2 closest(a.x + t * ab.x, a.y + t * ab.y);
    float dx = p.x - closest.x;
    float dy = p.y - closest.y;
    return sqrtf(dx * dx + dy * dy);
}

//------------------------------------------------------------------------------
Arc* PetriView::getArc(ImVec2 const& position)
{
    const float threshold = 10.0f;
    Arc* closest_arc = nullptr;
    float min_dist = threshold;

    for (auto& arc : m_current_net->arcs())
    {
        ImVec2 from_pos(arc.from.x, arc.from.y);
        ImVec2 to_pos;

        // In TimedEventGraph mode, arcs from Transition skip the implicit Place
        // and are drawn directly to the next Transition
        if (m_current_net->type() == TypeOfNet::TimedEventGraph &&
            arc.from.type == Node::Type::Transition &&
            !arc.to.arcsOut.empty())
        {
            Node const& next = arc.to.arcsOut[0]->to;
            to_pos = ImVec2(next.x, next.y);
        }
        else
        {
            to_pos = ImVec2(arc.to.x, arc.to.y);
        }

        float dist = pointToSegmentDistance(position, from_pos, to_pos);
        if (dist < min_dist)
        {
            min_dist = dist;
            closest_arc = &arc;
        }
    }
    return closest_arc;
}

//------------------------------------------------------------------------------
bool PetriView::isArcSelected(Arc const* arc) const
{
    if (arc == nullptr) return false;
    for (auto const* a : m_mouse.selected_arcs)
    {
        if (a == arc) return true;
    }
    return false;
}

//------------------------------------------------------------------------------
void PetriView::selectArc(Arc* arc, bool add_to_selection)
{
    if (arc == nullptr) return;
    if (!add_to_selection)
    {
        m_mouse.selected_arcs.clear();
        m_mouse.selected_nodes.clear();
    }
    if (!isArcSelected(arc))
    {
        m_mouse.selected_arcs.push_back(arc);
    }
}

//------------------------------------------------------------------------------
void PetriView::deselectArc(Arc* arc)
{
    auto it = std::find(m_mouse.selected_arcs.begin(), m_mouse.selected_arcs.end(), arc);
    if (it != m_mouse.selected_arcs.end())
    {
        m_mouse.selected_arcs.erase(it);
    }
}

//------------------------------------------------------------------------------
void PetriView::clearArcSelection()
{
    m_mouse.selected_arcs.clear();
}

//------------------------------------------------------------------------------
void PetriView::clearAllSelections()
{
    clearSelection();
    clearArcSelection();
    m_mouse.editing_node = nullptr;
    m_mouse.hovered_node = nullptr;
    m_mouse.hovered_arc = nullptr;
    m_mouse.is_rubber_band = false;
    m_mouse.is_dragging_nodes = false;
    m_mouse.drag_offsets.clear();
}

//------------------------------------------------------------------------------
void PetriView::copySelection()
{
    if (m_mouse.selected_nodes.empty())
        return;

    m_editor.clearClipboard();

    float sum_x = 0.0f; float sum_y = 0.0f;
    for (auto* node : m_mouse.selected_nodes)
    {
        sum_x += node->x;
        sum_y += node->y;

        if (node->type == Node::Type::Place)
        {
            m_editor.addToClipboard(*reinterpret_cast<Place*>(node));
        }
        else
        {
            m_editor.addToClipboard(*reinterpret_cast<Transition*>(node));
        }
    }
    m_editor.setClipboardCenter(sum_x / static_cast<float>(m_mouse.selected_nodes.size()),
                                sum_y / static_cast<float>(m_mouse.selected_nodes.size()));

    // Copy arcs between selected nodes
    for (auto const& arc : m_current_net->arcs())
    {
        bool from_selected = isNodeSelected(&arc.from);
        bool to_selected = isNodeSelected(&arc.to);
        if (from_selected && to_selected)
        {
            m_editor.addArcToClipboard(arc);
        }
    }
}

//------------------------------------------------------------------------------
void PetriView::pasteClipboard()
{
    if (m_editor.isClipboardEmpty())
        return;

    m_editor.pasteFromClipboard(*m_current_net, m_mouse.position,
                                m_mouse.selected_nodes);
    m_current_net->modified = true;
}

//------------------------------------------------------------------------------
void PetriView::handleSelection(ImGuiMouseButton button)
{
    if (button != MOUSE_BOUTON_ADD_PLACE)
        return;

    Node* node = m_editor.getNode(m_mouse.position);
    Arc* arc = getArc(m_mouse.position);
    bool ctrl_pressed = ImGui::GetIO().KeyCtrl;

    if (node != nullptr)
    {
        if (!ctrl_pressed)
        {
            clearArcSelection();
        }
        if (ctrl_pressed)
        {
            if (isNodeSelected(node))
                deselectNode(node);
            else
                selectNode(node, true);
        }
        else
        {
            if (!isNodeSelected(node))
            {
                selectNode(node, false);
            }
            m_mouse.is_dragging_nodes = true;
            m_mouse.drag_offsets.clear();
            for (auto const* n : m_mouse.selected_nodes)
            {
                m_mouse.drag_offsets.push_back(
                    ImVec2(n->x - m_mouse.position.x, n->y - m_mouse.position.y));
            }
        }
    }
    else if (arc != nullptr)
    {
        if (!ctrl_pressed)
        {
            clearSelection();
        }
        if (ctrl_pressed)
        {
            if (isArcSelected(arc))
                deselectArc(arc);
            else
                selectArc(arc, true);
        }
        else
        {
            selectArc(arc, false);
        }
    }
    else
    {
        if (!ctrl_pressed)
        {
            clearSelection();
            clearArcSelection();
        }
        m_mouse.is_rubber_band = true;
        m_mouse.rubber_band_start = m_mouse.position;
    }
}

//------------------------------------------------------------------------------
void PetriView::handleAddNode(ImGuiMouseButton button)
{
    // Close name edit widget if open
    if (m_mouse.editing_node != nullptr)
    {
        m_mouse.editing_node = nullptr;
        return;
    }

    if (!m_current_simulation->isRunning())
    {
        Node const* node = m_editor.getNode(m_mouse.position);
        Arc const* arc = getArc(m_mouse.position);

        if (button == MOUSE_BOUTON_ADD_PLACE)
        {
            // Click on existing node = selection
            if (node != nullptr)
            {
                handleSelection(button);
                return;
            }
            // Click on arc = arc selection
            if (arc != nullptr)
            {
                handleSelection(button);
                return;
            }
            // Click in empty space = create node or rubber band
            if (!ImGui::GetIO().KeyShift)
            {
                clearSelection();
                clearArcSelection();
                if (m_current_net->type() == TypeOfNet::TimedEventGraph)
                {
                    m_editor.addTransition(m_mouse.position.x, m_mouse.position.y);
                }
                else
                {
                    m_editor.addPlace(m_mouse.position.x, m_mouse.position.y);
                }
            }
            else
            {
                // Shift+click = rubber band only
                handleSelection(button);
            }
        }
        else if (button == MOUSE_BOUTON_ADD_TRANSITION)
        {
            if (node == nullptr && arc == nullptr)
            {
                clearSelection();
                clearArcSelection();
                m_editor.addTransition(m_mouse.position.x, m_mouse.position.y);
            }
        }
    }
    else if (m_current_net->type() == TypeOfNet::PetriNet)
    {
        // During simulation, click on transition to toggle receptivity
        Transition* transition = m_editor.getTransition(m_mouse.position);
        if (transition != nullptr)
        {
            transition->receptivity ^= true;
        }
    }
}

//------------------------------------------------------------------------------
void PetriView::handleArcOrigin()
{
    if (m_current_simulation->isRunning())
        return;

    m_mouse.clicked_at = m_mouse.position;
    m_mouse.from = m_editor.getNode(m_mouse.position);
    m_mouse.handling_arc = !ImGui::GetIO().KeyCtrl;
    m_mouse.to = nullptr;
}

//------------------------------------------------------------------------------
void PetriView::handleArcDestination()
{
    // Finish arc creation
    m_mouse.to = m_editor.getNode(m_mouse.position);
    m_mouse.handling_arc = false;

    if (m_current_net->type() == TypeOfNet::TimedEventGraph)
    {
        // In TimedEventGraph mode we only create transitions
        if (m_mouse.from == nullptr)
        {
            assert(m_mouse.to != nullptr);
            m_mouse.from = &m_editor.addTransition(m_mouse.clicked_at.x, m_mouse.clicked_at.y);
        }
        if (m_mouse.to == nullptr)
        {
            assert(m_mouse.from != nullptr);
            m_mouse.to = &m_editor.addTransition(m_mouse.position.x, m_mouse.position.y);
        }
    }
    else
    {
        // Arc to itself (or both nodes nullptr)
        if ((m_mouse.from == nullptr) && (m_mouse.to == nullptr))
            return;

        if (m_mouse.from == nullptr)
        {
            assert(m_mouse.to != nullptr);
            m_mouse.from = &m_editor.addOppositeNode(m_mouse.to->type, m_mouse.clicked_at.x, m_mouse.clicked_at.y);
        }
        else if (m_mouse.to == nullptr)
        {
            assert(m_mouse.from != nullptr);
            m_mouse.to = &m_editor.addOppositeNode(m_mouse.from->type, m_mouse.position.x, m_mouse.position.y);
        }
    }

    assert(m_mouse.from != nullptr);
    assert(m_mouse.to != nullptr);

    // The case where two nodes have the same type is managed by addArc
    float duration = 1.0f;
    m_editor.addArc(*m_mouse.from, *m_mouse.to, duration);

    // Reset states
    m_mouse.from = m_mouse.to = nullptr;
}

//------------------------------------------------------------------------------
void PetriView::updateHoverState()
{
    if (ImGui::IsItemHovered())
    {
        m_mouse.hovered_node = m_editor.getNode(m_mouse.position);
        m_mouse.hovered_arc = (m_mouse.hovered_node == nullptr)
            ? getArc(m_mouse.position) : nullptr;
    }
    else
    {
        m_mouse.hovered_node = nullptr;
        m_mouse.hovered_arc = nullptr;
    }
}

//------------------------------------------------------------------------------
void PetriView::handleContextMenu()
{
    if (!ImGui::IsMouseClicked(ImGuiMouseButton_Right) || !ImGui::IsItemHovered())
        return;

    Node* node = m_editor.getNode(m_mouse.position);
    Arc* arc = getArc(m_mouse.position);

    if (node != nullptr)
    {
        m_mouse.context_menu_node = node;
        m_mouse.context_menu_arc = nullptr;
        ImGui::OpenPopup("NodeContextMenu");
    }
    else if (arc != nullptr)
    {
        m_mouse.context_menu_node = nullptr;
        m_mouse.context_menu_arc = arc;
        ImGui::OpenPopup("ArcContextMenu");
    }
}

//------------------------------------------------------------------------------
void PetriView::handleDoubleClickRename(Simulation& simulation)
{
    if (simulation.isRunning() || !ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) ||
        !ImGui::IsItemHovered())
        return;

    Node* node = m_editor.getNode(m_mouse.position);
    if (node != nullptr && m_mouse.editing_node == nullptr)
    {
        m_mouse.editing_node = node;
        strncpy(m_mouse.edit_buffer, node->caption.c_str(), sizeof(m_mouse.edit_buffer) - 1);
        m_mouse.edit_buffer[sizeof(m_mouse.edit_buffer) - 1] = '\0';
        m_mouse.edit_focus_requested = true;
    }
}

//------------------------------------------------------------------------------
void PetriView::handleMouseClicksAndDrags(Net& net, Simulation& simulation)
{
    if (!ImGui::IsItemActive() || !ImGui::IsItemHovered())
        return;

    ImGuiMouseButton button;
    if (isMouseClicked(button))
    {
        m_editor.clearMarkedArcs();
        if (button == MOUSE_BOUTON_HANDLE_ARC)
            handleArcOrigin();
        else
            handleAddNode(button);
    }
    else if (isMouseDraggingView(button) && !m_mouse.handling_arc)
    {
        ImGuiIO const& io = ImGui::GetIO();
        m_canvas.scrolling.x += io.MouseDelta.x;
        m_canvas.scrolling.y += io.MouseDelta.y;
    }
    else if (m_mouse.is_dragging_nodes && !m_mouse.selected_nodes.empty())
    {
        for (size_t i = 0; i < m_mouse.selected_nodes.size(); ++i)
        {
            if (i < m_mouse.drag_offsets.size())
            {
                m_mouse.selected_nodes[i]->x = m_mouse.position.x + m_mouse.drag_offsets[i].x;
                m_mouse.selected_nodes[i]->y = m_mouse.position.y + m_mouse.drag_offsets[i].y;
            }
        }
        net.modified = true;
    }
}

//------------------------------------------------------------------------------
void PetriView::handleMouseRelease(Net& /*net*/)
{
    ImGuiMouseButton button;
    if (!isMouseReleased(button))
        return;

    m_mouse.is_dragging_view = false;
    m_mouse.selection.clear();

    if (m_mouse.is_rubber_band)
    {
        if (!ImGui::GetIO().KeyCtrl)
            clearSelection();
        selectNodesInRect(m_mouse.rubber_band_start, m_mouse.position);
        m_mouse.is_rubber_band = false;
    }

    if (m_mouse.is_dragging_nodes && grid.snap)
    {
        for (auto* node : m_mouse.selected_nodes)
        {
            node->x = grid.snapValue(node->x);
            node->y = grid.snapValue(node->y);
        }
    }

    m_mouse.is_dragging_nodes = false;
    m_mouse.drag_offsets.clear();

    if (button == MOUSE_BOUTON_HANDLE_ARC && !ImGui::GetIO().KeyCtrl)
    {
        handleArcDestination();
    }
}

//------------------------------------------------------------------------------
void PetriView::handleMouseWheelZoom()
{
    if (!ImGui::IsItemHovered())
        return;

    ImGuiIO const& io = ImGui::GetIO();
    if (io.MouseWheel == 0.0f)
        return;

    float old_zoom = m_canvas.zoom;
    float new_zoom = old_zoom + io.MouseWheel * 0.1f;
    new_zoom = std::max(Canvas::ZOOM_MIN, std::min(Canvas::ZOOM_MAX, new_zoom));

    if (new_zoom != old_zoom)
    {
        ImVec2 mouse_screen = io.MousePos - m_canvas.corners[0];
        m_canvas.zoom = new_zoom;
        m_canvas.scrolling.x = mouse_screen.x - m_mouse.position.x * new_zoom;
        m_canvas.scrolling.y = mouse_screen.y - m_mouse.position.y * new_zoom;
        m_canvas.origin = m_canvas.corners[0] + m_canvas.scrolling;
    }
}

//------------------------------------------------------------------------------
void PetriView::handleKeyboardShortcuts(Net& net, Simulation& simulation)
{
    if (ImGui::GetIO().WantTextInput)
        return;

    if (ImGui::IsKeyPressed(KEY_RUN_SIMULATION) ||
        ImGui::IsKeyPressed(KEY_RUN_SIMULATION_ALT))
    {
        m_editor.toogleStartAllSimulations();
        return;
    }

    if (!simulation.isRunning() && ImGui::GetIO().KeyCtrl)
    {
        if (ImGui::IsKeyPressed(KEY_UNDO, false))
            m_editor.undo();
        else if (ImGui::IsKeyPressed(KEY_REDO, false))
            m_editor.redo();
        else if (ImGui::IsKeyPressed(ImGuiKey_C, false))
            copySelection();
        else if (ImGui::IsKeyPressed(ImGuiKey_V, false))
            pasteClipboard();
        else if (ImGui::IsKeyPressed(ImGuiKey_A, false))
        {
            clearSelection();
            for (auto& place : net.places())
                m_mouse.selected_nodes.push_back(&place);
            for (auto& trans : net.transitions())
                m_mouse.selected_nodes.push_back(&trans);
        }
    }
}

//------------------------------------------------------------------------------
void PetriView::handleTokenShortcuts(Net& net)
{
    if (!ImGui::IsItemHovered() || ImGui::GetIO().WantTextInput)
        return;

    bool increment = ImGui::IsKeyPressed(KEY_INCREMENT_TOKENS) ||
                     ImGui::IsKeyPressed(ImGuiKey_Equal);
    bool decrement = ImGui::IsKeyPressed(KEY_DECREMENT_TOKENS) ||
                     ImGui::IsKeyPressed(ImGuiKey_Minus);

    if (!increment && !decrement)
        return;

    Node* node = m_editor.getNode(m_mouse.position);
    if (node != nullptr && node->type == Node::Type::Place)
    {
        Place* place = reinterpret_cast<Place*>(node);
        if (increment)
            place->increment(1u);
        else
            place->decrement(1u);
        net.modified = true;
    }
}

//------------------------------------------------------------------------------
void PetriView::handleEditingShortcuts(Net& net, Simulation& simulation)
{
    if (!ImGui::IsItemHovered() || ImGui::GetIO().WantTextInput || simulation.isRunning())
        return;

    if (ImGui::IsKeyPressed(KEY_MOVE_PETRI_NODE, false) ||
        ImGui::IsKeyPressed(ImGuiKey_M, false))
    {
        handleMoveNode();
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_G, false))
    {
        grid.snap ^= true;
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_C, false))
    {
        centerOnNet(net);
    }
    else if (ImGui::IsKeyPressed(KEY_SPRINGIFY_NET))
    {
        if (m_editor.m_force_directed.isRunning())
        {
            m_editor.m_force_directed.reset();
            m_editor.m_messages.setInfo("Auto-layout stopped");
        }
        else
        {
            m_editor.springify();
            m_editor.m_messages.setInfo("Auto-layout started");
        }
    }
    else if (ImGui::IsKeyPressed(KEY_DELETE_NODE))
    {
        bool deleted_something = false;

        if (!m_mouse.selected_arcs.empty())
        {
            for (auto const* arc : m_mouse.selected_arcs)
                m_editor.removeArc(*arc);
            m_mouse.selected_arcs.clear();
            deleted_something = true;
        }

        if (!m_mouse.selected_nodes.empty())
        {
            for (auto const* node : m_mouse.selected_nodes)
                m_editor.removeNode(const_cast<Node&>(*node));
            m_mouse.selected_nodes.clear();
            deleted_something = true;
        }

        if (!deleted_something)
        {
            Arc const* arc = getArc(m_mouse.position);
            if (arc != nullptr)
            {
                m_editor.removeArc(*arc);
            }
            else
            {
                Node* node = m_editor.getNode(m_mouse.position);
                if (node != nullptr)
                    m_editor.removeNode(*node);
            }
        }
    }
}

//------------------------------------------------------------------------------
void PetriView::onHandleInput(Net& net, Simulation& simulation)
{
    m_current_net = &net;
    m_current_simulation = &simulation;

    ImGui::InvisibleButton("canvas", m_canvas.size,
                           ImGuiButtonFlags_MouseButtonLeft |
                           ImGuiButtonFlags_MouseButtonRight |
                           ImGuiButtonFlags_MouseButtonMiddle);

    m_mouse.position = m_canvas.getMousePosition();

    updateHoverState();
    handleContextMenu();
    handleDoubleClickRename(simulation);
    handleMouseClicksAndDrags(net, simulation);
    handleMouseRelease(net);
    handleMouseWheelZoom();
    handleKeyboardShortcuts(net, simulation);
    handleTokenShortcuts(net);
    handleEditingShortcuts(net, simulation);

    if (m_editor.windowShouldClose())
    {
        m_editor.close();
    }
}

//------------------------------------------------------------------------------
void PetriView::drawNetElements(Net& net, Simulation& simulation, float alpha, float zoom)
{
    ImVec2 const& origin = m_canvas.origin;

    // Draw arcs
    for (auto const& arc : net.arcs())
    {
        drawArc(m_canvas.draw_list, arc, net.type(), origin, alpha, zoom);
    }

    // Draw places
    auto const& places = net.places();
    for (size_t i = 0; i < places.size(); ++i)
    {
        bool isInitialStep = false;
        if (net.type() == TypeOfNet::GRAFCET)
        {
            if (simulation.hasInitialMarking())
                isInitialStep = simulation.isInitialStep(i);
            else
                isInitialStep = (places[i].tokens > 0);
        }
        drawPlace(m_canvas.draw_list, places[i], net.type(), origin,
                  m_editor.showPlaceCaptions(), alpha, zoom, isInitialStep);
    }

    // Draw transitions
    for (auto const& trans : net.transitions())
    {
        drawTransition(m_canvas.draw_list, trans, net.type(), origin,
                       m_editor.showTransitionCaptions(), alpha, zoom);
    }

    // Draw animated tokens in transit
    for (auto const& token : simulation.animatedTokens())
    {
        drawTimedToken(m_canvas.draw_list, token.tokens,
                       origin.x + token.x * zoom, origin.y + token.y * zoom, zoom);
    }

    // Draw critical cycle arcs (highlighted)
    for (auto* arc : m_editor.markedArcs())
    {
        drawArc(m_canvas.draw_list, *arc, net.type(), origin, -1.0f, zoom);
    }
}

//------------------------------------------------------------------------------
void PetriView::drawInteractiveElements(float zoom)
{
    // Update node positions during legacy move
    for (auto* node : m_mouse.selection)
    {
        node->x = m_mouse.position.x;
        node->y = m_mouse.position.y;
    }

    // Draw arc being created
    if (m_mouse.handling_arc)
    {
        drawArc(m_canvas.draw_list, m_mouse.from, m_mouse.to, &m_mouse.clicked_at,
                m_canvas.origin, m_mouse.position, zoom);
    }
}

//------------------------------------------------------------------------------
void PetriView::showPlaceTooltip(Place const& place, TypeOfNet type)
{
    ImGui::BeginTooltip();

    if (type == TypeOfNet::GRAFCET)
    {
        ImGui::Text("%s: %s", place.key.c_str(), place.caption.c_str());
        if (place.tokens > 0)
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "[ACTIVE]");
        }
        if (!place.actions.empty())
        {
            ImGui::Separator();
            ImGui::Text("Actions:");
            for (const auto& action : place.actions)
            {
                const char* qual_str = qualifierToStr(action.qualifier);
                ImVec4 badge_color = ImVec4(0.7f, 0.5f, 0.2f, 1.0f);
                if (action.qualifier == Action::Qualifier::N)
                    badge_color = ImVec4(0.4f, 0.4f, 0.8f, 1.0f);
                else if (action.qualifier == Action::Qualifier::S)
                    badge_color = ImVec4(0.2f, 0.7f, 0.2f, 1.0f);
                else if (action.qualifier == Action::Qualifier::R)
                    badge_color = ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
                else if (action.qualifier == Action::Qualifier::P)
                    badge_color = ImVec4(0.6f, 0.2f, 0.6f, 1.0f);

                ImGui::TextColored(badge_color, "[%s]", qual_str);
                ImGui::SameLine();
                ImGui::Text("%s", action.name.c_str());
                if (!action.script.empty())
                {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "- %s", action.script.c_str());
                }
                if (action.duration > 0.0f)
                {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(%.1fs)", action.duration);
                }
            }
        }
    }
    else
    {
        ImGui::Text("%s", place.key.c_str());
        if (!place.caption.empty())
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(%s)", place.caption.c_str());
        }
        ImGui::Text("Tokens: %zu", place.tokens);
    }

    ImGui::EndTooltip();
}

//------------------------------------------------------------------------------
void PetriView::showTransitionTooltip(Transition const& trans, TypeOfNet type)
{
    ImGui::BeginTooltip();

    ImGui::Text("%s", trans.key.c_str());
    if (!trans.caption.empty())
    {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(%s)", trans.caption.c_str());
    }

    if (type == TypeOfNet::GRAFCET)
    {
        if (trans.receptivity)
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Receptivity: TRUE");
        else
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Receptivity: FALSE");

        if (trans.delay > 0.0f)
            ImGui::Text("Delay: %.1fs", trans.delay);
    }
    else if (type == TypeOfNet::TimedPetriNet || type == TypeOfNet::TimedEventGraph)
    {
        if (trans.isEnabled())
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Enabled");
        if (trans.canFire())
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Can Fire");
    }
    else
    {
        if (trans.isEnabled())
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Enabled (can fire)");
        else
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Not enabled");
    }

    ImGui::EndTooltip();
}

//------------------------------------------------------------------------------
void PetriView::drawHoverEffects(Net& net, float zoom)
{
    ImVec2 const& origin = m_canvas.origin;
    const ImU32 hover_color = IM_COL32(200, 200, 255, 180);

    // Draw hovered arc highlight
    if (m_mouse.hovered_arc != nullptr && !isArcSelected(m_mouse.hovered_arc))
    {
        ImVec2 from_pos = origin + ImVec2(m_mouse.hovered_arc->from.x * zoom,
                                          m_mouse.hovered_arc->from.y * zoom);
        ImVec2 to_pos;

        // In TimedEventGraph mode, arcs from Transition skip the implicit Place
        if (net.type() == TypeOfNet::TimedEventGraph &&
            m_mouse.hovered_arc->from.type == Node::Type::Transition &&
            !m_mouse.hovered_arc->to.arcsOut.empty())
        {
            Node const& next = m_mouse.hovered_arc->to.arcsOut[0]->to;
            to_pos = origin + ImVec2(next.x * zoom, next.y * zoom);
        }
        else
        {
            to_pos = origin + ImVec2(m_mouse.hovered_arc->to.x * zoom,
                                     m_mouse.hovered_arc->to.y * zoom);
        }
        m_canvas.draw_list->AddLine(from_pos, to_pos, hover_color, 3.5f * zoom);
    }

    // Draw hovered node highlight and tooltip
    if (m_mouse.hovered_node != nullptr && !isNodeSelected(m_mouse.hovered_node))
    {
        ImVec2 p = origin + ImVec2(m_mouse.hovered_node->x * zoom,
                                   m_mouse.hovered_node->y * zoom);

        if (m_mouse.hovered_node->type == Node::Type::Place)
        {
            if (net.type() == TypeOfNet::GRAFCET)
            {
                float w = TRANS_WIDTH * zoom / 2.0f + 2.0f * zoom;
                m_canvas.draw_list->AddRect(ImVec2(p.x - w, p.y - w),
                                            ImVec2(p.x + w, p.y + w),
                                            hover_color, 0.0f, ImDrawFlags_None, 2.0f * zoom);
            }
            else
            {
                float radius = PLACE_RADIUS * zoom + 2.0f * zoom;
                m_canvas.draw_list->AddCircle(p, radius, hover_color, 64, 2.0f * zoom);
            }

            Place const* place = reinterpret_cast<Place const*>(m_mouse.hovered_node);
            showPlaceTooltip(*place, net.type());
        }
        else
        {
            float w = TRANS_WIDTH * zoom / 2.0f + 2.0f * zoom;
            float h = TRANS_HEIGHT * zoom / 2.0f + 2.0f * zoom;
            m_canvas.draw_list->AddRect(ImVec2(p.x - w, p.y - h),
                                        ImVec2(p.x + w, p.y + h),
                                        hover_color, 0.0f, ImDrawFlags_None, 2.0f * zoom);

            Transition const* trans = reinterpret_cast<Transition const*>(m_mouse.hovered_node);
            showTransitionTooltip(*trans, net.type());
        }
    }
}

//------------------------------------------------------------------------------
void PetriView::drawSelectionHighlights(Net& net, float zoom)
{
    ImVec2 const& origin = m_canvas.origin;
    const ImU32 selection_color = IM_COL32(50, 150, 255, 255);

    // Draw selected arcs
    const float arc_thickness = 4.0f * zoom;
    for (auto const* arc : m_mouse.selected_arcs)
    {
        ImVec2 from_pos = origin + ImVec2(arc->from.x * zoom, arc->from.y * zoom);
        ImVec2 to_pos = origin + ImVec2(arc->to.x * zoom, arc->to.y * zoom);
        m_canvas.draw_list->AddLine(from_pos, to_pos, selection_color, arc_thickness);
    }

    // Draw selected nodes
    const float node_thickness = 3.0f * zoom;
    for (auto const* node : m_mouse.selected_nodes)
    {
        ImVec2 p = origin + ImVec2(node->x * zoom, node->y * zoom);

        if (node->type == Node::Type::Place)
        {
            if (net.type() == TypeOfNet::GRAFCET)
            {
                float w = TRANS_WIDTH * zoom / 2.0f + 4.0f * zoom;
                m_canvas.draw_list->AddRect(ImVec2(p.x - w, p.y - w),
                                            ImVec2(p.x + w, p.y + w),
                                            selection_color, 0.0f, ImDrawFlags_None, node_thickness);
            }
            else
            {
                float radius = PLACE_RADIUS * zoom + 4.0f * zoom;
                m_canvas.draw_list->AddCircle(p, radius, selection_color, 64, node_thickness);
            }
        }
        else
        {
            float w = TRANS_WIDTH * zoom / 2.0f + 4.0f * zoom;
            float h = TRANS_HEIGHT * zoom / 2.0f + 4.0f * zoom;
            m_canvas.draw_list->AddRect(ImVec2(p.x - w, p.y - h),
                                        ImVec2(p.x + w, p.y + h),
                                        selection_color, 0.0f, ImDrawFlags_None, node_thickness);
        }
    }

    drawRubberBand(m_canvas.draw_list);
}

//------------------------------------------------------------------------------
void PetriView::drawPetriNet(Net& net, Simulation& simulation, bool interactive,
                             bool show_hover)
{
    m_current_net = &net;
    m_current_simulation = &simulation;

    const float alpha = 1.0f;
    const float zoom = m_canvas.zoom;

    m_canvas.push();

    drawGrid(m_canvas.draw_list, simulation.isRunning(), simulation.hasReceptivityErrors());
    drawNetElements(net, simulation, alpha, zoom);

    if (interactive)
    {
        drawInteractiveElements(zoom);
    }

    if (show_hover)
    {
        drawHoverEffects(net, zoom);
        drawSelectionHighlights(net, zoom);
    }

    m_canvas.pop();

    // Draw node name edit widget
    if (interactive && m_mouse.editing_node != nullptr)
    {
        ImVec2 node_screen_pos = m_canvas.origin +
            ImVec2(m_mouse.editing_node->x * m_canvas.zoom, m_mouse.editing_node->y * m_canvas.zoom);

        ImGui::SetNextWindowPos(ImVec2(node_screen_pos.x - 75.0f, node_screen_pos.y + 35.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
        ImGui::Begin("##EditNodeName", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_AlwaysAutoResize);

        if (m_mouse.edit_focus_requested)
        {
            ImGui::SetKeyboardFocusHere();
            m_mouse.edit_focus_requested = false;
        }

        ImGui::SetNextItemWidth(150.0f);
        ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll;
        if (ImGui::InputText("##name", m_mouse.edit_buffer, sizeof(m_mouse.edit_buffer), flags))
        {
            m_mouse.editing_node->caption = m_mouse.edit_buffer;
            m_current_net->modified = true;
            m_mouse.editing_node = nullptr;
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            m_mouse.editing_node = nullptr;
        }

        if (!ImGui::IsWindowFocused() && !m_mouse.edit_focus_requested)
        {
            m_mouse.editing_node = nullptr;
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }

    // Node context menu
    if (ImGui::BeginPopup("NodeContextMenu"))
    {
        if (m_mouse.context_menu_node != nullptr)
        {
            bool sim_running = m_current_simulation && m_current_simulation->isRunning();
            ImGui::Text("%s", m_mouse.context_menu_node->key.c_str());
            ImGui::Separator();

            if (ImGui::MenuItem("Rename", nullptr, false, !sim_running))
            {
                m_mouse.editing_node = m_mouse.context_menu_node;
                strncpy(m_mouse.edit_buffer, m_mouse.context_menu_node->caption.c_str(), sizeof(m_mouse.edit_buffer) - 1);
                m_mouse.edit_buffer[sizeof(m_mouse.edit_buffer) - 1] = '\0';
                m_mouse.edit_focus_requested = true;
            }
            if (ImGui::MenuItem("Delete", nullptr, false, !sim_running))
            {
                m_editor.removeNode(*m_mouse.context_menu_node);
                m_mouse.context_menu_node = nullptr;
            }
            if (ImGui::MenuItem("Select", nullptr, false, !sim_running))
            {
                selectNode(m_mouse.context_menu_node, false);
            }
            if (m_mouse.context_menu_node != nullptr && m_mouse.context_menu_node->type == Node::Type::Place)
            {
                ImGui::Separator();
                if (ImGui::MenuItem("Add Token"))
                {
                    reinterpret_cast<Place*>(m_mouse.context_menu_node)->increment(1u);
                    m_current_net->modified = true;
                }
                if (ImGui::MenuItem("Remove Token"))
                {
                    reinterpret_cast<Place*>(m_mouse.context_menu_node)->decrement(1u);
                    m_current_net->modified = true;
                }

                // GRAFCET specific menu items
                if (m_current_net->type() == TypeOfNet::GRAFCET)
                {
                    ImGui::Separator();
                    if (ImGui::MenuItem("Edit Actions..."))
                    {
                        Place* place = reinterpret_cast<Place*>(m_mouse.context_menu_node);
                        if (place->actions.empty())
                        {
                            Action new_action;
                            new_action.name = "Action1";
                            place->actions.push_back(new_action);
                            m_current_net->modified = true;
                        }
                    }

                    ImGui::Separator();
                    Place* place = reinterpret_cast<Place*>(m_mouse.context_menu_node);
                    bool is_active = (place->tokens > 0);

                    if (ImGui::MenuItem("Force Active", nullptr, false, !is_active))
                    {
                        place->tokens = 1;
                        m_current_net->modified = true;
                    }
                    if (ImGui::MenuItem("Force Inactive", nullptr, false, is_active))
                    {
                        place->tokens = 0;
                        m_current_net->modified = true;
                    }
                    if (ImGui::MenuItem("Set Tokens..."))
                    {
                        place->tokens = is_active ? 0 : 1;
                        m_current_net->modified = true;
                    }
                }
            }
        }
        ImGui::EndPopup();
    }

    // Arc context menu
    if (ImGui::BeginPopup("ArcContextMenu"))
    {
        if (m_mouse.context_menu_arc != nullptr)
        {
            bool sim_running = m_current_simulation && m_current_simulation->isRunning();
            ImGui::Text("Arc: %s -> %s", m_mouse.context_menu_arc->from.key.c_str(),
                        m_mouse.context_menu_arc->to.key.c_str());
            ImGui::Separator();

            if (ImGui::MenuItem("Delete", nullptr, false, !sim_running))
            {
                m_editor.removeArc(*m_mouse.context_menu_arc);
                m_mouse.context_menu_arc = nullptr;
            }
            if (ImGui::MenuItem("Select", nullptr, false, !sim_running))
            {
                selectArc(m_mouse.context_menu_arc, false);
            }
        }
        ImGui::EndPopup();
    }
}

//------------------------------------------------------------------------------
void PetriView::drawRubberBand(ImDrawList* draw_list)
{
    if (!m_mouse.is_rubber_band)
        return;

    ImVec2 const& origin = m_canvas.origin;
    const float zoom = m_canvas.zoom;

    ImVec2 p1 = origin + ImVec2(m_mouse.rubber_band_start.x * zoom, m_mouse.rubber_band_start.y * zoom);
    ImVec2 p2 = origin + ImVec2(m_mouse.position.x * zoom, m_mouse.position.y * zoom);

    const ImU32 fill_color = IM_COL32(50, 150, 255, 50);
    const ImU32 border_color = IM_COL32(50, 150, 255, 200);

    draw_list->AddRectFilled(p1, p2, fill_color);
    draw_list->AddRect(p1, p2, border_color, 0.0f, ImDrawFlags_None, 1.5f);
}

//------------------------------------------------------------------------------
void PetriView::drawGrid(ImDrawList* draw_list, bool const running, bool const has_error)
{
    ImU32 border_color;
    ImU32 line_color;

    if (has_error)
    {
        // Red for errors (e.g., GRAFCET receptivity parsing errors)
        border_color = IM_COL32(255, 80, 80, 255);
        line_color = IM_COL32(255, 80, 80, 40);
    }
    else if (running)
    {
        // Green when simulation is running
        border_color = IM_COL32(0, 255, 0, 255);
        line_color = IM_COL32(0, 255, 0, 40);
    }
    else
    {
        // Default white/gray
        border_color = IM_COL32(255, 255, 255, 255);
        line_color = IM_COL32(200, 200, 200, 40);
    }

    // Draw background
    draw_list->ChannelsSetCurrent(0);
    draw_list->AddRectFilled(m_canvas.corners[0], m_canvas.corners[1],
        ((ThemeId::Light == theme())
            ? LIGHT_THEME_PETRI_VIEW_COLOR
            : DARK_THEME_PETRI_VIEW_COLOR));
    draw_list->AddRect(m_canvas.corners[0], m_canvas.corners[1], border_color);

    if (!grid.show)
        return;

    // Draw grid lines
    const float scaled_step = grid.step * m_canvas.zoom;
    for (float x = fmodf(m_canvas.scrolling.x, scaled_step);
         x < m_canvas.size.x; x += scaled_step)
    {
        draw_list->AddLine(
            ImVec2(m_canvas.corners[0].x + x, m_canvas.corners[0].y),
            ImVec2(m_canvas.corners[0].x + x, m_canvas.corners[1].y),
            line_color);
    }

    for (float y = fmodf(m_canvas.scrolling.y, scaled_step);
         y < m_canvas.size.y; y += scaled_step)
    {
        draw_list->AddLine(
            ImVec2(m_canvas.corners[0].x, m_canvas.corners[0].y + y),
            ImVec2(m_canvas.corners[1].x, m_canvas.corners[0].y + y),
            line_color);
    }
}

} // namespace tpne
