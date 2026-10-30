/**
 * @copyright Copyright (c) 2026 Davide Pierotti, MIT License (see LICENSE.md)
 * @author Davide Pierotti
 * @file Gamut.cppm
 * @brief Partition `aest.colors:gamut`: the matrices between RGB gamuts and
 *        CIE XYZ, derived at compile time from chromaticities.
 */
module;
#include <array>
#include <cstddef>
export module aest.colors:gamut;

import aest.blas;

namespace aest::colors
{
/** @brief A column of three values, such as XYZ or linear RGB channels. */
using Vector3 = blas::Vector<double, 3>;

/** @brief A 3x3 matrix, stored as three rows. */
using Matrix3 = blas::Matrix<double, 3, 3>;

/** @brief CIE 1931 xy chromaticity coordinates of a primary or white point. */
using Chromaticity = std::array<double, 2>;

/** @brief Chromaticity of the sRGB red primary, from IEC 61966-2-1. */
constexpr Chromaticity SRGB_RED = {0.64, 0.33};

/** @brief Chromaticity of the sRGB green primary, from IEC 61966-2-1. */
constexpr Chromaticity SRGB_GREEN = {0.30, 0.60};

/** @brief Chromaticity of the sRGB blue primary, from IEC 61966-2-1. */
constexpr Chromaticity SRGB_BLUE = {0.15, 0.06};

/** @brief Chromaticity of the D65 white point, as sRGB defines it. */
constexpr Chromaticity D65 = {0.3127, 0.3290};

/**
 * @brief The XYZ of a chromaticity at luminance Y = 1.
 * @param[in] chromaticity The xy coordinates
 * @return The XYZ coordinates, `(x / y, 1, (1 - x - y) / y)`.
 */
constexpr Vector3 xyz_of(const Chromaticity& chromaticity)
{
    const auto [x, y] = chromaticity;
    return {x / y, 1, (1 - x - y) / y};
}

/**
 * @brief Derives the matrix from linear RGB to XYZ for a set of primaries.
 *
 * Each primary contributes the XYZ of its chromaticity, scaled so that
 * equal channels of 1 give the white point at luminance Y = 1.
 *
 * @param[in] red Chromaticity of the red primary
 * @param[in] green Chromaticity of the green primary
 * @param[in] blue Chromaticity of the blue primary
 * @param[in] white Chromaticity of the white point
 * @return The matrix taking linear `(r, g, b)` to `(X, Y, Z)`.
 */
constexpr Matrix3 rgb_to_xyz(const Chromaticity& red,
                             const Chromaticity& green,
                             const Chromaticity& blue,
                             const Chromaticity& white)
{
    const std::array<Vector3, 3> primaries = {xyz_of(red), xyz_of(green), xyz_of(blue)};

    // The primaries as columns, then the weight of each that adds up to white
    Matrix3 matrix{};
    for (std::size_t row = 0; row < 3; ++row)
        for (std::size_t column = 0; column < 3; ++column)
            matrix[row][column] = primaries[column][row];
    const Vector3 scale = blas::multiply(blas::inverse(matrix), xyz_of(white));

    for (std::size_t row = 0; row < 3; ++row)
        for (std::size_t column = 0; column < 3; ++column)
            matrix[row][column] *= scale[column];
    return matrix;
}

/** @brief Matrix from linear sRGB to XYZ, D65. */
constexpr Matrix3 LIN_SRGB_TO_XYZ = rgb_to_xyz(SRGB_RED, SRGB_GREEN, SRGB_BLUE, D65);

/** @brief Matrix from XYZ, D65, to linear sRGB. */
constexpr Matrix3 XYZ_TO_LIN_SRGB = blas::inverse(LIN_SRGB_TO_XYZ);
} // namespace aest::colors
