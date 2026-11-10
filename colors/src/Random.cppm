/**
 * @copyright Copyright (c) 2026 Davide Pierotti, MIT License (see LICENSE.md)
 * @author Davide Pierotti
 * @file Random.cppm
 * @brief Partition `aest.colors:random`: random colors drawn from a
 *        caller-supplied random bit generator.
 */
module;
#include <array>
#include <concepts>
#include <cstdint>
#include <random>
export module aest.colors:random;

import :color;
import :converter;
import :gamut;
import :oklab;
import :operations;
import :space;

namespace aest::colors
{
/**
 * @brief A uniform random bit generator whose every output bit is random:
 *        its range is a power of two, of at least 2^24.
 *
 * std::mt19937, std::mt19937_64, std::ranlux24 and std::ranlux48 qualify;
 * std::minstd_rand does not, as its range is `2^31 - 2`. Its low 24 bits
 * make each random channel, so the results depend only on the generator's
 * outputs and are the same on every standard library.
 *
 * @tparam T The type to check
 */
export template <typename T>
concept RandomBitGenerator =
    std::uniform_random_bit_generator<T> && (T::max() - T::min() >= 0xFF'FF'FFu)
    && ((T::max() - T::min()) & (T::max() - T::min() + 1u)) == 0u;

/**
 * @brief Draw a float uniformly distributed in [0, 1).
 *
 * std::uniform_real_distribution is not used: its algorithm is left to the
 * standard library, so one seed would give different colors on each.
 *
 * @tparam Generator The random bit generator
 * @param[in,out] generator The generator to draw from; it advances by one output
 * @return The low 24 bits of the output, scaled to [0, 1).
 */
template <RandomBitGenerator Generator>
constexpr float unit_float(Generator& generator)
{
    // Bits a float mantissa holds, so every 24-bit value maps to a distinct float
    constexpr std::uint32_t bitMask = 0xFF'FF'FFu;
    return static_cast<float>(
               static_cast<std::uint32_t>((generator() - Generator::min()) & bitMask))
         * 0x1p-24f;
}

/**
 * @brief A random opaque color, uniform over the channels of an encoded RGB
 *        space.
 *
 * Each channel is drawn uniformly from [0, 1), red first. The draws are
 * uniform in the encoding, not to the eye: random_hue() gives colors of an
 * even lightness instead.
 *
 * @tparam Space The bounded RGB space to draw in, SRGB by default
 * @tparam Generator The random bit generator
 * @param[in,out] generator The generator to draw from; it advances by three outputs
 * @return The random color, with alpha 1.
 */
export template <BoundedRGBSpace Space = SRGB, RandomBitGenerator Generator>
constexpr Space random_color(Generator& generator)
{
    // A braced initializer evaluates its elements in order, so the channels
    // take the draws in order on every compiler
    const std::array<float, 3> channels{unit_float(generator), unit_float(generator),
                                        unit_float(generator)};
    return Space{channels[0], channels[1], channels[2]};
}

/**
 * @brief A copy of a color with a random hue, keeping the rest.
 *
 * The color is converted to @p Space, its hue is replaced by one drawn
 * uniformly from [0, 360) and it is converted back. In the default, OKLCH,
 * the perceived lightness and chroma are kept, so every result looks as
 * bright and as vivid as @p color: drawing from one base color gives a
 * palette of even weight. The result may fall outside the gamut and clip
 * when converted back. Gray colors have no hue and stay as they are.
 *
 * @tparam Space The color space the hue is drawn in
 * @tparam Color The color type of the input and the result
 * @tparam Generator The random bit generator
 * @param[in] color The color whose hue to replace
 * @param[in,out] generator The generator to draw from; it advances by one output
 * @param[in] space The working space, e.g. `in<HSL>`; OKLCH when omitted
 * @return The color with a random hue, and the alpha of @p color.
 */
export template <HueSpace Space = OKLCH, ColorType Color, RandomBitGenerator Generator>
    requires ConvertibleTo<Space, Color> && ConvertibleTo<Color, Space>
constexpr Color random_hue(const Color& color,
                           Generator& generator,
                           [[maybe_unused]] In<Space> space = {})
{
    // Degrees in a full turn of the hue circle
    constexpr float fullTurn = 360.f;
    return convert<Color>(Hue<Space>::set(convert<Space>(color), fullTurn * unit_float(generator)));
}
} // namespace aest::colors
