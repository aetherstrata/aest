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
#include <cstdint>
export module aest.colors:operations;

import :color;
import :converter;
import :cylindrical;
import :gamut;
import :oklab;
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

/**
 * @brief How much of a color's chroma is left when its lightness moves from
 *        @p from to @p to on a straight line toward @p end.
 * @param[in] from The current lightness
 * @param[in] to The new lightness
 * @param[in] end The lightness of white or black, where chroma reaches 0
 * @return `(to - end) / (from - end)`, or 1 when @p from is @p end.
 */
constexpr float chroma_scale(const float from, const float to, const float end)
{
    return from == end ? 1.f : (to - end) / (from - end);
}
} // namespace aest::colors

export namespace aest::colors
{
/**
 * @brief Reads and replaces the lightness of a color space.
 *
 * Specialize it for a color space to make it a LightnessSpace, usable with
 * lighten() and darken(). `set` receives the lightness of the end the color
 * moves toward, 1 for white and 0 for black, so a space whose colorfulness
 * does not vanish at those ends on its own, such as OKLCH, can shrink it on
 * the way and reach exactly white or black.
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
     *
     * HSL reaches white at lightness 1 and black at 0 whatever the
     * saturation, so the saturation is kept.
     *
     * @param[in] color The color to copy
     * @param[in] lightness The new lightness in the 0-1 range
     * @param[in] end The lightness the color moves toward, unused
     * @return The color with the same hue, saturation and alpha.
     */
    static constexpr BasicHSL<RGB> set(const BasicHSL<RGB>& color,
                                       const float lightness,
                                       [[maybe_unused]] const float end)
    {
        return BasicHSL<RGB>{color.hue(), color.saturation(), lightness, color.alpha()};
    }
};

/** @brief Lightness of OKLab, the perceived lightness, 1 being D65 white. */
template <>
struct Lightness<OKLab>
{
    /**
     * @brief Get the lightness of @p color.
     * @param[in] color The color to read
     * @return The perceived lightness.
     */
    static constexpr float get(const OKLab& color) noexcept { return color.lightness(); }

    /**
     * @brief Copy @p color with a new lightness, on the straight line toward
     *        white or black.
     * @param[in] color The color to copy
     * @param[in] lightness The new perceived lightness
     * @param[in] end The lightness the color moves toward: 1 for white, 0 for black
     * @return The color with a and b scaled by the same fraction as the
     *         distance to @p end, so the hue is kept and @p end is gray.
     */
    static constexpr OKLab set(const OKLab& color, const float lightness, const float end)
    {
        const float scale = chroma_scale(color.lightness(), lightness, end);
        return OKLab{lightness, color.a() * scale, color.b() * scale, color.alpha()};
    }
};

/** @brief Lightness of OKLCH, the perceived lightness, 1 being D65 white. */
template <>
struct Lightness<OKLCH>
{
    /**
     * @brief Get the lightness of @p color.
     * @param[in] color The color to read
     * @return The perceived lightness.
     */
    static constexpr float get(const OKLCH& color) noexcept { return color.lightness(); }

