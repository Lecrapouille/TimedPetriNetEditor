// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-TimedPetriNetEditor-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of TimedPetriNetEditor. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef TROPICAL_ALGEBRA_HPP
#  define TROPICAL_ALGEBRA_HPP

#  include "PetriNet/ZeroOne.hpp"
#  include "PetriNet/SafeFloat.hpp"

#  include <cmath>
#  include <ostream>
#  include <iostream>
#  include <limits>

namespace tpne {

// *****************************************************************************
//! \brief (max,+) algebra
// *****************************************************************************
class MaxPlus
{
public:

    MaxPlus() : val() {};
    MaxPlus(const MaxPlus & d) : val(d.val){}
    MaxPlus(const double t) : val(t){}
    inline bool operator==(const MaxPlus &rhs) const noexcept { return val == rhs.val; }
    inline bool operator==(const double &rhs) const noexcept { return val == rhs; }
    inline bool operator!=(const MaxPlus &rhs) const noexcept { return val != rhs.val; }
    inline bool operator!=(const double &rhs) const noexcept { return val != rhs; }
    inline MaxPlus & operator=(const MaxPlus & rhs) noexcept { val = rhs.val; return *this;}
    inline MaxPlus & operator=(const double rhs) noexcept { val = rhs; return *this;}
    inline double operator*=(const MaxPlus & rhs) noexcept { val += rhs.val; return val; }
    inline double operator+=(const MaxPlus & rhs) noexcept { val = std::max(val, rhs.val); return val; }
    inline double operator*(const MaxPlus & rhs) const noexcept { return val + rhs.val; }
    inline double operator+(const MaxPlus & rhs) const noexcept { return std::max(val, rhs.val); }
    inline double operator/(const MaxPlus & rhs) const noexcept { return val - rhs.val; }
    inline double operator-(const MaxPlus & rhs) const noexcept { return val - rhs.val; }
    inline double operator-() const noexcept { return -val; }
    inline double operator+() const noexcept { return val; }
    //inline operator double const& () const { return val; }
    //inline operator double& () { return val; }

    double val;
};

template<> inline MaxPlus zero<MaxPlus>() { return MaxPlus(safeNegInf()); }
template<> inline MaxPlus one<MaxPlus>()  { return 0.0; }
inline std::ostream& operator<<(std::ostream& os, MaxPlus const& m) { return os << m.val; }

// *****************************************************************************
//! \brief (min,+) algebra
// *****************************************************************************
class MinPlus
{
public:

    MinPlus() : val() {};
    MinPlus(const MinPlus & d) : val(d.val){}
    MinPlus(const double t) : val(t){}
    inline bool operator==(const MinPlus &rhs) const noexcept { return val == rhs.val; }
    inline bool operator==(const double &rhs) const noexcept { return val == rhs; }
    inline bool operator!=(const MinPlus &rhs) const noexcept { return val != rhs.val; }
    inline bool operator!=(const double &rhs) const noexcept { return val != rhs; }
    inline MinPlus & operator=(const MinPlus & rhs) noexcept { val = rhs.val; return *this;}
    inline MinPlus & operator=(const double rhs) noexcept { val = rhs; return *this;}
    inline double operator*=(const MinPlus & rhs) noexcept { val += rhs.val; return val; }
    inline double operator+=(const MinPlus & rhs) noexcept { val = std::min(val, rhs.val); return val; }
    inline double operator*(const MinPlus & rhs) const noexcept { return val + rhs.val; }
    inline double operator+(const MinPlus & rhs) const noexcept { return std::min(val, rhs.val); }
    inline double operator/(const MinPlus & rhs) const noexcept { return val - rhs.val; }
    inline double operator-(const MinPlus & rhs) const noexcept { return val - rhs.val; }
    inline double operator-() const noexcept { return -val; }
    inline double operator+() const noexcept { return val; }
    //inline operator double const& () const { return val; }
    //inline operator double& () { return val; }

    double val;
};

template<> inline MinPlus zero<MinPlus>() { return MinPlus(safePosInf()); }
template<> inline MinPlus one<MinPlus>()  { return 0.0; }
inline std::ostream& operator<<(std::ostream& os, MinPlus const& m) { return os << m.val; }

} // namespace tpne

#endif
