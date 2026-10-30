/**
 * @copyright Copyright (c) 2026 Davide Pierotti, MIT License (see LICENSE.md)
 * @author Davide Pierotti
 * @file Operations.cppm
 * @brief Partition `aest.colors:operations`: free functions that adjust colors.
 */
module;
#include <algorithm>
#include <cmath>
#include <concepts>
export module aest.colors:operations;

import :color;
import :converter;
import :space;

namespace aest::colors
{
/** @brief Weight of linear red in the luminance, from Rec. 709 / sRGB. */
constexpr float LUMINANCE_RED = 0.2126f;

/** @brief Weight of linear green in the luminance, from Rec. 709 / sRGB. */
constexpr float LUMINANCE_GREEN = 0.7152f;

/** @brief Weight of linear blue in the luminance, from Rec. 709 / sRGB. */
constexpr float LUMINANCE_BLUE = 0.0722f;

/** @brief Viewing flare WCAG adds to both luminances of a contrast ratio. */
constexpr float CONTRAST_FLARE = 0.05f;

/**
 * @brief Interpolates every channel of two colors, alpha included.
 * @tparam Space A color space whose channels interpolate linearly, built from
 *         `(ch1, ch2, ch3, alpha)`
 * @param[in] from The color at @p amount 0
 * @param[in] to The color at @p amount 1
 * @param[in] amount Position between the two colors, in the 0-1 range
 * @return The interpolated color.
 */
template <ColorType Space>
constexpr Space lerp_channels(const Space& from, const Space& to, const float amount)
{
    return Space{std::lerp(from.ch1(), to.ch1(), amount), std::lerp(from.ch2(), to.ch2(), amount),
                 std::lerp(from.ch3(), to.ch3(), amount),
                 std::lerp(from.alpha(), to.alpha(), amount)};
}
} // namespace aest::colors

export namespace aest::colors
{
/**
 * @brief Reads and replaces the lightness of a color space.
 *
 * Specialize it for a color space to make it a LightnessSpace, usable with
 * lighten() and darken().
 *
 * @tparam Space The color space.
 */
template <ColorType Space>
struct Lightness;

/** @brief Lightness of HSL, stored in the 0-1 range. */
template <>
struct Lightness<HSL>
{
    /**
     * @brief Get the lightness of @p color.
     * @param[in] color The color to read
     * @return The lightness in the 0-1 range.
     */
    static constexpr float get(const HSL& color) noexcept { return color.lightness(); }

