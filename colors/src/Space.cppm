/**
 * @copyright Copyright (c) 2026 Davide Pierotti, MIT License (see LICENSE.md)
 * @author Davide Pierotti
 * @file Space.cppm
 * @brief Partition `aest.colors:space`: the tag that selects the color space
 *        an operation works in, and the linear space a color follows by
 *        default.
 */
module;
#include <type_traits>
export module aest.colors:space;

import :color;
import :cylindrical;
import :gamut;
import :transfer;

export namespace aest::colors
{
/**
 * @brief Tag naming the color space an operation works in.
 *
 * `In<void>` is the default of the operations whose working space follows
 * their input, such as mix(): it stands for the LinearSpaceOf the input.
 *
 * @tparam Space The color space, or `void` for the input's default.
 */
template <typename Space>
struct In
{};

/**
 * @brief Selects the color space an operation works in, e.g.
 * `lighten(color, 0.2f, in<HSL>)`.
 * @tparam Space The color space.
 */
template <ColorType Space>
inline constexpr In<Space> in{};

/**
 * @brief The linear space that operations in linear light, such as mix()
 *        and over(), use for a color type when no space is given.
 *
 * It keeps the color's own gamut, so wide-gamut colors are not clipped to
 * sRGB: the linear space of the gamut for an RGB space, and for HSL and HSV
 * the one of their RGB space. XYZ is linear already. Any other type gets
 * LinSRGB; specialize the trait to give it another.
 *
 * @tparam Color The color type
 */
template <ColorType Color>
struct LinearSpaceOf
{
    /** @brief The linear space. */
    using type = LinSRGB;
};

/**
 * @brief An RGB space follows the linear space of its gamut.
 * @tparam Gamut The gamut tag
 * @tparam Transfer The transfer function tag
 */
template <RGBGamut Gamut, TransferFunction Transfer>
struct LinearSpaceOf<BasicRGB<Gamut, Transfer>>
{
    /** @brief The linear space. */
    using type = BasicRGB<Gamut, tf::Linear>;
};

/**
 * @brief HSL follows the linear space of its RGB space.
 * @tparam RGB The encoded RGB space
 */
template <BoundedRGBSpace RGB>
struct LinearSpaceOf<BasicHSL<RGB>>
{
    /** @brief The linear space. */
    using type = typename LinearSpaceOf<RGB>::type;
};

/**
 * @brief HSV follows the linear space of its RGB space.
 * @tparam RGB The encoded RGB space
 */
template <BoundedRGBSpace RGB>
struct LinearSpaceOf<BasicHSV<RGB>>
{
    /** @brief The linear space. */
    using type = typename LinearSpaceOf<RGB>::type;
};

/** @brief XYZ is linear already. */
template <>
struct LinearSpaceOf<XYZ>
{
    /** @brief The linear space. */
    using type = XYZ;
};
} // namespace aest::colors

namespace aest::colors
{
/**
 * @brief The working space of an operation that follows its input: @p Space
 *        when given, the LinearSpaceOf @p Color when it is `void`.
 * @tparam Space The space named by the caller, or `void`
 * @tparam Color The color type the operation follows
 */
template <typename Space, ColorType Color>
using WorkingSpace =
    std::conditional_t<std::is_void_v<Space>, typename LinearSpaceOf<Color>::type, Space>;
} // namespace aest::colors
