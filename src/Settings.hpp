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

#ifndef SETTINGS_HPP
#  define SETTINGS_HPP

//------------------------------------------------------------------------------
// Config for rendering the Petri net
const float TRANS_WIDTH = 50.0f;  // Rectangle width for rendering Transitions
const float TRANS_HEIGHT = 10.0f;  // Rectangle height for rendering Transitions
const float PLACE_RADIUS = 25.0f; // Circle radius for rendering Places
const float TOKEN_RADIUS = 4.0f;  // Circle radius for rendering tokens
const float CAPTION_FONT_SIZE = 24.0f; // Text size used in node captions
const float TOKEN_FONT_SIZE = 20.0f; // Text size used for token numbers
const int STEP_ANGLE = 45; // Angle of rotation in degree for turning Transitions
const float FADING_PERIOD = 0.5f; // seconds for fading colors
#define FILL_COLOR(a) sf::Color(255, 165, 0, a) // Place with tokens or fading color
#define OUTLINE_COLOR sf::Color(165, 42, 42) // Arcs, Places, Transitions
#define CRITICAL_COLOR sf::Color(255, 0, 0)

#endif
