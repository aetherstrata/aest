/**
 * @copyright Copyright (c) 2026 Davide Pierotti, MIT License (see LICENSE.md)
 * @author Davide Pierotti
 * @file Gamut.cppm
 * @brief Partition `aest.colors:gamut`: RGB gamuts, the BasicRGB color spaces
 *        built from a gamut and a transfer function, and their matrices to
 *        CIE XYZ, derived at compile time from chromaticities.
 */
module;
#include <array>
#include <bit>
#include <concepts>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <type_traits>
export module aest.colors:gamut;

import aest.blas;
import :color;
import :transfer;

namespace aest::colors
{
/** @brief CIE 1931 xy chromaticity coordinates of a primary or white point. */
export using Chromaticity = std::array<double, 2>;

/** @brief Chromaticity of the D65 white point, as sRGB defines it: the white of XYZ. */
export inline constexpr Chromaticity D65 = {0.3127, 0.3290};

/**
 * @brief A gamut tag, the first parameter of BasicRGB.
 *
 * A tag is a type with a static `NAME`, used in the color space name, and
 * the static Chromaticity values `RED`, `GREEN` and `BLUE` of its primaries
 * and `WHITE` of its white point. The matrices to and from XYZ are derived
 * from them; a white point other than D65 is adapted to it with the Bradford
 * transform.
 *
 * @tparam T The type to check
 */
export template <typename T>
concept RGBGamut = requires {
    { T::NAME } -> std::convertible_to<std::string_view>;
    { T::RED } -> std::convertible_to<Chromaticity>;
    { T::GREEN } -> std::convertible_to<Chromaticity>;
    { T::BLUE } -> std::convertible_to<Chromaticity>;
    { T::WHITE } -> std::convertible_to<Chromaticity>;
};

/** @brief Gamut tags, the first parameter of BasicRGB. */
export namespace gamut
{
/** @brief The sRGB and Rec. 709 primaries, D65 white, from IEC 61966-2-1. */
struct SRGB
{
    static constexpr std::string_view NAME = "SRGB";       ///< Color space name.
    static constexpr Chromaticity RED      = {0.64, 0.33}; ///< Red primary.
    static constexpr Chromaticity GREEN    = {0.30, 0.60}; ///< Green primary.
    static constexpr Chromaticity BLUE     = {0.15, 0.06}; ///< Blue primary.
    static constexpr Chromaticity WHITE    = D65;          ///< White point.
};

/** @brief The DCI-P3 primaries with D65 white, from SMPTE EG 432-1. */
struct DisplayP3
{
    static constexpr std::string_view NAME = "DisplayP3";    ///< Color space name.
    static constexpr Chromaticity RED      = {0.680, 0.320}; ///< Red primary.
    static constexpr Chromaticity GREEN    = {0.265, 0.690}; ///< Green primary.
    static constexpr Chromaticity BLUE     = {0.150, 0.060}; ///< Blue primary.
    static constexpr Chromaticity WHITE    = D65;            ///< White point.
};

/** @brief The Rec. 2020 / Rec. 2100 primaries, D65 white, from ITU-R BT.2020. */
struct Rec2020
{
    static constexpr std::string_view NAME = "Rec2020";      ///< Color space name.
    static constexpr Chromaticity RED      = {0.708, 0.292}; ///< Red primary.
    static constexpr Chromaticity GREEN    = {0.170, 0.797}; ///< Green primary.
    static constexpr Chromaticity BLUE     = {0.131, 0.046}; ///< Blue primary.
    static constexpr Chromaticity WHITE    = D65;            ///< White point.
};

/**
 * @brief The ACES AP1 primaries with the ACES white point (about D60), from
 *        SMPTE ST 2065-1, the gamut of ACEScg.
 */
struct AP1
{
    static constexpr std::string_view NAME = "AP1";              ///< Color space name.
    static constexpr Chromaticity RED      = {0.713, 0.293};     ///< Red primary.
    static constexpr Chromaticity GREEN    = {0.165, 0.830};     ///< Green primary.
    static constexpr Chromaticity BLUE     = {0.128, 0.044};     ///< Blue primary.
    static constexpr Chromaticity WHITE    = {0.32168, 0.33767}; ///< White point.
};
} // namespace gamut

/** @brief A column of three values, such as XYZ or linear RGB channels. */
using Vector3 = blas::Vector<double, 3>;

/** @brief A 3x3 matrix, stored as three rows. */
using Matrix3 = blas::Matrix<double, 3, 3>;

/** @brief The Bradford cone response matrix, used to adapt a white point. */
constexpr Matrix3 BRADFORD = {{
    {0.8951, 0.2664, -0.1614},
    {-0.7502, 1.7135, 0.0367},
    {0.0389, -0.0685, 1.0296},
}};

/**
 * @brief The XYZ of a chromaticity at luminance Y = 1.
 * @param[in] chromaticity The xy coordinates
 * @return The XYZ coordinates, `(x / y, 1, (1 - x - y) / y)`.
 */
constexpr Vector3 xyz_of(const Chromaticity& chromaticity)
{
    const auto [x, y] = chromaticity;
    return {x / y, 1, (1 - x - y) / y};
}

/**
 * @brief Derives the matrix from linear RGB to XYZ for a set of primaries.
 *
 * Each primary contributes the XYZ of its chromaticity, scaled so that
 * equal channels of 1 give the white point at luminance Y = 1.
 *
 * @param[in] red Chromaticity of the red primary
 * @param[in] green Chromaticity of the green primary
 * @param[in] blue Chromaticity of the blue primary
 * @param[in] white Chromaticity of the white point
 * @return The matrix taking linear `(r, g, b)` to `(X, Y, Z)`, relative to @p white.
 */
constexpr Matrix3 rgb_to_xyz(const Chromaticity& red,
                             const Chromaticity& green,
                             const Chromaticity& blue,
                             const Chromaticity& white)
{
    // The primaries as columns, each weighted so that together they add up to white
    const Matrix3 primaries = blas::transpose(Matrix3{xyz_of(red), xyz_of(green), xyz_of(blue)});
    const Vector3 weights   = blas::multiply(blas::inverse(primaries), xyz_of(white));
    return blas::multiply(primaries, blas::diagonal(weights));
}

/**
 * @brief Derives the Bradford matrix that adapts XYZ colors from a white
 *        point to D65.
 * @param[in] white Chromaticity of the source white point
 * @return The matrix taking XYZ relative to @p white to XYZ relative to D65.
 */
constexpr Matrix3 bradford_to_d65(const Chromaticity& white)
{
    // Scale each cone response by the ratio of the two whites
    const Vector3 source = blas::multiply(BRADFORD, xyz_of(white));
    const Vector3 target = blas::multiply(BRADFORD, xyz_of(D65));
    const Vector3 ratios = {target[0] / source[0], target[1] / source[1], target[2] / source[2]};
    return blas::multiply(blas::inverse(BRADFORD),
                          blas::multiply(blas::diagonal(ratios), BRADFORD));
}

/**
 * @brief Derives the matrix from linear RGB of a gamut to XYZ relative to D65.
 * @tparam Gamut The gamut tag
 * @return The matrix of Gamut, adapted to D65 when its white point differs.
 */
template <RGBGamut Gamut>
constexpr Matrix3 gamut_to_xyz()
{
    const Matrix3 native = rgb_to_xyz(Gamut::RED, Gamut::GREEN, Gamut::BLUE, Gamut::WHITE);
    return Gamut::WHITE == D65 ? native : blas::multiply(bradford_to_d65(Gamut::WHITE), native);
}

/**
 * @brief Matrix from linear RGB of @p Gamut to XYZ, D65.
 * @tparam Gamut The gamut tag
 */
template <RGBGamut Gamut>
inline constexpr Matrix3 RGB_TO_XYZ = gamut_to_xyz<Gamut>();

/**
 * @brief Matrix from XYZ, D65, to linear RGB of @p Gamut.
 * @tparam Gamut The gamut tag
 */
template <RGBGamut Gamut>
inline constexpr Matrix3 XYZ_TO_RGB = blas::inverse(RGB_TO_XYZ<Gamut>);

/**
 * @brief The characters of the default BasicRGB name: the transfer prefix,
 *        then the gamut name.
 * @tparam Gamut The gamut tag
 * @tparam Transfer The transfer function tag
 */
template <RGBGamut Gamut, TransferFunction Transfer>
inline constexpr auto RGB_NAME_CHARS =
    join_names<Transfer::PREFIX.size() + Gamut::NAME.size()>({Transfer::PREFIX, Gamut::NAME});

/**
 * @brief Name of the BasicRGB space of a gamut and a transfer function,
 *        printed by `std::format`.
 *
 * By default the transfer prefix followed by the gamut name, as in
 * `LinSRGB`; specialize it for spaces known by another name, as ACEScg is.
 *
 * @tparam Gamut The gamut tag
 * @tparam Transfer The transfer function tag
 */
export template <RGBGamut Gamut, TransferFunction Transfer>
inline constexpr std::string_view RGB_NAME{RGB_NAME_CHARS<Gamut, Transfer>.data(),
                                           RGB_NAME_CHARS<Gamut, Transfer>.size()};

/** @brief Linear AP1 goes by the name of its ACES working space. */
template <>
inline constexpr std::string_view RGB_NAME<gamut::AP1, tf::Linear> = "ACEScg";

/** @brief Rec. 2020 with PQ is the PQ space of Rec. 2100. */
template <>
inline constexpr std::string_view RGB_NAME<gamut::Rec2020, tf::PQ> = "Rec2100PQ";

/** @brief Rec. 2020 with HLG is the HLG space of Rec. 2100. */
template <>
inline constexpr std::string_view RGB_NAME<gamut::Rec2020, tf::HLG> = "Rec2100HLG";

/** @brief Empty base standing in for ColorArithmetic in encoded RGB spaces. */
struct NoArithmetic
{};

/**
 * @brief A color in an RGB color space: the primaries and white point of
 *        @p Gamut, with channels encoded by @p Transfer.
 *
 * With tf::Linear the channels are proportional to light: they are clamped
 * at 0 only, so they hold HDR values, and the space has the ColorArithmetic
 * operators, since blending and lighting are physically correct only there.
 * With an encoding such as tf::SRGB the channels are clamped to the 0-1
 * range and have no arithmetic. Convert between spaces with convert(): a
 * color outside the target gamut clips at 0.
 *
 * Use it through the aliases, such as SRGB, LinSRGB or DisplayP3.
 *
 * @tparam Gamut The gamut tag, such as gamut::SRGB
 * @tparam Transfer The transfer function tag, such as tf::SRGB or tf::Linear
 */
export template <RGBGamut Gamut, TransferFunction Transfer>
class BasicRGB final : public ColorBase<BasicRGB<Gamut, Transfer>>,
                       public std::conditional_t<std::same_as<Transfer, tf::Linear>,
                                                 ColorArithmetic<BasicRGB<Gamut, Transfer>>,
                                                 NoArithmetic>
{
    /** @brief Whether this is SRGB, the space of hex codes and CSS colors. */
    static constexpr bool IS_SRGB =
        std::same_as<Gamut, gamut::SRGB> && std::same_as<Transfer, tf::SRGB>;

public:
    /** @brief Color space name, used by the std::formatter specialization. */
    static constexpr std::string_view TYPE = RGB_NAME<Gamut, Transfer>;

    /** @brief Default constructor initializes the color to opaque black (0, 0, 0, 1). */
    constexpr BasicRGB()
        : ColorBase<BasicRGB>(1.0f, 0.0f, 0.0f, 0.0f)
    {}

    /**
     * @brief Construct a color from floating-point RGBA values.
     * @param[in] r Red channel value
     * @param[in] g Green channel value
     * @param[in] b Blue channel value
     * @param[in] a Alpha channel value (default 1.0)
     * @pre Every channel is finite: NaN and infinities are undefined behavior.
     */
    constexpr BasicRGB(const float r, const float g, const float b, const float a = 1.0f)
        : ColorBase<BasicRGB>(a, Transfer::clamp(r), Transfer::clamp(g), Transfer::clamp(b))
    {}

    ///@name Color Channels
    ///@{
    /**
     * @brief Get the red channel value.
     * @return The red channel value, in the range of @p Transfer
     */
    [[nodiscard]] constexpr float red() const noexcept { return this->_ch1; }

    /**
     * @brief Get the green channel value.
     * @return The green channel value, in the range of @p Transfer
     */
    [[nodiscard]] constexpr float green() const noexcept { return this->_ch2; }

    /**
     * @brief Get the blue channel value.
     * @return The blue channel value, in the range of @p Transfer
     */
    [[nodiscard]] constexpr float blue() const noexcept { return this->_ch3; }

    ///@}
    ///@name Bytes and hex codes (SRGB only)
    ///@{
    /**
     * @brief Create the string representation for this color in `#RRGGBB` format.
     * @return A new string containing the hex color value, without alpha.
     */
    [[nodiscard]] std::string hex_string() const
        requires IS_SRGB
    {
        return std::format("#{:02X}{:02X}{:02X}", to_byte(this->_ch1), to_byte(this->_ch2),
                           to_byte(this->_ch3));
    }

    /**
     * @brief Construct a color from 8-bit RGBA values.
     * @param[in] r Red channel value
     * @param[in] g Green channel value
     * @param[in] b Blue channel value
     * @param[in] a Alpha channel value (default 255)
     * @return The color with each channel scaled to the 0-1 range.
     */
    static constexpr BasicRGB from_bytes(const std::uint8_t r,
                                         const std::uint8_t g,
                                         const std::uint8_t b,
                                         const std::uint8_t a = 255) noexcept
        requires IS_SRGB
    {
        return {to_float(r), to_float(g), to_float(b), to_float(a)};
    }

    /**
     * @brief Create a color from a 32-bit integer in @c 0xRRGGBB format.
     * @param[in] hex The color as an HTML color code
     * @return The color, fully opaque.
     */
    static constexpr BasicRGB from_hex(const std::uint32_t hex) noexcept
        requires IS_SRGB
    {
        // Shift the hex color code up one byte and fill with 0xFF
        return from_rgba(hex << 8 | BYTE_MAX);
    }

    /**
     * @brief Create a color from a 32-bit integer in @c 0xRRGGBBAA format.
     * @param[in] color The color as a 32-bit integer
     * @return The unpacked color.
     */
    static constexpr BasicRGB from_rgba(const std::uint32_t color) noexcept
        requires IS_SRGB
    {
        return from_bytes(byte_at(color, 3), byte_at(color, 2), byte_at(color, 1),
                          byte_at(color, 0));
    }

    /**
     * @brief Converts the color to a 32-bit integer in @c 0xRRGGBBAA format.
     * @return A 32-bit integer representing this color.
     * @note Channels are stored as floats and rounded to the nearest byte,
     * so the conversion loses precision.
     */
    [[nodiscard]] constexpr std::uint32_t to_rgba() const noexcept
        requires IS_SRGB
    {
        return pack(to_byte(this->_ch1), to_byte(this->_ch2), to_byte(this->_ch3),
                    to_byte(this->_a));
    }

    /**
     * @brief Create a color from a 32-bit integer in @c 0xAARRGGBB format.
     * @param[in] color The color as a 32-bit integer
     * @return The unpacked color.
     */
    static constexpr BasicRGB from_argb(const std::uint32_t color) noexcept
        requires IS_SRGB
    {
        // AARRGGBB -> RRGGBBAA
        return from_rgba(std::rotl(color, 8));
    }

    /**
     * @brief Copy this color into a 32-bit integer in @c 0xAARRGGBB format.
     * @return A 32-bit integer representing this color.
     * @note Channels are stored as floats and rounded to the nearest byte,
     * so the conversion loses precision.
     */
    [[nodiscard]] constexpr std::uint32_t to_argb() const noexcept
        requires IS_SRGB
    {
        // RRGGBBAA -> AARRGGBB
        return std::rotr(to_rgba(), 8);
    }
    ///@}
};

/**
 * @brief The gamut and transfer function of a BasicRGB space; empty for other types.
 * @tparam T The type to inspect
 */
template <typename T>
struct RGBParts
{};

/**
 * @brief The gamut and transfer function of `BasicRGB<Gamut, Transfer>`.
 * @tparam Gamut The gamut tag
 * @tparam Transfer The transfer function tag
 */
template <RGBGamut Gamut, TransferFunction Transfer>
struct RGBParts<BasicRGB<Gamut, Transfer>>
{
    using GamutTag    = Gamut;    ///< The gamut tag.
    using TransferTag = Transfer; ///< The transfer function tag.
};
} // namespace aest::colors

