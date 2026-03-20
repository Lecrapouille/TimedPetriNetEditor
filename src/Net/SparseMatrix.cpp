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

#include "TimedPetriNetEditor/SparseMatrix.hpp"
#include "TimedPetriNetEditor/TropicalAlgebra.hpp"

#include <iostream>
#include <sstream>
#include <algorithm>

namespace tpne {

// === CONSTRUCTORS / DESTRUCTOR ===========================================

template<typename T>
SparseMatrix<T>::SparseMatrix(size_t n)
    : m_rows_count(n), m_cols_count(n), m_rows(n + 1, 0)
{
}

template<typename T>
SparseMatrix<T>::SparseMatrix(size_t rows, size_t cols)
    : m_rows_count(rows), m_cols_count(cols), m_rows(rows + 1, 0)
{
}

template<typename T>
SparseMatrix<T>::SparseMatrix(const SparseMatrix<T>& other)
    : m_rows_count(other.m_rows_count)
    , m_cols_count(other.m_cols_count)
    , m_vals(other.m_vals)
    , m_cols(other.m_cols)
    , m_rows(other.m_rows)
{
}

template<typename T>
SparseMatrix<T>::SparseMatrix(SparseMatrix<T>&& other) noexcept
    : m_rows_count(other.m_rows_count)
    , m_cols_count(other.m_cols_count)
    , m_vals(std::move(other.m_vals))
    , m_cols(std::move(other.m_cols))
    , m_rows(std::move(other.m_rows))
{
    other.m_rows_count = 0;
    other.m_cols_count = 0;
}

template<typename T>
SparseMatrix<T>& SparseMatrix<T>::operator=(const SparseMatrix<T>& other)
{
    if (this != &other)
    {
        m_rows_count = other.m_rows_count;
        m_cols_count = other.m_cols_count;
        m_vals = other.m_vals;
        m_cols = other.m_cols;
        m_rows = other.m_rows;
    }
    return *this;
}

template<typename T>
SparseMatrix<T>& SparseMatrix<T>::operator=(SparseMatrix<T>&& other) noexcept
{
    if (this != &other)
    {
        m_rows_count = other.m_rows_count;
        m_cols_count = other.m_cols_count;
        m_vals = std::move(other.m_vals);
        m_cols = std::move(other.m_cols);
        m_rows = std::move(other.m_rows);
        other.m_rows_count = 0;
        other.m_cols_count = 0;
    }
    return *this;
}

// === DIMENSIONS ======================================================

template<typename T>
void SparseMatrix<T>::reshape(size_t rows, size_t cols)
{
    m_rows_count = rows;
    m_cols_count = cols;
    m_vals.clear();
    m_cols.clear();
    m_rows.assign(rows + 1, 0);
}

template<typename T>
void SparseMatrix<T>::clear()
{
    m_vals.clear();
    m_cols.clear();
    m_rows.assign(m_rows_count + 1, 0);
}

// === ELEMENT ACCESS ==================================================

template<typename T>
T SparseMatrix<T>::get(size_t row, size_t col) const
{
    validateCoordinates(row, col);

    const size_t row_start = m_rows[row];
    const size_t row_end = m_rows[row + 1];

    for (size_t pos = row_start; pos < row_end; ++pos)
    {
        const size_t curr_col = m_cols[pos];
        
        if (curr_col == col)
        {
            return m_vals[pos];
        }
        else if (curr_col > col)
        {
            break;
        }
    }

    return zero<T>();
}

template<typename T>
SparseMatrix<T>& SparseMatrix<T>::set(size_t row, size_t col, T val)
{
    validateCoordinates(row, col);

    const size_t row_start = m_rows[row];
    const size_t row_end = m_rows[row + 1];
    size_t pos = row_start;
    size_t curr_col = m_cols_count;

    for (; pos < row_end; ++pos)
    {
        curr_col = m_cols[pos];
        if (curr_col >= col)
        {
            break;
        }
    }

    const bool element_exists = (pos < row_end) && (curr_col == col);
    if (!element_exists)
    {
        if (!(val == zero<T>()))
        {
            insert(pos, row, col, val);
        }
    }
    else if (val == zero<T>())
    {
        remove(pos, row);
    }
    else
    {
        m_vals[pos] = val;
    }

    return *this;
}

// === OPERATIONS ======================================================

template<typename T>
std::vector<T> SparseMatrix<T>::multiply(const std::vector<T>& vec) const
{
    if (m_cols_count != vec.size())
    {
        throw std::invalid_argument(
            "Cannot multiply: Matrix column count and vector size don't match.");
    }

    std::vector<T> result(m_rows_count, zero<T>());

    for (size_t i = 0; i < m_rows_count; ++i)
    {
        T sum = zero<T>();
        const size_t row_start = m_rows[i];
        const size_t row_end = m_rows[i + 1];

        for (size_t pos = row_start; pos < row_end; ++pos)
        {
            sum = sum + m_vals[pos] * vec[m_cols[pos]];
        }

        result[i] = sum;
    }

    return result;
}

template<typename T>
std::vector<T> SparseMatrix<T>::operator*(const std::vector<T>& vec) const
{
    return multiply(vec);
}

template<typename T>
SparseMatrix<T> SparseMatrix<T>::multiply(const SparseMatrix<T>& other) const
{
    if (m_cols_count != other.m_rows_count)
    {
        throw std::invalid_argument(
            "Cannot multiply: Left matrix column count and right matrix row count don't match.");
    }

    SparseMatrix<T> result(m_rows_count, other.m_cols_count);

    for (size_t i = 0; i < m_rows_count; ++i)
    {
        for (size_t j = 0; j < other.m_cols_count; ++j)
        {
            T sum = zero<T>();

            for (size_t k = 0; k < m_cols_count; ++k)
            {
                sum = sum + get(i, k) * other.get(k, j);
            }

            if (!(sum == zero<T>()))
            {
                result.set(i, j, sum);
            }
        }
    }

    return result;
}

template<typename T>
SparseMatrix<T> SparseMatrix<T>::operator*(const SparseMatrix<T>& other) const
{
    return multiply(other);
}

template<typename T>
SparseMatrix<T> SparseMatrix<T>::add(const SparseMatrix<T>& other) const
{
    if (m_rows_count != other.m_rows_count || m_cols_count != other.m_cols_count)
    {
        throw std::invalid_argument(
            "Cannot add: matrices dimensions don't match.");
    }

    SparseMatrix<T> result(m_rows_count, m_cols_count);

    for (size_t i = 0; i < m_rows_count; ++i)
    {
        for (size_t j = 0; j < m_cols_count; ++j)
        {
            T sum = get(i, j) + other.get(i, j);
            if (!(sum == zero<T>()))
            {
                result.set(i, j, sum);
            }
        }
    }

    return result;
}

template<typename T>
SparseMatrix<T> SparseMatrix<T>::operator+(const SparseMatrix<T>& other) const
{
    return add(other);
}

template<typename T>
SparseMatrix<T> SparseMatrix<T>::subtract(const SparseMatrix<T>& other) const
{
    if (m_rows_count != other.m_rows_count || m_cols_count != other.m_cols_count)
    {
        throw std::invalid_argument(
            "Cannot subtract: matrices dimensions don't match.");
    }

    SparseMatrix<T> result(m_rows_count, m_cols_count);

    for (size_t i = 0; i < m_rows_count; ++i)
    {
        for (size_t j = 0; j < m_cols_count; ++j)
        {
            T diff = get(i, j) - other.get(i, j);
            if (!(diff == zero<T>()))
            {
                result.set(i, j, diff);
            }
        }
    }

    return result;
}

template<typename T>
SparseMatrix<T> SparseMatrix<T>::operator-(const SparseMatrix<T>& other) const
{
    return subtract(other);
}

// === INTERNAL HELPERS ================================================

template<typename T>
void SparseMatrix<T>::validateCoordinates(size_t row, size_t col) const
{
    if (row >= m_rows_count || col >= m_cols_count)
    {
        throw std::out_of_range("Matrix coordinates out of range.");
    }
}

template<typename T>
void SparseMatrix<T>::insert(size_t index, size_t row, size_t col, T val)
{
    m_vals.insert(m_vals.begin() + index, val);
    m_cols.insert(m_cols.begin() + index, col);

    for (size_t i = row + 1; i <= m_rows_count; ++i)
    {
        ++m_rows[i];
    }
}

template<typename T>
void SparseMatrix<T>::remove(size_t index, size_t row)
{
    m_vals.erase(m_vals.begin() + index);
    m_cols.erase(m_cols.begin() + index);

    for (size_t i = row + 1; i <= m_rows_count; ++i)
    {
        --m_rows[i];
    }
}

// === COMPARISON OPERATORS ============================================

template<typename T>
bool operator==(const SparseMatrix<T>& a, const SparseMatrix<T>& b)
{
    return (a.m_rows_count == b.m_rows_count) &&
           (a.m_cols_count == b.m_cols_count) &&
           (a.m_vals == b.m_vals) &&
           (a.m_cols == b.m_cols) &&
           (a.m_rows == b.m_rows);
}

template<typename T>
bool operator!=(const SparseMatrix<T>& a, const SparseMatrix<T>& b)
{
    return !(a == b);
}

// === OUTPUT ==========================================================

template<typename T>
std::string SparseMatrix<T>::toString(const DisplayOptions& options) const
{
    std::ostringstream os;
    std::string separator;
    const bool julia_indexing = (options.indexing == IndexingStyle::JuliaStyle);
    const bool as_dense = (options.format == DisplayFormat::Dense);

    if (as_dense)
    {
        if (!julia_indexing)
        {
            os << m_rows_count << "x" << m_cols_count << " dense matrix:" << std::endl;
        }

        if (m_cols_count > 0)
        {
            for (size_t i = 0; i < m_rows_count; ++i)
            {
                for (size_t j = 0; j < m_cols_count; ++j)
                {
                    T val = get(i, j);
                    if (!(val == zero<T>()))
                    {
                        os << val << " ";
                    }
                    else
                    {
                        os << ". ";
                    }
                }
                os << std::endl;
            }
        }
    }
    else
    {
        if (!julia_indexing)
        {
            os << m_rows_count << "x" << m_cols_count << " sparse matrix with "
               << m_vals.size() << " stored entries:" << std::endl;
        }

        os << "[";
        for (size_t i = 0; i < m_rows_count; ++i)
        {
            const size_t row_start = m_rows[i];
            const size_t row_end = m_rows[i + 1];
            
            for (size_t pos = row_start; pos < row_end; ++pos)
            {
                os << separator << (julia_indexing ? (i + 1) : i);
                separator = ", ";
            }
        }

        os << "], [";
        separator.clear();
        for (size_t col : m_cols)
        {
            os << separator << (julia_indexing ? (col + 1) : col);
            separator = ", ";
        }

        os << "], ";
        
        std::string type_prefix = getTypePrefix<T>();
        if (!type_prefix.empty() && julia_indexing)
        {
            os << type_prefix << "(";
        }
        
        os << "[";
        separator.clear();
        for (const T& val : m_vals)
        {
            os << separator << val;
            separator = ", ";
        }
        os << "]";
        
        if (!type_prefix.empty() && julia_indexing)
        {
            os << ")";
        }

        if (julia_indexing)
        {
            os << ", " << m_rows_count << ", " << m_cols_count;
        }
    }

    return os.str();
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const SparseMatrix<T>& matrix)
{
    DisplayOptions options;
    options.indexing = IndexingStyle::CppStyle;
    options.format = DisplayFormat::Sparse;
    os << matrix.toString(options);
    return os;
}

// === HELPER FUNCTIONS FOR OUTPUT =====================================

template<typename T>
std::ostream& printSparseMatrix(std::ostream& os, const SparseMatrix<T>& matrix,
                                IndexingStyle indexing, DisplayFormat format)
{
    DisplayOptions options;
    options.indexing = indexing;
    options.format = format;
    os << matrix.toString(options);
    return os;
}

// === EXPLICIT TEMPLATE INSTANTIATIONS ================================

template class SparseMatrix<double>;
template class SparseMatrix<MaxPlus>;
template class SparseMatrix<MinPlus>;

template bool operator==<double>(const SparseMatrix<double>&, const SparseMatrix<double>&);
template bool operator!=<double>(const SparseMatrix<double>&, const SparseMatrix<double>&);
template std::ostream& operator<<<double>(std::ostream&, const SparseMatrix<double>&);

template bool operator==<MaxPlus>(const SparseMatrix<MaxPlus>&, const SparseMatrix<MaxPlus>&);
template bool operator!=<MaxPlus>(const SparseMatrix<MaxPlus>&, const SparseMatrix<MaxPlus>&);
template std::ostream& operator<<<MaxPlus>(std::ostream&, const SparseMatrix<MaxPlus>&);

template bool operator==<MinPlus>(const SparseMatrix<MinPlus>&, const SparseMatrix<MinPlus>&);
template bool operator!=<MinPlus>(const SparseMatrix<MinPlus>&, const SparseMatrix<MinPlus>&);
template std::ostream& operator<<<MinPlus>(std::ostream&, const SparseMatrix<MinPlus>&);

template std::ostream& printSparseMatrix<double>(std::ostream&, const SparseMatrix<double>&, IndexingStyle, DisplayFormat);
template std::ostream& printSparseMatrix<MaxPlus>(std::ostream&, const SparseMatrix<MaxPlus>&, IndexingStyle, DisplayFormat);
template std::ostream& printSparseMatrix<MinPlus>(std::ostream&, const SparseMatrix<MinPlus>&, IndexingStyle, DisplayFormat);

}  // namespace tpne
