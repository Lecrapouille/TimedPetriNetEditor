// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

// ImVec2 operators (+, -, *): also defined when the host does not.
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#  define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "Editor/PetriView.hpp"

#include "Editor/Editor.hpp"
#include "Editor/Drawable.hpp"
#include "Editor/EditorState.hpp"
#include "Editor/KeyBindings.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace tpne {

//! \brief Mouse travel (pixels) under which a press and release is a click.
static constexpr float CLICK_DISTANCE = 4.0f;
//! \brief Extra pick distance around nodes and arcs (pixels).
static constexpr float PICK_MARGIN = 6.0f;

//------------------------------------------------------------------------------
PetriView::PetriView(Editor& editor)
    : m_editor(editor)
{}

//------------------------------------------------------------------------------
PetriView::~PetriView() = default;

// ============================================================================
// Canvas
// ============================================================================

//------------------------------------------------------------------------------
void PetriView::Canvas::push()
{
    draw_list = ImGui::GetWindowDrawList();
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
    corners[0] = ImGui::GetCursorScreenPos();
    size = ImGui::GetContentRegionAvail();
    size.x = std::max(size.x, 50.0f);
    size.y = std::max(size.y, 50.0f);
    corners[1] = corners[0] + size;
    origin = corners[0] + scrolling;
}

//------------------------------------------------------------------------------
ImVec2 PetriView::Canvas::getMousePosition() const
{
    ImVec2 const pos = ImGui::GetIO().MousePos - origin;
    return ImVec2(pos.x / zoom, pos.y / zoom);
}

//------------------------------------------------------------------------------
void PetriView::reshape()
{
    m_canvas.reshape();
}

//------------------------------------------------------------------------------
bool PetriView::screenshotRect(int& x, int& y, int& width, int& height) const
{
    if (m_canvas.size.x < 1.0f || m_canvas.size.y < 1.0f)
        return false;

    ImVec2 const scale = ImGui::GetIO().DisplayFramebufferScale;
    width = std::max(1, int(m_canvas.size.x * scale.x + 0.5f));
    height = std::max(1, int(m_canvas.size.y * scale.y + 0.5f));
    x = std::max(0, int(m_canvas.corners[0].x * scale.x + 0.5f));
    y = std::max(0, int(m_canvas.corners[0].y * scale.y + 0.5f));
    return true;
}

//------------------------------------------------------------------------------
void PetriView::loadViewState(Document::ViewState const& state)
{
    m_canvas.scrolling = ImVec2(state.scrolling_x, state.scrolling_y);
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
    if (net.places().empty() && net.transitions().empty())
    {
        m_canvas.zoom = 1.0f;
        m_canvas.scrolling = m_canvas.size * 0.5f;
        m_canvas.origin = m_canvas.corners[0] + m_canvas.scrolling;
        return;
    }

    // Bounding box of the nodes, with room for captions and GRAFCET actions.
    float min_x = std::numeric_limits<float>::max();
    float min_y = std::numeric_limits<float>::max();
    float max_x = std::numeric_limits<float>::lowest();
    float max_y = std::numeric_limits<float>::lowest();
    auto extend = [&](float x0, float y0, float x1, float y1) {
        min_x = std::min(min_x, x0); min_y = std::min(min_y, y0);
        max_x = std::max(max_x, x1); max_y = std::max(max_y, y1);
    };
    bool const grafcet = (net.type() == TypeOfNet::GRAFCET);
    for (auto const& place : net.places())
    {
        if (net.type() == TypeOfNet::TimedEventGraph)
            continue;
        ImVec2 const half = nodeHalfSize(place, net.type());
        extend(place.x - half.x - (grafcet ? 90.0f : 0.0f), place.y - half.y,
               place.x + half.x + (grafcet && !place.actions.empty() ? 160.0f : 0.0f),
               place.y + half.y + 20.0f);
    }
    for (auto const& trans : net.transitions())
    {
        ImVec2 const half = nodeHalfSize(trans, net.type());
        extend(trans.x - half.x - GRAFCET_LOOP, trans.y - half.y,
               trans.x + half.x + (grafcet ? 140.0f : 0.0f), trans.y + half.y + 20.0f);
    }

    float const margin = 30.0f;
    float const width = std::max(1.0f, max_x - min_x);
    float const height = std::max(1.0f, max_y - min_y);
    float zoom = std::min((m_canvas.size.x - 2.0f * margin) / width,
                          (m_canvas.size.y - 2.0f * margin) / height);
    // Never bigger than 1.5: a small net stays at a readable size.
    zoom = std::clamp(zoom, Canvas::ZOOM_MIN, 1.5f);
    m_canvas.zoom = zoom;
    m_canvas.scrolling.x = m_canvas.size.x * 0.5f - (min_x + max_x) * 0.5f * zoom;
    m_canvas.scrolling.y = m_canvas.size.y * 0.5f - (min_y + max_y) * 0.5f * zoom;
    m_canvas.origin = m_canvas.corners[0] + m_canvas.scrolling;
}

//------------------------------------------------------------------------------
void PetriView::zoomBy(float const factor)
{
    ImVec2 const center = m_canvas.size * 0.5f;
    ImVec2 const at = (center - m_canvas.scrolling) / m_canvas.zoom;
    m_canvas.zoom = std::clamp(m_canvas.zoom * factor, Canvas::ZOOM_MIN, Canvas::ZOOM_MAX);
    m_canvas.scrolling = center - at * m_canvas.zoom;
    m_canvas.origin = m_canvas.corners[0] + m_canvas.scrolling;
}

// ============================================================================
// Selection
// ============================================================================

//------------------------------------------------------------------------------
bool PetriView::isNodeSelected(Node const* node) const
{
    return (node != nullptr) &&
        (std::find(m_mouse.selected_nodes.begin(), m_mouse.selected_nodes.end(), node)
         != m_mouse.selected_nodes.end());
}

//------------------------------------------------------------------------------
void PetriView::selectNode(Node* node, bool add_to_selection)
{
    if (!add_to_selection)
        clearSelection();
    if (!isNodeSelected(node))
        m_mouse.selected_nodes.push_back(node);
}

