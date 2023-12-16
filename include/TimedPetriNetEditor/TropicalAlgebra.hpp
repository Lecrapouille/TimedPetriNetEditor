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

#ifndef TROPICAL_ALGEBRA_HPP
#  define TROPICAL_ALGEBRA_HPP

#  include "Net/ZeroOne.hpp"

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
    inline bool operator==(const MaxPlus &rhs) const { return val == rhs.val; }
    inline bool operator==(const double &rhs) const { return val == rhs; }
    inline MaxPlus & operator=(const MaxPlus & rhs) { val = rhs.val; return *this;}
    inline MaxPlus & operator=(const double rhs) { val = rhs; return *this;}
    inline double operator*=(const MaxPlus & rhs) { val += rhs.val; return val; }
    inline double operator+=(const MaxPlus & rhs) { val = std::max(val, rhs.val); return val; }
    inline double operator*(const MaxPlus & rhs) const { return val + rhs.val; }
    inline double operator+(const MaxPlus & rhs) const { return std::max(val, rhs.val); }
    inline double operator/(const MaxPlus & rhs) const { return val - rhs.val; }
    inline double operator-(const MaxPlus & rhs) const { return val - rhs.val; }
    inline double operator-() const { return -val; }
    inline double operator+() const { return val; }
    //inline operator double const& () const { return val; }
    //inline operator double& () { return val; }

    double val;
};

template<> inline MaxPlus zero<MaxPlus>() { return -std::numeric_limits<double>::infinity(); }
template<> inline MaxPlus one<MaxPlus>()  { return 0.0; }

inline std::ostream& operator<<(std::ostream& os, MaxPlus const& m)
{
    std::cout << m.val;
    return os;
}

// *****************************************************************************
//! \brief (min,+) algebra
// *****************************************************************************
class MinPlus
{
public:

    MinPlus() : val() {};
    MinPlus(const MinPlus & d) : val(d.val){}
    MinPlus(const double t) : val(t){}
    inline bool operator==(const MinPlus &rhs) const { return val == rhs.val; }
    inline bool operator==(const double &rhs) const { return val == rhs; }
    inline MinPlus & operator=(const MinPlus & rhs) { val = rhs.val; return *this;}
    inline MinPlus & operator=(const double rhs) { val = rhs; return *this;}
    inline double operator*=(const MinPlus & rhs) { val += rhs.val; return val; }
    inline double operator+=(const MinPlus & rhs) { val = std::min(val, rhs.val); return val; }
    inline double operator*(const MinPlus & rhs) const { return val + rhs.val; }
    inline double operator+(const MinPlus & rhs) const { return std::min(val, rhs.val); }
    inline double operator/(const MinPlus & rhs) const { return val - rhs.val; }
    inline double operator-(const MinPlus & rhs) const { return val - rhs.val; }
    inline double operator-() const { return -val; }
    inline double operator+() const { return val; }
    //inline operator double const& () const { return val; }
    //inline operator double& () { return val; }

    double val;
};

template<> inline MinPlus zero<MinPlus>() { return std::numeric_limits<double>::infinity(); }
template<> inline MinPlus one<MinPlus>()  { return 0.0; }

inline std::ostream& operator<<(std::ostream& os, MinPlus const& m)
{
    std::cout << m.val;
    return os;
}

} // namespace tpne

#endif
