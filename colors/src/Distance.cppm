/**
 * @copyright Copyright (c) 2026 Davide Pierotti, MIT License (see LICENSE.md)
 * @author Davide Pierotti
 * @file Distance.cppm
 * @brief Partition `aest.colors:distance`: distance metrics between colors,
 *        Euclidean and the CIE color difference formulas.
 */
module;
#include <cmath>
#include <concepts>
#include <cstdint>
export module aest.colors:distance;

import :cie_lab;
import :color;
import :converter;
import :gamut;
import :oklab;
import :transfer;
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

/**
 * @brief Measures the Euclidean distance between two colors of a color space.
 *
 * Specialize it for a color space to make it a EuclideanSpace, usable with
 * euclidean(). Only spaces whose channels are Cartesian coordinates qualify:
 * the hue of HSL and HSV is an angle, so they are left out.
 *
 * @tparam Space The color space.
 */
export template <ColorType Space>
struct Euclidean;

/**
 * @brief Euclidean distance in an RGB space: between the encoded values of
 *        encoded spaces, between the light of linear ones.
 * @tparam Gamut The gamut tag
 * @tparam Transfer The transfer function tag
 */
template <RGBGamut Gamut, TransferFunction Transfer>
struct Euclidean<BasicRGB<Gamut, Transfer>>
{
    /**
     * @brief Distance between the channels of two colors, ignoring alpha.
     * @param[in] color1 First color
     * @param[in] color2 Second color
     * @return The distance, 0 or greater: in the 0-sqrt(3) range when no
     *         channel exceeds 1, unbounded for HDR colors.
     */
    static float distance(const BasicRGB<Gamut, Transfer>& color1,
                          const BasicRGB<Gamut, Transfer>& color2)
    {
        return channel_distance(color1, color2);
    }
};

/**
 * @brief Euclidean distance in OKLab: ΔE_OK, the color difference of CSS
 *        Color 4, where equal distances look about equally different.
 */
template <>
struct Euclidean<OKLab>
{
    /**
     * @brief Perceived difference between two colors, ignoring alpha.
     * @param[in] color1 First color
     * @param[in] color2 Second color
     * @return The distance, 0 or greater: about 0.02 is just noticeable,
     *         and black to white is 1.
     */
    static float distance(const OKLab& color1, const OKLab& color2)
    {
        return channel_distance(color1, color2);
    }
};

/**
 * @brief Euclidean distance in CIELab: ΔE*ab, the CIE 1976 color difference
 *        that de_76() measures.
 */
template <>
struct Euclidean<CIELab>
{
    /**
     * @brief CIE 1976 difference between two colors, ignoring alpha.
     * @param[in] color1 First color
     * @param[in] color2 Second color
     * @return The distance, 0 or greater: about 2.3 is just noticeable, and
     *         black to white is 100.
     */
    static float distance(const CIELab& color1, const CIELab& color2)
    {
        return channel_distance(color1, color2);
    }
};

/**
 * @brief A color space in which euclidean() can measure distances through
 * Euclidean.
 * @tparam T The type to check
 */
export template <typename T>
concept EuclideanSpace = ColorType<T> && requires(const T& color) {
    { Euclidean<T>::distance(color, color) } -> std::same_as<float>;
};

/**
 * @brief Euclidean distance between two colors.
 *
 * Both colors are converted to @p Space and the distance between their
 * channels is measured there, ignoring alpha. The default, OKLab, gives
 * ΔE_OK, where equal distances look about equally different;
 * `euclidean<SRGB>(color1, color2)` measures the encoded values, and
 * `euclidean<LinSRGB>(color1, color2)` the difference in light.
 *
 * @tparam Space The color space the distance is measured in
 * @tparam Color1 Color type of @p color1
 * @tparam Color2 Color type of @p color2
 * @param[in] color1 First color
 * @param[in] color2 Second color
 * @param[in] space The working space, e.g. `in<LinSRGB>`; OKLab when omitted
 * @return The distance: 1 from black to white in OKLab, and in the
 *         0-sqrt(3) range for SRGB and for LinSRGB colors without HDR values.
 */
export template <EuclideanSpace Space = OKLab, ColorType Color1, ColorType Color2>
    requires ConvertibleTo<Space, Color1> && ConvertibleTo<Space, Color2>
float euclidean(const Color1& color1, const Color2& color2, [[maybe_unused]] In<Space> space = {})
{
    return Euclidean<Space>::distance(convert<Space>(color1), convert<Space>(color2));
}