//------------------------------------------------------------------------------
void PetriView::deselectNode(Node* node)
{
    auto it = std::find(m_mouse.selected_nodes.begin(), m_mouse.selected_nodes.end(), node);
    if (it != m_mouse.selected_nodes.end())
        m_mouse.selected_nodes.erase(it);
}

//------------------------------------------------------------------------------
void PetriView::clearSelection()
{
    m_mouse.selected_nodes.clear();
}

//------------------------------------------------------------------------------
void PetriView::selectNodesInRect(ImVec2 const& a, ImVec2 const& b)
{
    float const x0 = std::min(a.x, b.x), x1 = std::max(a.x, b.x);
    float const y0 = std::min(a.y, b.y), y1 = std::max(a.y, b.y);
    auto inside = [&](Node const& n) { return n.x >= x0 && n.x <= x1 && n.y >= y0 && n.y <= y1; };
    bool const teg = (m_current_net->type() == TypeOfNet::TimedEventGraph);
    for (auto& place : m_current_net->places())
    {
        if (!teg && inside(place) && !isNodeSelected(&place))
            m_mouse.selected_nodes.push_back(&place);
    }
    for (auto& trans : m_current_net->transitions())
    {
        if (inside(trans) && !isNodeSelected(&trans))
            m_mouse.selected_nodes.push_back(&trans);
    }
}

//------------------------------------------------------------------------------
Node* PetriView::getNode(ImVec2 const& position) const
{
    if (m_current_net == nullptr)
        return nullptr;
    // Pick with a margin in pixels, whatever the zoom; the nearest node wins.
    float const margin = PICK_MARGIN / m_canvas.zoom;
    TypeOfNet const type = m_current_net->type();
    Node* best = nullptr;
    float best_distance = std::numeric_limits<float>::max();
    auto test = [&](Node& node) {
        ImVec2 const half = nodeHalfSize(node, type);
        float const dx = std::fabs(position.x - node.x);
        float const dy = std::fabs(position.y - node.y);
        // GRAFCET transitions are thin: easier to pick vertically.
        float const extra_y = (type == TypeOfNet::GRAFCET && node.type == Node::Type::Transition) ? 6.0f : 0.0f;
        if (dx <= half.x + margin && dy <= half.y + margin + extra_y)
        {
            float const d = dx * dx + dy * dy;
            if (d < best_distance) { best_distance = d; best = &node; }
        }
    };
    if (type != TypeOfNet::TimedEventGraph)
    {
        for (auto& place : m_current_net->places())
            test(place);
    }
    for (auto& trans : m_current_net->transitions())
        test(trans);
    return best;
}

//------------------------------------------------------------------------------
Arc* PetriView::getArc(ImVec2 const& position) const
{
    if (m_current_net == nullptr)
        return nullptr;
    float best = PICK_MARGIN / m_canvas.zoom;
    Arc* found = nullptr;
    for (auto& arc : m_current_net->arcs())
    {
        float const d = distanceToArc(arc, m_current_net->type(), position);
        if (d < best)
        {
            best = d;
            found = &arc;
        }
    }
    return found;
}

//------------------------------------------------------------------------------
bool PetriView::isArcSelected(Arc const* arc) const
{
    return std::find(m_mouse.selected_arcs.begin(), m_mouse.selected_arcs.end(), arc)
        != m_mouse.selected_arcs.end();
}

//------------------------------------------------------------------------------
void PetriView::selectArc(Arc* arc, bool add_to_selection)
{
    if (!add_to_selection)
        m_mouse.selected_arcs.clear();
    if (!isArcSelected(arc))
        m_mouse.selected_arcs.push_back(arc);
}

//------------------------------------------------------------------------------
void PetriView::deselectArc(Arc* arc)
{
    auto it = std::find(m_mouse.selected_arcs.begin(), m_mouse.selected_arcs.end(), arc);
    if (it != m_mouse.selected_arcs.end())
        m_mouse.selected_arcs.erase(it);
}

//------------------------------------------------------------------------------
void PetriView::clearArcSelection()
{
    m_mouse.selected_arcs.clear();
}

//------------------------------------------------------------------------------
void PetriView::sanitize(Net& net)
{
    // The selection keeps pointers: forget the ones that are not nodes and
    // arcs of this net any more (load, undo, other net, cleared net).
    std::vector<Node const*> nodes;
    for (auto const& place : net.places())
        nodes.push_back(&place);
    for (auto const& trans : net.transitions())
        nodes.push_back(&trans);
    std::sort(nodes.begin(), nodes.end());
    auto const known = [&nodes](Node const* n) {
        return std::binary_search(nodes.begin(), nodes.end(), n);
    };
    std::vector<Arc const*> arcs;
    for (auto const& arc : net.arcs())
        arcs.push_back(&arc);
    std::sort(arcs.begin(), arcs.end());
    auto const known_arc = [&arcs](Arc const* a) {
        return std::binary_search(arcs.begin(), arcs.end(), a);
    };

    auto& sel = m_mouse.selected_nodes;
    sel.erase(std::remove_if(sel.begin(), sel.end(), [&](Node* n) { return !known(n); }), sel.end());
    auto& asel = m_mouse.selected_arcs;
    asel.erase(std::remove_if(asel.begin(), asel.end(), [&](Arc* a) { return !known_arc(a); }), asel.end());
    if ((m_mouse.hovered_node != nullptr) && !known(m_mouse.hovered_node)) m_mouse.hovered_node = nullptr;
    if ((m_mouse.hovered_arc != nullptr) && !known_arc(m_mouse.hovered_arc)) m_mouse.hovered_arc = nullptr;
    if ((m_mouse.editing_node != nullptr) && !known(m_mouse.editing_node)) m_mouse.editing_node = nullptr;
    if ((m_mouse.editing_arc != nullptr) && !known_arc(m_mouse.editing_arc)) m_mouse.editing_arc = nullptr;
    if ((m_mouse.context_menu_node != nullptr) && !known(m_mouse.context_menu_node)) m_mouse.context_menu_node = nullptr;
    if ((m_mouse.context_menu_arc != nullptr) && !known_arc(m_mouse.context_menu_arc)) m_mouse.context_menu_arc = nullptr;
    if ((m_mouse.pressed_node != nullptr) && !known(m_mouse.pressed_node))
    {
        m_mouse.pressed_node = nullptr;
        m_mouse.gesture = Gesture::None;
    }
}

