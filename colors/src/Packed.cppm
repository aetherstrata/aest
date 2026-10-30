/**
 * @copyright Copyright (c) 2026 Davide Pierotti, MIT License (see LICENSE.md)
 * @author Davide Pierotti
 * @file Packed.cppm
 * @brief Partition `aest.colors:packed`: GPU pixel formats, packed and
 *        unpacked with the bit layouts of Vulkan and Direct3D.
 */
module;
#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <cstdint>
#include <limits>
export module aest.colors:packed;

import :color;
import :compositing;
import :gamut;

namespace aest::colors
{
/** @brief Exponent bias of the 5-bit exponent shared by half, 11-bit and 10-bit floats. */
constexpr std::int32_t FLOAT5_BIAS = 15;

/** @brief Largest biased exponent of a finite 5-bit-exponent float. */
constexpr std::int32_t FLOAT5_MAX_EXPONENT = 30;

/** @brief Exponent bias of a 32-bit float. */
constexpr std::int32_t FLOAT32_BIAS = 127;

/** @brief Width of the mantissa of a 32-bit float. */
constexpr std::int32_t FLOAT32_MANTISSA = 23;

/**
 * @brief A float with value 2 raised to @p exponent.
 * @param[in] exponent The power of 2, in the range of normal floats (-126 to 127)
 * @return 2^@p exponent, exactly.
 */
constexpr float power_of_two(const std::int32_t exponent)
{
    return std::bit_cast<float>(static_cast<std::uint32_t>(exponent + FLOAT32_BIAS)
                                << FLOAT32_MANTISSA);
}

/**
 * @brief Encodes a non-negative float into an unsigned float with a 5-bit
 *        exponent (bias 15) and @p Mantissa bits, rounding to nearest even.
 *
 * Values too large for the format round to infinity; values too small
 * become subnormals or 0.
 *
 * @tparam Mantissa The mantissa width: 10 for half, 6 or 5 for the packed formats
 * @param[in] value The value, finite and 0 or greater
 * @return The bits, exponent above the mantissa.
 */
template <std::int32_t Mantissa>
constexpr std::uint32_t encode_float5(const float value)
{
    const auto bits     = std::bit_cast<std::uint32_t>(value);
    const auto exponent = static_cast<std::int32_t>(bits >> FLOAT32_MANTISSA) - FLOAT32_BIAS;
    const std::uint32_t mantissa = bits & ((1u << FLOAT32_MANTISSA) - 1);

    if (exponent > FLOAT5_MAX_EXPONENT - FLOAT5_BIAS)
        return static_cast<std::uint32_t>(FLOAT5_MAX_EXPONENT + 1) << Mantissa; // infinity

    // The significand with its implicit 1, and how far to shift it into the
    // mantissa: further for subnormals, whose exponent is fixed at 1 - bias
    const std::uint32_t significand = (1u << FLOAT32_MANTISSA) | mantissa;
    const bool subnormal            = exponent < 1 - FLOAT5_BIAS;
    const std::int32_t shift =
        FLOAT32_MANTISSA - Mantissa + (subnormal ? 1 - FLOAT5_BIAS - exponent : 0);
    if (shift > FLOAT32_MANTISSA + 1)
        return 0; // below half of the smallest subnormal, float zero included

    // Biased exponent above the mantissa; a rounding carry moves into the
    // exponent, up to infinity
    const std::uint32_t biased =
        subnormal ? 0u : static_cast<std::uint32_t>(exponent + FLOAT5_BIAS) << Mantissa;
    const std::uint32_t kept      = significand >> shift;
    const std::uint32_t remainder = significand & ((1u << shift) - 1);
    const std::uint32_t halfway   = 1u << (shift - 1);
    const bool roundUp = remainder > halfway || (remainder == halfway && (kept & 1u) != 0);
    return (biased | (kept & ((1u << Mantissa) - 1))) + (roundUp ? 1u : 0u);
}

/**
 * @brief Decodes an unsigned float with a 5-bit exponent (bias 15) and
 *        @p Mantissa bits.
 * @tparam Mantissa The mantissa width: 10 for half, 6 or 5 for the packed formats
 * @param[in] bits The bits, exponent above the mantissa
 * @return The value, exactly: subnormals, infinity and NaN included.
 */
template <std::int32_t Mantissa>
constexpr float decode_float5(const std::uint32_t bits)
{
    const auto exponent          = static_cast<std::int32_t>(bits >> Mantissa);
    const std::uint32_t mantissa = bits & ((1u << Mantissa) - 1);

    if (exponent == FLOAT5_MAX_EXPONENT + 1)
        return mantissa == 0 ? std::numeric_limits<float>::infinity()
                             : std::numeric_limits<float>::quiet_NaN();
    if (exponent == 0)
        return static_cast<float>(mantissa) * power_of_two(1 - FLOAT5_BIAS - Mantissa);

    return std::bit_cast<float>(static_cast<std::uint32_t>(exponent - FLOAT5_BIAS + FLOAT32_BIAS)
                                    << FLOAT32_MANTISSA
                                | mantissa << (FLOAT32_MANTISSA - Mantissa));
}

/**
 * @brief Largest finite value of an unsigned float with a 5-bit exponent
 *        and @p Mantissa bits.
 * @tparam Mantissa The mantissa width
 */
template <std::int32_t Mantissa>
constexpr float FLOAT5_MAX = decode_float5<Mantissa>(
    static_cast<std::uint32_t>(FLOAT5_MAX_EXPONENT) << Mantissa | ((1u << Mantissa) - 1));

/**
 * @brief Clamps a channel to the finite range of an unsigned float format.
 * @tparam Mantissa The mantissa width of the format
 * @param[in] value The channel value
 * @return @p value clamped to [0, largest finite value].
 */
template <std::int32_t Mantissa>
constexpr float clamp_float5(const float value)
{
    return std::clamp(value, 0.f, FLOAT5_MAX<Mantissa>);
}

/**
 * @brief Quantizes a value in the 0-1 range to an unsigned normalized integer.
 * @param[in] value The value, clamped to the 0-1 range
 * @param[in] max The largest integer, `2^bits - 1`
 * @return The nearest integer to `value * max`.
 */
constexpr std::uint32_t to_unorm(const float value, const std::uint32_t max)
{
    return static_cast<std::uint32_t>(clamp(value) * static_cast<float>(max) + 0.5f);
}

/**
 * @brief Converts an unsigned normalized integer back to the 0-1 range.
 * @param[in] value The integer
 * @param[in] max The largest integer, `2^bits - 1`
 * @return `value / max`.
 */
constexpr float from_unorm(const std::uint32_t value, const std::uint32_t max)
{
    return static_cast<float>(value) / static_cast<float>(max);
}

/**
 * @brief Whether a type is the premultiplied form of a linear RGB space.
 * @tparam T The type to check
 */
template <typename T>
constexpr bool IS_PREMULTIPLIED_RGB = false;

/**
 * @brief The premultiplied form of a linear RGB space.
 * @tparam Space The linear RGB space
 */
template <LinearRGBSpace Space>
constexpr bool IS_PREMULTIPLIED_RGB<Premultiplied<Space>> = true;

/** @brief Mantissa width of a half float. */
constexpr std::int32_t HALF_MANTISSA = 10;

/** @brief Mantissa width of the red and green channels of B10G11R11. */
constexpr std::int32_t FLOAT11_MANTISSA = 6;

/** @brief Mantissa width of the blue channel of B10G11R11. */
constexpr std::int32_t FLOAT10_MANTISSA = 5;

/** @brief Mantissa width of each channel of E5B9G9R9. */
constexpr std::int32_t RGB9E5_MANTISSA = 9;

/** @brief Largest value of a channel of E5B9G9R9: `511 / 512 * 2^16`. */
constexpr float RGB9E5_MAX = 65408.f;
} // namespace aest::colors