/**
 * @brief The CIE 1976 color difference ΔE*ab: the Euclidean distance in
 *        CIELab.
 *
 * The first CIE formula and the simplest: it overstates differences between
 * saturated colors, which de_94() and de_2000() correct. It is
 * `euclidean<CIELab>(color1, color2)`.
 *
 * @tparam Color1 Color type of @p color1
 * @tparam Color2 Color type of @p color2
 * @param[in] color1 First color
 * @param[in] color2 Second color
 * @return The difference, ignoring alpha: 0 or greater, 100 from black to
 *         white, about 2.3 just noticeable.
 */
export template <ColorType Color1, ColorType Color2>
    requires ConvertibleTo<CIELab, Color1> && ConvertibleTo<CIELab, Color2>
float de_76(const Color1& color1, const Color2& color2)
{
    return euclidean<CIELab>(color1, color2);
}

/**
 * @brief The application whose weights de_94() uses, as CIE 116-1995
 *        defines them.
 */
export enum class CIE94Application : std::uint8_t
{
    graphic_arts, ///< kL = 1, K1 = 0.045, K2 = 0.015: images, prints and screens.
    textiles,     ///< kL = 2, K1 = 0.048, K2 = 0.014: lightness counts half as much.
};

/**
 * @brief The CIE 1994 color difference ΔE*94.
 *
 * It weights the differences in lightness, chroma and hue of CIELCH: the
 * chroma and hue differences count less as the chroma of @p reference grows,
 * where the eye tells them apart less. The weights depend on @p reference
 * alone, so the formula is not symmetric: swapping the colors gives another
 * result unless their chromas are equal.
 *
 * @tparam Color1 Color type of @p reference
 * @tparam Color2 Color type of @p sample
 * @param[in] reference The reference color, whose chroma sets the weights
 * @param[in] sample The color compared to @p reference
 * @param[in] application The weights to use, those for graphic arts by default
 * @return The difference, ignoring alpha: 0 or greater, about 1 just noticeable.
 */
export template <ColorType Color1, ColorType Color2>
    requires ConvertibleTo<CIELab, Color1> && ConvertibleTo<CIELab, Color2>
float de_94(const Color1& reference,
            const Color2& sample,
            const CIE94Application application = CIE94Application::graphic_arts)
{
    const bool textiles = application == CIE94Application::textiles;
    const double kL     = textiles ? 2.0 : 1.0;
    const double k1     = textiles ? 0.048 : 0.045;
    const double k2     = textiles ? 0.014 : 0.015;

    const CIELab lab1 = convert<CIELab>(reference);
    const CIELab lab2 = convert<CIELab>(sample);

    const double chroma1   = std::hypot(double{lab1.a()}, double{lab1.b()});
    const double chroma2   = std::hypot(double{lab2.a()}, double{lab2.b()});
    const double lightness = double{lab1.lightness()} - lab2.lightness();
    const double chroma    = chroma1 - chroma2;
    const double diffA     = double{lab1.a()} - lab2.a();
    const double diffB     = double{lab1.b()} - lab2.b();

    // ΔH² is what of Δa² + Δb² the chroma does not explain; rounding can push it below 0
    const double hueSquared = std::fmax(diffA * diffA + diffB * diffB - chroma * chroma, 0.0);

    const double lightnessTerm = lightness / kL;
    const double chromaTerm    = chroma / (1 + k1 * chroma1);
    const double hueScale      = 1 + k2 * chroma1;
    return static_cast<float>(std::sqrt(lightnessTerm * lightnessTerm + chromaTerm * chromaTerm
                                        + hueSquared / (hueScale * hueScale)));
}

/**
 * @brief Hue angle of a CIEDE2000 a', b' pair.
 * @param[in] a The a' channel, a* rescaled by CIEDE2000
 * @param[in] b The b* channel
 * @return The hue in degrees, in the [0, 360) range; 0 when both are 0.
 */
double ciede2000_hue(const double a, const double b)
{
    if (a == 0 && b == 0)
        return 0;
    const double hue = std::atan2(b, a) * DEGREES_PER_RADIAN;
    return hue < 0 ? hue + 360 : hue;
}

/**
 * @brief The CIEDE2000 chroma weight `sqrt(C^7 / (C^7 + 25^7))`.
 * @param[in] chroma A mean chroma
 * @return The weight in the 0-1 range, near 1 for saturated colors.
 */
double ciede2000_chroma_weight(const double chroma)
{
    // 25^7: the weight is sqrt(1/2) at chroma 25
    constexpr double pivot = 6103515625.0;

    const double power = std::pow(chroma, 7);
    return std::sqrt(power / (power + pivot));
}

