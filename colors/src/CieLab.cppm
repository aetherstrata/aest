/**
 * @copyright Copyright (c) 2026 Davide Pierotti, MIT License (see LICENSE.md)
 * @author Davide Pierotti
 * @file CieLab.cppm
 * @brief Partition `aest.colors:cie_lab`: the CIE 1976 L*a*b* space and its
 *        polar form, relative to D50 as in CSS Color 4.
 */
module;
#include <cmath>
#include <string_view>
export module aest.colors:cie_lab;

import aest.blas;
import :color;
import :converter;
import :gamut;

namespace aest::colors
{
/** @brief Matrix adapting XYZ, D50, to XYZ, D65, with the Bradford transform. */
constexpr Matrix3 D50_TO_D65 = bradford_to_d65(D50);

/** @brief Matrix adapting XYZ, D65, to XYZ, D50, with the Bradford transform. */
constexpr Matrix3 D65_TO_D50 = blas::inverse(D50_TO_D65);

/** @brief XYZ of the D50 white at luminance Y = 1, the reference white of CIELab. */
constexpr Vector3 D50_WHITE = xyz_of(D50);

/** @brief CIE ε, `216 / 24389`: below it, CIELab follows a line instead of the cube root. */
constexpr double LAB_EPSILON = 216.0 / 24389.0;

/** @brief CIE κ, `24389 / 27`: the slope of the line near black. */
constexpr double LAB_KAPPA = 24389.0 / 27.0;

/**
 * @brief The CIELab compression of a ratio to the white: a cube root, with
 *        a line near black.
 * @param[in] ratio A tristimulus value divided by the one of the white
 * @return `cbrt(ratio)` above LAB_EPSILON, `(κ * ratio + 16) / 116` below.
 * @note Not constexpr: std::cbrt is constexpr only since C++26.
 */
double lab_compress(const double ratio)
{
    return ratio > LAB_EPSILON ? std::cbrt(ratio) : (LAB_KAPPA * ratio + 16) / 116;
}

/**
 * @brief Inverse of lab_compress() for the X and Z ratios.
 * @param[in] value The compressed value
 * @return The ratio to the white: the cube of @p value above the cube root
 *         of LAB_EPSILON, `(116 * value - 16) / κ` below.
 */
constexpr double lab_expand(const double value)
{
    const double cube = value * value * value;
    return cube > LAB_EPSILON ? cube : (116 * value - 16) / LAB_KAPPA;
}

/**
 * @brief A color in CIELab, the CIE 1976 L*a*b* space, relative to D50.
 *
 * L is the lightness, 0 for black and 100 for the D50 white; a runs from
 * green to red and b from blue to yellow, both 0 for grays. The white point
 * and its Bradford adaptation from D65 are the ones of CSS Color 4, so the
 * values match CSS `lab()`. CIELab is the space of the CIE color difference
 * formulas, de_76(), de_94() and de_2000(); OKLab is more
 * even across hues for gradients and adjustments. Channels are not clamped:
 * HDR colors have L above 100.
 */
export class CIELab final : public ColorBase<CIELab>
{
public:
    /** @brief Color space name, used by the std::formatter specialization. */
    static constexpr std::string_view TYPE = "CIELab";

    /** @brief Default constructor initializes the color to opaque black (0, 0, 0, 1). */
    constexpr CIELab()
        : ColorBase(1.0f, 0.0f, 0.0f, 0.0f)
    {}

    /**
     * @brief Construct a color from CIELab values and alpha.
     * @param[in] l Lightness, 100 being D50 white
     * @param[in] a Green-red axis
     * @param[in] b Blue-yellow axis
     * @param[in] alpha Alpha channel value (default 1.0)
     * @pre Every channel is finite: NaN and infinities are undefined behavior.
     */
    constexpr CIELab(const float l, const float a, const float b, const float alpha = 1.0f)
        : ColorBase(alpha, l, a, b)
    {}

    ///@name Color Channels
    ///@{
    /**
     * @brief Get the lightness, 100 being D50 white.
     * @return The L* channel
     */
    [[nodiscard]] constexpr float lightness() const noexcept { return _ch1; }

    /**
     * @brief Get the green-red axis, negative toward green.
     * @return The a* channel
     */
    [[nodiscard]] constexpr float a() const noexcept { return _ch2; }

    /**
     * @brief Get the blue-yellow axis, negative toward blue.
     * @return The b* channel
     */
    [[nodiscard]] constexpr float b() const noexcept { return _ch3; }
    ///@}
};

/**
 * @brief A color in CIELCH: CIELab in polar form, as lightness, chroma and hue.
 *
 * The values match CSS `lch()`. The hue is wrapped to the [0, 360) range
 * and chroma clamped at 0; for grays, whose chroma is 0, the hue has no
 * meaning. Lightness is not clamped.
 */
export class CIELCH final : public ColorBase<CIELCH>
{
public:
    /** @brief Color space name, used by the std::formatter specialization. */
    static constexpr std::string_view TYPE = "CIELCH";

