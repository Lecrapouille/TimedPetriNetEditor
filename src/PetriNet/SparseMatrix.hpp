//=============================================================================
// TimedPetriNetEditor: A timed Petri net editor.
// Copyright 2021 -- 2026 Quentin Quadrat <lecrapouille@gmail.com>
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

#ifndef TPNE_SPARSE_MATRIX_H
#  define TPNE_SPARSE_MATRIX_H

#  include "PetriNet/ZeroOne.hpp"
#  include "PetriNet/TropicalAlgebra.hpp"

#  include <vector>
#  include <iostream>
#  include <string>
#  include <cstddef>

namespace tpne {

// *****************************************************************************
//! \brief Indexing style for matrix display
// *****************************************************************************
enum class IndexingStyle
{
    CppStyle,    //!< 0-based indexing (C++ convention)
    JuliaStyle   //!< 1-based indexing (Julia convention)
};

// *****************************************************************************
//! \brief Matrix display format
// *****************************************************************************
enum class DisplayFormat
{
    Sparse,  //!< Display as sparse format (i, j, d vectors)
    Dense    //!< Display as dense matrix
};

// *****************************************************************************
//! \brief Options for displaying sparse matrices
// *****************************************************************************
struct DisplayOptions
{
    //! \brief Indexing style (0-based or 1-based)
    IndexingStyle indexing = IndexingStyle::CppStyle;
    //! \brief Display format (sparse or dense)
    DisplayFormat format = DisplayFormat::Sparse;
};

// Helper functions for type prefixes in Julia format
template<typename T>
inline std::string getTypePrefix()
{
    return "";
}

template<>
inline std::string getTypePrefix<MaxPlus>()
{
    return "MP";
}

template<>
inline std::string getTypePrefix<MinPlus>()
{
    return "mp";
}

// *****************************************************************************
//! \brief Sparse matrix implementation using Compressed Row Storage (CRS) format.
//!
//! This class stores sparse matrices efficiently using the CRS format:
//! - Only non-zero elements are stored
//! - Uses 0-based indexing for all operations (C++ convention)
//! - Supports generic types including tropical algebras (MaxPlus, MinPlus)
//! - Compatible with Julia export (1-based indexing) via DisplayOptions
//!
//! The CRS format uses three arrays:
//! - m_vals: values of non-zero elements
//! - m_cols: column indices of non-zero elements
//! - m_rows: row pointers (m_rows[i] points to the start of row i in m_vals)
//!
//! \tparam T Element type (supports double, MaxPlus, MinPlus, etc.)
// *****************************************************************************
template<typename T>
class SparseMatrix
{
public:

    // === CONSTRUCTORS / DESTRUCTOR =======================================

    //! \brief Construct a square sparse matrix of size n×n
    //! \param n Number of rows and columns
    explicit SparseMatrix(size_t n = 0u);

    //! \brief Construct a rectangular sparse matrix
    //! \param rows Number of rows
    //! \param cols Number of columns
    SparseMatrix(size_t rows, size_t cols);

    //! \brief Copy constructor
    SparseMatrix(const SparseMatrix<T>& other);

    //! \brief Move constructor
    SparseMatrix(SparseMatrix<T>&& other) noexcept;

    //! \brief Copy assignment operator
    SparseMatrix<T>& operator=(const SparseMatrix<T>& other);

    //! \brief Move assignment operator
    SparseMatrix<T>& operator=(SparseMatrix<T>&& other) noexcept;

    //! \brief Destructor
    ~SparseMatrix() = default;

    // === DIMENSIONS ======================================================

    //! \brief Get the number of rows
    //! \return Number of rows in the matrix
    size_t nbRows() const { return m_rows_count; }

    //! \brief Get the number of columns
    //! \return Number of columns in the matrix
    size_t nbColumns() const { return m_cols_count; }

    //! \brief Resize the matrix (clears all elements)
    //! \param rows New number of rows
    //! \param cols New number of columns
    void reshape(size_t rows, size_t cols);

    //! \brief Clear all elements (keeps dimensions)
    void clear();

    // === ELEMENT ACCESS ==================================================

