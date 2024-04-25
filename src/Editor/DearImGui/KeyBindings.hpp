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

#ifndef KEY_BINDINGS_HPP
#  define KEY_BINDINGS_HPP

// -----------------------------------------------------------------------------
// FIXME: The backend raylib does not support other than US keyboard meaning that
// other keyboard mapping are fucked up.
#  define KEY_QUIT_APPLICATION      ImGuiKey_Escape
#  define KEY_RUN_SIMULATION        ImGuiKey_Space
#  define KEY_RUN_SIMULATION_ALT    ImGuiKey_R
#  define KEY_ROTATE_CW             ImGuiKey_PageUp
#  define KEY_ROTATE_CCW            ImGuiKey_PageDown
#  define KEY_MOVE_PETRI_NODE       ImGuiKey_M
#  define KEY_INCREMENT_TOKENS      ImGuiKey_KeypadAdd
#  define KEY_DECREMENT_TOKENS      ImGuiKey_KeypadSubtract
#  define KEY_DELETE_NODE           ImGuiKey_Delete
#  define KEY_UNDO                  ImGuiKey_W // remapping for Z
#  define KEY_REDO                  ImGuiKey_Y

// -----------------------------------------------------------------------------
#  define MOUSE_BOUTON_DRAGGING_VIEW   ImGuiMouseButton_Middle
#  define MOUSE_BOUTON_ADD_PLACE       ImGuiMouseButton_Left
#  define MOUSE_BOUTON_ADD_TRANSITION  ImGuiMouseButton_Right
#  define MOUSE_BOUTON_HANDLE_ARC      ImGuiMouseButton_Middle

#  define MOUSE_BOUTON_MOVE_NODE       ImGuiMouseButton_Left


#endif