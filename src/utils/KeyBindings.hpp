//=====================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2022 Quentin Quadrat <lecrapouille@gmail.com>
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
//=====================================================================

#ifndef KEY_BINDINGS_HPP
#  define KEY_BINDINGS_HPP

#  include <string>
#  include <SFML/Window/Keyboard.hpp>

// -----------------------------------------------------------------------------
#  define KEY_BINDING_QUIT_APPLICATION sf::Keyboard::Escape
#  define KEY_BINDING_RUN_SIMULATION sf::Keyboard::Space
#  define KEY_BINDING_ROTATE_CW sf::Keyboard::PageUp
#  define KEY_BINDING_ROTATE_CCW sf::Keyboard::PageDown
#  define KEY_BINDING_SCREEN_SHOT sf::Keyboard::F1

#  define KEY_BINDING_ALIGN_NODES sf::Keyboard::A
// #  define KEY_BINDING_EXPORT_PETRI_TO_DRAWIO sf::Keyboard::B
#  define KEY_BINDING_SHOW_CRITICAL_CYCLE sf::Keyboard::C
#  define KEY_BINDING_SHOW_GRID sf::Keyboard::D
#  define KEY_BINDING_IS_EVENT_GRAPH sf::Keyboard::E
#  define KEY_BINDING_LOAD_FLOWSHOP sf::Keyboard::F
// #  define KEY_BINDING_EXPORT_PETRI_TO_GRAFCET_CXX sf::Keyboard::G
#  define KEY_BINDING_SHOW_HELP sf::Keyboard::H
// #  define KEY_BINDING_EXPORT_PETRI_TO_GRAFCET_LATEX sf::Keyboard::I
// #  define KEY_BINDING_EXPORT_PETRI_TO_JULIA sf::Keyboard::J
// #  define KEY_BINDING_EXPORT_PETRI_TO_PNEDITOR sf::Keyboard::K
#  define KEY_BINDING_ARC_FROM_NODE sf::Keyboard::L
#  define KEY_BINDING_MOVE_PETRI_NODE sf::Keyboard::M
#  define KEY_BINDING_LOAD_PETRI_NET sf::Keyboard::O
// #  define KEY_BINDING_EXPORT_PETRI_TO_GRAPHVIZ sf::Keyboard::P
#  define KEY_BINDING_RUN_SIMULATION_ALT sf::Keyboard::R
#  define KEY_BINDING_SAVE_PETRI_NET sf::Keyboard::S
// #  define KEY_BINDING_EXPORT_PETRI_TO_PETRI_LATEX sf::Keyboard::X
// #  define KEY_BINDING_EXPORT_PETRI_TO_SYMFONY sf::Keyboard::Y
#  define KEY_BINDING_ERASE_PETRI_NET sf::Keyboard::Z

// SFML bug for macOS ?
#  if defined(__APPLE__)
#    define KEY_BINDING_INCREMENT_TOKENS sf::Keyboard::RBracket
#    define KEY_BINDING_DECREMENT_TOKENS sf::Keyboard::LBracket
#    define KEY_BINDING_DELETE_PETRI_ELEMENT sf::Keyboard::Backslash
#  else
#    define KEY_BINDING_INCREMENT_TOKENS sf::Keyboard::Add
#    define KEY_BINDING_DECREMENT_TOKENS sf::Keyboard::Subtract
#    define KEY_BINDING_DELETE_PETRI_ELEMENT sf::Keyboard::Delete
#  endif

// -----------------------------------------------------------------------------
std::string const& to_str(sf::Keyboard::Key const key);

#endif
