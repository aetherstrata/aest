/**
 * @copyright Copyright (c) 2026 Davide Pierotti, MIT License (see LICENSE.md)
 * @author Davide Pierotti
 * @file OkLab.cppm
 * @brief Partition `aest.colors:oklab`: the perceptual spaces OKLab and
 *        OKLCH by Björn Ottosson.
 */
module;
#include <array>
#include <cmath>
#include <numbers>
#include <string_view>
export module aest.colors:oklab;

import aest.blas;
import :color;
import :converter;
import :gamut;

namespace aest::colors
{
/**
 * @brief Matrix from XYZ, D65, to the LMS cone responses of OKLab, as
 *        recomputed by CSS Color 4 so that D65 white has no chroma.
 */
constexpr Matrix3 XYZ_TO_LMS = {{
    {0.8190224379967030, 0.3619062600528904, -0.1288737815209879},
    {0.0329836539323885, 0.9292868615863434, 0.0361446663506424},
    {0.0481771893596242, 0.2642395317527308, 0.6335478284694309},
}};

/** @brief Matrix from the cube roots of LMS to OKLab, from CSS Color 4. */
constexpr Matrix3 LMS_TO_OKLAB = {{
    {0.2104542683093140, 0.7936177747023054, -0.0040720430116193},
    {1.9779985324311684, -2.4285922420485799, 0.4505937096174110},
    {0.0259040424655478, 0.7827717124575296, -0.8086757549230774},
}};

/** @brief Matrix from LMS back to XYZ, D65. */
constexpr Matrix3 LMS_TO_XYZ = blas::inverse(XYZ_TO_LMS);

/** @brief Matrix from OKLab back to the cube roots of LMS. */
constexpr Matrix3 OKLAB_TO_LMS = blas::inverse(LMS_TO_OKLAB);

/** @brief Degrees in a radian. */
constexpr double DEGREES_PER_RADIAN = 180.0 / std::numbers::pi;

/**
 * @brief A color in OKLab, the perceptual space by Björn Ottosson.
 *
 * L is the perceived lightness, 0 for black and 1 for the D65 white; a runs
 * from green to red and b from blue to yellow, both 0 for grays. Equal
 * distances look about equally different, which makes OKLab the space for
 * color differences and for gradients that keep their lightness even.
 * Channels are not clamped: HDR colors have L above 1, and colors outside
 * any display's gamut are valid until converted to it.
 */
export class OKLab final : public ColorBase<OKLab>
{
public:
    /** @brief Color space name, used by the std::formatter specialization. */
    static constexpr std::string_view TYPE = "OKLab";

    /** @brief Default constructor initializes the color to opaque black (0, 0, 0, 1). */
    constexpr OKLab()
        : ColorBase(1.0f, 0.0f, 0.0f, 0.0f)
    {}

    /**
     * @brief Construct a color from OKLab values and alpha.
     * @param[in] l Perceived lightness, 1 being D65 white
     * @param[in] a Green-red axis
     * @param[in] b Blue-yellow axis
     * @param[in] alpha Alpha channel value (default 1.0)
     * @pre Every channel is finite: NaN and infinities are undefined behavior.
     */
    constexpr OKLab(const float l, const float a, const float b, const float alpha = 1.0f)
        : ColorBase(alpha, l, a, b)
    {}

    ///@name Color Channels
    ///@{
    /**
     * @brief Get the perceived lightness, 1 being D65 white.
     * @return The L channel
     */
    [[nodiscard]] constexpr float lightness() const noexcept { return _ch1; }

    /**
     * @brief Get the green-red axis, negative toward green.
     * @return The a channel
     */
    [[nodiscard]] constexpr float a() const noexcept { return _ch2; }

    /**
     * @brief Get the blue-yellow axis, negative toward blue.
     * @return The b channel
     */
    [[nodiscard]] constexpr float b() const noexcept { return _ch3; }
    ///@}
};

/**
 * @brief A color in OKLCH: OKLab in polar form, as lightness, chroma and hue.
 *
 * Chroma is the distance from gray and hue the angle around it, so changing
 * one leaves the other two and the perceived lightness alone, unlike HSL.
 * The hue is wrapped to the [0, 360) range and chroma clamped at 0; for
 * grays, whose chroma is 0, the hue has no meaning. Lightness is not
 * clamped.
 */
export class OKLCH final : public ColorBase<OKLCH>
{
public:
    /** @brief Color space name, used by the std::formatter specialization. */
    static constexpr std::string_view TYPE = "OKLCH";

    /** @brief Default constructor initializes the color to opaque black (0, 0, 0, 1). */
    constexpr OKLCH()
        : ColorBase(1.0f, 0.0f, 0.0f, 0.0f)
    {}

