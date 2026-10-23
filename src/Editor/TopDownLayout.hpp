// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef TPNE_TOP_DOWN_LAYOUT_HPP
#  define TPNE_TOP_DOWN_LAYOUT_HPP

namespace tpne {

class Net;

//------------------------------------------------------------------------------
//! \brief Place the nodes from top to bottom, as a GRAFCET is drawn: the layer
//! of a node is its distance from the initial steps (links going back up are
//! ignored), each node near its parent, nodes of a layer kept apart.
//------------------------------------------------------------------------------
void arrangeTopDown(Net& net);

} // namespace tpne

#endif
