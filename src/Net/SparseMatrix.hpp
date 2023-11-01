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

#ifndef SPARSE_MATRIX_HPP
#  define SPARSE_MATRIX_HPP

// FIXME Add template and manage (max,+) types
//#  include <cstdint>
#  include <vector>
#  include <iostream>

namespace tpne {

template<typename T> T zero();
template<typename T> T one();

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
template<typename T>
struct SparseMatrix // FIXME: redo manage (max,+) type
{
    SparseMatrix(size_t const N_ = 0u, size_t const M_ = 0u)
        : N(N_), M(M_)
    {}

    void reshape(size_t const N_, size_t const M_)
    {
        N = N_;
        M = M_;
    }

    void clear()
    {
        i.clear();
        j.clear();
        d.clear();
    }

    //! \brief Beware double inclusions are not checked
    void set(size_t i_, size_t j_, float d_)
    {
        i.push_back(i_ + 1u);
        j.push_back(j_ + 1u);
        d.push_back(d_);
    }

    double get(size_t i_, size_t j_) const
    {
        size_t currCol;

		for (size_t pos = 0u; pos < i.size(); ++pos)
        {
            if (i[pos] == i_)
            {
                if (j[pos] == j_)
                {
                    return d[pos];
                }
            }
		}

		return zero<T>();
    }

    //! \brief Option to display for C++ or for Julia
    static bool display_for_julia;
    //! \brief Option to force display as dense matrix
    static bool display_as_dense;

    //! \brief (I,J) Coordinates
    std::vector<size_t> i, j;
    //! \brief Non zero element (double to be usable by Julia)
    std::vector<double> d;
    //! \brief Matrix dimension
    size_t N, M;
};

//------------------------------------------------------------------------------
//! \brief Output sparse matrix as I, J, D where I, J and D are 3 vectors.
//------------------------------------------------------------------------------
template<typename T>
inline std::ostream & operator<<(std::ostream &os, SparseMatrix<T> const& matrix)
{
    std::string separator;

    if (matrix.display_as_dense)
    {
        if (!matrix.display_for_julia)
        {
            os << matrix.M << "x" << matrix.N << " (max,+) dense matrix:"
               << std::endl;
        }

        // FIXME: manage column alignement
        for (size_t i = 0u; i < matrix.M; ++i)
        {
            for (size_t j = 0u; j < matrix.N; ++j)
            {
                double d = matrix.get(i + 1u, j + 1u);
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
        if (!matrix.display_for_julia)
        {
            os << matrix.M << "x" << matrix.N << " sparse (max,+) matrix with "
               << matrix.d.size() << " stored entry:" << std::endl;
        }
        os << "[";
        for (auto const& it: matrix.i)
        {
            os << separator << (matrix.display_for_julia ? it : (it - 1));
            separator = ", ";
        }

        os << "], [";
        separator.clear();
        for (auto const& it: matrix.j)
        {
            os << separator << (matrix.display_for_julia ? it : (it - 1));
            separator = ", ";
        }

        os << "], MP([";
        separator.clear();
        for (auto const& it: matrix.d)
        {
            os << separator << it;
            separator = ", ";
        }
        os << "])";
        if (matrix.display_for_julia)
        {
            os << ", " << matrix.M << ", " << matrix.N;
        }
    }
    return os;
}

} // namespace tpne

#endif