//------------------------------------------------------------------------------
void PetriView::clearAllSelections()
{
    clearSelection();
    clearArcSelection();
    m_mouse.hovered_node = nullptr;
    m_mouse.hovered_arc = nullptr;
    m_mouse.context_menu_node = nullptr;
    m_mouse.context_menu_arc = nullptr;
    m_mouse.editing_node = nullptr;
    m_mouse.editing_arc = nullptr;
    m_mouse.gesture = Gesture::None;
    m_move_action.reset();
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
            m_editor.addToClipboard(*static_cast<Place*>(node));
        else
            m_editor.addToClipboard(*static_cast<Transition*>(node));
    }
    float const n = static_cast<float>(m_mouse.selected_nodes.size());
    m_editor.setClipboardCenter(sum_x / n, sum_y / n);
    for (auto const& arc : m_current_net->arcs())
    {
        if (isNodeSelected(&arc.from) && isNodeSelected(&arc.to))
            m_editor.addArcToClipboard(arc);
    }
    m_editor.m_messages.setInfo(std::to_string(m_mouse.selected_nodes.size()) + " node(s) copied");
}

//------------------------------------------------------------------------------
void PetriView::pasteClipboard()
{
    if (m_editor.isClipboardEmpty())
        return;
    m_editor.pasteFromClipboard(*m_current_net, m_mouse.position, m_mouse.selected_nodes);
    m_current_net->modified = true;
}

// ============================================================================
// Mouse gestures
// ============================================================================

//------------------------------------------------------------------------------
void PetriView::pressed(ImGuiMouseButton const button, Net& net)
{
    m_mouse.button = button;
    m_mouse.pressed_at = m_mouse.position;
    m_mouse.pressed_screen = ImGui::GetIO().MousePos;
    m_mouse.pressed_node = getNode(m_mouse.position);
    m_mouse.pressed_arc = (m_mouse.pressed_node == nullptr) ? getArc(m_mouse.position) : nullptr;
    m_mouse.pressed_ctrl = ImGui::GetIO().KeyCtrl;
    m_mouse.editing_node = nullptr;
    m_mouse.editing_arc = nullptr;
    m_editor.clearMarkedArcs();
    (void) net;

    // Middle button, or Space held: always move the view.
    bool const space = ImGui::IsKeyDown(ImGuiKey_Space);
    m_mouse.gesture = ((button == ImGuiMouseButton_Middle) || space) ? Gesture::Pan : Gesture::Pending;
}

//------------------------------------------------------------------------------
void PetriView::dragged(Net& net)
{
    ImGuiIO const& io = ImGui::GetIO();
    bool const running = (m_current_simulation != nullptr) && m_current_simulation->isRunning();

    if (m_mouse.gesture == Gesture::Pending)
    {
        ImVec2 const d = io.MousePos - m_mouse.pressed_screen;
        if (d.x * d.x + d.y * d.y < CLICK_DISTANCE * CLICK_DISTANCE)
            return;

        // The mouse moved: decide what the drag does.
        if (m_mouse.button == ImGuiMouseButton_Left)
        {
            if ((m_mouse.pressed_node != nullptr) && !running)
            {
                if (!isNodeSelected(m_mouse.pressed_node))
                    selectNode(m_mouse.pressed_node, m_mouse.pressed_ctrl);
                m_mouse.drag_offsets.clear();
                for (auto const* n : m_mouse.selected_nodes)
                    m_mouse.drag_offsets.push_back(ImVec2(n->x, n->y) - m_mouse.pressed_at);
                m_move_action = std::make_unique<NetModificationAction>(
                    [this]() -> Net& { return m_editor.net(); });
                m_move_action->before(net);
                m_mouse.gesture = Gesture::MoveNodes;
            }
            else
            {
                if (!m_mouse.pressed_ctrl)
                {
                    clearSelection();
                    clearArcSelection();
                }
                m_mouse.gesture = Gesture::RubberBand;
            }
        }
        else if (m_mouse.button == ImGuiMouseButton_Right)
        {
            m_mouse.gesture = ((m_mouse.pressed_node != nullptr) && !running)
                ? Gesture::Arc : Gesture::Pan;
        }
    }

    switch (m_mouse.gesture)
    {
    case Gesture::Pan:
        m_canvas.scrolling += io.MouseDelta;
        m_canvas.origin = m_canvas.corners[0] + m_canvas.scrolling;
        break;
    case Gesture::MoveNodes:
        for (size_t i = 0; (i < m_mouse.selected_nodes.size()) && (i < m_mouse.drag_offsets.size()); ++i)
        {
            Node* node = m_mouse.selected_nodes[i];
            node->x = grid.snapValue(m_mouse.position.x + m_mouse.drag_offsets[i].x);
            node->y = grid.snapValue(m_mouse.position.y + m_mouse.drag_offsets[i].y);
        }
        break;
    default:
        break;
    }
}

//------------------------------------------------------------------------------
void PetriView::clicked(Net& net, Simulation& simulation)
{
    Node* node = m_mouse.pressed_node;
    Arc* arc = m_mouse.pressed_arc;
    bool const ctrl = m_mouse.pressed_ctrl;
    ImVec2 const at(grid.snapValue(m_mouse.pressed_at.x), grid.snapValue(m_mouse.pressed_at.y));

    // While simulating: a click on a transition of a Petri net fires it.
    if (simulation.isRunning())
    {
        if ((m_mouse.button == ImGuiMouseButton_Left) && (node != nullptr) &&
            (node->type == Node::Type::Transition) && (net.type() == TypeOfNet::PetriNet))
        {
            static_cast<Transition*>(node)->receptivity ^= true;
        }
        return;
    }

    if (m_mouse.button == ImGuiMouseButton_Left)
    {
        if (node != nullptr)
        {
            clearArcSelection();
            if (ctrl && isNodeSelected(node))
                deselectNode(node);
            else
                selectNode(node, ctrl);
        }
        else if (arc != nullptr)
        {
            clearSelection();
            if (ctrl && isArcSelected(arc))
                deselectArc(arc);
            else
                selectArc(arc, ctrl);
        }
        else if (!m_mouse.selected_nodes.empty() || !m_mouse.selected_arcs.empty())
        {
            // First click in the void: only forget the selection.
            clearSelection();
            clearArcSelection();
        }
        else if (net.type() == TypeOfNet::TimedEventGraph)
        {
            m_editor.addTransition(at.x, at.y);
        }
        else
        {
            m_editor.addPlace(at.x, at.y);
        }
    }
    else if (m_mouse.button == ImGuiMouseButton_Right)
    {
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
        else
        {
            clearSelection();
            clearArcSelection();
            m_editor.addTransition(at.x, at.y);
        }
    }
}

