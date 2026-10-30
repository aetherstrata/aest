/**
 * @copyright Copyright (c) 2026 Davide Pierotti, MIT License (see LICENSE.md)
 * @author Davide Pierotti
 * @file Converter.cppm
 * @brief Partition `aest.colors:converter`: conversions between color spaces.
 */
module;
#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <numeric>
#include <type_traits>
export module aest.colors:converter;

import :color;

namespace aest::colors
{
/** @brief Exponent of the sRGB transfer function. */
constexpr float SRGB_GAMMA = 2.4f;

/**
 * @brief Hue of an sRGB color, shared by the HSL and HSV conversions.
 * @param[in] r Red channel value
 * @param[in] g Green channel value
 * @param[in] b Blue channel value
 * @param[in] max The largest of @p r, @p g and @p b
 * @param[in] delta @p max minus the smallest channel value
 * @return The hue in degrees, in the [-60, 300) range; 0 when @p delta is 0.
 *         The color constructors wrap negative hues.
 */
constexpr float hue_of(
    const float r, const float g, const float b, const float max, const float delta)
{
    if (delta == 0)
        return 0;
    if (max == r)
        return 60.0f * ((g - b) / delta);
    if (max == g)
        return 60.0f * ((b - r) / delta + 2);
    return 60.0f * ((r - g) / delta + 4);
}

/**
 * @brief Builds an sRGB color from a hue, a chroma and an offset, shared by
 *        the HSL and HSV conversions.
 * @param[in] hue The hue in degrees, in the [0, 360) range
 * @param[in] chroma Difference between the largest and smallest channel
 * @param[in] offset Value of the smallest channel
 * @param[in] alpha Alpha channel value
 * @return The SRGB color.
 */
constexpr SRGB rgb_from_hue(const float hue,
                            const float chroma,
                            const float offset,
                            const float alpha)
{
    // Distance from the middle of the current pair of 60 degree sectors;
    // std::fmod and std::fabs are constexpr only since C++23
    const float sector   = hue / 60.0f;
    const float distance = sector - 2 * static_cast<std::int32_t>(sector / 2) - 1;
    const float second   = chroma * (1 - (distance < 0 ? -distance : distance));

    float r = 0, g = 0, b = 0;
    if (hue < 60)
    {
        r = chroma;
        g = second;
    }
    else if (hue < 120)
    {
        r = second;
        g = chroma;
    }
    else if (hue < 180)
    {
        g = chroma;
        b = second;
    }
    else if (hue < 240)
    {
        g = second;
        b = chroma;
    }
    else if (hue < 300)
    {
        r = second;
        b = chroma;
    }
    else
    {
        r = chroma;
        b = second;
    }

    return SRGB{r + offset, g + offset, b + offset, alpha};
}
} // namespace aest::colors

export namespace aest::colors
{
/**
 * @brief Converts colors from one color space to another.
 *
 * The primary template is only declared: specialize it for each conversion a
 * color space provides directly, with a static `convert` method. Every space
 * needs at least the conversions to and from its ParentSpace; convert()
 * reaches every other space of the tree through them. Other specializations
 * are shortcuts, preferred over the path through the tree.
 *
 * Specializations must be visible before the conversion is first used:
 * concepts such as ConvertibleTo are evaluated once per translation unit.
 *
 * @tparam ColorOut The desired output color space.
 * @tparam ColorIn The input color space.
 */
template <ColorType ColorOut, ColorType ColorIn>
struct Converter;

/**
 * @brief Names the color space a space converts through, joining every space
 *        in one tree.
 *
 * The primary template is only declared: a space without a specialization is
 * a root. A specialization holds the parent as `type`, and requires the
 * Converter specializations to and from that parent.
 *
 * @tparam Space The color space.
 */
template <ColorType Space>
struct ParentSpace;
} // namespace aest::colors

