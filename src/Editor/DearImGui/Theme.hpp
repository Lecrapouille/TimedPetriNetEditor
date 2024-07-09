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

#ifndef DEAR_IMGUI_THEME_HPP
#  define DEAR_IMGUI_THEME_HPP

enum ThemeId { Dark = 0, Light, Calssic };
ThemeId& theme();

static const float ARROW_WIDTH = 14.0f;
static const float ARROW_SPACING = 5.0f;
static const float TRANS_WIDTH = 36.0f;  // Rectangle width for rendering Transitions
static const float TRANS_HEIGHT = TRANS_WIDTH / 3.0f;  // Rectangle height for rendering Transitions
static const float PLACE_RADIUS = TRANS_WIDTH / 2.0f; // Circle radius for rendering Places
static const float TRANS_WIDTH2 = 45.0f; // For GRAFCET initial steps (double square)
static const float TOKEN_RADIUS = 2.0f;  // Circle radius for rendering tokens

// Light theme
#  define LIGHT_THEME_FILL_COLOR(alpha)      IM_COL32(255, 165, 0, alpha)
#  define LIGHT_THEME_OUTLINE_COLOR          IM_COL32(165, 42, 42, 255)
#  define LIGHT_THEME_CAPTION_COLOR          IM_COL32(0, 0, 0, 255)
#  define LIGHT_THEME_DURATION_COLOR         IM_COL32(0, 0, 0, 255)
#  define LIGHT_THEME_TOKEN_COLOR            IM_COL32(0, 0, 0, 255)
#  define LIGHT_THEME_CRITICAL_COLOR         IM_COL32(255, 0, 0, 255)
#  define LIGHT_THEME_TRANS_FIREABLE_COLOR   IM_COL32(0, 255, 0, 255)
#  define LIGHT_THEME_TRANS_VALIDATED_COLOR  IM_COL32(0, 255, 0, 255)
#  define LIGHT_THEME_TRANS_ENABLED_COLOR    IM_COL32(205, 205, 60, 255)
#  define LIGHT_THEME_PETRI_VIEW_COLOR       IM_COL32(255, 255, 255, 255)

// Dark theme
#  define DARK_THEME_FILL_COLOR(alpha)       ImGui::GetColorU32(ImGuiCol_FrameBg, alpha)
#  define DARK_THEME_OUTLINE_COLOR           ImGui::GetColorU32(ImGuiCol_FrameBgActive)
#  define DARK_THEME_CAPTION_COLOR           ImGui::GetColorU32(ImGuiCol_Text)
#  define DARK_THEME_DURATION_COLOR          ImGui::GetColorU32(ImGuiCol_FrameBgActive)
#  define DARK_THEME_TOKEN_COLOR             ImGui::GetColorU32(ImGuiCol_Text)
#  define DARK_THEME_CRITICAL_COLOR          ImGui::GetColorU32(ImGuiCol_PlotLinesHovered)
#  define DARK_THEME_TRANS_FIREABLE_COLOR    IM_COL32(0, 255, 0, 255)
#  define DARK_THEME_TRANS_VALIDATED_COLOR   IM_COL32(0, 255, 0, 255)
#  define DARK_THEME_TRANS_ENABLED_COLOR     IM_COL32(205, 205, 60, 255)
#  define DARK_THEME_PETRI_VIEW_COLOR        IM_COL32(50, 50, 50, 255)

#endif