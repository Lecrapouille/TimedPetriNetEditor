//=============================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2026 Quentin Quadrat <lecrapouille@gmail.com>
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

#ifndef SAFE_FLOAT_HPP
#  define SAFE_FLOAT_HPP

#  include <cstdint>
#  include <cstring>
#  include <limits>

// *****************************************************************************
//! \file SafeFloat.hpp
//! \brief Floating-point classification helpers that stay correct even when the
//! whole project is compiled with -ffast-math.
//!
//! The build flags (see .makefile/rules/Makefile, PERFORMANCE_FLAGS) enable
//! -ffast-math, which implies -ffinite-math-only. Under this assumption the
//! optimizer considers that NaN and +/-Inf never occur, so std::isnan() and
//! std::isinf() are constant-folded to false and direct comparisons such as
//! `x == -inf` become unreliable.
//!
//! The helpers below inspect the raw IEEE-754 bit pattern instead, which the
//! optimizer cannot reason away, so they keep working whatever the math flags.
//! Use them everywhere a special value must be detected (e.g. the (max,+) zero
//! %0 == -inf, or the "no duration yet" NaN sentinel of Place -> Transition
//! arcs).
// *****************************************************************************

namespace tpne {

//------------------------------------------------------------------------------
//! \brief Quiet NaN constant robust to -ffast-math / -Wnan-infinity-disabled (float).
inline float safeNaNF()
{
    float value;
    uint32_t const bits = 0x7FC00000u;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

//------------------------------------------------------------------------------
//! \brief NaN test robust to -ffast-math (32-bit IEEE-754).
inline bool safeIsNaN(float value)
{
    uint32_t bits;
    static_assert(sizeof(bits) == sizeof(value), "float must be 32-bit IEEE-754");
    std::memcpy(&bits, &value, sizeof(bits));
    return ((bits & 0x7F800000u) == 0x7F800000u) && ((bits & 0x007FFFFFu) != 0u);
}

//------------------------------------------------------------------------------
//! \brief NaN test robust to -ffast-math (64-bit IEEE-754).
inline bool safeIsNaN(double value)
{
    uint64_t bits;
    static_assert(sizeof(bits) == sizeof(value), "double must be 64-bit IEEE-754");
    std::memcpy(&bits, &value, sizeof(bits));
    return ((bits & 0x7FF0000000000000ULL) == 0x7FF0000000000000ULL) &&
           ((bits & 0x000FFFFFFFFFFFFFULL) != 0ULL);
}

//------------------------------------------------------------------------------
//! \brief -Inf test robust to -ffast-math (32-bit IEEE-754).
inline bool safeIsNegInf(float value)
{
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits == 0xFF800000u;
}

//------------------------------------------------------------------------------
//! \brief -Inf test robust to -ffast-math (64-bit IEEE-754).
inline bool safeIsNegInf(double value)
{
    uint64_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits == 0xFFF0000000000000ULL;
}

//------------------------------------------------------------------------------
//! \brief +Inf test robust to -ffast-math (32-bit IEEE-754).
inline bool safeIsPosInf(float value)
{
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits == 0x7F800000u;
}

//------------------------------------------------------------------------------
//! \brief +Inf test robust to -ffast-math (64-bit IEEE-754).
inline bool safeIsPosInf(double value)
{
    uint64_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits == 0x7FF0000000000000ULL;
}

//------------------------------------------------------------------------------
//! \brief +/-Inf test robust to -ffast-math.
template<typename T>
inline bool safeIsInf(T value)
{
    return safeIsNegInf(value) || safeIsPosInf(value);
}

//------------------------------------------------------------------------------
//! \brief -Inf constant robust to -ffast-math / -Wnan-infinity-disabled (float).
inline float safeNegInfF()
{
    float value;
    uint32_t const bits = 0xFF800000u;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

//------------------------------------------------------------------------------
//! \brief +Inf constant robust to -ffast-math / -Wnan-infinity-disabled (float).
inline float safePosInfF()
{
    float value;
    uint32_t const bits = 0x7F800000u;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

//------------------------------------------------------------------------------
//! \brief -Inf constant robust to -ffast-math / -Wnan-infinity-disabled (double).
inline double safeNegInf()
{
    double value;
    uint64_t const bits = 0xFFF0000000000000ULL;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

//------------------------------------------------------------------------------
//! \brief +Inf constant robust to -ffast-math / -Wnan-infinity-disabled (double).
inline double safePosInf()
{
    double value;
    uint64_t const bits = 0x7FF0000000000000ULL;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

} // namespace tpne

#endif // SAFE_FLOAT_HPP
