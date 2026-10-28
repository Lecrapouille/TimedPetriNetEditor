// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef TPNE_RANDOM_HPP
#  define TPNE_RANDOM_HPP

#  include <cstddef>
#  include <cstdint>
#  include <limits>
#  include <random>

namespace tpne {

//------------------------------------------------------------------------------
//! \brief An index in [0, count), each with the same chance, and the same on
//! every platform for the same seed. std::uniform_int_distribution does not
//! give that: libstdc++ (Linux) and libc++ (macOS) draw different numbers
//! from the same generator, so a simulation replayed from its seed, or a
//! golden journal, would differ. The generator (std::mt19937_64) is the same
//! everywhere.
//! \pre count > 0.
//------------------------------------------------------------------------------
inline size_t randomIndex(std::mt19937_64& random, size_t const count)
{
    // The values above the last multiple of count are drawn again: else the
    // first indices would come a bit more often.
    // No cast: size_t is uint64_t on Linux (a cast would be useless), not on
    // macOS.
    uint64_t const n = count;
    uint64_t const max = std::numeric_limits<uint64_t>::max();
    uint64_t const limit = max - (max % n);
    uint64_t value = random();
    while (value >= limit)
        value = random();
    size_t const index = value % n;
    return index;
}

} // namespace tpne

#endif