//------------------------------------------------------------------------------
void PetriView::finishArc(Net& net)
{
    Node* from = m_mouse.pressed_node;
    Node* to = getNode(m_mouse.position);
    if ((from == nullptr) || (to == from))
        return;

    ImVec2 const at(grid.snapValue(m_mouse.position.x), grid.snapValue(m_mouse.position.y));
    if (net.type() == TypeOfNet::TimedEventGraph)
    {
        // Arcs link transitions; addArc() inserts the hidden place.
        if (to == nullptr)
            to = &m_editor.addTransition(at.x, at.y);
    }
    else if (to == nullptr)
    {
        // Released in the void: a node of the other kind is created there.
        to = &m_editor.addOppositeNode(from->type, at.x, at.y);
    }
    // Two nodes of the same kind: addArc() inserts a node between them.
    m_editor.addArc(*from, *to, 1.0f);
}

//------------------------------------------------------------------------------
void PetriView::released(Net& net, Simulation& simulation)
{
    switch (m_mouse.gesture)
    {
    case Gesture::Pending:
        clicked(net, simulation);
        break;
    case Gesture::RubberBand:
        selectNodesInRect(m_mouse.pressed_at, m_mouse.position);
        break;
    case Gesture::MoveNodes:
        if (m_move_action)
        {
            m_move_action->after(net);
            m_editor.m_history.add(std::move(m_move_action));
        }
        net.modified = true;
        break;
    case Gesture::Arc:
        finishArc(net);
        break;
    default:
        break;
    }
    m_mouse.gesture = Gesture::None;
    m_mouse.drag_offsets.clear();
    m_move_action.reset();
}

//------------------------------------------------------------------------------
void PetriView::handleWheel()
{
    ImGuiIO const& io = ImGui::GetIO();
    if (io.MouseWheel == 0.0f && io.MouseWheelH == 0.0f)
        return;

    // Shift + wheel, or a horizontal wheel (touchpad): move the view.
    if (io.KeyShift || io.MouseWheelH != 0.0f)
    {
        float const step = 40.0f;
        m_canvas.scrolling.x += (io.KeyShift ? io.MouseWheel : io.MouseWheelH) * step;
        if (!io.KeyShift)
            m_canvas.scrolling.y += io.MouseWheel * step;
        m_canvas.origin = m_canvas.corners[0] + m_canvas.scrolling;
        return;
    }

    // Zoom around the mouse: the point under it does not move.
    ImVec2 const mouse = io.MousePos - m_canvas.corners[0];
    ImVec2 const at = (mouse - m_canvas.scrolling) / m_canvas.zoom;
    m_canvas.zoom = std::clamp(m_canvas.zoom * std::pow(1.15f, io.MouseWheel),
                               Canvas::ZOOM_MIN, Canvas::ZOOM_MAX);
    m_canvas.scrolling = mouse - at * m_canvas.zoom;
    m_canvas.origin = m_canvas.corners[0] + m_canvas.scrolling;
}

//------------------------------------------------------------------------------
void PetriView::deleteSelection()
{
    // Removing a node or an arc moves the others in their containers:
    // remember the keys first, and look each one up again before removal.
    std::vector<std::pair<std::string, std::string>> arcs;
    for (auto const* arc : m_mouse.selected_arcs)
        arcs.emplace_back(arc->from.key, arc->to.key);
    std::vector<std::string> nodes;
    for (auto const* node : m_mouse.selected_nodes)
        nodes.push_back(node->key);
    if (arcs.empty() && nodes.empty())
    {
        if (m_mouse.hovered_node != nullptr)
            nodes.push_back(m_mouse.hovered_node->key);
        else if (m_mouse.hovered_arc != nullptr)
            arcs.emplace_back(m_mouse.hovered_arc->from.key, m_mouse.hovered_arc->to.key);
    }
    clearAllSelections();

    for (auto const& key : arcs)
    {
        Node* from = m_current_net->findNode(key.first);
        Node* to = m_current_net->findNode(key.second);
        Arc* arc = ((from != nullptr) && (to != nullptr)) ? m_current_net->findArc(*from, *to) : nullptr;
        if (arc != nullptr)
            m_editor.removeArc(*arc);
    }
    for (auto const& key : nodes)
    {
        Node* node = m_current_net->findNode(key);
        if (node != nullptr)
            m_editor.removeNode(*node);
    }
}

//------------------------------------------------------------------------------
void PetriView::nudgeSelection(Net& net, float dx, float dy)
{
    if (m_mouse.selected_nodes.empty())
        return;
    auto action = std::make_unique<NetModificationAction>([this]() -> Net& { return m_editor.net(); });
    action->before(net);
    for (auto* node : m_mouse.selected_nodes)
    {
        node->x += dx;
        node->y += dy;
    }
    action->after(net);
    m_editor.m_history.add(std::move(action));
    net.modified = true;
}