    //! \brief Get element at position (row, col) using 0-based indexing
    //! \param row Row index (0-based)
    //! \param col Column index (0-based)
    //! \return Element value, or zero<T>() if not stored
    T get(size_t row, size_t col) const;

    //! \brief Set element at position (row, col) using 0-based indexing
    //! \param row Row index (0-based)
    //! \param col Column index (0-based)
    //! \param val Value to set (zero<T>() values are not stored)
    //! \return Reference to this matrix for chaining
    SparseMatrix<T>& set(size_t row, size_t col, T val);

    // === OPERATIONS ======================================================

    //! \brief Matrix-vector multiplication
    //! \param vec Vector to multiply with
    //! \return Resulting vector
    std::vector<T> multiply(const std::vector<T>& vec) const;

    //! \brief Matrix-vector multiplication operator
    std::vector<T> operator*(const std::vector<T>& vec) const;

    //! \brief Matrix-matrix multiplication
    //! \param other Matrix to multiply with
    //! \return Resulting matrix
    SparseMatrix<T> multiply(const SparseMatrix<T>& other) const;

    //! \brief Matrix-matrix multiplication operator
    SparseMatrix<T> operator*(const SparseMatrix<T>& other) const;

    //! \brief Matrix addition
    //! \param other Matrix to add
    //! \return Resulting matrix
    SparseMatrix<T> add(const SparseMatrix<T>& other) const;

    //! \brief Matrix addition operator
    SparseMatrix<T> operator+(const SparseMatrix<T>& other) const;

    //! \brief Matrix subtraction
    //! \param other Matrix to subtract
    //! \return Resulting matrix
    SparseMatrix<T> subtract(const SparseMatrix<T>& other) const;

    //! \brief Matrix subtraction operator
    SparseMatrix<T> operator-(const SparseMatrix<T>& other) const;

    // === COMPARISON OPERATORS ============================================

    //! \brief Equality comparison
    template<typename X>
    friend bool operator==(const SparseMatrix<X>& a, const SparseMatrix<X>& b);

    //! \brief Inequality comparison
    template<typename X>
    friend bool operator!=(const SparseMatrix<X>& a, const SparseMatrix<X>& b);

    // === OUTPUT ==========================================================

    //! \brief Output stream operator (uses C++ indexing and sparse format)
    template<typename X>
    friend std::ostream& operator<<(std::ostream& os, const SparseMatrix<X>& matrix);

    //! \brief Convert to string representation
    //! \param options Display options (Julia indexing, dense format, etc.)
    //! \return String representation of the matrix
    std::string toString(const DisplayOptions& options = DisplayOptions{}) const;

    // === HELPER FUNCTIONS FOR OUTPUT =====================================

    //! \brief Print sparse matrix to output stream with custom options
    //! \param os Output stream
    //! \param matrix Matrix to print
    //! \param indexing Indexing style (CppStyle or JuliaStyle)
    //! \param format Display format (Sparse or Dense)
    //! \return Output stream
    template<typename X>
    friend std::ostream& printSparseMatrix(std::ostream& os, const SparseMatrix<X>& matrix,
                                           IndexingStyle indexing, DisplayFormat format);

private:

    // === INTERNAL HELPERS ================================================

    //! \brief Validate that coordinates are within bounds
    void validateCoordinates(size_t row, size_t col) const;

    //! \brief Insert a new non-zero element at the specified position
    void insert(size_t index, size_t row, size_t col, T val);

    //! \brief Remove a non-zero element at the specified position
    void remove(size_t index, size_t row);

    // === MEMBER VARIABLES ================================================

    //! \brief Number of rows
    size_t m_rows_count;
    //! \brief Number of columns
    size_t m_cols_count;
    //! \brief Values of non-zero elements
    std::vector<T> m_vals;
    //! \brief Column indices of non-zero elements
    std::vector<size_t> m_cols;
    //! \brief Row pointers (CRS format): m_rows[i] = index of first element in row i
    std::vector<size_t> m_rows;
};

}  // namespace tpne

#endif // TPNE_SPARSE_MATRIX_H