namespace aest::colors
{
/**
 * @brief A color space with a parent in the conversion tree.
 * @tparam T The type to check
 */
template <typename T>
concept HasParentSpace = ColorType<T> && requires { typename ParentSpace<T>::type; };

/**
 * @brief A pair of color spaces with a Converter specialization between them.
 * @tparam ColorOut The output color space
 * @tparam ColorIn The input color space
 */
template <typename ColorOut, typename ColorIn>
concept DirectConverter = requires(const ColorIn& color) {
    { Converter<ColorOut, ColorIn>::convert(color) } -> std::same_as<ColorOut>;
};

/**
 * @brief Whether @p Ancestor is @p Space or one of its ancestors in the
 *        conversion tree.
 * @tparam Ancestor The candidate ancestor
 * @tparam Space The color space whose ancestors are searched
 */
template <typename Ancestor, typename Space>
constexpr bool IsAncestorOrSelf = [] {
    if constexpr (std::is_same_v<Ancestor, Space>)
        return true;
    else if constexpr (HasParentSpace<Space>)
        return IsAncestorOrSelf<Ancestor, typename ParentSpace<Space>::type>;
    else
        return false;
}();

/**
 * @brief The closest common ancestor of two color spaces, as `type`; no
 *        `type` when they are in different trees.
 * @tparam From The space whose ancestors are tried, closest first
 * @tparam To The other space
 */
template <typename From, typename To>
struct CommonAncestor
{};

/**
 * @brief The common ancestor when @p From is itself an ancestor of @p To.
 * @tparam From The space whose ancestors are tried, closest first
 * @tparam To The other space
 */
template <typename From, typename To>
    requires IsAncestorOrSelf<From, To>
struct CommonAncestor<From, To>
{
    /** @brief The common ancestor. */
    using type = From;
};

/**
 * @brief Moves the search to the parent of @p From.
 * @tparam From The space whose ancestors are tried, closest first
 * @tparam To The other space
 */
template <typename From, typename To>
    requires(!IsAncestorOrSelf<From, To> && HasParentSpace<From>)
struct CommonAncestor<From, To> : CommonAncestor<typename ParentSpace<From>::type, To>
{};

/**
 * @brief Converts a color up the tree, parent by parent, to one of its ancestors.
 * @tparam Ancestor The ancestor to reach
 * @tparam Space The color space of @p color
 * @param[in] color The color to convert
 * @return The color in @p Ancestor.
 */
template <ColorType Ancestor, ColorType Space>
constexpr Ancestor ascend(const Space& color)
{
    if constexpr (std::is_same_v<Ancestor, Space>)
        return color;
    else
    {
        using Parent = typename ParentSpace<Space>::type;
        return ascend<Ancestor>(Converter<Parent, Space>::convert(color));
    }
}

/**
 * @brief Converts a color down the tree, child by child, to one of its descendants.
 * @tparam Descendant The descendant to reach
 * @tparam Space The color space of @p color, an ancestor of @p Descendant
 * @param[in] color The color to convert
 * @return The color in @p Descendant.
 */
template <ColorType Descendant, ColorType Space>
constexpr Descendant descend(const Space& color)
{
    if constexpr (std::is_same_v<Descendant, Space>)
        return color;
    else
    {
        using Parent = typename ParentSpace<Descendant>::type;
        return Converter<Descendant, Parent>::convert(descend<Parent>(color));
    }
}
} // namespace aest::colors

