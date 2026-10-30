/**
 * @copyright Copyright (c) 2026 Davide Pierotti, MIT License (see LICENSE.md)
 * @author Davide Pierotti
 * @file basic.cpp
 * @brief Minimal usage example of `aest.blas`.
 */

#include <array>
#include <cstdio>

import aest.blas;

int main()
{
    using aest::blas::Matrix;
    using aest::blas::Vector;

    constexpr Matrix<double, 3, 3> scale = {{{2, 0, 0}, {0, 4, 0}, {0, 0, 8}}};
    constexpr Vector<double, 3> point    = {1, 1, 1};

    // Undo the scaling: inverse(scale) * (scale * point) == point
    constexpr auto back =
        aest::blas::multiply(aest::blas::inverse(scale), aest::blas::multiply(scale, point));
    std::printf("%g %g %g\n", back[0], back[1], back[2]);
}
