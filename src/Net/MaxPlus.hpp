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

#ifndef MAXPLUS_HPP
#  define MAXPLUS_HPP

#  include <cmath>
#  include <ostream>
#  include <iostream>
#  include <limits>

namespace tpne {

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

inline std::ostream& operator<<(std::ostream& os, MaxPlus const& m)
{
    std::cout << m.val;
    return os;
}

template<class T> T one()           { return T(1); }
template<class T> T zero()          { return T(0); }

} // namespace tpne

#endif