export namespace aest::colors
{
/**
 * @brief sRGB with gamma-encoded channels: the encoding of hex codes, 8-bit
 *        channels and CSS colors.
 *
 * Convert it to LinSRGB with convert() before blending or other color math.
 */
using SRGB = BasicRGB<gamut::SRGB, tf::SRGB>;

/**
 * @brief Linear sRGB: the sRGB primaries without the gamma encoding, so
 *        channels are proportional to light and can exceed 1 for HDR values.
 */
using LinSRGB = BasicRGB<gamut::SRGB, tf::Linear>;

/// Display P3: the P3 primaries with the sRGB transfer function, as on Apple displays and in CSS
using DisplayP3 = BasicRGB<gamut::DisplayP3, tf::SRGB>;

/// Linear Display P3
using LinDisplayP3 = BasicRGB<gamut::DisplayP3, tf::Linear>;

/// Linear Rec. 2020, the gamut of UHD and HDR video
using LinRec2020 = BasicRGB<gamut::Rec2020, tf::Linear>;

/// ACEScg: linear AP1, the working space of ACES rendering and compositing
using ACEScg = BasicRGB<gamut::AP1, tf::Linear>;

/// Rec.2100 PQ: Rec. 2020 primaries with the PQ signal of HDR10, linear 1.0 at 203 cd/m²
using Rec2100PQ = BasicRGB<gamut::Rec2020, tf::PQ>;

/// Rec.2100 HLG: Rec. 2020 primaries with the HLG signal, linear 1.0 at 75%
using Rec2100HLG = BasicRGB<gamut::Rec2020, tf::HLG>;

/**
 * @brief Any BasicRGB color space.
 * @tparam T The type to check
 */
template <typename T>
concept RGBSpace = requires { typename RGBParts<T>::TransferTag; };

/**
 * @brief A BasicRGB space with linear channels, tf::Linear.
 * @tparam T The type to check
 */
template <typename T>
concept LinearRGBSpace = RGBSpace<T> && std::same_as<typename RGBParts<T>::TransferTag, tf::Linear>;

/**
 * @brief A BasicRGB space with an SDR encoding: channels in the 0-1 range, relative to the
 *        display's white. Linear spaces and HDR signals such as PQ are left out.
 * @tparam T The type to check
 */
template <typename T>
concept BoundedRGBSpace = RGBSpace<T> && !LinearRGBSpace<T>
                       && !aest::colors::HDRTransfer<typename RGBParts<T>::TransferTag>;
} // namespace aest::colors