    /**
     * @brief Copy @p color with a new lightness, on the straight line toward
     *        white or black.
     * @param[in] color The color to copy
     * @param[in] lightness The new perceived lightness
     * @param[in] end The lightness the color moves toward: 1 for white, 0 for black
     * @return The color with its chroma scaled by the same fraction as the
     *         distance to @p end, the same hue and alpha: @p end is gray.
     */
    static constexpr OKLCH set(const OKLCH& color, const float lightness, const float end)
    {
        return OKLCH{lightness, color.chroma() * chroma_scale(color.lightness(), lightness, end),
                     color.hue(), color.alpha()};
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
    { Lightness<T>::set(color, value, value) } -> std::same_as<T>;
};

/**
 * @brief Moves the lightness of a color a fraction of the way toward white.
 *
 * The color is converted to @p Space, its lightness `l` becomes
 * `l + (1 - l) * amount` and it is converted back, keeping hue and alpha.
 * By default @p Space is OKLCH, so equal amounts make equal perceived steps
 * for every hue: chroma shrinks by the same fraction, so the color moves on
 * a straight line toward white and 1 gives exactly white. In HSL,
 * `lighten<HSL>(color, amount)`, saturation is kept instead.
 *
 * @tparam Space The color space the lightness is adjusted in
 * @tparam Color The color type of the input and the result
 * @param[in] color The color to lighten
 * @param[in] amount Fraction of the distance to white, clamped to the 0-1 range:
 *            0 keeps the color, 1 gives white
 * @param[in] space The working space, e.g. `in<HSL>`; OKLCH when omitted
 * @return The lightened color.
 */
template <LightnessSpace Space = OKLCH, ColorType Color>
    requires ConvertibleTo<Space, Color> && ConvertibleTo<Color, Space>
constexpr Color lighten(const Color& color,
                        const float amount,
                        [[maybe_unused]] In<Space> space = {})
{
    const auto working    = convert<Space>(color);
    const float lightness = Lightness<Space>::get(working);
    return convert<Color>(
        Lightness<Space>::set(working, lightness + (1 - lightness) * clamp(amount), 1.f));
}

/**
 * @brief Moves the lightness of a color a fraction of the way toward black.
 *
 * The color is converted to @p Space, its lightness `l` becomes
 * `l * (1 - amount)` and it is converted back, keeping hue and alpha. By
 * default @p Space is OKLCH, where chroma shrinks by the same fraction, so
 * 1 gives exactly black; in HSL saturation is kept instead.
 *
 * @tparam Space The color space the lightness is adjusted in
 * @tparam Color The color type of the input and the result
 * @param[in] color The color to darken
 * @param[in] amount Fraction of the distance to black, clamped to the 0-1 range:
 *            0 keeps the color, 1 gives black
 * @param[in] space The working space, e.g. `in<HSL>`; OKLCH when omitted
 * @return The darkened color.
 */
template <LightnessSpace Space = OKLCH, ColorType Color>
    requires ConvertibleTo<Space, Color> && ConvertibleTo<Color, Space>
constexpr Color darken(const Color& color,
                       const float amount,
                       [[maybe_unused]] In<Space> space = {})
{
    const auto working = convert<Space>(color);
    return convert<Color>(
        Lightness<Space>::set(working, Lightness<Space>::get(working) * (1 - clamp(amount)), 0.f));
}

/**
 * @brief How mix() goes around the hue circle between two hues, as in CSS
 *        Color 4.
 */
enum class HueMode : std::uint8_t
{
    shorter,    ///< The shorter arc, at most 180 degrees: the default.
    longer,     ///< The longer arc, at least 180 degrees; equal hues make a full turn.
    increasing, ///< Clockwise, toward greater hues, wrapping past 360.
    decreasing, ///< Counterclockwise, toward smaller hues, wrapping past 0.
};
} // namespace aest::colors

namespace aest::colors
{
/**
 * @brief Chroma below which an OKLCH color counts as gray, with no hue.
 *
 * Converting an sRGB gray leaves float noise of about 1e-7 in its chroma; a
 * just noticeable difference is about 0.02.
 */
constexpr float ACHROMATIC_CHROMA = 1e-4f;

/**
 * @brief Interpolates between two hues along the arc @p mode picks, as in
 *        CSS Color 4.
 * @param[in] from The hue at @p amount 0, in degrees in the [0, 360) range
 * @param[in] to The hue at @p amount 1, in degrees in the [0, 360) range
 * @param[in] amount Position between the two hues, in the 0-1 range
 * @param[in] mode The arc to follow
 * @return The interpolated hue, wrapped to the [0, 360) range.
 */
constexpr float lerp_hue(float from, float to, const float amount, const HueMode mode)
{
    // Unwrap one end by a full turn so that the plain lerp follows the arc
    const float difference = to - from;
    switch (mode)
    {
    case HueMode::shorter:
        if (difference > 180)
            from += 360;
        else if (difference < -180)
            to += 360;
        break;
    case HueMode::longer:
        if (difference > 0 && difference < 180)
            from += 360;
        else if (difference > -180 && difference <= 0)
            to += 360;
        break;
    case HueMode::increasing:
        if (to < from)
            to += 360;
        break;
    case HueMode::decreasing:
        if (from < to)
            from += 360;
        break;
    }
    return wrap_hue(std::lerp(from, to, amount));
}

/**
 * @brief Interpolates the hues of two colors, either of which may be gray.
 *
 * A gray color has no hue, so it takes the hue of the other: a gradient
 * from gray to red stays red instead of sweeping through other hues. Two
 * grays give hue 0.
 *
 * @param[in] from The hue of the color at @p amount 0
 * @param[in] fromGray Whether that color is gray
 * @param[in] to The hue of the color at @p amount 1
 * @param[in] toGray Whether that color is gray
 * @param[in] amount Position between the two colors, in the 0-1 range
 * @param[in] mode The arc to follow
 * @return The interpolated hue, wrapped to the [0, 360) range.
 */
constexpr float lerp_hues(const float from,
                          const bool fromGray,
                          const float to,
                          const bool toGray,
                          const float amount,
                          const HueMode mode)
{
    if (fromGray && toGray)
        return 0.f;
    return lerp_hue(fromGray ? to : from, toGray ? from : to, amount, mode);
}
} // namespace aest::colors