    /** @brief Default constructor initializes the color to opaque black (0, 0, 0, 1). */
    constexpr CIELCH()
        : ColorBase(1.0f, 0.0f, 0.0f, 0.0f)
    {}

    /**
     * @brief Construct a color from CIELCH values and alpha.
     * @param[in] l Lightness, 100 being D50 white
     * @param[in] c Chroma, clamped at 0
     * @param[in] h Hue in degrees, wrapped to the 0-360 range
     * @param[in] alpha Alpha channel value (default 1.0)
     * @pre Every channel is finite: NaN and infinities are undefined behavior.
     */
    constexpr CIELCH(const float l, const float c, const float h, const float alpha = 1.0f)
        : ColorBase(alpha, l, non_negative(c), wrap_hue(h))
    {}

    ///@name Color Channels
    ///@{
    /**
     * @brief Get the lightness, 100 being D50 white.
     * @return The L* channel
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

/** @brief CIELab converts through XYZ, adapted to its D50 white. */
template <>
struct ParentSpace<CIELab>
{
    /** @brief The parent space. */
    using type = XYZ;
};

/** @brief CIELCH converts through CIELab, of which it is the polar form. */
template <>
struct ParentSpace<CIELCH>
{
    /** @brief The parent space. */
    using type = CIELab;
};

/** @brief Converts XYZ to CIELab. */
template <>
struct Converter<CIELab, XYZ>
{
    /**
     * @brief Converts @p color to CIELab: Bradford to D50, then the CIELab
     *        compression of each ratio to the white.
     * @param[in] color The color to convert
     * @return The same color in CIELab, with the same alpha
     * @note Not constexpr: std::cbrt is constexpr only since C++26.
     */
    static CIELab convert(const XYZ& color)
    {
        const Vector3 xyz = blas::multiply(D65_TO_D50, {color.x(), color.y(), color.z()});
        const double fx   = lab_compress(xyz[0] / D50_WHITE[0]);
        const double fy   = lab_compress(xyz[1] / D50_WHITE[1]);
        const double fz   = lab_compress(xyz[2] / D50_WHITE[2]);
        return CIELab{static_cast<float>(116 * fy - 16), static_cast<float>(500 * (fx - fy)),
                      static_cast<float>(200 * (fy - fz)), color.alpha()};
    }
};

/** @brief Converts CIELab to XYZ. */
template <>
struct Converter<XYZ, CIELab>
{
    /**
     * @brief Converts @p color to XYZ: the inverse compression, then Bradford
     *        back to D65.
     * @param[in] color The color to convert
     * @return The same color in XYZ, with the same alpha
     */
    static constexpr XYZ convert(const CIELab& color)
    {
        const double lightness = color.lightness();
        const double fy        = (lightness + 16) / 116;
        const double fx        = fy + color.a() / 500.0;
        const double fz        = fy - color.b() / 200.0;

        // Y is expanded from the lightness, so the line and the cube meet at κε
        const double y = lightness > LAB_KAPPA * LAB_EPSILON ? fy * fy * fy : lightness / LAB_KAPPA;
        const Vector3 xyz =
            blas::multiply(D50_TO_D65, {lab_expand(fx) * D50_WHITE[0], y * D50_WHITE[1],
                                        lab_expand(fz) * D50_WHITE[2]});
        return XYZ{static_cast<float>(xyz[0]), static_cast<float>(xyz[1]),
                   static_cast<float>(xyz[2]), color.alpha()};
    }
};

/** @brief Converts CIELab to CIELCH, its polar form. */
template <>
struct Converter<CIELCH, CIELab>
{
    /**
     * @brief Converts @p color to CIELCH.
     * @param[in] color The color to convert
     * @return The same color in CIELCH, with the same alpha; hue 0 for grays
     * @note Not constexpr: std::hypot and std::atan2 are constexpr only since C++26.
     */
    static CIELCH convert(const CIELab& color)
    {
        const double a = color.a();
        const double b = color.b();
        return CIELCH{color.lightness(), static_cast<float>(std::hypot(a, b)),
                      static_cast<float>(std::atan2(b, a) * DEGREES_PER_RADIAN), color.alpha()};
    }
};

/** @brief Converts CIELCH to CIELab. */
template <>
struct Converter<CIELab, CIELCH>
{
    /**
     * @brief Converts @p color to CIELab.
     * @param[in] color The color to convert
     * @return The same color in CIELab, with the same alpha
     * @note Not constexpr: std::cos and std::sin are constexpr only since C++26.
     */
    static CIELab convert(const CIELCH& color)
    {
        const double chroma = color.chroma();
        const double angle  = color.hue() / DEGREES_PER_RADIAN;
        return CIELab{color.lightness(), static_cast<float>(chroma * std::cos(angle)),
                      static_cast<float>(chroma * std::sin(angle)), color.alpha()};
    }
};
} // namespace aest::colors
