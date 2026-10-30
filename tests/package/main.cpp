/**
 * @copyright Copyright (c) 2026 Davide Pierotti, MIT License (see LICENSE.md)
 * @author Davide Pierotti
 * @file main.cpp
 * @brief Checks that the umbrella module re-exports every library.
 */

#include <array>
#include <string>

import aest;

int main()
{
    using aest::blas::Matrix;
    using aest::blas::Vector;
    using aest::colors::SRGB;

    constexpr Matrix<double, 2, 2> swap = {{{0, 1}, {1, 0}}};
    const bool blasOk =
        aest::blas::multiply(swap, Vector<double, 2>{1, 2}) == Vector<double, 2>{2, 1};
    const bool colorsOk = SRGB::from_hex(0x12'34'56).hex_string() == "#123456";
    return blasOk && colorsOk ? 0 : 1;
}