export namespace aest::colors
{
/**
 * @brief A color whose channels are red, green and blue: any RGB space, or
 *        the premultiplied form of a linear one.
 * @tparam T The type to check
 */
template <typename T>
concept PackableRGB = RGBSpace<T> || IS_PREMULTIPLIED_RGB<T>;

/**
 * @brief A color whose channels are linear red, green and blue: a linear RGB
 *        space, or its premultiplied form.
 * @tparam T The type to check
 */
template <typename T>
concept PackableLinearRGB = LinearRGBSpace<T> || IS_PREMULTIPLIED_RGB<T>;

/**
 * @brief Converts a float to an IEEE 754 half, rounding to nearest even.
 *
 * Values beyond 65504 round to infinity, infinities stay infinite with
 * their sign, and NaN becomes the quiet NaN `0x7E00` with its sign.
 *
 * @param[in] value The float to convert
 * @return The bits of the half.
 */
constexpr std::uint16_t to_half(const float value)
{
    const auto bits                 = std::bit_cast<std::uint32_t>(value);
    const std::uint32_t sign        = (bits >> 16) & 0x8000u;
    const std::uint32_t magnitude   = bits & 0x7FFF'FFFFu;
    constexpr std::uint32_t infBits = 0x7F80'0000u;

    if (magnitude > infBits)
        return static_cast<std::uint16_t>(sign | 0x7E00u);
    if (magnitude == infBits)
        return static_cast<std::uint16_t>(sign | 0x7C00u);
    return static_cast<std::uint16_t>(
        sign | encode_float5<HALF_MANTISSA>(std::bit_cast<float>(magnitude)));
}

/**
 * @brief Converts an IEEE 754 half to a float, exactly.
 * @param[in] half The bits of the half
 * @return The value: subnormals, signed zeros, infinities and NaN included.
 */
constexpr float from_half(const std::uint16_t half)
{
    const float magnitude = decode_float5<HALF_MANTISSA>(half & 0x7FFFu);
    return (half & 0x8000u) != 0 ? -magnitude : magnitude;
}

/**
 * @brief Packs a color into RGBA8 unorm bytes, in memory order, as
 *        `R8G8B8A8_UNORM` and `R8G8B8A8_SRGB` store them.
 *
 * Each channel is clamped to [0, 1] and rounded to the nearest of 256
 * levels. The color's own encoding is stored: pack SRGB for an `_SRGB`
 * format, a linear space for `_UNORM` data such as masks.
 *
 * @tparam Color An RGB space, or premultiplied linear RGB
 * @param[in] color The color to pack
 * @return The bytes red, green, blue, alpha.
 */
template <PackableRGB Color>
constexpr std::array<std::uint8_t, 4> pack_rgba8(const Color& color)
{
    return {static_cast<std::uint8_t>(to_unorm(color.ch1(), 255)),
            static_cast<std::uint8_t>(to_unorm(color.ch2(), 255)),
            static_cast<std::uint8_t>(to_unorm(color.ch3(), 255)),
            static_cast<std::uint8_t>(to_unorm(color.alpha(), 255))};
}

/**
 * @brief Unpacks RGBA8 unorm bytes, in memory order, into a color.
 * @tparam Color The color type the bytes are in
 * @param[in] bytes The bytes red, green, blue, alpha
 * @return The color, each channel `byte / 255`.
 */
template <PackableRGB Color>
constexpr Color unpack_rgba8(const std::array<std::uint8_t, 4>& bytes)
{
    return Color{from_unorm(bytes[0], 255), from_unorm(bytes[1], 255), from_unorm(bytes[2], 255),
                 from_unorm(bytes[3], 255)};
}

/**
 * @brief Packs a color into `A2B10G10R10_UNORM_PACK32`: red in bits 0-9,
 *        green in 10-19, blue in 20-29 and alpha in 30-31.
 *
 * Each channel is clamped to [0, 1] and rounded to the nearest of 1024
 * levels, alpha to the nearest of 4. As with pack_rgba8(), the color's own
 * encoding is stored.
 *
 * @tparam Color An RGB space, or premultiplied linear RGB
 * @param[in] color The color to pack
 * @return The packed 32-bit word.
 */
template <PackableRGB Color>
constexpr std::uint32_t pack_rgb10a2(const Color& color)
{
    return to_unorm(color.ch1(), 1023) | to_unorm(color.ch2(), 1023) << 10
         | to_unorm(color.ch3(), 1023) << 20 | to_unorm(color.alpha(), 3) << 30;
}

/**
 * @brief Unpacks an `A2B10G10R10_UNORM_PACK32` word into a color.
 * @tparam Color The color type the word is in
 * @param[in] bits The packed word
 * @return The color, each channel `value / 1023` and alpha `value / 3`.
 */
template <PackableRGB Color>
constexpr Color unpack_rgb10a2(const std::uint32_t bits)
{
    return Color{from_unorm(bits & 0x3FFu, 1023), from_unorm(bits >> 10 & 0x3FFu, 1023),
                 from_unorm(bits >> 20 & 0x3FFu, 1023), from_unorm(bits >> 30, 3)};
}

/**
 * @brief Packs a linear color into RGBA16F halves, in memory order, as
 *        `R16G16B16A16_SFLOAT` stores them.
 *
 * Channels are clamped to 65504, the largest finite half, so the result is
 * never infinite, then rounded to nearest even.
 *
 * @tparam Color A linear RGB space, or its premultiplied form
 * @param[in] color The color to pack
 * @return The halves red, green, blue, alpha.
 */
template <PackableLinearRGB Color>
constexpr std::array<std::uint16_t, 4> pack_rgba16f(const Color& color)
{
    const auto half = [](const float channel) {
        return to_half(clamp_float5<HALF_MANTISSA>(channel));
    };
    return {half(color.ch1()), half(color.ch2()), half(color.ch3()), half(color.alpha())};
}

/**
 * @brief Unpacks RGBA16F halves, in memory order, into a linear color.
 * @tparam Color The color type the halves are in
 * @param[in] halves The halves red, green, blue, alpha
 * @return The color.
 */
template <PackableLinearRGB Color>
constexpr Color unpack_rgba16f(const std::array<std::uint16_t, 4>& halves)
{
    return Color{from_half(halves[0]), from_half(halves[1]), from_half(halves[2]),
                 from_half(halves[3])};
}

/**
 * @brief Packs a linear color into `B10G11R11_UFLOAT_PACK32`: red in bits
 *        0-10 and green in 11-21 as 11-bit floats, blue in 22-31 as a 10-bit
 *        float, alpha dropped.
 *
 * The floats are unsigned with a 5-bit exponent, like half without the sign
 * and with 6 or 5 mantissa bits: about 2 and 1.5 significant digits. Each
 * channel is clamped to the largest finite value, 65024 for red and green,
 * 64512 for blue, and rounded to nearest even.
 *
 * @tparam Color A linear RGB space, or its premultiplied form
 * @param[in] color The color to pack
 * @return The packed 32-bit word.
 */
template <PackableLinearRGB Color>
constexpr std::uint32_t pack_r11g11b10f(const Color& color)
{
    return encode_float5<FLOAT11_MANTISSA>(clamp_float5<FLOAT11_MANTISSA>(color.ch1()))
         | encode_float5<FLOAT11_MANTISSA>(clamp_float5<FLOAT11_MANTISSA>(color.ch2())) << 11
         | encode_float5<FLOAT10_MANTISSA>(clamp_float5<FLOAT10_MANTISSA>(color.ch3())) << 22;
}

/**
 * @brief Unpacks a `B10G11R11_UFLOAT_PACK32` word into a linear color.
 * @tparam Color The color type the word is in
 * @param[in] bits The packed word
 * @return The color, opaque.
 */
template <PackableLinearRGB Color>
constexpr Color unpack_r11g11b10f(const std::uint32_t bits)
{
    return Color{decode_float5<FLOAT11_MANTISSA>(bits & 0x7FFu),
                 decode_float5<FLOAT11_MANTISSA>(bits >> 11 & 0x7FFu),
                 decode_float5<FLOAT10_MANTISSA>(bits >> 22), 1.0f};
}

/**
 * @brief Packs a linear color into `E5B9G9R9_UFLOAT_PACK32`: 9-bit mantissas
 *        of red, green and blue in bits 0-8, 9-17 and 18-26, sharing the
 *        5-bit exponent in bits 27-31; alpha dropped.
 *
 * Follows the conversion of the Vulkan specification: channels are clamped
 * to [0, 65408], the exponent is chosen for the largest channel, and each
 * mantissa is rounded to nearest. The largest channel keeps 9 significant
 * bits; a channel far dimmer than it keeps fewer, down to none.
 *
 * @tparam Color A linear RGB space, or its premultiplied form
 * @param[in] color The color to pack
 * @return The packed 32-bit word.
 */
template <PackableLinearRGB Color>
constexpr std::uint32_t pack_rgb9e5(const Color& color)
{
    const float red     = std::clamp(color.ch1(), 0.f, RGB9E5_MAX);
    const float green   = std::clamp(color.ch2(), 0.f, RGB9E5_MAX);
    const float blue    = std::clamp(color.ch3(), 0.f, RGB9E5_MAX);
    const float largest = std::max({red, green, blue});

    // floor(log2(largest)) from the float exponent, at least -bias - 1
    const auto floorLog2 =
        static_cast<std::int32_t>(std::bit_cast<std::uint32_t>(largest) >> FLOAT32_MANTISSA)
        - FLOAT32_BIAS;
    std::int32_t exponent = std::max(-FLOAT5_BIAS - 1, floorLog2) + 1 + FLOAT5_BIAS;

    // Rounding the largest channel up to 2^9 needs the next exponent
    const auto quantize = [&exponent](const float channel) {
        return static_cast<std::uint32_t>(
            channel / power_of_two(exponent - FLOAT5_BIAS - RGB9E5_MANTISSA) + 0.5f);
    };
    if (quantize(largest) == 1u << RGB9E5_MANTISSA)
        ++exponent;

    return quantize(red) | quantize(green) << 9 | quantize(blue) << 18
         | static_cast<std::uint32_t>(exponent) << 27;
}

/**
 * @brief Unpacks an `E5B9G9R9_UFLOAT_PACK32` word into a linear color.
 * @tparam Color The color type the word is in
 * @param[in] bits The packed word
 * @return The color, opaque.
 */
template <PackableLinearRGB Color>
constexpr Color unpack_rgb9e5(const std::uint32_t bits)
{
    const float scale =
        power_of_two(static_cast<std::int32_t>(bits >> 27) - FLOAT5_BIAS - RGB9E5_MANTISSA);
    return Color{static_cast<float>(bits & 0x1FFu) * scale,
                 static_cast<float>(bits >> 9 & 0x1FFu) * scale,
                 static_cast<float>(bits >> 18 & 0x1FFu) * scale, 1.0f};
}
} // namespace aest::colors
