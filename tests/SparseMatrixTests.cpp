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
#  include "TimedPetriNetEditor/SparseMatrix.hpp"
#undef protected
#undef private

using namespace ::tpne;

//------------------------------------------------------------------------------
TEST(TestSparseMatrix, TestConstructor)
{
    SparseMatrix<double> M;

    ASSERT_EQ(M.m_vals.size(), 0u);
    ASSERT_EQ(M.m_cols.size(), 0u);
    ASSERT_EQ(M.m_rows.size(), 1u);
    ASSERT_EQ(M.nbRows(), 0u);
    ASSERT_EQ(M.nbColumns(), 0u);

    SparseMatrix<double> M2(4u, 5u);
    ASSERT_EQ(M2.m_vals.size(), 0u);
    ASSERT_EQ(M2.m_cols.size(), 0u);
    ASSERT_EQ(M2.m_rows.size(), 5u);
    ASSERT_EQ(M2.nbRows(), 4u);
    ASSERT_EQ(M2.nbColumns(), 5u);

    SparseMatrix<double> M3(3u);
    ASSERT_EQ(M3.nbRows(), 3u);
    ASSERT_EQ(M3.nbColumns(), 3u);
}

//------------------------------------------------------------------------------
TEST(TestSparseMatrix, TestReshapeAndClear)
{
    SparseMatrix<double> M;
    M.reshape(4u, 5u);

    ASSERT_EQ(M.m_vals.size(), 0u);
    ASSERT_EQ(M.m_cols.size(), 0u);
    ASSERT_EQ(M.nbRows(), 4u);
    ASSERT_EQ(M.nbColumns(), 5u);

    M.set(0u, 0u, 42.0);
    ASSERT_EQ(M.m_vals.size(), 1u);
    ASSERT_EQ(M.m_vals[0], 42.0);

    M.clear();
    ASSERT_EQ(M.m_vals.size(), 0u);
    ASSERT_EQ(M.m_cols.size(), 0u);
    ASSERT_EQ(M.nbRows(), 4u);
    ASSERT_EQ(M.nbColumns(), 5u);
}

//------------------------------------------------------------------------------
TEST(TestSparseMatrix, TestGetSet)
{
    SparseMatrix<double> M(5u, 4u);

    M.set(0u, 0u, 42.0);
    ASSERT_EQ(M.m_vals.size(), 1u);
    ASSERT_EQ(M.m_cols.size(), 1u);
    ASSERT_EQ(M.m_vals[0], 42.0);
    ASSERT_EQ(M.m_cols[0], 0u);
    ASSERT_EQ(M.get(0u, 0u), 42.0);
    ASSERT_EQ(M.get(1u, 1u), 0.0);

    M.set(4u, 3u, 43.0);
    ASSERT_EQ(M.m_vals.size(), 2u);
    ASSERT_EQ(M.m_cols.size(), 2u);
    ASSERT_EQ(M.m_vals[1], 43.0);
    ASSERT_EQ(M.m_cols[1], 3u);
    ASSERT_EQ(M.get(4u, 3u), 43.0);

    M.set(0u, 0u, 0.0);
    ASSERT_EQ(M.m_vals.size(), 1u);
    ASSERT_EQ(M.get(0u, 0u), 0.0);

    M.set(4u, 3u, 44.0);
    ASSERT_EQ(M.m_vals.size(), 1u);
    ASSERT_EQ(M.m_vals[0], 44.0);
    ASSERT_EQ(M.get(4u, 3u), 44.0);
}

//------------------------------------------------------------------------------
TEST(TestSparseMatrix, TestMaxPlusConstructor)
{
    SparseMatrix<MaxPlus> M;

    ASSERT_EQ(M.m_vals.size(), 0u);
    ASSERT_EQ(M.m_cols.size(), 0u);
    ASSERT_EQ(M.nbRows(), 0u);
    ASSERT_EQ(M.nbColumns(), 0u);

    M.reshape(4u, 5u);
    ASSERT_EQ(M.m_vals.size(), 0u);
    ASSERT_EQ(M.nbRows(), 4u);
    ASSERT_EQ(M.nbColumns(), 5u);

    M.set(0u, 0u, MaxPlus(42.0));
    ASSERT_EQ(M.m_vals.size(), 1u);
    ASSERT_EQ(M.m_vals[0].val, 42.0);
    ASSERT_EQ(M.m_cols[0], 0u);

    M.set(3u, 4u, MaxPlus(43.0));
    ASSERT_EQ(M.m_vals.size(), 2u);
    ASSERT_EQ(M.m_vals[1].val, 43.0);
    ASSERT_EQ(M.m_cols[1], 4u);

    M.clear();
    ASSERT_EQ(M.m_vals.size(), 0u);
    ASSERT_EQ(M.nbRows(), 4u);
    ASSERT_EQ(M.nbColumns(), 5u);
}

