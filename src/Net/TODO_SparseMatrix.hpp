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
//
// This file is a modification of the original code:
// This file is part of the SparseMatrix library
//
// @license  MIT
// @author   Petr Kessler (https://kesspess.cz)
// @link     https://github.com/uestla/Sparse-Matrix
//=============================================================================

#ifndef SPARSEMATRIX_HPP
#  define SPARSEMATRIX_HPP

#  include "Net/MaxPlus.hpp"
#  include <vector>
#  include <iostream>
#  include <ostream>

namespace tpne {

// ****************************************************************************
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
// ****************************************************************************
template<typename T>
class SparseMatrix
{
public:

    SparseMatrix(std::vector<size_t> const& rows, std::vector<size_t> const& cols,
                 std::vector<T> const& vals);
    SparseMatrix(size_t const n = 0); // square matrix n×n
    SparseMatrix(size_t const rows, size_t const columns); // general matrix
    SparseMatrix(SparseMatrix<T> const& m); // copy constructor
    SparseMatrix<T> & operator=(SparseMatrix<T> const& m);

    size_t nbRows() const;
    size_t nbColumns() const;
    void reshape(size_t const rows, size_t const cols);
    void clear();

    T get(size_t const row, size_t const col) const;
    SparseMatrix& set(T const val, size_t const row, size_t const col);

    std::vector<T> operator*(std::vector<T> const& x) const;
    SparseMatrix<T> operator*(SparseMatrix<T> const& m) const;
    SparseMatrix<T> operator+(SparseMatrix<T> const& m) const;
    SparseMatrix<T> operator-(SparseMatrix<T> const& m) const;

    template<typename X>
    friend bool operator==(SparseMatrix<X> const& a, SparseMatrix<X> const& b);

    template<typename X>
    friend bool operator!=(SparseMatrix<X> const& a, SparseMatrix<X> const& b);

    //template<typename X>
    //friend std::ostream& operator<<(std::ostream & os, SparseMatrix<X> const& matrix);

private:

    void validateCoordinates(size_t const row, size_t const col) const;
    void deepCopy(SparseMatrix<T> const& matrix);
    void insert(size_t const index, size_t const row, size_t const col, T const val);
    void remove(size_t const index, size_t const row);

public:

    //! \brief Option to display for C++ or for Julia
    static bool display_for_julia;
    //! \brief Option to force display as dense matrix
    static bool display_as_dense;

//private:

    //! \brief Matrix dimension
    size_t r, c;
    //! \brief Non zero element (double to be usable by Julia)
    std::vector<T> m_vals;
    //! \brief (I,J) Coordinates
    std::vector<size_t> m_rows, m_cols;
};

template<typename T>
std::ostream & operator<<(std::ostream& os, SparseMatrix<T> const& other)
{
    std::string separator;
    const size_t M = other.nbRows();
    const size_t N = other.nbColumns();

    if (other.display_as_dense)
    {
        if (!other.display_for_julia)
        {
            os << M << "x" << N << " (max,+) dense matrix:"
               << std::endl;
        }

        // FIXME: manage column alignement
        for (size_t i = 0u; i < M; ++i)
        {
            for (size_t j = 0u; j < N; ++j)
            {
                double d = other.get(i + 1u, j + 1u);
                if (d != zero<T>())
                    os << d << " ";
                else
                    os << ". ";
            }
            os << std::endl;
        }
    }
    else
    {
        if (!other.display_for_julia)
        {
            os << M << "x" << N << " sparse (max,+) matrix with "
               << other.m_vals.size() << " stored entry:" << std::endl;
        }

/* TODO
    for (size_t i = 0u; i < M; ++i)
    {
        size_t nz_start = other.m_rows[i];
        size_t nz_end = other.m_rows[i+1];
        for (size_t nz_id = nz_start; nz_id < nz_end; ++nz_id)
        {
            size_t j = other.m_cols[nz_id];
            double val = other.m_vals[nz_id];
            os << i << " x " << j << ": " << val << std::endl;
        }
    }

    return os;
*/

        os << "[";
        for (auto const& it: other.m_rows)
        {
            os << separator << (other.display_for_julia ? it : (it - 1));
            separator = ", ";
        }

        os << "], [";
        separator.clear();
        for (auto const& it: other.m_cols)
        {
            os << separator << (other.display_for_julia ? it : (it - 1));
            separator = ", ";
        }

        os << "], MP([";
        separator.clear();
        for (auto const& it: other.m_vals)
        {
            os << separator << it;
            separator = ", ";
        }
        os << "])";
        if (other.display_for_julia)
        {
            os << ", " << M << ", " << N;
        }
    }
    return os;
}

} // namespace tpne

#endif