//------------------------------------------------------------------------------
void PetriView::handleKeyboard(Net& net, Simulation& simulation)
{
    ImGuiIO const& io = ImGui::GetIO();
    if (io.WantTextInput || !ImGui::IsWindowFocused())
        return;

    if (ImGui::IsKeyPressed(KEY_RUN_SIMULATION_ALT, false) && !io.KeyCtrl)
    {
        m_editor.toogleStartAllSimulations();
        return;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_F, false) && !io.KeyCtrl)
    {
        centerOnNet(net);
        return;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
    {
        if (m_mouse.gesture != Gesture::None)
            m_mouse.gesture = Gesture::None;
        else
            clearAllSelections();
        return;
    }

    // Tokens of the place under the mouse, also while simulating.
    if ((m_mouse.hovered_node != nullptr) && (m_mouse.hovered_node->type == Node::Type::Place))
    {
        bool const more = ImGui::IsKeyPressed(KEY_INCREMENT_TOKENS) || ImGui::IsKeyPressed(ImGuiKey_Equal);
        bool const less = ImGui::IsKeyPressed(KEY_DECREMENT_TOKENS) || ImGui::IsKeyPressed(ImGuiKey_Minus);
        Place* place = static_cast<Place*>(m_mouse.hovered_node);
        if (more) { place->increment(1u); net.modified = true; }
        if (less) { place->decrement(1u); net.modified = true; }
    }

    if (simulation.isRunning())
        return;

    if (io.KeyCtrl)
    {
        if (ImGui::IsKeyPressed(KEY_UNDO, false))
            io.KeyShift ? m_editor.redo() : m_editor.undo();
        else if (ImGui::IsKeyPressed(KEY_REDO, false))
            m_editor.redo();
        else if (ImGui::IsKeyPressed(ImGuiKey_C, false))
            copySelection();
        else if (ImGui::IsKeyPressed(ImGuiKey_X, false))
        {
            copySelection();
            deleteSelection();
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_V, false))
            pasteClipboard();
        else if (ImGui::IsKeyPressed(ImGuiKey_A, false))
        {
            clearSelection();
            selectNodesInRect(ImVec2(-1e9f, -1e9f), ImVec2(1e9f, 1e9f));
        }
        return;
    }

    if (ImGui::IsKeyPressed(KEY_DELETE_NODE) || ImGui::IsKeyPressed(ImGuiKey_Backspace))
        deleteSelection();
    else if (ImGui::IsKeyPressed(ImGuiKey_G, false))
        grid.snap ^= true;
    else if (ImGui::IsKeyPressed(ImGuiKey_L, false))
    {
        if (m_editor.m_force_directed.isRunning())
        {
            m_editor.m_force_directed.reset();
            m_editor.m_messages.setInfo("Auto-layout stopped");
        }
        else
        {
            m_editor.springify();
            m_editor.m_messages.setInfo("Auto-layout started (L to stop)");
        }
    }
    else
    {
        float const step = io.KeyShift ? 1.0f : grid.step * 0.5f;
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) nudgeSelection(net, -step, 0.0f);
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) nudgeSelection(net, step, 0.0f);
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) nudgeSelection(net, 0.0f, -step);
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) nudgeSelection(net, 0.0f, step);
    }
}

//------------------------------------------------------------------------------
void PetriView::updateHint(Net const& net, Simulation const& simulation)
{
    bool const teg = (net.type() == TypeOfNet::TimedEventGraph);
    switch (m_mouse.gesture)
    {
    case Gesture::MoveNodes:
        m_hint = grid.snap ? "Moving (snapped to the grid: G to toggle)" : "Moving (G: snap to the grid)";
        return;
    case Gesture::RubberBand:
        m_hint = "Selecting: release to select the nodes in the rectangle";
        return;
    case Gesture::Arc:
        m_hint = teg ? "Release on a transition to link it, in the void to create one"
                     : "Release on a node to link it, in the void to create a node of the other kind";
        return;
    case Gesture::Pan:
        m_hint = "Moving the view";
        return;
    default:
        break;
    }
    if (simulation.isRunning())
    {
        m_hint = (net.type() == TypeOfNet::PetriNet)
            ? "Simulating: click a transition to fire it | +/-: tokens | R: stop"
            : "Simulating | +/-: tokens under the mouse | R: stop";
        return;
    }
    if (m_mouse.hovered_node != nullptr)
    {
        m_hint = "Drag: move | Right drag: draw an arc | Double click: rename | Right click: menu | Del: delete";
        if (m_mouse.hovered_node->type == Node::Type::Place)
            m_hint += " | +/-: tokens";
    }
    else if (m_mouse.hovered_arc != nullptr)
    {
        m_hint = "Click: select | Double click: duration | Right click: menu | Del: delete";
    }
    else
    {
        m_hint = teg ? "Click: add a transition" : "Click: add a place | Right click: add a transition";
        m_hint += " | Drag: select | Right drag or middle drag: move the view | Wheel: zoom | F: fit";
    }
}

//------------------------------------------------------------------------------
void PetriView::updateCursor() const
{
    switch (m_mouse.gesture)
    {
    case Gesture::MoveNodes: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll); break;
    case Gesture::Pan: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll); break;
    case Gesture::Arc: ImGui::SetMouseCursor(ImGuiMouseCursor_Hand); break;
    default:
        if ((m_mouse.hovered_node != nullptr) || (m_mouse.hovered_arc != nullptr))
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        break;
    }
}

//------------------------------------------------------------------------------
void PetriView::onHandleInput(Net& net, Simulation& simulation)
{
    m_current_net = &net;
    m_current_simulation = &simulation;
    sanitize(net);

    ImGui::InvisibleButton("canvas", m_canvas.size,
                           ImGuiButtonFlags_MouseButtonLeft |
                           ImGuiButtonFlags_MouseButtonRight |
                           ImGuiButtonFlags_MouseButtonMiddle);
    bool const hovered = ImGui::IsItemHovered();
    m_mouse.position = m_canvas.getMousePosition();

    // What is under the mouse (not while a gesture runs).
    if (hovered && (m_mouse.gesture == Gesture::None))
    {
        m_mouse.hovered_node = getNode(m_mouse.position);
        m_mouse.hovered_arc = (m_mouse.hovered_node == nullptr) ? getArc(m_mouse.position) : nullptr;
    }
    else if (!hovered && (m_mouse.gesture == Gesture::None))
    {
        m_mouse.hovered_node = nullptr;
        m_mouse.hovered_arc = nullptr;
    }

    // Press, drag, release.
    if (hovered && (m_mouse.gesture == Gesture::None))
    {
        for (ImGuiMouseButton button : { ImGuiMouseButton_Left, ImGuiMouseButton_Right, ImGuiMouseButton_Middle })
        {
            if (ImGui::IsMouseClicked(button))
            {
                ImGui::SetWindowFocus();
                pressed(button, net);
                break;
            }
        }
    }
    if (m_mouse.gesture != Gesture::None)
    {
        if (ImGui::IsMouseDown(m_mouse.button))
            dragged(net);
        else
            released(net, simulation);
    }

    // Double click: rename a node, edit the duration of an arc.
    if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && !simulation.isRunning())
    {
        if (Node* node = getNode(m_mouse.position))
        {
            m_mouse.editing_node = node;
            std::strncpy(m_mouse.edit_buffer, node->caption.c_str(), sizeof(m_mouse.edit_buffer) - 1);
            m_mouse.edit_buffer[sizeof(m_mouse.edit_buffer) - 1] = '\0';
            m_mouse.edit_focus_requested = true;
        }
        else if (Arc* arc = getArc(m_mouse.position))
        {
            if ((arc->from.type == Node::Type::Transition) && (net.type() != TypeOfNet::GRAFCET) &&
                (net.type() != TypeOfNet::PetriNet))
            {
                m_mouse.editing_arc = arc;
                m_mouse.edit_duration = std::isnan(arc->duration) ? 0.0f : arc->duration;
                m_mouse.edit_focus_requested = true;
            }
        }
    }

    if (hovered)
        handleWheel();
    handleKeyboard(net, simulation);
    updateHint(net, simulation);
    if (hovered)
        updateCursor();

    if (m_editor.windowShouldClose())
        m_editor.close();
}