/**
 * @brief The CIEDE2000 color difference ΔE00, the current CIE formula.
 *
 * It refines de_94() to follow the eye more closely: it rescales a* so
 * that grays are judged evenly, weights lightness by its distance from the
 * mid-gray, weights hue by the hue itself, and rotates the chroma and hue
 * differences against each other around blue, where CIELab is least even.
 * Unlike de_94(), it is symmetric. The parametric factors kL, kC and
 * kH are all 1, the CIE reference conditions. This follows CIE 142-2001,
 * with the hue rules that Sharma, Wu and Dalal (2005) made exact.
 *
 * @tparam Color1 Color type of @p color1
 * @tparam Color2 Color type of @p color2
 * @param[in] color1 First color
 * @param[in] color2 Second color
 * @return The difference, ignoring alpha: 0 or greater, 100 from black to
 *         white, about 1 just noticeable.
 */
export template <ColorType Color1, ColorType Color2>
    requires ConvertibleTo<CIELab, Color1> && ConvertibleTo<CIELab, Color2>
float de_2000(const Color1& color1, const Color2& color2)
{
    const CIELab lab1 = convert<CIELab>(color1);
    const CIELab lab2 = convert<CIELab>(color2);
    const double l1 = lab1.lightness(), a1 = lab1.a(), b1 = lab1.b();
    const double l2 = lab2.lightness(), a2 = lab2.a(), b2 = lab2.b();

    // Rescale a* so that the differences between near-grays are not understated
    const double meanChroma = (std::hypot(a1, b1) + std::hypot(a2, b2)) / 2;
    const double aScale     = 1.5 - ciede2000_chroma_weight(meanChroma) / 2;
    const double primeA1    = a1 * aScale;
    const double primeA2    = a2 * aScale;
    const double chroma1    = std::hypot(primeA1, b1);
    const double chroma2    = std::hypot(primeA2, b2);
    const double hue1       = ciede2000_hue(primeA1, b1);
    const double hue2       = ciede2000_hue(primeA2, b2);

    // The hue difference takes the shorter arc, and is 0 when either color is gray
    const bool anyGray = chroma1 * chroma2 == 0;
    double hueDiff     = hue2 - hue1;
    if (hueDiff > 180)
        hueDiff -= 360;
    else if (hueDiff < -180)
        hueDiff += 360;
    if (anyGray)
        hueDiff = 0;

    const double lightnessDiff = l2 - l1;
    const double chromaDiff    = chroma2 - chroma1;
    const double hueDistance =
        2 * std::sqrt(chroma1 * chroma2) * std::sin(hueDiff / 2 / DEGREES_PER_RADIAN);

    // The mean hue is the middle of the shorter arc, or the sum when a color is gray
    const double hueSum = hue1 + hue2;
    double meanHue      = hueSum;
    if (!anyGray)
    {
        if (std::fabs(hue1 - hue2) <= 180)
            meanHue = hueSum / 2;
        else
            meanHue = hueSum < 360 ? (hueSum + 360) / 2 : (hueSum - 360) / 2;
    }

    const auto cosine = [](const double degrees) {
        return std::cos(degrees / DEGREES_PER_RADIAN);
    };

    const double meanLightness   = (l1 + l2) / 2;
    const double meanPrimeChroma = (chroma1 + chroma2) / 2;
    const double hueWeight       = 1 - 0.17 * cosine(meanHue - 30) + 0.24 * cosine(2 * meanHue)
                                 + 0.32 * cosine(3 * meanHue + 6) - 0.20 * cosine(4 * meanHue - 63);

    const double fromMidGray     = (meanLightness - 50) * (meanLightness - 50);
    const double lightnessScale  = 1 + 0.015 * fromMidGray / std::sqrt(20 + fromMidGray);
    const double chromaScale     = 1 + 0.045 * meanPrimeChroma;
    const double hueScale        = 1 + 0.015 * meanPrimeChroma * hueWeight;
    const double blueOffset      = (meanHue - 275) / 25;
    const double rotationDegrees = 30 * std::exp(-blueOffset * blueOffset);
    const double rotation        = -std::sin(2 * rotationDegrees / DEGREES_PER_RADIAN) * 2
                                 * ciede2000_chroma_weight(meanPrimeChroma);

    const double lightnessTerm = lightnessDiff / lightnessScale;
    const double chromaTerm    = chromaDiff / chromaScale;
    const double hueTerm       = hueDistance / hueScale;
    return static_cast<float>(std::sqrt(lightnessTerm * lightnessTerm + chromaTerm * chromaTerm
                                        + hueTerm * hueTerm + rotation * chromaTerm * hueTerm));
}
} // namespace aest::colors
