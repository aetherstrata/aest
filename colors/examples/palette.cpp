/**
 * @copyright Copyright (c) 2026 Davide Pierotti, MIT License (see LICENSE.md)
 * @author Davide Pierotti
 * @file palette.cpp
 * @brief Categorical palettes for charts with `aest.colors`: random hues
 *        kept apart by color difference, and evenly turned golden-angle hues.
 */

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <random>
#include <string>
#include <vector>

import aest.colors;

namespace
{
/** @brief Colors in each palette. */
constexpr std::size_t PALETTE_SIZE = 6;

/**
 * @brief Smallest ΔE_OK between two colors of the random palette; about 0.02
 *        is just noticeable, so this keeps series easy to tell apart.
 */
constexpr float MIN_DIFFERENCE = 0.1f;

/** @brief Draws before giving up on a palette that cannot fit. */
constexpr std::int32_t MAX_DRAWS = 1000;

/** @brief The golden angle in degrees, 360 / φ². */
constexpr float GOLDEN_ANGLE = 137.507764f;

/**
 * @brief Print a palette as hex codes.
 * @param[in] name The label of the palette
 * @param[in] palette The colors to print
 */
void print_palette(const char* name, const std::vector<aest::colors::SRGB>& palette)
{
    std::printf("%-8s", name);
    for (const aest::colors::SRGB& color : palette)
        std::printf(" %s", aest::colors::hex_string(color).c_str());
    std::printf("\n");
}
} // namespace

int main()
{
    using namespace aest::colors;

    // Every color shares the lightness and chroma of the base, so no series
    // looks heavier than another
    const SRGB base = from_hex(0x3B'82'F6);

    // A fixed seed to give the same palette on every run and every platform
    std::mt19937 generator{2026u};
    std::vector<SRGB> random{base};

    for (std::int32_t draw = 0; draw < MAX_DRAWS && random.size() < PALETTE_SIZE; ++draw)
    {
        bool distinct        = true;
        const SRGB candidate = random_hue(base, generator);
        for (const SRGB& color : random)
            distinct = distinct && euclidean(candidate, color) >= MIN_DIFFERENCE;
        if (distinct)
            random.push_back(candidate);
    }
    print_palette("random", random);

    // Turning by the golden angle needs no generator: each new hue lands in
    // one of the widest gaps left, so any prefix of the palette is well spread
    std::vector<SRGB> golden;
    for (std::size_t index = 0; index < PALETTE_SIZE; ++index)
        golden.push_back(rotate_hue(base, GOLDEN_ANGLE * static_cast<float>(index)));
    print_palette("golden", golden);
}
