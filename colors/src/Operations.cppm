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
import :cylindrical;
import :gamut;
import :transfer;
import :space;

namespace aest::colors
{
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

/**
 * @brief Lightness of HSL, stored in the 0-1 range.
 * @tparam RGB The encoded RGB space of the HSL
 */
template <BoundedRGBSpace RGB>
struct Lightness<BasicHSL<RGB>>
{
    /**
     * @brief Get the lightness of @p color.
     * @param[in] color The color to read
     * @return The lightness in the 0-1 range.
     */
    static constexpr float get(const BasicHSL<RGB>& color) noexcept { return color.lightness(); }

    /**
     * @brief Copy @p color with a new lightness.
     * @param[in] color The color to copy
     * @param[in] lightness The new lightness in the 0-1 range
     * @return The color with the same hue, saturation and alpha.
     */
    static constexpr BasicHSL<RGB> set(const BasicHSL<RGB>& color, const float lightness)
    {
        return BasicHSL<RGB>{color.hue(), color.saturation(), lightness, color.alpha()};
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

/**
 * @brief Interpolation of an RGB space, channel by channel: linear spaces
 *        blend the light, encoded ones the encoded values.
 * @tparam Gamut The gamut tag
 * @tparam Transfer The transfer function tag
 */
template <RGBGamut Gamut, TransferFunction Transfer>
struct Interpolation<BasicRGB<Gamut, Transfer>>
{
    /**
     * @brief Interpolates the channels and alpha of two colors.
     * @param[in] from The color at @p amount 0
     * @param[in] to The color at @p amount 1
     * @param[in] amount Position between the two colors, in the 0-1 range
     * @return The interpolated color.
     */
    static constexpr BasicRGB<Gamut, Transfer> lerp(const BasicRGB<Gamut, Transfer>& from,
                                                    const BasicRGB<Gamut, Transfer>& to,
                                                    const float amount)
    {
        return lerp_channels(from, to, amount);
    }
};

/** @brief Interpolation of XYZ, channel by channel: XYZ is linear in light. */
template <>
struct Interpolation<XYZ>
{
    /**
     * @brief Interpolates the tristimulus values and alpha of two colors.
     * @param[in] from The color at @p amount 0
     * @param[in] to The color at @p amount 1
     * @param[in] amount Position between the two colors, in the 0-1 range
     * @return The interpolated color.
     */
    static constexpr XYZ lerp(const XYZ& from, const XYZ& to, const float amount)
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
 * Both colors are converted to the working space, interpolated there
 * channel by channel, alpha included, and converted back. By default it is
 * the LinearSpaceOf @p Color, linear light in the color's own gamut: LinSRGB
 * for SRGB and HSL, LinDisplayP3 for DisplayP3. That blends the light
 * physically without clipping wide-gamut colors to sRGB, and linear
 * blending gives the same result in every linear gamut otherwise.
 * `mix<SRGB>(from, to, amount)` blends the encoded values instead, like CSS
 * `color-mix(in srgb, …)`, which gives darker midpoints.
 *
 * Both colors have the same type. To blend colors of two spaces, convert
 * one into the other's first, into the wider gamut so nothing clips:
 * `mix(convert<DisplayP3>(srgbColor), p3Color, amount)`.
 *
 * @tparam Space The color space the colors are blended in; `void`, the
 *         default, follows @p Color
 * @tparam Color The color type of the inputs and the result
 * @param[in] from The color at @p amount 0
 * @param[in] to The color at @p amount 1
 * @param[in] amount Fraction of the way from @p from to @p to, clamped to the
 *            0-1 range: 0 gives @p from, 1 gives @p to
 * @param[in] space The working space, e.g. `in<SRGB>`; the LinearSpaceOf
 *            @p Color when omitted
 * @return The blended color.
 */
template <typename Space = void, ColorType Color>
    requires InterpolationSpace<WorkingSpace<Space, Color>>
          && ConvertibleTo<WorkingSpace<Space, Color>, Color>
          && ConvertibleTo<Color, WorkingSpace<Space, Color>>
constexpr Color mix(const Color& from,
                    const Color& to,
                    const float amount,
                    [[maybe_unused]] In<Space> space = {})
{
    using Working = WorkingSpace<Space, Color>;
    return convert<Color>(
        Interpolation<Working>::lerp(convert<Working>(from), convert<Working>(to), clamp(amount)));
}

/**
 * @brief Rejects blending colors of two different types.
 *
 * No result type fits every pair: gamuts are not nested, so neither is
 * always the wider one, and taking the type of @p from would make mix()
 * depend on the order of its arguments. Convert one color into the other's
 * type first, into the wider gamut so nothing clips:
 * `mix(convert<DisplayP3>(srgbColor), p3Color, amount)`.
 *
 * @tparam Space The color space the colors would be blended in
 * @tparam From Color type of @p from
 * @tparam To Color type of @p to, other than @p From
 * @param[in] from The color at @p amount 0
 * @param[in] to The color at @p amount 1
 * @param[in] amount Fraction of the way from @p from to @p to
 * @param[in] space The working space
 * @return Nothing: the function is deleted.
 */
template <typename Space = void, ColorType From, ColorType To>
    requires(!std::same_as<From, To>)
From mix(const From& from, const To& to, float amount, In<Space> space = {}) = delete;

/**
 * @brief Relative luminance of a color: the light it emits, weighted by how
 *        bright each primary looks.
 *
 * The Y of the color converted to XYZ, ignoring alpha: for LinSRGB, its
 * channels weighted with the Rec. 709 / sRGB coefficients, about 0.2126,
 * 0.7152 and 0.0722, and likewise for every gamut with its own matrix. White
 * gives 1 and black 0; HDR colors give more than 1.
 *
 * @tparam Color The color type of @p color
 * @param[in] color The color to measure
 * @return The relative luminance, 0 or greater.
 */
template <ColorType Color>
    requires ConvertibleTo<XYZ, Color>
constexpr float luminance(const Color& color)
{
    return convert<XYZ>(color).y();
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
