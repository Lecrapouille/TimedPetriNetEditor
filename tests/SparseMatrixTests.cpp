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

#include "main.hpp"
#define protected public
#define private public
#  include "src/utils/Utils.hpp"
#undef protected
#undef private

//------------------------------------------------------------------------------
static std::string stream(SparseMatrix const& M, bool display_for_julia)
{
    std::stringstream ss;
    M.display_for_julia = display_for_julia;
    ss << M;
    return ss.str();
}

//------------------------------------------------------------------------------
TEST(TestEventGraph, TestSparseMatrixConstructor)
{
    SparseMatrix M;

    ASSERT_EQ(M.i.size(), 0u);
    ASSERT_EQ(M.j.size(), 0u);
    ASSERT_EQ(M.d.size(), 0u);
    ASSERT_EQ(M.N, 0u);
    ASSERT_EQ(M.M, 0u);
    ASSERT_STREQ(stream(M, true).c_str(), "[], [], MP([]), 0, 0");
    ASSERT_STREQ(stream(M, false).c_str(), "0x0 sparse (max,+) matrix with 0 stored entry:\n[], [], MP([])");

    M.dim(4u, 5u);
    ASSERT_EQ(M.i.size(), 0u);
    ASSERT_EQ(M.j.size(), 0u);
    ASSERT_EQ(M.d.size(), 0u);
    ASSERT_EQ(M.N, 4u);
    ASSERT_EQ(M.M, 5u);
    ASSERT_STREQ(stream(M, true).c_str(), "[], [], MP([]), 5, 4");
    ASSERT_STREQ(stream(M, false).c_str(), "5x4 sparse (max,+) matrix with 0 stored entry:\n[], [], MP([])");

    M.add(0u, 0u, 42.0);
    ASSERT_EQ(M.i.size(), 1u);
    ASSERT_EQ(M.j.size(), 1u);
    ASSERT_EQ(M.d.size(), 1u);
    ASSERT_EQ(M.N, 4u);
    ASSERT_EQ(M.M, 5u);
    ASSERT_EQ(M.i[0], 1u);
    ASSERT_EQ(M.j[0], 1u);
    ASSERT_EQ(M.d[0], 42.0);
    ASSERT_STREQ(stream(M, true).c_str(), "[1], [1], MP([42]), 5, 4");
    ASSERT_STREQ(stream(M, false).c_str(), "5x4 sparse (max,+) matrix with 1 stored entry:\n[0], [0], MP([42])");

    M.add(4u, 5u, 43.0);
    ASSERT_EQ(M.i.size(), 2u);
    ASSERT_EQ(M.j.size(), 2u);
    ASSERT_EQ(M.d.size(), 2u);
    ASSERT_EQ(M.N, 4u);
    ASSERT_EQ(M.M, 5u);
    ASSERT_EQ(M.i[0], 1u);
    ASSERT_EQ(M.j[0], 1u);
    ASSERT_EQ(M.d[0], 42.0);
    ASSERT_EQ(M.i[1], 5u);
    ASSERT_EQ(M.j[1], 6u);
    ASSERT_EQ(M.d[1], 43.0);
    ASSERT_STREQ(stream(M, true).c_str(), "[1, 5], [1, 6], MP([42, 43]), 5, 4");
    ASSERT_STREQ(stream(M, false).c_str(), "5x4 sparse (max,+) matrix with 2 stored entry:\n[0, 4], [0, 5], MP([42, 43])");

    // Check double insertion is possible (no security check)
    M.add(4u, 5u, 44.0);
    ASSERT_EQ(M.i.size(), 3u);
    ASSERT_EQ(M.j.size(), 3u);
    ASSERT_EQ(M.d.size(), 3u);
    ASSERT_EQ(M.N, 4u);
    ASSERT_EQ(M.M, 5u);
    ASSERT_EQ(M.i[0], 1u);
    ASSERT_EQ(M.j[0], 1u);
    ASSERT_EQ(M.d[0], 42.0);
    ASSERT_EQ(M.i[1], 5u);
    ASSERT_EQ(M.j[1], 6u);
    ASSERT_EQ(M.d[1], 43.0);
    ASSERT_EQ(M.i[2], 5u);
    ASSERT_EQ(M.j[2], 6u);
    ASSERT_EQ(M.d[2], 44.0);
    ASSERT_STREQ(stream(M, true).c_str(), "[1, 5, 5], [1, 6, 6], MP([42, 43, 44]), 5, 4");
    ASSERT_STREQ(stream(M, false).c_str(), "5x4 sparse (max,+) matrix with 3 stored entry:\n[0, 4, 4], [0, 5, 5], MP([42, 43, 44])");

    M.clear();
    ASSERT_EQ(M.i.size(), 0u);
    ASSERT_EQ(M.j.size(), 0u);
    ASSERT_EQ(M.d.size(), 0u);
    ASSERT_EQ(M.N, 4u);
    ASSERT_EQ(M.M, 5u);
    ASSERT_STREQ(stream(M, true).c_str(), "[], [], MP([]), 5, 4");
    ASSERT_STREQ(stream(M, false).c_str(), "5x4 sparse (max,+) matrix with 0 stored entry:\n[], [], MP([])");
}