    /**
     * @brief Construct a color from OKLCH values and alpha.
     * @param[in] l Perceived lightness, 1 being D65 white
     * @param[in] c Chroma, clamped at 0
     * @param[in] h Hue in degrees, wrapped to the 0-360 range
     * @param[in] alpha Alpha channel value (default 1.0)
     * @pre Every channel is finite: NaN and infinities are undefined behavior.
     */
    constexpr OKLCH(const float l, const float c, const float h, const float alpha = 1.0f)
        : ColorBase(alpha, l, non_negative(c), wrap_hue(h))
    {}

    ///@name Color Channels
    ///@{
    /**
     * @brief Get the perceived lightness, 1 being D65 white.
     * @return The L channel
     */
    [[nodiscard]] constexpr float lightness() const noexcept { return _ch1; }

    /**
     * @brief Get the chroma, the distance from gray.
     * @return The chroma, 0 or greater
     */
    [[nodiscard]] constexpr float chroma() const noexcept { return _ch2; }

    /**
     * @brief Get the hue angle in the 0-360 range.
     * @return The hue in degrees
     */
    [[nodiscard]] constexpr float hue() const noexcept { return _ch3; }
    ///@}
};

/** @brief OKLab converts through XYZ, by way of the LMS cone responses. */
template <>
struct ParentSpace<OKLab>
{
    /** @brief The parent space. */
    using type = XYZ;
};

/** @brief OKLCH converts through OKLab, of which it is the polar form. */
template <>
struct ParentSpace<OKLCH>
{
    /** @brief The parent space. */
    using type = OKLab;
};

/** @brief Converts XYZ to OKLab. */
template <>
struct Converter<OKLab, XYZ>
{
    /**
     * @brief Converts @p color to OKLab: LMS, cube root, then the OKLab matrix.
     * @param[in] color The color to convert
     * @return The same color in OKLab, with the same alpha
     * @note Not constexpr: std::cbrt is constexpr only since C++26.
     */
    static OKLab convert(const XYZ& color)
    {
        const Vector3 lms = blas::multiply(XYZ_TO_LMS, {color.x(), color.y(), color.z()});
        const Vector3 lab =
            blas::multiply(LMS_TO_OKLAB, {std::cbrt(lms[0]), std::cbrt(lms[1]), std::cbrt(lms[2])});
        return OKLab{static_cast<float>(lab[0]), static_cast<float>(lab[1]),
                     static_cast<float>(lab[2]), color.alpha()};
    }
};

/** @brief Converts OKLab to XYZ. */
template <>
struct Converter<XYZ, OKLab>
{
    /**
     * @brief Converts @p color to XYZ: the inverse matrix, cube, then back from LMS.
     * @param[in] color The color to convert
     * @return The same color in XYZ, with the same alpha
     */
    static constexpr XYZ convert(const OKLab& color)
    {
        const Vector3 roots =
            blas::multiply(OKLAB_TO_LMS, {color.lightness(), color.a(), color.b()});
        const Vector3 xyz = blas::multiply(LMS_TO_XYZ, {roots[0] * roots[0] * roots[0],
                                                        roots[1] * roots[1] * roots[1],
                                                        roots[2] * roots[2] * roots[2]});
        return XYZ{static_cast<float>(xyz[0]), static_cast<float>(xyz[1]),
                   static_cast<float>(xyz[2]), color.alpha()};
    }
};

/** @brief Converts OKLab to OKLCH, its polar form. */
template <>
struct Converter<OKLCH, OKLab>
{
    /**
     * @brief Converts @p color to OKLCH.
     * @param[in] color The color to convert
     * @return The same color in OKLCH, with the same alpha; hue 0 for grays
     * @note Not constexpr: std::hypot and std::atan2 are constexpr only since C++26.
     */
    static OKLCH convert(const OKLab& color)
    {
        const double a = color.a();
        const double b = color.b();
        return OKLCH{color.lightness(), static_cast<float>(std::hypot(a, b)),
                     static_cast<float>(std::atan2(b, a) * DEGREES_PER_RADIAN), color.alpha()};
    }
};

/** @brief Converts OKLCH to OKLab. */
template <>
struct Converter<OKLab, OKLCH>
{
    /**
     * @brief Converts @p color to OKLab.
     * @param[in] color The color to convert
     * @return The same color in OKLab, with the same alpha
     * @note Not constexpr: std::cos and std::sin are constexpr only since C++26.
     */
    static OKLab convert(const OKLCH& color)
    {
        const double chroma = color.chroma();
        const double angle  = color.hue() / DEGREES_PER_RADIAN;
        return OKLab{color.lightness(), static_cast<float>(chroma * std::cos(angle)),
                     static_cast<float>(chroma * std::sin(angle)), color.alpha()};
    }
};
} // namespace aest::colors