// ============================================================================
// Drawing
// ============================================================================

//------------------------------------------------------------------------------
void PetriView::drawGrid(ImDrawList* draw_list, bool const running, bool const has_error)
{
    Palette const& pal = palette();
    draw_list->AddRectFilled(m_canvas.corners[0], m_canvas.corners[1], pal.canvas);

    if (grid.show)
    {
        float step = grid.step * m_canvas.zoom;
        int major_every = 5;
        while (step < 12.0f) { step *= 5.0f; major_every = 5; } // keep lines apart when far
        float const start_x = std::fmod(m_canvas.scrolling.x, step);
        float const start_y = std::fmod(m_canvas.scrolling.y, step);
        int index_x = int(std::floor(-m_canvas.scrolling.x / step + start_x / step));
        int index_y = int(std::floor(-m_canvas.scrolling.y / step + start_y / step));
        for (float x = start_x; x < m_canvas.size.x; x += step, ++index_x)
        {
            ImU32 const c = (index_x % major_every == 0) ? pal.grid_major : pal.grid;
            draw_list->AddLine(ImVec2(m_canvas.corners[0].x + x, m_canvas.corners[0].y),
                               ImVec2(m_canvas.corners[0].x + x, m_canvas.corners[1].y), c);
        }
        for (float y = start_y; y < m_canvas.size.y; y += step, ++index_y)
        {
            ImU32 const c = (index_y % major_every == 0) ? pal.grid_major : pal.grid;
            draw_list->AddLine(ImVec2(m_canvas.corners[0].x, m_canvas.corners[0].y + y),
                               ImVec2(m_canvas.corners[1].x, m_canvas.corners[0].y + y), c);
        }
    }

    // A colored frame says the state: green simulating, red syntax errors.
    if (has_error)
        draw_list->AddRect(m_canvas.corners[0], m_canvas.corners[1], pal.critical, 0.0f, 0, 2.0f);
    else if (running)
        draw_list->AddRect(m_canvas.corners[0], m_canvas.corners[1], pal.active, 0.0f, 0, 2.0f);
}

//------------------------------------------------------------------------------
void PetriView::drawNetElements(Net& net, Simulation& simulation, float zoom)
{
    ImVec2 const& origin = m_canvas.origin;
    ImDrawList* draw_list = m_canvas.draw_list;

    for (auto const& arc : net.arcs())
        drawArc(draw_list, arc, net.type(), origin, 1.0f, zoom);
    for (auto* arc : m_editor.markedArcs())
        drawArc(draw_list, *arc, net.type(), origin, -1.0f, zoom);

    auto const& places = net.places();
    for (size_t i = 0; i < places.size(); ++i)
    {
        bool initial = false;
        if (net.type() == TypeOfNet::GRAFCET)
        {
            initial = simulation.hasInitialMarking() ? simulation.isInitialStep(i)
                                                     : (places[i].tokens > 0);
        }
        drawPlace(draw_list, places[i], net.type(), origin, m_editor.showPlaceCaptions(), 1.0f,
                  zoom, initial);

        // GRAFCET actions on the right of their step, green while running.
        if (net.type() == TypeOfNet::GRAFCET)
        {
            struct Step { Simulation const* simulation; size_t place_id; };
            Step step { &simulation, places[i].id };
            drawStepActions(draw_list, places[i], origin, zoom,
                [](size_t const index, void* data) -> bool {
                    Step const& s = *static_cast<Step const*>(data);
                    return s.simulation->isRunning() &&
                           s.simulation->actionState(s.place_id, index).active;
                }, &step);
        }
    }
    for (auto const& trans : net.transitions())
    {
        drawTransition(draw_list, trans, net.type(), origin, m_editor.showTransitionCaptions(),
                       1.0f, zoom);
    }
    for (auto const& token : simulation.animatedTokens())
    {
        drawTimedToken(draw_list, token.tokens, origin.x + token.x * zoom,
                       origin.y + token.y * zoom, zoom);
    }
}

