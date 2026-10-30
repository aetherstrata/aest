/**
 * @copyright Copyright (c) 2026 Davide Pierotti, MIT License (see LICENSE.md)
 * @author Davide Pierotti
 * @file blas.test.cpp
 * @brief Unit tests for `aest.blas`.
 */

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cstddef>
#include <cstdint>

import aest.blas;

using aest::blas::Matrix;
using aest::blas::Vector;
using Catch::Matchers::WithinAbs;

TEST_CASE("multiply takes a matrix times a column vector", "[blas]")
{
    using aest::blas::multiply;

    constexpr Matrix<double, 3, 3> identity = {{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
    constexpr Vector<double, 3> vector      = {1, 2, 3};
    STATIC_REQUIRE(multiply(identity, vector) == vector);
    STATIC_REQUIRE(multiply(identity, {4, 5, 6}) == Vector<double, 3>{4, 5, 6});

    constexpr Matrix<std::int32_t, 2, 3> wide = {{{1, 2, 3}, {4, 5, 6}}};
    constexpr Vector<std::int32_t, 3> ones    = {1, 1, 1};
    STATIC_REQUIRE(multiply(wide, ones) == Vector<std::int32_t, 2>{6, 15});
}

TEST_CASE("determinant of a 3x3 matrix", "[blas]")
{
    using aest::blas::determinant;

    STATIC_REQUIRE(determinant(Matrix<std::int32_t, 3, 3>{{{2, 0, 0}, {0, 3, 0}, {0, 0, 4}}})
                   == 24);
    STATIC_REQUIRE(determinant(Matrix<std::int32_t, 3, 3>{{{1, 2, 3}, {4, 5, 6}, {7, 8, 10}}})
                   == -3);
    STATIC_REQUIRE(determinant(Matrix<std::int32_t, 3, 3>{{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}}}) == 0);
}

TEST_CASE("inverse of a 3x3 matrix", "[blas]")
{
    using aest::blas::inverse;
    using aest::blas::multiply;

    constexpr Matrix<double, 3, 3> diagonal = {{{2, 0, 0}, {0, 4, 0}, {0, 0, 8}}};
    STATIC_REQUIRE(inverse(diagonal)
                   == Matrix<double, 3, 3>{{{0.5, 0, 0}, {0, 0.25, 0}, {0, 0, 0.125}}});

    constexpr Matrix<double, 3, 3> matrix = {{{1, 2, 3}, {0, 1, 4}, {5, 6, 0}}};
    constexpr Matrix<double, 3, 3> known  = {{{-24, 18, 5}, {20, -15, -4}, {-5, 4, 1}}};
    STATIC_REQUIRE(inverse(matrix) == known);

    // Each column of the inverse maps back to a column of the identity
    constexpr Matrix<double, 3, 3> skewed = {
        {{0.4, 0.35, 0.2}, {0.2, 0.7, 0.1}, {0.02, 0.1, 0.95}}};
    constexpr Matrix<double, 3, 3> inverted = inverse(skewed);
    for (std::size_t column = 0; column < 3; ++column)
    {
        const Vector<double, 3> unit =
            multiply(skewed, Vector<double, 3>{inverted[0][column], inverted[1][column],
                                               inverted[2][column]});
        for (std::size_t row = 0; row < 3; ++row)
            CHECK_THAT(unit[row], WithinAbs(row == column ? 1. : 0., 1e-12));
    }
}