export namespace aest::colors
{
/**
 * @brief Interpolates between two colors of a color space.
 *
 * Specialize it for a color space to make it an InterpolationSpace, usable
 * with mix(). Spaces with a hue also take a HueMode, which makes them a
 * HueInterpolationSpace.
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
 * @brief Interpolation of OKLab, channel by channel: gradients with even
 *        perceived steps, like CSS `color-mix(in oklab, …)`.
 */
template <>
struct Interpolation<OKLab>
{
    /**
     * @brief Interpolates the lightness, a, b and alpha of two colors.
     * @param[in] from The color at @p amount 0
     * @param[in] to The color at @p amount 1
     * @param[in] amount Position between the two colors, in the 0-1 range
     * @return The interpolated color.
     */
    static constexpr OKLab lerp(const OKLab& from, const OKLab& to, const float amount)
    {
        return lerp_channels(from, to, amount);
    }
};

/**
 * @brief Interpolation of HSL: the hue along an arc of the hue circle,
 *        saturation, lightness and alpha linearly.
 * @tparam RGB The encoded RGB space of the HSL
 */
template <BoundedRGBSpace RGB>
struct Interpolation<BasicHSL<RGB>>
{
    /**
     * @brief Interpolates two colors, the hue along the arc @p mode picks.
     *
     * A color with saturation 0 is gray, and takes the hue of the other.
     *
     * @param[in] from The color at @p amount 0
     * @param[in] to The color at @p amount 1
     * @param[in] amount Position between the two colors, in the 0-1 range
     * @param[in] mode The arc of the hue circle to follow
     * @return The interpolated color.
     */
    static constexpr BasicHSL<RGB> lerp(const BasicHSL<RGB>& from,
                                        const BasicHSL<RGB>& to,
                                        const float amount,
                                        const HueMode mode)
    {
        return BasicHSL<RGB>{lerp_hues(from.hue(), from.saturation() == 0, to.hue(),
                                       to.saturation() == 0, amount, mode),
                             std::lerp(from.saturation(), to.saturation(), amount),
                             std::lerp(from.lightness(), to.lightness(), amount),
                             std::lerp(from.alpha(), to.alpha(), amount)};
    }

    /**
     * @brief Interpolates two colors, the hue along the shorter arc.
     * @param[in] from The color at @p amount 0
     * @param[in] to The color at @p amount 1
     * @param[in] amount Position between the two colors, in the 0-1 range
     * @return The interpolated color.
     */
    static constexpr BasicHSL<RGB> lerp(const BasicHSL<RGB>& from,
                                        const BasicHSL<RGB>& to,
                                        const float amount)
    {
        return lerp(from, to, amount, HueMode::shorter);
    }
};

/**
 * @brief Interpolation of HSV: the hue along an arc of the hue circle,
 *        saturation, value and alpha linearly.
 * @tparam RGB The encoded RGB space of the HSV
 */
template <BoundedRGBSpace RGB>
struct Interpolation<BasicHSV<RGB>>
{
    /**
     * @brief Interpolates two colors, the hue along the arc @p mode picks.
     *
     * A color with saturation 0 is gray, and takes the hue of the other.
     *
     * @param[in] from The color at @p amount 0
     * @param[in] to The color at @p amount 1
     * @param[in] amount Position between the two colors, in the 0-1 range
     * @param[in] mode The arc of the hue circle to follow
     * @return The interpolated color.
     */
    static constexpr BasicHSV<RGB> lerp(const BasicHSV<RGB>& from,
                                        const BasicHSV<RGB>& to,
                                        const float amount,
                                        const HueMode mode)
    {
        return BasicHSV<RGB>{lerp_hues(from.hue(), from.saturation() == 0, to.hue(),
                                       to.saturation() == 0, amount, mode),
                             std::lerp(from.saturation(), to.saturation(), amount),
                             std::lerp(from.brightness(), to.brightness(), amount),
                             std::lerp(from.alpha(), to.alpha(), amount)};
    }

