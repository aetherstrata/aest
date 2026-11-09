/**
 * @copyright Copyright (c) 2026 Davide Pierotti, MIT License (see LICENSE.md)
 * @author Davide Pierotti
 * @file Hex.cppm
 * @brief Partition `aest.colors:hex`: hex codes, 8-bit channels and packed
 *        32-bit colors, read and written as SRGB for any color space.
 */
module;
#include <bit>
#include <cstdint>
#include <format>
#include <string>
export module aest.colors:hex;

import :color;
import :converter;
import :gamut;

export namespace aest::colors
{
/**
 * @brief Build a color from 8-bit SRGB channels.
 *
 * The bytes are SRGB, the encoding of hex codes and CSS colors, whatever
 * the target space: they are scaled to the 0-1 range and converted to
 * @p Color, as in `from_rgba<HSL>(0, 120, 80)`.
 *
 * @tparam Color The color space to build, SRGB by default
 * @param[in] r Red channel value
 * @param[in] g Green channel value
 * @param[in] b Blue channel value
 * @param[in] a Alpha channel value (default 255)
 * @return The color in @p Color.
 */
template <ColorType Color = SRGB>
    requires ConvertibleTo<Color, SRGB>
constexpr Color from_rgba(const std::uint8_t r,
                          const std::uint8_t g,
                          const std::uint8_t b,
                          const std::uint8_t a = BYTE_MAX)
{
    return convert<Color>(SRGB{to_float(r), to_float(g), to_float(b), to_float(a)});
}

/**
 * @brief Build a color from a 32-bit SRGB integer in @c 0xRRGGBBAA format.
 * @tparam Color The color space to build, SRGB by default
 * @param[in] rgba The color as a 32-bit integer
 * @return The unpacked color, converted to @p Color.
 */
template <ColorType Color = SRGB>
    requires ConvertibleTo<Color, SRGB>
constexpr Color from_rgba(const std::uint32_t rgba)
{
    return from_rgba<Color>(byte_at(rgba, 3), byte_at(rgba, 2), byte_at(rgba, 1), byte_at(rgba, 0));
}

/**
 * @brief Build a color from a 32-bit SRGB integer in @c 0xAARRGGBB format.
 * @tparam Color The color space to build, SRGB by default
 * @param[in] argb The color as a 32-bit integer
 * @return The unpacked color, converted to @p Color.
 */
template <ColorType Color = SRGB>
    requires ConvertibleTo<Color, SRGB>
constexpr Color from_argb(const std::uint32_t argb)
{
    // AARRGGBB -> RRGGBBAA
    return from_rgba<Color>(std::rotl(argb, 8));
}

/**
 * @brief Build a color from an SRGB hex code, a 32-bit integer in
 *        @c 0xRRGGBB format, as in `from_hex(0xFF7F50)`.
 * @tparam Color The color space to build, SRGB by default
 * @param[in] hex The color as an HTML color code
 * @return The color, fully opaque, converted to @p Color.
 */
template <ColorType Color = SRGB>
    requires ConvertibleTo<Color, SRGB>
constexpr Color from_hex(const std::uint32_t hex)
{
    // Shift the hex color code up one byte and fill with 0xFF
    return from_rgba<Color>(hex << 8 | BYTE_MAX);
}

/**
 * @brief Convert a color to SRGB and pack it into a 32-bit integer in
 *        @c 0xRRGGBBAA format.
 *
 * A color outside the SRGB gamut clips. Unlike pack_rgba8(), which stores a
 * color's own encoding, this always writes SRGB.
 *
 * @tparam Color The color space of @p color
 * @param[in] color The color to pack
 * @return The SRGB color as a 32-bit integer.
 * @note Channels are rounded to the nearest byte, so the conversion loses
 * precision.
 */
template <ColorType Color>
    requires ConvertibleTo<SRGB, Color>
[[nodiscard]] constexpr std::uint32_t to_rgba(const Color& color)
{
    const SRGB srgb = convert<SRGB>(color);
    return pack(to_byte(srgb.red()), to_byte(srgb.green()), to_byte(srgb.blue()),
                to_byte(srgb.alpha()));
}

/**
 * @brief Convert a color to SRGB and pack it into a 32-bit integer in
 *        @c 0xAARRGGBB format.
 *
 * A color outside the SRGB gamut clips, as with to_rgba().
 *
 * @tparam Color The color space of @p color
 * @param[in] color The color to pack
 * @return The SRGB color as a 32-bit integer.
 * @note Channels are rounded to the nearest byte, so the conversion loses
 * precision.
 */
template <ColorType Color>
    requires ConvertibleTo<SRGB, Color>
[[nodiscard]] constexpr std::uint32_t to_argb(const Color& color)
{
    // RRGGBBAA -> AARRGGBB
    return std::rotr(to_rgba(color), 8);
}

/**
 * @brief Convert a color to SRGB and write it as a hex code in `#RRGGBB`
 *        format, without alpha.
 *
 * A color outside the SRGB gamut clips, as with to_rgba().
 *
 * @tparam Color The color space of @p color
 * @param[in] color The color to write
 * @return A new string containing the hex code.
 */
template <ColorType Color>
    requires ConvertibleTo<SRGB, Color>
[[nodiscard]] std::string hex_string(const Color& color)
{
    return std::format("#{:06X}", to_rgba(color) >> 8);
}
} // namespace aest::colors
