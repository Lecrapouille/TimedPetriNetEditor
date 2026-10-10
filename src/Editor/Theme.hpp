// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef DEAR_IMGUI_THEME_HPP
#define DEAR_IMGUI_THEME_HPP

#include "imgui/imgui.h"

enum class ThemeId { Dark = 0, Light, Calssic };
ThemeId& theme();

static const float ARROW_WIDTH = 12.0f;
static const float ARROW_SPACING = 6.0f;

// Rectangle width for rendering Transitions
static const float TRANS_WIDTH = 40.0f;
static const float TRANS_HEIGHT = 20.0f;  // Rectangle height (ratio 2:1)
static const float PLACE_RADIUS = 20.0f;  // Circle radius for rendering Places
// For GRAFCET initial steps (double square)
static const float TRANS_WIDTH2 = 48.0f;
static const float TOKEN_RADIUS = 3.0f;   // Circle radius for rendering tokens
static const float SHADOW_OFFSET = 3.0f;  // Shadow offset for nodes
static const float NODE_ROUNDING = 2.0f;  // Corner rounding for transitions

// Transition states: same color in light and dark themes.
constexpr ImU32 COLOR_FIREABLE_TRANSITION = IM_COL32(0, 255, 0, 255);
constexpr ImU32 COLOR_VALIDATED_TRANSITION = IM_COL32(0, 255, 0, 255);
constexpr ImU32 COLOR_ENABLED_TRANSITION = IM_COL32(205, 205, 60, 255);

inline ImU32 themeFillColor(int const alpha) {
  if (ThemeId::Light == theme()) return IM_COL32(255, 165, 0, alpha);
  return ImGui::GetColorU32(ImGuiCol_FrameBg, static_cast<float>(alpha));
}

inline ImU32 themeOutlineColor() {
  if (ThemeId::Light == theme()) return IM_COL32(165, 42, 42, 255);
  return ImGui::GetColorU32(ImGuiCol_FrameBgActive);
}

inline ImU32 themeCaptionColor() {
  if (ThemeId::Light == theme()) return IM_COL32(0, 0, 0, 255);
  return ImGui::GetColorU32(ImGuiCol_Text);
}

inline ImU32 themeDurationColor() {
  if (ThemeId::Light == theme()) return IM_COL32(0, 0, 0, 255);
  return ImGui::GetColorU32(ImGuiCol_FrameBgActive);
}

inline ImU32 themeTokenColor() {
  if (ThemeId::Light == theme()) return IM_COL32(0, 0, 0, 255);
  return ImGui::GetColorU32(ImGuiCol_Text);
}

inline ImU32 themeCriticalColor() {
  if (ThemeId::Light == theme()) return IM_COL32(255, 0, 0, 255);
  return ImGui::GetColorU32(ImGuiCol_PlotLinesHovered);
}

inline ImU32 themePetriViewColor() {
  if (ThemeId::Light == theme()) return IM_COL32(255, 255, 255, 255);
  return IM_COL32(50, 50, 50, 255);
}

#endif
