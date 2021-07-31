//=====================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 Quentin Quadrat <lecrapouille@gmail.com>
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
//=====================================================================

#ifndef UTILS_HPP
#  define UTILS_HPP

#  include <SFML/System.hpp>
#  include <math.h>
#  include <sstream>
#  include <vector>

//------------------------------------------------------------------------------
inline float norm(const float xa, const float ya, const float xb, const float yb)
{
    return sqrtf((xb - xa) * (xb - xa) + (yb - ya) * (yb - ya));
}

//------------------------------------------------------------------------------
inline float random(int lower, int upper)
{
    srand(time(NULL));
    return (rand() % (upper - lower + 1)) + lower;
}

//------------------------------------------------------------------------------
inline const char* current_time()
{
    static char buffer[32];

    time_t current_time = ::time(nullptr);
    strftime(buffer, sizeof (buffer), "[%H:%M:%S] ", localtime(&current_time));
    return buffer;
}

//------------------------------------------------------------------------------
template<typename T> T convert_to(const std::string &str)
{
    std::istringstream ss(str); T num; ss >> num; return num;
}

template<typename T> T convert_to(const char* str)
{
    std::istringstream ss(str); T num; ss >> num; return num;
}

//------------------------------------------------------------------------------
inline uint8_t fading(sf::Clock& timer, bool restart, float blink_period)
{
    if (restart)
        timer.restart();

    float period = timer.getElapsedTime().asSeconds();
    if (period >= blink_period)
        period = blink_period;

    return uint8_t(255.0f - (255.0f * period / blink_period));
}

//------------------------------------------------------------------------------
//! \brief Helper structure for building sparse matrix for the exportation of
//! the Petri net to Julia language (Julia is a vectorial language mixing Matlab
//! and Python syntax but with a faster runtime) as Max-Plus dynamical linear
//! systems (State space representation).
//!
//! This class is only used for storing elements not for doing matrix
//! operations. In Julia, a sparse matrix of dimensions m x n is built with the
//! function sparse(I, J, D, n, m) where I, J are two column vectors indicating
//! coordinates of the non-zero elements, D is column vector holding values to
//! be stored. Note that in Julia indexes starts at 1, contrary to C/C++
//! starting at 0.
//------------------------------------------------------------------------------
struct SparseElement
{
    SparseElement(size_t i_, size_t j_, float d_)
        : i(i_ + 1u), j(j_ + 1u), d(d_)
    {}

    // (I,J) Coordinate
    size_t i, j;
    // Non zero element
    float d;
};

//------------------------------------------------------------------------------
//! \brief Julia sparse matrix.
//------------------------------------------------------------------------------
using SparseMatrix = std::vector<SparseElement>;

//------------------------------------------------------------------------------
//! \brief Julia sparse is built as sparse(I, J, D) where I, J and D are 3
//! vectors.
//------------------------------------------------------------------------------
inline std::ostream & operator<<(std::ostream &os, SparseMatrix const& matrix)
{
    std::string separator;

    os << "[";
    for (auto const& e: matrix)
    {
        os << separator << e.i;
        separator = ", ";
    }

    os << "], [";
    separator.clear();
    for (auto const& e: matrix)
    {
        os << separator << e.j;
        separator = ", ";
    }

    os << "], MP([";
    separator.clear();
    for (auto const& e: matrix)
    {
        os << separator << e.d;
        separator = ", ";
    }
    os << "])";

    return os;
}

#endif
