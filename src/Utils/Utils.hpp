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

#ifndef UTILS_HPP
#  define UTILS_HPP

#  include <chrono>
#  include <string>

namespace tpne {

//------------------------------------------------------------------------------
template<typename T>
inline float norm(T const& A, T const& B)
{
    return sqrtf((B.x - A.x) * (B.x - A.x) + (B.y - A.y) * (B.y - A.y));
}

//------------------------------------------------------------------------------
template<typename T>
T rotate(T const& v, float const cos_a, float const sin_a)
{
    return T(v.x * cos_a - v.y * sin_a, v.x * sin_a + v.y * cos_a);
}

//------------------------------------------------------------------------------
int randomInt(int lower, int upper);
float randomFloat(int lower, int upper);

//------------------------------------------------------------------------------
//! \brief give the file extension
//------------------------------------------------------------------------------
std::string extension(std::string const& path);

//------------------------------------------------------------------------------
//! \brief give the file name without its extension from a given path
//------------------------------------------------------------------------------
std::string baseName(std::string const& path);

// *****************************************************************************
//! \brief
// *****************************************************************************
class Timer
{
public:
    Timer()
        : m_begin(Clock::now())
    {}

    float restart()
    {
        float res = elapsed();
        m_begin = Clock::now();
        return res;
    }

    float elapsed() const
    {
        return std::chrono::duration_cast<Second>(Clock::now() - m_begin).count();
    }

private:
    typedef std::chrono::steady_clock Clock;
    typedef std::chrono::duration<float, std::ratio<1>> Second;
    std::chrono::time_point<Clock> m_begin;
};

} // namespace tpne

#endif
