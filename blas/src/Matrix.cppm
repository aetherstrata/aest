/**
 * @copyright Copyright (c) 2026 Davide Pierotti, MIT License (see LICENSE.md)
 * @author Davide Pierotti
 * @file Matrix.cppm
 * @brief Partition `aest.blas:matrix`: fixed-size vectors and matrices and
 *        their compile-time operations.
 */
module;
#include <array>
#include <concepts>
#include <cstddef>
export module aest.blas:matrix;

export namespace aest::blas
{
/**
 * @brief A column vector of fixed size.
 * @tparam T The element type
 * @tparam Size The number of elements
 */
template <typename T, std::size_t Size>
using Vector = std::array<T, Size>;

/**
 * @brief A matrix of fixed size, stored as an array of rows.
 * @tparam T The element type
 * @tparam Rows The number of rows
 * @tparam Columns The number of columns
 */
template <typename T, std::size_t Rows, std::size_t Columns>
using Matrix = std::array<Vector<T, Columns>, Rows>;

/**
 * @brief Multiplies a matrix by a column vector.
 * @tparam T The element type
 * @tparam Rows The number of rows of @p matrix and elements of the result
 * @tparam Columns The number of columns of @p matrix and elements of @p vector
 * @param[in] matrix The matrix
 * @param[in] vector The column vector
 * @return The product `matrix * vector`.
 */
template <typename T, std::size_t Rows, std::size_t Columns>
constexpr Vector<T, Rows> multiply(const Matrix<T, Rows, Columns>& matrix,
                                   const Vector<T, Columns>& vector)
{
    Vector<T, Rows> product{};
    for (std::size_t row = 0; row < Rows; ++row)
        for (std::size_t column = 0; column < Columns; ++column)
            product[row] += matrix[row][column] * vector[column];
    return product;
}
} // namespace aest::blas

namespace aest::blas
{
/**
 * @brief Cofactor of one element of a 3x3 matrix, sign included.
 * @tparam T The element type
 * @param[in] matrix The matrix
 * @param[in] row Row of the element
 * @param[in] column Column of the element
 * @return The determinant of the minor without @p row and @p column, with
 *         the sign of the element's position.
 */
template <typename T>
constexpr T cofactor(const Matrix<T, 3, 3>& matrix, const std::size_t row, const std::size_t column)
{
    // Taking the other rows and columns in cyclic order folds the sign in
    const std::size_t row1    = (row + 1) % 3;
    const std::size_t row2    = (row + 2) % 3;
    const std::size_t column1 = (column + 1) % 3;
    const std::size_t column2 = (column + 2) % 3;
    return matrix[row1][column1] * matrix[row2][column2]
         - matrix[row1][column2] * matrix[row2][column1];
}
} // namespace aest::blas

export namespace aest::blas
{
/**
 * @brief Determinant of a 3x3 matrix, by cofactor expansion along the first row.
 * @tparam T The element type
 * @param[in] matrix The matrix
 * @return The determinant of @p matrix.
 */
template <typename T>
constexpr T determinant(const Matrix<T, 3, 3>& matrix)
{
    T result{};
    for (std::size_t column = 0; column < 3; ++column)
        result += matrix[0][column] * cofactor(matrix, 0, column);
    return result;
}

/**
 * @brief Inverts a 3x3 matrix through its adjugate.
 * @tparam T The element type, a floating-point type
 * @param[in] matrix The matrix to invert
 * @return The inverse of @p matrix.
 * @pre @p matrix is invertible: its determinant is not 0.
 */
template <std::floating_point T>
constexpr Matrix<T, 3, 3> inverse(const Matrix<T, 3, 3>& matrix)
{
    const T matrixDeterminant = determinant(matrix);

    Matrix<T, 3, 3> result{};
    for (std::size_t row = 0; row < 3; ++row)
        for (std::size_t column = 0; column < 3; ++column)
            result[row][column] = cofactor(matrix, column, row) / matrixDeterminant;
    return result;
}
} // namespace aest::blas
