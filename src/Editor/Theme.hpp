// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef DEAR_IMGUI_THEME_HPP
#define DEAR_IMGUI_THEME_HPP

#include "imgui/imgui.h"

// *****************************************************************************
//! \brief Look of the nets: one palette per theme of the interface, and the
//! sizes of the nodes in net units (pixels at zoom 1).
// *****************************************************************************

enum class ThemeId { Dark = 0, Light, Calssic };
ThemeId& theme();

// --- Sizes (net units) -------------------------------------------------------
static const float PLACE_RADIUS = 20.0f;   //!< Petri place (circle)
static const float TRANS_WIDTH = 36.0f;    //!< Petri transition (bar), also hit size
static const float TRANS_HEIGHT = 12.0f;
static const float STEP_SIZE = 40.0f;      //!< GRAFCET step (square)
static const float TRANS_WIDTH2 = 48.0f;   //!< GRAFCET initial step (outer square)
static const float GRAFCET_BAR_WIDTH = 34.0f;  //!< GRAFCET transition (thick line)
static const float GRAFCET_BAR_HEIGHT = 5.0f;
static const float GRAFCET_GAP = 18.0f;    //!< distance of AND/OR lines to their node
static const float GRAFCET_LOOP = 40.0f;   //!< margin of the upward links on the left
static const float TOKEN_RADIUS = 3.5f;
static const float ARROW_WIDTH = 11.0f;
static const float ARROW_SPACING = 3.0f;
static const float NODE_ROUNDING = 3.0f;
static const float SHADOW_OFFSET = 2.0f;

// --- Palette -----------------------------------------------------------------
struct Palette
{
    ImU32 canvas;     //!< background of the view
    ImU32 grid;       //!< grid lines
    ImU32 grid_major; //!< every 5 lines
    ImU32 fill;       //!< inside of places, steps, transitions
    ImU32 outline;    //!< borders and arcs
    ImU32 caption;    //!< node captions
    ImU32 secondary;  //!< identifiers, comments
    ImU32 token;
    ImU32 duration;   //!< timing labels
    ImU32 label_bg;   //!< background of the labels on arcs
    ImU32 active;     //!< active GRAFCET step, running action, fireable
    ImU32 enabled;    //!< enabled transition, true receptivity
    ImU32 critical;   //!< critical circuit
    ImU32 selection;
    ImU32 hover;
    ImU32 shadow;
};

inline Palette const& palette()
{
    static const Palette dark = {
        IM_COL32(30, 31, 34, 255), IM_COL32(255, 255, 255, 12), IM_COL32(255, 255, 255, 26),
        IM_COL32(43, 45, 49, 255), IM_COL32(201, 209, 217, 255), IM_COL32(230, 237, 243, 255),
        IM_COL32(139, 148, 158, 255), IM_COL32(230, 237, 243, 255), IM_COL32(121, 192, 255, 255),
        IM_COL32(30, 31, 34, 230), IM_COL32(63, 185, 80, 255), IM_COL32(210, 153, 34, 255),
        IM_COL32(248, 81, 73, 255), IM_COL32(56, 139, 253, 255), IM_COL32(56, 139, 253, 110),
        IM_COL32(0, 0, 0, 70)
    };
    static const Palette light = {
        IM_COL32(250, 250, 250, 255), IM_COL32(0, 0, 0, 12), IM_COL32(0, 0, 0, 26),
        IM_COL32(255, 255, 255, 255), IM_COL32(55, 71, 79, 255), IM_COL32(38, 50, 56, 255),
        IM_COL32(120, 130, 140, 255), IM_COL32(38, 50, 56, 255), IM_COL32(21, 101, 192, 255),
        IM_COL32(250, 250, 250, 235), IM_COL32(46, 160, 67, 255), IM_COL32(191, 135, 0, 255),
        IM_COL32(207, 34, 46, 255), IM_COL32(9, 105, 218, 255), IM_COL32(9, 105, 218, 90),
        IM_COL32(0, 0, 0, 30)
    };
    return (theme() == ThemeId::Light) ? light : dark;
}

// Short names kept for the code written before the palette.
constexpr ImU32 COLOR_FIREABLE_TRANSITION = IM_COL32(63, 185, 80, 255);
constexpr ImU32 COLOR_VALIDATED_TRANSITION = IM_COL32(63, 185, 80, 255);
constexpr ImU32 COLOR_ENABLED_TRANSITION = IM_COL32(210, 153, 34, 255);
inline ImU32 themeFillColor(int const /*alpha*/) { return palette().fill; }
inline ImU32 themeOutlineColor() { return palette().outline; }
inline ImU32 themeCaptionColor() { return palette().caption; }
inline ImU32 themeDurationColor() { return palette().duration; }
inline ImU32 themeTokenColor() { return palette().token; }
inline ImU32 themeCriticalColor() { return palette().critical; }
inline ImU32 themePetriViewColor() { return palette().canvas; }

#endif
