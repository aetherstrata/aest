/**
 * @copyright Copyright (c) 2026 Davide Pierotti, MIT License (see LICENSE.md)
 * @author Davide Pierotti
 * @file Distance.cppm
 * @brief Partition `aest.colors:distance`: distance metrics between colors.
 */
module;
#include <cmath>
#include <concepts>
export module aest.colors:distance;

import :color;
import :converter;
import :space;

namespace aest::colors
{
/**
 * @brief Euclidean distance between the three channels of two colors,
 *        ignoring alpha.
 * @tparam Space A color space whose channels form Cartesian coordinates
 * @param[in] color1 First color
 * @param[in] color2 Second color
 * @return The distance between the channels.
 */
template <ColorType Space>
float channel_distance(const Space& color1, const Space& color2)
{
    const float diff1 = color1.ch1() - color2.ch1();
    const float diff2 = color1.ch2() - color2.ch2();
    const float diff3 = color1.ch3() - color2.ch3();

    return std::sqrt(diff1 * diff1 + diff2 * diff2 + diff3 * diff3);
}
} // namespace aest::colors

export namespace aest::colors
{
/**
 * @brief Measures the Euclidean distance between two colors of a color space.
 *
 * Specialize it for a color space to make it a EuclideanSpace, usable with
 * euclidean(). Only spaces whose channels are Cartesian coordinates qualify:
 * the hue of HSL and HSV is an angle, so they are left out.
 *
 * @tparam Space The color space.
 */
template <ColorType Space>
struct Euclidean;

/** @brief Euclidean distance in gamma-encoded sRGB. */
template <>
struct Euclidean<SRGB>
{
    /**
     * @brief Distance between the encoded channels of two colors, ignoring alpha.
     * @param[in] color1 First color
     * @param[in] color2 Second color
     * @return The distance, in the 0-sqrt(3) range.
     */
    static float distance(const SRGB& color1, const SRGB& color2)
    {
        return channel_distance(color1, color2);
    }
};

/** @brief Euclidean distance in linear sRGB. */
template <>
struct Euclidean<LinSRGB>
{
    /**
     * @brief Distance between the light of two colors, ignoring alpha.
     * @param[in] color1 First color
     * @param[in] color2 Second color
     * @return The distance, in the 0-sqrt(3) range.
     */
    static float distance(const LinSRGB& color1, const LinSRGB& color2)
    {
        return channel_distance(color1, color2);
    }
};

/**
 * @brief A color space in which euclidean() can measure distances through
 * Euclidean.
 * @tparam T The type to check
 */
template <typename T>
concept EuclideanSpace = ColorType<T> && requires(const T& color) {
    { Euclidean<T>::distance(color, color) } -> std::same_as<float>;
};

/**
 * @brief Euclidean distance between two colors.
 *
 * Both colors are converted to @p Space and the distance between their
 * channels is measured there, ignoring alpha. The default, gamma-encoded
 * SRGB, is closer to how different the colors look; `in<LinSRGB>` measures
 * the difference in light.
 *
 * @tparam Space The color space the distance is measured in
 * @tparam Color1 Color type of @p color1
 * @tparam Color2 Color type of @p color2
 * @param[in] color1 First color
 * @param[in] color2 Second color
 * @param[in] space The working space, e.g. `in<LinSRGB>`; SRGB when omitted
 * @return The distance, in the 0-sqrt(3) range for SRGB and LinSRGB.
 */
template <EuclideanSpace Space = SRGB, ColorType Color1, ColorType Color2>
    requires ConvertibleTo<Space, Color1> && ConvertibleTo<Space, Color2>
float euclidean(const Color1& color1, const Color2& color2, [[maybe_unused]] In<Space> space = {})
{
    return Euclidean<Space>::distance(convert<Space>(color1), convert<Space>(color2));
}
} // namespace aest::colors