//------------------------------------------------------------------------------
TEST(TestSparseMatrix, TestMaxPlusOperations)
{
    SparseMatrix<MaxPlus> A(2u, 2u);
    SparseMatrix<MaxPlus> B(2u, 2u);

    A.set(0u, 0u, MaxPlus(1.0));
    A.set(0u, 1u, MaxPlus(2.0));
    A.set(1u, 0u, MaxPlus(3.0));
    A.set(1u, 1u, MaxPlus(4.0));

    B.set(0u, 0u, MaxPlus(0.5));
    B.set(0u, 1u, MaxPlus(1.5));
    B.set(1u, 0u, MaxPlus(2.5));
    B.set(1u, 1u, MaxPlus(3.5));

    SparseMatrix<MaxPlus> C = A + B;
    ASSERT_EQ(C.get(0u, 0u).val, 1.0);
    ASSERT_EQ(C.get(0u, 1u).val, 2.0);
    ASSERT_EQ(C.get(1u, 0u).val, 3.0);
    ASSERT_EQ(C.get(1u, 1u).val, 4.0);

    SparseMatrix<MaxPlus> D = A * B;
    ASSERT_EQ(D.get(0u, 0u).val, 4.5);
    ASSERT_EQ(D.get(0u, 1u).val, 5.5);
    ASSERT_EQ(D.get(1u, 0u).val, 6.5);
    ASSERT_EQ(D.get(1u, 1u).val, 7.5);
}

//------------------------------------------------------------------------------
TEST(TestSparseMatrix, TestMatrixVectorMultiplication)
{
    SparseMatrix<double> M(3u, 3u);
    M.set(0u, 0u, 1.0);
    M.set(0u, 2u, 2.0);
    M.set(1u, 1u, 3.0);
    M.set(2u, 0u, 4.0);
    M.set(2u, 2u, 5.0);

    std::vector<double> v = {1.0, 2.0, 3.0};
    std::vector<double> result = M * v;

    ASSERT_EQ(result.size(), 3u);
    ASSERT_EQ(result[0], 7.0);
    ASSERT_EQ(result[1], 6.0);
    ASSERT_EQ(result[2], 19.0);
}

//------------------------------------------------------------------------------
TEST(TestSparseMatrix, TestMaxPlusMatrixVectorMultiplication)
{
    SparseMatrix<MaxPlus> M(3u, 3u);
    M.set(0u, 0u, MaxPlus(1.0));
    M.set(0u, 2u, MaxPlus(2.0));
    M.set(1u, 1u, MaxPlus(3.0));
    M.set(2u, 0u, MaxPlus(4.0));
    M.set(2u, 2u, MaxPlus(5.0));

    std::vector<MaxPlus> v = {MaxPlus(1.0), MaxPlus(2.0), MaxPlus(3.0)};
    std::vector<MaxPlus> result = M * v;

    ASSERT_EQ(result.size(), 3u);
    ASSERT_EQ(result[0].val, 5.0);
    ASSERT_EQ(result[1].val, 5.0);
    ASSERT_EQ(result[2].val, 8.0);
}

//------------------------------------------------------------------------------
TEST(TestSparseMatrix, TestDisplayOptions)
{
    SparseMatrix<MaxPlus> M(2u, 2u);
    M.set(0u, 0u, MaxPlus(1.0));
    M.set(1u, 1u, MaxPlus(2.0));

    DisplayOptions cpp_opts;
    cpp_opts.indexing = IndexingStyle::CppStyle;
    cpp_opts.format = DisplayFormat::Sparse;
    std::string cpp_str = M.toString(cpp_opts);
    ASSERT_TRUE(cpp_str.find("[0, 1]") != std::string::npos);

    DisplayOptions julia_opts;
    julia_opts.indexing = IndexingStyle::JuliaStyle;
    julia_opts.format = DisplayFormat::Sparse;
    std::string julia_str = M.toString(julia_opts);
    ASSERT_TRUE(julia_str.find("[1, 2]") != std::string::npos);
    ASSERT_TRUE(julia_str.find(", 2, 2") != std::string::npos);
}