    /**
     * @brief Interpolates two colors, the hue along the shorter arc.
     * @param[in] from The color at @p amount 0
     * @param[in] to The color at @p amount 1
     * @param[in] amount Position between the two colors, in the 0-1 range
     * @return The interpolated color.
     */
    static constexpr BasicHSV<RGB> lerp(const BasicHSV<RGB>& from,
                                        const BasicHSV<RGB>& to,
                                        const float amount)
    {
        return lerp(from, to, amount, HueMode::shorter);
    }
};

/**
 * @brief Interpolation of OKLCH: the hue along an arc of the hue circle,
 *        lightness, chroma and alpha linearly, like CSS
 *        `color-mix(in oklch, …)`.
 */
template <>
struct Interpolation<OKLCH>
{
    /**
     * @brief Interpolates two colors, the hue along the arc @p mode picks.
     *
     * A color with chroma below ACHROMATIC_CHROMA is gray, and takes the hue
     * of the other.
     *
     * @param[in] from The color at @p amount 0
     * @param[in] to The color at @p amount 1
     * @param[in] amount Position between the two colors, in the 0-1 range
     * @param[in] mode The arc of the hue circle to follow
     * @return The interpolated color.
     */
    static constexpr OKLCH lerp(const OKLCH& from,
                                const OKLCH& to,
                                const float amount,
                                const HueMode mode)
    {
        return OKLCH{std::lerp(from.lightness(), to.lightness(), amount),
                     std::lerp(from.chroma(), to.chroma(), amount),
                     lerp_hues(from.hue(), from.chroma() < ACHROMATIC_CHROMA, to.hue(),
                               to.chroma() < ACHROMATIC_CHROMA, amount, mode),
                     std::lerp(from.alpha(), to.alpha(), amount)};
    }

    /**
     * @brief Interpolates two colors, the hue along the shorter arc.
     * @param[in] from The color at @p amount 0
     * @param[in] to The color at @p amount 1
     * @param[in] amount Position between the two colors, in the 0-1 range
     * @return The interpolated color.
     */
    static constexpr OKLCH lerp(const OKLCH& from, const OKLCH& to, const float amount)
    {
        return lerp(from, to, amount, HueMode::shorter);
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
 * @brief A color space with a hue, whose Interpolation takes a HueMode.
 * @tparam T The type to check
 */
template <typename T>
concept HueInterpolationSpace =
    InterpolationSpace<T> && requires(const T& color, const float amount, const HueMode mode) {
        { Interpolation<T>::lerp(color, color, amount, mode) } -> std::same_as<T>;
    };

/**
 * @brief Blends two colors in a space with a hue, going around the hue
 *        circle the way @p mode picks.
 *
 * Like mix(), but the working space must have a hue, so it is named
 * explicitly: `mix<OKLCH>(from, to, amount, HueMode::longer)` blends
 * through the hues on the long way around, as CSS
 * `color-mix(in oklch longer hue, …)` does. Gray colors take the hue of the
 * other color. The default space, linear light, has no hue, so the call
 * fails its constraint without one.
 *
 * @tparam Space The color space with a hue the colors are blended in, such
 *         as OKLCH or HSL
 * @tparam Color The color type of the inputs and the result
 * @param[in] from The color at @p amount 0
 * @param[in] to The color at @p amount 1
 * @param[in] amount Fraction of the way from @p from to @p to, clamped to the
 *            0-1 range: 0 gives @p from, 1 gives @p to
 * @param[in] mode The arc of the hue circle to follow
 * @param[in] space The working space, e.g. `in<OKLCH>`
 * @return The blended color.
 */
template <typename Space = void, ColorType Color>
    requires HueInterpolationSpace<WorkingSpace<Space, Color>>
          && ConvertibleTo<WorkingSpace<Space, Color>, Color>
          && ConvertibleTo<Color, WorkingSpace<Space, Color>>
constexpr Color mix(const Color& from,
                    const Color& to,
                    const float amount,
                    const HueMode mode,
                    [[maybe_unused]] In<Space> space = {})
{
    using Working = WorkingSpace<Space, Color>;
    return convert<Color>(Interpolation<Working>::lerp(convert<Working>(from), convert<Working>(to),
                                                       clamp(amount), mode));
}

/**
 * @brief Rejects blending colors of two different types along a hue arc.
 *
 * As for the overload without a HueMode, convert one color into the
 * other's type first, into the wider gamut so nothing clips.
 *
 * @tparam Space The color space the colors would be blended in
 * @tparam From Color type of @p from
 * @tparam To Color type of @p to, other than @p From
 * @param[in] from The color at @p amount 0
 * @param[in] to The color at @p amount 1
 * @param[in] amount Fraction of the way from @p from to @p to
 * @param[in] mode The arc of the hue circle to follow
 * @param[in] space The working space
 * @return Nothing: the function is deleted.
 */
template <typename Space = void, ColorType From, ColorType To>
    requires(!std::same_as<From, To>)
From mix(const From& from, const To& to, float amount, HueMode mode, In<Space> space = {}) = delete;

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