    /**
     * @brief Copy @p color with a new lightness.
     * @param[in] color The color to copy
     * @param[in] lightness The new lightness in the 0-1 range
     * @return The color with the same hue, saturation and alpha.
     */
    static constexpr HSL set(const HSL& color, const float lightness)
    {
        return HSL{color.hue(), color.saturation(), lightness, color.alpha()};
    }
};

/**
 * @brief A color space whose lightness can be read and replaced through
 * Lightness.
 * @tparam T The type to check
 */
template <typename T>
concept LightnessSpace = ColorType<T> && requires(const T& color, const float value) {
    { Lightness<T>::get(color) } -> std::same_as<float>;
    { Lightness<T>::set(color, value) } -> std::same_as<T>;
};

/**
 * @brief Moves the lightness of a color a fraction of the way toward white.
 *
 * The color is converted to @p Space, its lightness `l` becomes
 * `l + (1 - l) * amount` and it is converted back, so hue, saturation and
 * alpha are kept.
 *
 * @tparam Space The color space the lightness is adjusted in
 * @tparam Color The color type of the input and the result
 * @param[in] color The color to lighten
 * @param[in] amount Fraction of the distance to white, clamped to the 0-1 range:
 *            0 keeps the color, 1 gives white
 * @param[in] space The working space, e.g. `in<HSL>`; HSL when omitted
 * @return The lightened color.
 */
template <LightnessSpace Space = HSL, ColorType Color>
    requires ConvertibleTo<Space, Color> && ConvertibleTo<Color, Space>
constexpr Color lighten(const Color& color,
                        const float amount,
                        [[maybe_unused]] In<Space> space = {})
{
    const auto working    = convert<Space>(color);
    const float lightness = Lightness<Space>::get(working);
    return convert<Color>(
        Lightness<Space>::set(working, lightness + (1 - lightness) * clamp(amount)));
}

/**
 * @brief Moves the lightness of a color a fraction of the way toward black.
 *
 * The color is converted to @p Space, its lightness `l` becomes
 * `l * (1 - amount)` and it is converted back, so hue, saturation and alpha
 * are kept.
 *
 * @tparam Space The color space the lightness is adjusted in
 * @tparam Color The color type of the input and the result
 * @param[in] color The color to darken
 * @param[in] amount Fraction of the distance to black, clamped to the 0-1 range:
 *            0 keeps the color, 1 gives black
 * @param[in] space The working space, e.g. `in<HSL>`; HSL when omitted
 * @return The darkened color.
 */
template <LightnessSpace Space = HSL, ColorType Color>
    requires ConvertibleTo<Space, Color> && ConvertibleTo<Color, Space>
constexpr Color darken(const Color& color,
                       const float amount,
                       [[maybe_unused]] In<Space> space = {})
{
    const auto working = convert<Space>(color);
    return convert<Color>(
        Lightness<Space>::set(working, Lightness<Space>::get(working) * (1 - clamp(amount))));
}

/**
 * @brief Interpolates between two colors of a color space.
 *
 * Specialize it for a color space to make it an InterpolationSpace, usable
 * with mix().
 *
 * @tparam Space The color space.
 */
template <ColorType Space>
struct Interpolation;

/** @brief Interpolation of linear sRGB, channel by channel. */
template <>
struct Interpolation<LinSRGB>
{
    /**
     * @brief Interpolates the light and alpha of two colors.
     * @param[in] from The color at @p amount 0
     * @param[in] to The color at @p amount 1
     * @param[in] amount Position between the two colors, in the 0-1 range
     * @return The interpolated color.
     */
    static constexpr LinSRGB lerp(const LinSRGB& from, const LinSRGB& to, const float amount)
    {
        return lerp_channels(from, to, amount);
    }
};

/** @brief Interpolation of gamma-encoded sRGB, channel by channel. */
template <>
struct Interpolation<SRGB>
{
    /**
     * @brief Interpolates the encoded channels and alpha of two colors.
     * @param[in] from The color at @p amount 0
     * @param[in] to The color at @p amount 1
     * @param[in] amount Position between the two colors, in the 0-1 range
     * @return The interpolated color.
     */
    static constexpr SRGB lerp(const SRGB& from, const SRGB& to, const float amount)
    {
        return lerp_channels(from, to, amount);
    }
};

/**
 * @brief A color space whose colors can be interpolated through
 * Interpolation.
 * @tparam T The type to check
 */
template <typename T>
concept InterpolationSpace = ColorType<T> && requires(const T& color, const float amount) {
    { Interpolation<T>::lerp(color, color, amount) } -> std::same_as<T>;
};

/**
 * @brief Blends two colors.
 *
 * Both colors are converted to @p Space, interpolated there channel by
 * channel, alpha included, and converted back. The default, linear sRGB,
 * blends the light physically; `mix<SRGB>(from, to, amount)` blends the
 * encoded values instead, like CSS `color-mix(in srgb, …)`, which gives
 * darker midpoints.
 *
 * @tparam Space The color space the colors are blended in
 * @tparam Color The color type of the inputs and the result
 * @param[in] from The color at @p amount 0
 * @param[in] to The color at @p amount 1
 * @param[in] amount Fraction of the way from @p from to @p to, clamped to the
 *            0-1 range: 0 gives @p from, 1 gives @p to
 * @param[in] space The working space, e.g. `in<SRGB>`; LinSRGB when omitted
 * @return The blended color.
 */
template <InterpolationSpace Space = LinSRGB, ColorType Color>
    requires ConvertibleTo<Space, Color> && ConvertibleTo<Color, Space>
constexpr Color mix(const Color& from,
                    const Color& to,
                    const float amount,
                    [[maybe_unused]] In<Space> space = {})
{
    return convert<Color>(
        Interpolation<Space>::lerp(convert<Space>(from), convert<Space>(to), clamp(amount)));
}

/**
 * @brief Relative luminance of a color: the light it emits, weighted by how
 *        bright each primary looks.
 *
 * The color is converted to LinSRGB and its channels weighted with the
 * Rec. 709 / sRGB coefficients, ignoring alpha. White gives 1 and black 0;
 * HDR colors give more than 1.
 *
 * @tparam Color The color type of @p color
 * @param[in] color The color to measure
 * @return The relative luminance, 0 or greater.
 */
template <ColorType Color>
    requires ConvertibleTo<LinSRGB, Color>
constexpr float luminance(const Color& color)
{
    const LinSRGB linear = convert<LinSRGB>(color);
    return LUMINANCE_RED * linear.red() + LUMINANCE_GREEN * linear.green()
         + LUMINANCE_BLUE * linear.blue();
}

/**
 * @brief WCAG 2.x contrast ratio between two colors, such as text and its
 *        background.
 *
 * Both colors are first converted to SRGB, as a display shows them, so HDR
 * values clip to 1. The ratio is `(L1 + 0.05) / (L2 + 0.05)`, with `L1` the
 * luminance() of the lighter color. Alpha is ignored: composite a
 * translucent color onto its background with over() first.
 *
 * @tparam Color1 Color type of @p color1
 * @tparam Color2 Color type of @p color2
 * @param[in] color1 One of the colors; the order does not matter
 * @param[in] color2 The other color
 * @return The contrast ratio, in the 1-21 range: 1 for equal luminances,
 *         21 for black on white.
 */
template <ColorType Color1, ColorType Color2>
    requires ConvertibleTo<SRGB, Color1> && ConvertibleTo<SRGB, Color2>
float contrast_ratio(const Color1& color1, const Color2& color2)
{
    const float luminance1 = luminance(convert<SRGB>(color1));
    const float luminance2 = luminance(convert<SRGB>(color2));
    return (std::max(luminance1, luminance2) + CONTRAST_FLARE)
         / (std::min(luminance1, luminance2) + CONTRAST_FLARE);
}
} // namespace aest::colors