export namespace aest::colors
{
/**
 * @brief A color space that convert() can convert @p ColorIn to: directly
 *        through a Converter, or along the conversion tree.
 * @tparam ColorOut The output color space
 * @tparam ColorIn The input color space
 */
template <typename ColorOut, typename ColorIn>
concept ConvertibleTo =
    ColorType<ColorOut> && ColorType<ColorIn> && (DirectConverter<ColorOut, ColorIn> || requires {
        typename CommonAncestor<ColorIn, ColorOut>::type;
    });

/**
 * @brief Convert a color from one color space to another.
 *
 * Uses the Converter specialization between the two spaces when there is
 * one. Otherwise the color goes up the conversion tree to the closest
 * ancestor the spaces share, and down from it to @p ColorOut; each step
 * clamps to the range of its space.
 *
 * @tparam ColorOut The desired color space
 * @tparam ColorIn The current color space
 * @param[in] color The color to convert
 * @return A new color representing the input value in another color space
 */
template <ColorType ColorOut, ColorType ColorIn>
    requires ConvertibleTo<ColorOut, ColorIn>
constexpr ColorOut convert(const ColorIn& color)
{
    if constexpr (DirectConverter<ColorOut, ColorIn>)
        return Converter<ColorOut, ColorIn>::convert(color);
    else
    {
        using Ancestor = typename CommonAncestor<ColorIn, ColorOut>::type;
        return descend<ColorOut>(ascend<Ancestor>(color));
    }
}

/**
 * @brief Identity conversion between a color space and itself.
 * @tparam ColorOut The color space, same as @p ColorIn
 * @tparam ColorIn The color space, same as @p ColorOut
 */
template <ColorType ColorOut, ColorType ColorIn>
    requires(std::is_same_v<ColorOut, ColorIn>)
struct Converter<ColorOut, ColorIn>
{
    /**
     * @brief Returns the color unchanged.
     * @param[in] color The color to convert
     * @return A copy of @p color
     */
    static constexpr auto convert(const ColorIn& color) -> ColorOut { return color; }
};

/** @brief SRGB converts through LinSRGB, the root of the conversion tree. */
template <>
struct ParentSpace<SRGB>
{
    /** @brief The parent space. */
    using type = LinSRGB;
};

/** @brief HSL converts through SRGB. */
template <>
struct ParentSpace<HSL>
{
    /** @brief The parent space. */
    using type = SRGB;
};

/** @brief HSV converts through SRGB. */
template <>
struct ParentSpace<HSV>
{
    /** @brief The parent space. */
    using type = SRGB;
};

/** @brief Converts HSL to SRGB. */
template <>
struct Converter<SRGB, HSL>
{
    /**
     * @brief Converts @p color to SRGB.
     * @param[in] color The color to convert
     * @return The same color in SRGB, with the same alpha
     */
    static constexpr SRGB convert(const HSL& color)
    {
        const float l = color.lightness();
        const float c = 2 * std::min(l, 1 - l) * color.saturation();
        return rgb_from_hue(color.hue(), c, l - c / 2, color.alpha());
    }
};

/** @brief Converts SRGB to HSL. */
template <>
struct Converter<HSL, SRGB>
{
    /**
     * @brief Converts @p color to HSL.
     * @param[in] color The color to convert
     * @return The same color in HSL, with the same alpha
     */
    static constexpr HSL convert(const SRGB& color)
    {
        const float r = color.red();
        const float g = color.green();
        const float b = color.blue();

        const float max   = std::max({r, g, b});
        const float min   = std::min({r, g, b});
        const float delta = max - min;

        const float l = std::midpoint(max, min);
        const float s = delta == 0 ? 0 : delta / (2 * std::min(l, 1 - l));

        return HSL{hue_of(r, g, b, max, delta), s, l, color.alpha()};
    }
};

/** @brief Converts SRGB to HSV. */
template <>
struct Converter<HSV, SRGB>
{
    /**
     * @brief Converts @p color to HSV.
     * @param[in] color The color to convert
     * @return The same color in HSV, with the same alpha
     */
    static constexpr HSV convert(const SRGB& color)
    {
        const float r = color.red();
        const float g = color.green();
        const float b = color.blue();

        const float max   = std::max({r, g, b});
        const float min   = std::min({r, g, b});
        const float delta = max - min;

        const float s = max == 0.0f ? 0.0f : delta / max;

        return HSV{hue_of(r, g, b, max, delta), s, max, color.alpha()};
    }
};

/** @brief Converts HSV to SRGB. */
template <>
struct Converter<SRGB, HSV>
{
    /**
     * @brief Converts @p color to SRGB.
     * @param[in] color The color to convert
     * @return The same color in SRGB, with the same alpha
     */
    static constexpr SRGB convert(const HSV& color)
    {
        const float v = color.brightness();
        const float c = v * color.saturation();
        return rgb_from_hue(color.hue(), c, v - c, color.alpha());
    }
};

/** @brief Converts HSL to HSV, a shortcut that keeps the hue exactly. */
template <>
struct Converter<HSV, HSL>
{
    /**
     * @brief Converts @p color to HSV, keeping the hue.
     * @param[in] color The color to convert
     * @return The same color in HSV, with the same alpha
     */
    static constexpr HSV convert(const HSL& color)
    {
        const float l = color.lightness();
        const float v = l + color.saturation() * std::min(l, 1 - l);
        const float s = v == 0 ? 0 : 2 * (1 - l / v);

        return HSV{color.hue(), s, v, color.alpha()};
    }
};

/** @brief Converts HSV to HSL, a shortcut that keeps the hue exactly. */
template <>
struct Converter<HSL, HSV>
{
    /**
     * @brief Converts @p color to HSL, keeping the hue.
     * @param[in] color The color to convert
     * @return The same color in HSL, with the same alpha
     */
    static constexpr HSL convert(const HSV& color)
    {
        const float v = color.brightness();
        const float l = v * (1 - color.saturation() / 2);
        const float s = l == 0 || l == 1 ? 0 : (v - l) / std::min(l, 1 - l);

        return HSL{color.hue(), s, l, color.alpha()};
    }
};

/** @brief Converts sRGB to linear sRGB by removing the gamma encoding. */
template <>
struct Converter<LinSRGB, SRGB>
{
    /**
     * @brief Convert a color channel value from sRGB to linear space.
     * @param[in] value The sRGB value in the 0-1 range.
     * @return The linear value in the 0-1 range.
     * @note This function is implemented according to the sRGB standard,
     * as defined by W3C in "https://www.w3.org/Graphics/Color/sRGB.html".
     * @note Not constexpr: std::pow is constexpr only since C++26.
     */
    static float srgb_to_linear(const float value)
    {
        if (value == 1)
            return 1;

        return value <= 0.04045f ? value / 12.92f : std::pow((value + 0.055f) / 1.055f, SRGB_GAMMA);
    }

