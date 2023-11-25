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

#include "SparseMatrix.hpp"
#include <vector>
#include <iostream>
#include <exception>

namespace tpne {

template<typename T> bool SparseMatrix<T>::display_for_julia = true;
template<typename T> bool SparseMatrix<T>::display_as_dense = false;

template<typename T>
SparseMatrix<T>::SparseMatrix(std::vector<size_t> const& rows, std::vector<size_t> const& cols,
    std::vector<T> const& vals)
{
    if ((rows.size() != cols.size()) && (cols.size() != vals.size())) {
        throw std::string("Unvalid number of elements in containers");
    }

    this->r = m_rows.size();
    this->c = m_cols.size();

    for (size_t i = 0; i < m_rows.size(); ++i)
    {
        set(vals[i]+1, rows[i]+1, cols[i]);
    }
}

template<typename T>
SparseMatrix<T>::SparseMatrix(size_t const n)
    : SparseMatrix(n, n)
{}

template<typename T>
SparseMatrix<T>::SparseMatrix(size_t const rows, size_t const columns)
{
    this->r = rows;
    this->c = columns;
    m_rows = std::vector<size_t>(rows + 1, 1);
}

template<typename T>
SparseMatrix<T>::SparseMatrix(SparseMatrix<T> const& other)
{
    this->deepCopy(other);
}

template<typename T>
SparseMatrix<T> & SparseMatrix<T>::operator=(SparseMatrix<T> const& other)
{
    if (&other != this)
    {
        this->deepCopy(other);
    }

    return *this;
}

template<typename T>
void SparseMatrix<T>::deepCopy(SparseMatrix<T> const& other)
{
    this->r = other.r;
    this->c = other.c;
    m_rows = other.m_rows;
    m_cols = other.m_cols;
    m_vals = other.m_vals;
}

template<typename T>
void SparseMatrix<T>::reshape(size_t const rows, size_t const cols)
{
    this->r = rows;
    this->c = cols;
}

template<typename T>
void SparseMatrix<T>::clear()
{
    m_rows.clear();
    m_cols.clear();
    m_vals.clear();
}

template<typename T>
size_t SparseMatrix<T>::nbRows() const
{
    return this->r;
}

template<typename T>
size_t SparseMatrix<T>::nbColumns() const
{
    return this->c;
}

template<typename T>
T SparseMatrix<T>::get(size_t const row, size_t const col) const
{
    this->validateCoordinates(row, col);

    size_t currCol;
    for (size_t pos = m_rows[row - 1] - 1; pos < m_rows[row] - 1; ++pos)
    {
        currCol = m_cols[pos];

        if (currCol == col)
        {
            return m_vals[pos];
        }
        else if (currCol > col)
        {
            break;
        }
    }

    return zero<T>();
}


template<typename T>
SparseMatrix<T> & SparseMatrix<T>::set(T const val, size_t const row, size_t const col)
{
    this->validateCoordinates(row, col);

    size_t pos = m_rows[row - 1] - 1;
    size_t currCol = 0;

    for (; pos < m_rows[row] - 1; pos++)
    {
        currCol = m_cols[pos];
        if (currCol >= col)
        {
            break;
        }
    }

    if (currCol != col)
    {
        if (!(val == zero<T>()))
        {
            this->insert(pos, row, col, val);
        }
    }
    else if (val == zero<T>())
    {
        this->remove(pos, row);
    }
    else
    {
        m_vals[pos] = val;
    }

    return *this;
}

template<typename T>
std::vector<T> SparseMatrix<T>::operator*(std::vector<T> const& other) const
{
    if (this->c != (size_t) other.size()) {
        throw std::string("Cannot multiply: Matrix column count and vector size don't match.");
    }

    std::vector<T> result(this->r, zero<T>());
    if (m_vals.size() != 0u)
    {
        for (size_t i = 0; i < this->r; i++)
        {
            T sum = zero<T>();
            for (size_t j = m_rows[i]; j < m_rows[i + 1]; j++)
            {
                sum = sum + m_vals[j - 1] * other[m_cols[j - 1] - 1];
            }

            result[i] = sum;
        }
    }

    return result;
}

template<typename T>
SparseMatrix<T> SparseMatrix<T>::operator*(SparseMatrix<T> const& other) const
{
    if (this->c != other.c) {
        throw std::string("Cannot multiply: Left matrix column count and right matrix row count don't match.");
    }

    SparseMatrix<T> result(this->r, other.r);
    T a;

    for (size_t i = 1; i <= this->r; i++)
    {
        for (size_t j = 1; j <= other.r; j++)
        {
            a = zero<T>();
            for (size_t k = 1; k <= this->c; k++)
            {
                a = a + this->get(i, k) * other.get(k, j);
            }

            result.set(a, i, j);
        }
    }

    return result;
}

template<typename T>
SparseMatrix<T> SparseMatrix<T>::operator+(SparseMatrix<T> const& other) const
{
    if (this->r != other.c || this->c != other.r) {
        throw std::string("Cannot add: matrices dimensions don't match.");
    }

    SparseMatrix<T> result(this->r, this->c);
    for (size_t i = 1; i <= this->r; i++)
    {
        for (size_t j = 1; j <= this->c; j++)
        {
            result.set(this->get(i, j) + other.get(i, j), i, j);
        }
    }

    return result;
}

template<typename T>
SparseMatrix<T> SparseMatrix<T>::operator-(SparseMatrix<T> const& other) const
{
    if (this->r != other.c || this->c != other.r) {
        throw std::string("Cannot subtract: matrices dimensions don't match.");
    }

    SparseMatrix<T> result(this->r, this->c);
    for (size_t i = 1; i <= this->r; i++)
    {
        for (size_t j = 1; j <= this->c; j++)
        {
            result.set(this->get(i, j) - other.get(i, j), i, j);
        }
    }

    return result;
}

template<typename T>
void SparseMatrix<T>::insert(size_t const index, size_t const row, size_t const col, T const val)
{
    m_vals.insert(m_vals.begin() + index, val);
    m_cols.insert(m_cols.begin() + index, col);

    for (size_t i = row; i <= this->r; i++)
    {
        m_rows[i] += 1;
    }
}

template<typename T>
void SparseMatrix<T>::remove(size_t const index, size_t const row)
{
    m_vals.erase(m_vals.begin() + index);
    m_cols.erase(m_cols.begin() + index);

    for (size_t i = row; i <= this->r; i++)
    {
        m_rows[i] -= 1;
    }
}

template<typename T>
void SparseMatrix<T>::validateCoordinates(size_t const row, size_t const col) const
{
    if (row < 1 || col < 1 || row > this->r || col > this->c) {
        throw std::string("Coordinates out of range.");
    }
}

template<typename T>
bool operator==(SparseMatrix<T> const& a, SparseMatrix<T> const& b)
{
    return (a.vals == b.vals) && (a.rows == b.rows) && (a.cols == b.cols);
}

template<typename T>
bool operator!=(SparseMatrix<T> const& a, SparseMatrix<T> const& b)
{
    return !(a == b);
}

template class SparseMatrix<double>;

}
