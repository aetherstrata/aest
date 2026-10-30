/**
 * @copyright Copyright (c) 2026 Davide Pierotti, MIT License (see LICENSE.md)
 * @author Davide Pierotti
 * @file main.cpp
 * @brief Checks that the umbrella module re-exports every library.
 */

#include <string>

import aest;

int main()
{
    using aest::colors::SRGB;
    return SRGB::from_hex(0x12'34'56).hex_string() == "#123456" ? 0 : 1;
}