//------------------------------------------------------------------------------
void PetriView::drawOverlays(Net& net, Simulation& simulation, float zoom)
{
    Palette const& pal = palette();
    ImVec2 const& origin = m_canvas.origin;
    ImDrawList* draw_list = m_canvas.draw_list;
    TypeOfNet const type = net.type();

    // Selection, then hover.
    for (auto const* arc : m_mouse.selected_arcs)
        drawArcHighlight(draw_list, *arc, type, origin, zoom, pal.selection, std::max(2.0f, 3.5f * zoom));
    for (auto const* node : m_mouse.selected_nodes)
        drawNodeOutline(draw_list, *node, type, origin, zoom, pal.selection, 4.0f, 2.5f);
    if ((m_mouse.gesture == Gesture::None) || (m_mouse.gesture == Gesture::Arc))
    {
        if ((m_mouse.hovered_arc != nullptr) && !isArcSelected(m_mouse.hovered_arc))
            drawArcHighlight(draw_list, *m_mouse.hovered_arc, type, origin, zoom, pal.hover,
                             std::max(3.0f, 5.0f * zoom));
        Node const* hovered = (m_mouse.gesture == Gesture::Arc) ? getNode(m_mouse.position)
                                                                 : m_mouse.hovered_node;
        if ((hovered != nullptr) && !isNodeSelected(hovered))
            drawNodeOutline(draw_list, *hovered, type, origin, zoom, pal.hover, 4.0f, 3.0f);
    }

    // Arc being drawn.
    if (m_mouse.gesture == Gesture::Arc)
    {
        drawArc(draw_list, m_mouse.pressed_node, nullptr, nullptr, origin, m_mouse.position, zoom);
    }

    // Rubber band.
    if (m_mouse.gesture == Gesture::RubberBand)
    {
        ImVec2 const a = origin + m_mouse.pressed_at * zoom;
        ImVec2 const b = origin + m_mouse.position * zoom;
        ImVec2 const lo(std::min(a.x, b.x), std::min(a.y, b.y));
        ImVec2 const hi(std::max(a.x, b.x), std::max(a.y, b.y));
        draw_list->AddRectFilled(lo, hi, (pal.selection & 0x00FFFFFF) | 0x30000000);
        draw_list->AddRect(lo, hi, pal.selection, 0.0f, 0, 1.5f);
    }

    // Tooltip of the node under the mouse, after a short rest.
    if ((m_mouse.gesture == Gesture::None) && (m_mouse.hovered_node != nullptr) &&
        ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay))
    {
        if (m_mouse.hovered_node->type == Node::Type::Place)
            showPlaceTooltip(*static_cast<Place const*>(m_mouse.hovered_node), type);
        else
            showTransitionTooltip(*static_cast<Transition const*>(m_mouse.hovered_node), type);
    }
    (void) simulation;
}

//------------------------------------------------------------------------------
void PetriView::drawBadges(Net const& net, Simulation const& simulation)
{
    Palette const& pal = palette();
    ImDrawList* draw_list = m_canvas.draw_list;
    float const pad = 8.0f;

    // Top left: type of net and state.
    std::string label = to_str(net.type());
    ImU32 color = pal.secondary;
    if (simulation.isRunning())
    {
        label += "  -  simulating";
        color = pal.active;
    }
    else if (simulation.hasReceptivityErrors())
    {
        label += "  -  receptivity errors";
        color = pal.critical;
    }
    ImVec2 const p = m_canvas.corners[0] + ImVec2(pad, pad);
    ImVec2 const dim = ImGui::CalcTextSize(label.c_str());
    draw_list->AddRectFilled(p - ImVec2(4, 2), p + dim + ImVec2(4, 2), pal.label_bg, 4.0f);
    draw_list->AddText(p, color, label.c_str());

    // Bottom right: zoom.
    char zoom[32];
    std::snprintf(zoom, sizeof(zoom), "%d %%", int(std::round(m_canvas.zoom * 100.0f)));
    ImVec2 const zdim = ImGui::CalcTextSize(zoom);
    ImVec2 const zp = m_canvas.corners[1] - zdim - ImVec2(pad, pad);
    draw_list->AddRectFilled(zp - ImVec2(4, 2), zp + zdim + ImVec2(4, 2), pal.label_bg, 4.0f);
    draw_list->AddText(zp, pal.secondary, zoom);

    // Empty net: say how to start.
    if (net.isEmpty())
    {
        char const* help = (net.type() == TypeOfNet::TimedEventGraph)
            ? "Click to add a transition, right drag from it to draw an arc"
            : "Click to add a place, right click to add a transition, right drag from a node to draw an arc";
        ImVec2 const hdim = ImGui::CalcTextSize(help);
        ImVec2 const c = (m_canvas.corners[0] + m_canvas.corners[1]) * 0.5f - hdim * 0.5f;
        draw_list->AddText(c, pal.secondary, help);
    }
}

//------------------------------------------------------------------------------
void PetriView::drawEditWidgets(Net& net)
{
    auto const popup = [&](Node const& anchor) {
        ImVec2 const at = m_canvas.origin + ImVec2(anchor.x, anchor.y) * m_canvas.zoom;
        ImGui::SetNextWindowPos(ImVec2(at.x - 90.0f, at.y + 24.0f * m_canvas.zoom));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 6.0f));
        bool const open = ImGui::Begin("##EditNode", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::PopStyleVar();
        return open;
    };

    if (m_mouse.editing_node != nullptr)
    {
        popup(*m_mouse.editing_node);
        if (m_mouse.edit_focus_requested)
        {
            ImGui::SetKeyboardFocusHere();
            m_mouse.edit_focus_requested = false;
        }
        ImGui::SetNextItemWidth(180.0f);
        char const* label = ((net.type() == TypeOfNet::GRAFCET) &&
                             (m_mouse.editing_node->type == Node::Type::Transition))
            ? "##receptivity" : "##caption";
        if (ImGui::InputTextWithHint(label, (label[2] == 'r') ? "receptivity: a . /b" : "caption",
                                     m_mouse.edit_buffer, sizeof(m_mouse.edit_buffer),
                                     ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
        {
            auto action = std::make_unique<NetModificationAction>([this]() -> Net& { return m_editor.net(); });
            action->before(net);
            m_mouse.editing_node->caption = m_mouse.edit_buffer;
            action->after(net);
            m_editor.m_history.add(std::move(action));
            net.modified = true;
            m_mouse.editing_node = nullptr;
        }
        if (m_mouse.editing_node != nullptr && (net.type() == TypeOfNet::GRAFCET) &&
            (m_mouse.editing_node->type == Node::Type::Transition))
        {
            std::string const error = Receptivity::check(m_mouse.edit_buffer);
            if (!error.empty())
                ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(palette().critical), "%s", error.c_str());
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape) ||
            (!ImGui::IsWindowFocused() && !m_mouse.edit_focus_requested))
            m_mouse.editing_node = nullptr;
        ImGui::End();
    }
    else if (m_mouse.editing_arc != nullptr)
    {
        popup(m_mouse.editing_arc->from);
        if (m_mouse.edit_focus_requested)
        {
            ImGui::SetKeyboardFocusHere();
            m_mouse.edit_focus_requested = false;
        }
        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::InputFloat("duration##arc", &m_mouse.edit_duration, 0.0f, 0.0f, "%.2f",
                              ImGuiInputTextFlags_EnterReturnsTrue))
        {
            auto action = std::make_unique<NetModificationAction>([this]() -> Net& { return m_editor.net(); });
            action->before(net);
            m_mouse.editing_arc->duration = std::max(0.0f, m_mouse.edit_duration);
            action->after(net);
            m_editor.m_history.add(std::move(action));
            net.modified = true;
            m_mouse.editing_arc = nullptr;
        }
        if (m_mouse.editing_arc != nullptr &&
            (ImGui::IsKeyPressed(ImGuiKey_Escape) || (!ImGui::IsWindowFocused() && !m_mouse.edit_focus_requested)))
            m_mouse.editing_arc = nullptr;
        ImGui::End();
    }
}

