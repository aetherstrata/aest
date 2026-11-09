/**
 * @copyright Copyright (c) 2026 Davide Pierotti, MIT License (see LICENSE.md)
 * @author Davide Pierotti
 * @file basic.cpp
 * @brief Minimal usage example of `aest.colors`.
 */

#include <cstdio>
#include <string>

import aest.colors;

int main()
{
    constexpr auto coral  = aest::colors::from_hex(0xFF'7F'50);
    const std::string hex = hex_string(coral.opacity(0.5f));
    std::puts(hex.c_str());
}
