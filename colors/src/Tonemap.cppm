/**
 * @copyright Copyright (c) 2026 Davide Pierotti, MIT License (see LICENSE.md)
 * @author Davide Pierotti
 * @file Tonemap.cppm
 * @brief Partition `aest.colors:tonemap`: operators that compress HDR linear
 *        light into the 0-1 range of a display.
 */
module;
#include <array>
export module aest.colors:tonemap;

import aest.blas;
import :color;
import :converter;
import :gamut;

namespace aest::colors::tonemap
{
/**
 * @brief Input matrix of the ACES fit by Stephen Hill: linear sRGB to the
 *        ACES AP1 primaries, with the saturation step of the RRT folded in.
 */
constexpr Matrix3 ACES_INPUT = {{
    {0.59719, 0.35458, 0.04823},
    {0.07600, 0.90834, 0.01566},
    {0.02840, 0.13383, 0.83777},
}};

/**
 * @brief Output matrix of the ACES fit by Stephen Hill: back from the ODT to
 *        linear sRGB, with the saturation step of the ODT folded in.
 */
constexpr Matrix3 ACES_OUTPUT = {{
    {1.60475, -0.53108, -0.07367},
    {-0.10208, 1.10813, -0.00605},
    {-0.00327, -0.07276, 1.07602},
}};

/**
 * @brief Rational fit of the ACES RRT and sRGB ODT curve, by Stephen Hill.
 * @param[in] value One channel, after ACES_INPUT
 * @return The tonemapped channel, before ACES_OUTPUT.
 */
constexpr double rrt_and_odt_fit(const double value)
{
    const double numerator   = value * (value + 0.0245786) - 0.000090537;
    const double denominator = value * (0.983729 * value + 0.4329510) + 0.238081;
    return numerator / denominator;
}
} // namespace aest::colors::tonemap

/** @brief Tonemapping operators, from HDR linear light to the 0-1 range of a display. */
export namespace aest::colors::tonemap
{
/**
 * @brief Reinhard tonemapping, `c / (1 + c)` on each channel.
 *
 * Every channel goes from [0, ∞) into [0, 1), so the result never clips,
 * but white is reached only at infinity. Working per channel, it also
 * desaturates bright colors toward white. Alpha is kept.
 *
 * @tparam Color A linear RGB space
 * @param[in] color The HDR color
 * @return The tonemapped color, every channel in [0, 1).
 */
template <LinearRGBSpace Color>
constexpr Color reinhard(const Color& color)
{
    return Color{color.red() / (1 + color.red()), color.green() / (1 + color.green()),
                 color.blue() / (1 + color.blue()), color.alpha()};
}

/**
 * @brief Extended Reinhard tonemapping, `c (1 + c / w²) / (1 + c)` on each
 *        channel, which maps @p white to 1.
 *
 * Channels from 0 to @p white go into [0, 1]; brighter ones go above 1 and
 * clip when encoded. Pick @p white as the brightest value of the scene, or
 * the peak of the display over REFERENCE_WHITE_NITS. Alpha is kept.
 *
 * @tparam Color A linear RGB space
 * @param[in] color The HDR color
 * @param[in] white The linear value that becomes 1
 * @return The tonemapped color.
 * @pre @p white is greater than 0.
 */
template <LinearRGBSpace Color>
constexpr Color reinhard_extended(const Color& color, const float white)
{
    const float whiteSquared = white * white;
    const auto map           = [whiteSquared](const float channel) -> float {
        return channel * (1 + channel / whiteSquared) / (1 + channel);
    };
    return Color{map(color.red()), map(color.green()), map(color.blue()), color.alpha()};
}

/**
 * @brief Fitted ACES tonemapping by Stephen Hill: an approximation of the
 *        ACES Reference Rendering Transform and sRGB Output Device Transform.
 *
 * The fit is defined on linear sRGB, so the color is converted to LinSRGB,
 * mapped, clamped to [0, 1] and converted back: colors outside the sRGB
 * gamut clip, as the fit targets an sRGB display. It raises contrast and
 * desaturates highlights, like the full ACES transforms. Alpha is kept.
 *
 * @tparam Color A linear RGB space
 * @param[in] color The HDR color
 * @return The tonemapped color, in the sRGB gamut with channels in [0, 1].
 */
template <LinearRGBSpace Color>
constexpr Color aces_fitted(const Color& color)
{
    const LinSRGB linear = convert<LinSRGB>(color);
    const Vector3 input = blas::multiply(ACES_INPUT, {linear.red(), linear.green(), linear.blue()});
    const Vector3 fit   = {rrt_and_odt_fit(input[0]), rrt_and_odt_fit(input[1]),
                           rrt_and_odt_fit(input[2])};
    const Vector3 output = blas::multiply(ACES_OUTPUT, fit);

    return convert<Color>(LinSRGB{clamp(static_cast<float>(output[0])),
                                  clamp(static_cast<float>(output[1])),
                                  clamp(static_cast<float>(output[2])), color.alpha()});
}
} // namespace aest::colors::tonemap
