/**
 * @copyright Copyright (c) 2026 Davide Pierotti, MIT License (see LICENSE.md)
 * @author Davide Pierotti
 * @file gradient.cpp
 * @brief Gradients with `aest.colors`: one pair of colors mixed in several
 *        spaces, the hue arcs of OKLCH, and gray stops.
 */

#include <cstdint>
#include <cstdio>
#include <string>

import aest.colors;

namespace
{
/** @brief Stops sampled along each gradient, both ends included. */
constexpr std::int32_t STOPS = 7;

/**
 * @brief Print the stops of a gradient as hex codes.
 * @tparam Mix A callable that takes an amount in [0, 1] and returns an SRGB
 * @param[in] name The label of the gradient
 * @param[in] mix The gradient to sample
 */
template <typename Mix>
void print_gradient(const char* name, const Mix& mix)
{
    std::printf("%-15s", name);
    for (std::int32_t stop = 0; stop < STOPS; ++stop)
    {
        const float amount = static_cast<float>(stop) / (STOPS - 1);
        std::printf(" %s", aest::colors::hex_string(mix(amount)).c_str());
    }
    std::printf("\n");
}
} // namespace

int main()
{
    using namespace aest::colors;

    const SRGB blue   = from_hex(0x00'00'FF);
    const SRGB yellow = from_hex(0xFF'FF'00);

    // The default is linear light, which blends like light does but runs
    // bright: the midpoint sits well above the perceived middle
    print_gradient("linear light", [&](const float t) {
        return mix(blue, yellow, t);
    });

    // Blending the encoded values, as CSS did before Color 4, passes through
    // a muddy gray
    print_gradient("sRGB", [&](const float t) {
        return mix<SRGB>(blue, yellow, t);
    });

    // OKLab spaces the lightness evenly, from dark blue to light yellow
    // through a muted teal, with neither the gray dip nor the bright bias
    print_gradient("OKLab", [&](const float t) {
        return mix<OKLab>(blue, yellow, t);
    });

    // OKLCH keeps the chroma and turns the hue instead: the shorter arc
    // goes through cyan and green, the longer one through magenta and red
    print_gradient("OKLCH shorter", [&](const float t) {
        return mix<OKLCH>(blue, yellow, t);
    });
    print_gradient("OKLCH longer", [&](const float t) {
        return mix<OKLCH>(blue, yellow, t, HueMode::longer);
    });

    // A gray has no hue and takes the other color's, so the ramp stays red
    // instead of turning around the hue circle from the gray's hue of 0
    const SRGB gray = from_hex(0x80'80'80);
    const SRGB red  = from_hex(0xE0'20'20);
    print_gradient("gray to red", [&](const float t) {
        return mix<OKLCH>(gray, red, t);
    });
}
