// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef KEY_BINDINGS_HPP
#  define KEY_BINDINGS_HPP

// -----------------------------------------------------------------------------
#  define KEY_QUIT_APPLICATION      ImGuiKey_Escape
#  define KEY_SPRINGIFY_NET         ImGuiKey_L
#  define KEY_RUN_SIMULATION        ImGuiKey_Space
#  define KEY_INCREMENT_TOKENS      ImGuiKey_KeypadAdd
#  define KEY_DECREMENT_TOKENS      ImGuiKey_KeypadSubtract
#  define KEY_DELETE_NODE           ImGuiKey_Delete
#  define KEY_UNDO                  ImGuiKey_Z
#  define KEY_REDO                  ImGuiKey_Y

// -----------------------------------------------------------------------------
// The left button draws (places, arcs, selection rectangle), the right button
// moves (nodes, view). See PetriView.
#  define MOUSE_BOUTON_DRAGGING_VIEW   ImGuiMouseButton_Right
#  define MOUSE_BOUTON_ADD_PLACE       ImGuiMouseButton_Left
#  define MOUSE_BOUTON_ADD_TRANSITION  ImGuiMouseButton_Right
#  define MOUSE_BOUTON_HANDLE_ARC      ImGuiMouseButton_Left
#  define MOUSE_BOUTON_MOVE_NODE       ImGuiMouseButton_Right

#endif