//------------------------------------------------------------------------------
void PetriView::drawContextMenus(Net& net)
{
    bool const running = (m_current_simulation != nullptr) && m_current_simulation->isRunning();

    if (ImGui::BeginPopup("NodeContextMenu"))
    {
        Node* node = m_mouse.context_menu_node;
        if (node != nullptr)
        {
            ImGui::TextDisabled("%s  %s", node->key.c_str(), node->caption.c_str());
            ImGui::Separator();
            if (ImGui::MenuItem("Rename", "double click", false, !running))
            {
                m_mouse.editing_node = node;
                std::strncpy(m_mouse.edit_buffer, node->caption.c_str(), sizeof(m_mouse.edit_buffer) - 1);
                m_mouse.edit_focus_requested = true;
            }
            if (ImGui::MenuItem("Select", nullptr, false, !running))
                selectNode(node, false);
            if (node->type == Node::Type::Place)
            {
                Place* place = static_cast<Place*>(node);
                ImGui::Separator();
                if (net.type() == TypeOfNet::GRAFCET)
                {
                    bool active = (place->tokens > 0u);
                    if (ImGui::MenuItem(active ? "Deactivate step" : "Activate step"))
                    {
                        place->tokens = active ? 0u : 1u;
                        net.modified = true;
                    }
                    if (ImGui::MenuItem("Add an action", nullptr, false, !running))
                    {
                        Action action;
                        action.name = "Action" + std::to_string(place->actions.size() + 1u);
                        place->actions.push_back(action);
                        net.modified = true;
                    }
                }
                else
                {
                    if (ImGui::MenuItem("Add a token", "+"))
                    {
                        place->increment(1u);
                        net.modified = true;
                    }
                    if (ImGui::MenuItem("Remove a token", "-", false, place->tokens > 0u))
                    {
                        place->decrement(1u);
                        net.modified = true;
                    }
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Delete", "Del", false, !running))
            {
                m_editor.removeNode(*node);
                clearAllSelections();
            }
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("ArcContextMenu"))
    {
        Arc* arc = m_mouse.context_menu_arc;
        if (arc != nullptr)
        {
            ImGui::TextDisabled("%s -> %s", arc->from.key.c_str(), arc->to.key.c_str());
            ImGui::Separator();
            if ((arc->from.type == Node::Type::Transition) && (net.type() != TypeOfNet::GRAFCET) &&
                (net.type() != TypeOfNet::PetriNet) &&
                ImGui::MenuItem("Duration...", "double click", false, !running))
            {
                m_mouse.editing_arc = arc;
                m_mouse.edit_duration = std::isnan(arc->duration) ? 0.0f : arc->duration;
                m_mouse.edit_focus_requested = true;
            }
            if (ImGui::MenuItem("Select", nullptr, false, !running))
                selectArc(arc, false);
            if (ImGui::MenuItem("Delete", "Del", false, !running))
            {
                m_editor.removeArc(*arc);
                clearAllSelections();
            }
        }
        ImGui::EndPopup();
    }
}

//------------------------------------------------------------------------------
void PetriView::drawPetriNet(Net& net, Simulation& simulation, bool interactive, bool show_hover)
{
    m_current_net = &net;
    m_current_simulation = &simulation;
    sanitize(net);
    float const zoom = m_canvas.zoom;

    m_canvas.push();
    drawGrid(m_canvas.draw_list, simulation.isRunning(), simulation.hasReceptivityErrors());
    drawNetElements(net, simulation, zoom);
    if (show_hover)
        drawOverlays(net, simulation, zoom);
    drawBadges(net, simulation);
    m_canvas.pop();

    if (interactive || show_hover)
    {
        drawEditWidgets(net);
        drawContextMenus(net);
    }
}

// ============================================================================
// Tooltips
// ============================================================================

//------------------------------------------------------------------------------
void PetriView::showPlaceTooltip(Place const& place, TypeOfNet type)
{
    ImGui::BeginTooltip();
    if (type == TypeOfNet::GRAFCET)
    {
        ImGui::Text("Step %zu", place.id);
        if (!place.caption.empty())
        {
            ImGui::SameLine();
            ImGui::TextDisabled("%s", place.caption.c_str());
        }
        if (place.tokens > 0)
            ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.4f, 1.0f), "active");
        for (auto const& action : place.actions)
        {
            ImGui::BulletText("%s  %s", qualifierToStr(action.qualifier), action.name.c_str());
            if (!action.condition.empty())
            {
                ImGui::SameLine();
                ImGui::TextDisabled("if %s", action.condition.c_str());
            }
        }
    }
    else
    {
        ImGui::Text("%s", place.key.c_str());
        if (!place.caption.empty())
        {
            ImGui::SameLine();
            ImGui::TextDisabled("%s", place.caption.c_str());
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
        ImGui::TextDisabled("%s", trans.caption.c_str());
    }
    if (type == TypeOfNet::GRAFCET)
    {
        ImGui::Text("Receptivity: %s", trans.receptivity ? "true" : "false");
        std::string const error = Receptivity::check(trans.caption);
        if (!error.empty())
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", error.c_str());
        if (trans.delay > 0.0f)
            ImGui::Text("Delay: %.2f s", double(trans.delay));
    }
    else
    {
        ImGui::Text("%s", trans.canFire() ? "Can fire" : (trans.isEnabled() ? "Enabled" : "Not enabled"));
    }
    ImGui::EndTooltip();
}

} // namespace tpne
