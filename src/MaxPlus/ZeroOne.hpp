// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef ZERO_OR_ONE_ALGEBRA_HPP
#  define ZERO_OR_ONE_ALGEBRA_HPP

namespace tpne {

template<class T> T one() { return T(1); }
template<class T> T zero() { return T(0); }

} // namespace tpne

#endif
