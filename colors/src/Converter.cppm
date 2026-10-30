/**
 * @copyright Copyright (c) 2026 Davide Pierotti, MIT License (see LICENSE.md)
 * @author Davide Pierotti
 * @file Converter.cppm
 * @brief Partition `aest.colors:converter`: conversions between color spaces.
 */
module;
#include <algorithm>
#include <array>
#include <concepts>
#include <cstdint>
#include <numeric>
#include <type_traits>
export module aest.colors:converter;

import aest.blas;
import :color;
import :cylindrical;
import :gamut;
import :transfer;

namespace aest::colors
{
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
 * @brief Builds an RGB color from a hue, a chroma and an offset, shared by
 *        the HSL and HSV conversions.
 * @tparam RGB The encoded RGB space to build the color in
 * @param[in] hue The hue in degrees, in the [0, 360) range
 * @param[in] chroma Difference between the largest and smallest channel
 * @param[in] offset Value of the smallest channel
 * @param[in] alpha Alpha channel value
 * @return The color in @p RGB.
 */
template <BoundedRGBSpace RGB>
constexpr RGB rgb_from_hue(const float hue,
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

    return RGB{r + offset, g + offset, b + offset, alpha};
}

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
export template <ColorType ColorOut, ColorType ColorIn>
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
export template <ColorType Space>
struct ParentSpace;

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

/**
 * @brief A color space that convert() can convert @p ColorIn to: directly
 *        through a Converter, or along the conversion tree.
 * @tparam ColorOut The output color space
 * @tparam ColorIn The input color space
 */
export template <typename ColorOut, typename ColorIn>
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
export template <ColorType ColorOut, ColorType ColorIn>
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

/**
 * @brief A linear RGB space converts through XYZ, the root of the conversion
 *        tree, with the matrix of its gamut.
 * @tparam Gamut The gamut tag
 */
template <RGBGamut Gamut>
struct ParentSpace<BasicRGB<Gamut, tf::Linear>>
{
    /** @brief The parent space. */
    using type = XYZ;
};

/**
 * @brief An encoded RGB space converts through the linear space of its gamut.
 * @tparam Gamut The gamut tag
 * @tparam Transfer The transfer function tag, other than tf::Linear
 */
template <RGBGamut Gamut, TransferFunction Transfer>
    requires(!std::same_as<Transfer, tf::Linear>)
struct ParentSpace<BasicRGB<Gamut, Transfer>>
{
    /** @brief The parent space. */
    using type = BasicRGB<Gamut, tf::Linear>;
};

/**
 * @brief HSL converts through the RGB space it is a view of.
 * @tparam RGB The encoded RGB space
 */
template <BoundedRGBSpace RGB>
struct ParentSpace<BasicHSL<RGB>>
{
    /** @brief The parent space. */
    using type = RGB;
};

/**
 * @brief HSV converts through the RGB space it is a view of.
 * @tparam RGB The encoded RGB space
 */
template <BoundedRGBSpace RGB>
struct ParentSpace<BasicHSV<RGB>>
{
    /** @brief The parent space. */
    using type = RGB;
};

/**
 * @brief Converts HSL to its RGB space.
 * @tparam RGB The encoded RGB space
 */
template <BoundedRGBSpace RGB>
struct Converter<RGB, BasicHSL<RGB>>
{
    /**
     * @brief Converts @p color to @p RGB.
     * @param[in] color The color to convert
     * @return The same color in @p RGB, with the same alpha
     */
    static constexpr RGB convert(const BasicHSL<RGB>& color)
    {
        const float l = color.lightness();
        const float c = 2 * std::min(l, 1 - l) * color.saturation();
        return rgb_from_hue<RGB>(color.hue(), c, l - c / 2, color.alpha());
    }
};

/**
 * @brief Converts an RGB space to its HSL.
 * @tparam RGB The encoded RGB space
 */
template <BoundedRGBSpace RGB>
struct Converter<BasicHSL<RGB>, RGB>
{
    /**
     * @brief Converts @p color to HSL.
     * @param[in] color The color to convert
     * @return The same color in HSL, with the same alpha
     */
    static constexpr BasicHSL<RGB> convert(const RGB& color)
    {
        const float r = color.red();
        const float g = color.green();
        const float b = color.blue();

        const float max   = std::max({r, g, b});
        const float min   = std::min({r, g, b});
        const float delta = max - min;

        const float l = std::midpoint(max, min);
        const float s = delta == 0 ? 0 : delta / (2 * std::min(l, 1 - l));

        return BasicHSL<RGB>{hue_of(r, g, b, max, delta), s, l, color.alpha()};
    }
};

/**
 * @brief Converts an RGB space to its HSV.
 * @tparam RGB The encoded RGB space
 */
template <BoundedRGBSpace RGB>
struct Converter<BasicHSV<RGB>, RGB>
{
    /**
     * @brief Converts @p color to HSV.
     * @param[in] color The color to convert
     * @return The same color in HSV, with the same alpha
     */
    static constexpr BasicHSV<RGB> convert(const RGB& color)
    {
        const float r = color.red();
        const float g = color.green();
        const float b = color.blue();

        const float max   = std::max({r, g, b});
        const float min   = std::min({r, g, b});
        const float delta = max - min;

        const float s = max == 0.0f ? 0.0f : delta / max;

        return BasicHSV<RGB>{hue_of(r, g, b, max, delta), s, max, color.alpha()};
    }
};

/**
 * @brief Converts HSV to its RGB space.
 * @tparam RGB The encoded RGB space
 */
template <BoundedRGBSpace RGB>
struct Converter<RGB, BasicHSV<RGB>>
{
    /**
     * @brief Converts @p color to @p RGB.
     * @param[in] color The color to convert
     * @return The same color in @p RGB, with the same alpha
     */
    static constexpr RGB convert(const BasicHSV<RGB>& color)
    {
        const float v = color.brightness();
        const float c = v * color.saturation();
        return rgb_from_hue<RGB>(color.hue(), c, v - c, color.alpha());
    }
};

/**
 * @brief Converts HSL to HSV over the same RGB space, a shortcut that keeps
 *        the hue exactly.
 * @tparam RGB The encoded RGB space
 */
template <BoundedRGBSpace RGB>
struct Converter<BasicHSV<RGB>, BasicHSL<RGB>>
{
    /**
     * @brief Converts @p color to HSV, keeping the hue.
     * @param[in] color The color to convert
     * @return The same color in HSV, with the same alpha
     */
    static constexpr BasicHSV<RGB> convert(const BasicHSL<RGB>& color)
    {
        const float l = color.lightness();
        const float v = l + color.saturation() * std::min(l, 1 - l);
        const float s = v == 0 ? 0 : 2 * (1 - l / v);

        return BasicHSV<RGB>{color.hue(), s, v, color.alpha()};
    }
};

/**
 * @brief Converts HSV to HSL over the same RGB space, a shortcut that keeps
 *        the hue exactly.
 * @tparam RGB The encoded RGB space
 */
template <BoundedRGBSpace RGB>
struct Converter<BasicHSL<RGB>, BasicHSV<RGB>>
{
    /**
     * @brief Converts @p color to HSL, keeping the hue.
     * @param[in] color The color to convert
     * @return The same color in HSL, with the same alpha
     */
    static constexpr BasicHSL<RGB> convert(const BasicHSV<RGB>& color)
    {
        const float v = color.brightness();
        const float l = v * (1 - color.saturation() / 2);
        const float s = l == 0 || l == 1 ? 0 : (v - l) / std::min(l, 1 - l);

        return BasicHSL<RGB>{color.hue(), s, l, color.alpha()};
    }
};

/**
 * @brief Decodes an encoded RGB space to the linear space of its gamut.
 * @tparam Gamut The gamut tag
 * @tparam Transfer The transfer function tag, other than tf::Linear
 */
template <RGBGamut Gamut, TransferFunction Transfer>
    requires(!std::same_as<Transfer, tf::Linear>)
struct Converter<BasicRGB<Gamut, tf::Linear>, BasicRGB<Gamut, Transfer>>
{
    /**
     * @brief Applies the inverse transfer function to each color channel.
     * @param[in] color The color to convert
     * @return The same color in linear light, with the same alpha
     */
    static BasicRGB<Gamut, tf::Linear> convert(const BasicRGB<Gamut, Transfer>& color)
    {
        return {Transfer::decode(color.red()), Transfer::decode(color.green()),
                Transfer::decode(color.blue()), color.alpha()};
    }
};

/**
 * @brief Encodes the linear space of a gamut with a transfer function.
 * @tparam Gamut The gamut tag
 * @tparam Transfer The transfer function tag, other than tf::Linear
 */
template <RGBGamut Gamut, TransferFunction Transfer>
    requires(!std::same_as<Transfer, tf::Linear>)
struct Converter<BasicRGB<Gamut, Transfer>, BasicRGB<Gamut, tf::Linear>>
{
    /**
     * @brief Applies the transfer function to each color channel.
     * @param[in] color The color to convert
     * @return The same color encoded, with the same alpha
     */
    static BasicRGB<Gamut, Transfer> convert(const BasicRGB<Gamut, tf::Linear>& color)
    {
        return {Transfer::encode(color.red()), Transfer::encode(color.green()),
                Transfer::encode(color.blue()), color.alpha()};
    }
};

/**
 * @brief Converts linear RGB to XYZ with the matrix of its gamut.
 * @tparam Gamut The gamut tag
 */
template <RGBGamut Gamut>
struct Converter<XYZ, BasicRGB<Gamut, tf::Linear>>
{
    /**
     * @brief Converts @p color to XYZ.
     * @param[in] color The color to convert
     * @return The same color in XYZ, with the same alpha
     */
    static constexpr XYZ convert(const BasicRGB<Gamut, tf::Linear>& color)
    {
        const Vector3 xyz =
            blas::multiply(RGB_TO_XYZ<Gamut>, {color.red(), color.green(), color.blue()});
        return XYZ{static_cast<float>(xyz[0]), static_cast<float>(xyz[1]),
                   static_cast<float>(xyz[2]), color.alpha()};
    }
};

/**
 * @brief Converts XYZ to linear RGB, clipping colors outside the gamut at 0.
 * @tparam Gamut The gamut tag
 */
template <RGBGamut Gamut>
struct Converter<BasicRGB<Gamut, tf::Linear>, XYZ>
{
    /**
     * @brief Converts @p color to the linear RGB of @p Gamut.
     * @param[in] color The color to convert
     * @return The same color in linear RGB, with the same alpha; channels a
     *         color outside the gamut would need below 0 are 0
     */
    static constexpr BasicRGB<Gamut, tf::Linear> convert(const XYZ& color)
    {
        const Vector3 rgb = blas::multiply(XYZ_TO_RGB<Gamut>, {color.x(), color.y(), color.z()});
        return {static_cast<float>(rgb[0]), static_cast<float>(rgb[1]), static_cast<float>(rgb[2]),
                color.alpha()};
    }
};
} // namespace aest::colors