    /**
     * @brief Applies the inverse sRGB transfer function to each color channel.
     * @param[in] color The color to convert
     * @return The same color in linear sRGB, with the same alpha
     */
    static LinSRGB convert(const SRGB& color)
    {
        return LinSRGB{srgb_to_linear(color.red()), srgb_to_linear(color.green()),
                       srgb_to_linear(color.blue()), color.alpha()};
    }
};

/** @brief Converts linear sRGB to sRGB by applying the gamma encoding. */
template <>
struct Converter<SRGB, LinSRGB>
{
    /**
     * @brief Convert a color channel value from linear space to sRGB.
     * @param[in] value The linear value, 0 or greater.
     * @return The sRGB value in the 0-1 range: values from 1 up give 1.
     * @note This function is implemented according to the sRGB standard, as defined
     *       by W3C in "https://www.w3.org/Graphics/Color/sRGB.html".
     * @note Not constexpr: std::pow is constexpr only since C++26.
     */
    static float linear_to_srgb(const float value)
    {
        if (value >= 1.f)
            return 1.f;

        return value <= 0.0031308f ? 12.92f * value
                                   : 1.055f * std::pow(value, 1.0f / SRGB_GAMMA) - 0.055f;
    }

    /**
     * @brief Applies the sRGB transfer function to each color channel.
     * @param[in] color The color to convert
     * @return The same color in sRGB, with the same alpha
     */
    static SRGB convert(const LinSRGB& color)
    {
        return SRGB{linear_to_srgb(color.red()), linear_to_srgb(color.green()),
                    linear_to_srgb(color.blue()), color.alpha()};
    }
};

} // namespace aest::colors
