/**
 * @copyright Copyright (c) 2026 Davide Pierotti, MIT License (see LICENSE.md)
 * @author Davide Pierotti
 * @file Transfer.cppm
 * @brief Partition `aest.colors:transfer`: the transfer functions that encode
 *        linear light into RGB channel values.
 */
module;
#include <algorithm>
#include <cmath>
#include <concepts>
#include <string_view>
export module aest.colors:transfer;

import :color;

namespace aest::colors
{
/** @brief Exponent of the sRGB transfer function. */
constexpr float SRGB_GAMMA = 2.4f;

/** @brief Luminance of the PQ signal 1.0 in cd/m², the peak of SMPTE ST 2084. */
constexpr double PQ_PEAK_NITS = 10000.0;

/** @brief PQ constant m1 = 2610 / 16384, from SMPTE ST 2084. */
constexpr double PQ_M1 = 2610.0 / 16384.0;

/** @brief PQ constant m2 = 2523 / 4096 * 128, from SMPTE ST 2084. */
constexpr double PQ_M2 = 2523.0 / 4096.0 * 128.0;

/** @brief PQ constant c1 = 3424 / 4096, from SMPTE ST 2084. */
constexpr double PQ_C1 = 3424.0 / 4096.0;

/** @brief PQ constant c2 = 2413 / 4096 * 32, from SMPTE ST 2084. */
constexpr double PQ_C2 = 2413.0 / 4096.0 * 32.0;

/** @brief PQ constant c3 = 2392 / 4096 * 32, from SMPTE ST 2084. */
constexpr double PQ_C3 = 2392.0 / 4096.0 * 32.0;

/** @brief HLG constant a, from ITU-R BT.2100. */
constexpr double HLG_A = 0.17883277;

/** @brief HLG constant b = 1 - 4a, from ITU-R BT.2100. */
constexpr double HLG_B = 1 - 4 * HLG_A;

/**
 * @brief HLG constant c = 0.5 - a ln(4a), from ITU-R BT.2100.
 * @note std::log is not constexpr.
 */
constexpr double HLG_C = 0.559910729529562;

/**
 * @brief Scene light that HLG encodes to 75%, its BT.2408 reference white:
 *        `(exp((0.75 - c) / a) + b) / 12`
 * @note std::exp is not constexpr.
 */
constexpr double HLG_REFERENCE_WHITE = 0.26496256042100724;

/**
 * @brief A transfer function tag, the second parameter of BasicRGB.
 *
 * A tag is a type with:
 * - a static `PREFIX`, put before the gamut name in the color space name
 *   (`"Lin"` gives `LinSRGB`);
 * - static `encode(float)` and `decode(float)`, from linear light to the
 *   channel value and back;
 * - a static `clamp(float)`, bringing a channel value into the range of the
 *   encoding.
 *
 * @tparam T The type to check
 */
export template <typename T>
concept TransferFunction = requires(const float value) {
    { T::PREFIX } -> std::convertible_to<std::string_view>;
    { T::encode(value) } -> std::same_as<float>;
    { T::decode(value) } -> std::same_as<float>;
    { T::clamp(value) } -> std::same_as<float>;
};

/**
 * @brief A transfer function for HDR signals, such as PQ or HLG: a tag with
 *        a static `IS_HDR` set to `true`. Tags without it are SDR.
 *
 * HDR signals are in the 0-1 range too, but not relative to the display's
 * white, so HSL and HSV over them are rejected.
 *
 * @tparam T The type to check
 */
export template <typename T>
concept HDRTransfer = TransferFunction<T> && requires { requires T::IS_HDR; };

/**
 * @brief Luminance of linear 1.0 in cd/m²: the HDR reference white of
 *        ITU-R BT.2408, and the scale of every linear space.
 *
 * Divide an absolute luminance by it to get a linear value, as in
 * `tf::PQ::encode(1000.f / REFERENCE_WHITE_NITS)` for the PQ signal of
 * 1000 cd/m², or the linear headroom of a 1000 cd/m² display.
 */
export inline constexpr float REFERENCE_WHITE_NITS = 203.f;
} // namespace aest::colors

/** @brief Transfer function tags, the second parameter of BasicRGB. */
export namespace aest::colors::tf
{
/**
 * @brief No transfer function: channel values are proportional to light.
 *
 * Channels are clamped at 0 only, so they can hold HDR values above 1.
 */
struct Linear
{
    /** @brief Prefix of the color space name. */
    static constexpr std::string_view PREFIX = "Lin";

    /**
     * @brief Encodes linear light, which this function leaves unchanged.
     * @param[in] value The linear value
     * @return @p value
     */
    static constexpr float encode(const float value) { return value; }

    /**
     * @brief Decodes a channel value to linear light, leaving it unchanged.
     * @param[in] value The channel value
     * @return @p value
     */
    static constexpr float decode(const float value) { return value; }

    /**
     * @brief Brings a channel value into the range of linear light.
     * @param[in] value The channel value
     * @return @p value, or 0 when it is negative
     */
    static constexpr float clamp(const float value) { return non_negative(value); }
};

/**
 * @brief The sRGB transfer function of IEC 61966-2-1: a short linear segment
 *        near black, then a power curve with exponent 2.4.
 *
 * Channels are clamped to the 0-1 range. Display P3 uses it too.
 */
struct SRGB
{
    /** @brief Prefix of the color space name: none, encoded spaces go by the gamut name. */
    static constexpr std::string_view PREFIX = "";

    /**
     * @brief Encodes linear light into an sRGB channel value.
     * @param[in] value The linear value, 0 or greater
     * @return The encoded value in the 0-1 range: values from 1 up give 1.
     * @note Implemented as defined by W3C in
     *       "https://www.w3.org/Graphics/Color/sRGB.html".
     * @note Not constexpr: std::pow is constexpr only since C++26.
     */
    static float encode(const float value)
    {
        if (value >= 1.f)
            return 1.f;

        return value <= 0.0031308f ? 12.92f * value
                                   : 1.055f * std::pow(value, 1.0f / SRGB_GAMMA) - 0.055f;
    }

    /**
     * @brief Decodes an sRGB channel value to linear light.
     * @param[in] value The encoded value in the 0-1 range
     * @return The linear value in the 0-1 range.
     * @note Implemented as defined by W3C in
     *       "https://www.w3.org/Graphics/Color/sRGB.html".
     * @note Not constexpr: std::pow is constexpr only since C++26.
     */
    static float decode(const float value)
    {
        if (value == 1)
            return 1;

        return value <= 0.04045f ? value / 12.92f : std::pow((value + 0.055f) / 1.055f, SRGB_GAMMA);
    }

    /**
     * @brief Brings a channel value into the encoded range.
     * @param[in] value The channel value
     * @return @p value clamped to the 0-1 range
     */
    static constexpr float clamp(const float value) { return colors::clamp(value); }
};

/**
 * @brief The Perceptual Quantizer of SMPTE ST 2084 and ITU-R BT.2100, the
 *        absolute HDR transfer function of HDR10 and Dolby Vision.
 *
 * Linear 1.0 is REFERENCE_WHITE_NITS, 203 cd/m², so it encodes to about 58%;
 * the signal 1.0 is 10000 cd/m², linear 49.26. Channels are clamped to the
 * 0-1 range.
 */
struct PQ
{
    /** @brief Prefix of the color space name. */
    static constexpr std::string_view PREFIX = "PQ";

    /** @brief PQ is an HDR signal. */
    static constexpr bool IS_HDR = true;

    /**
     * @brief Encodes linear light into a PQ signal.
     * @param[in] value The linear value, 0 or greater, 1 being 203 cd/m²
     * @return The PQ signal, in the 0-1 range up to 10000 cd/m².
     * @note Not constexpr: std::pow is constexpr only since C++26.
     */
    static float encode(const float value)
    {
        const double y = std::pow(value * REFERENCE_WHITE_NITS / PQ_PEAK_NITS, PQ_M1);
        return static_cast<float>(std::pow((PQ_C1 + PQ_C2 * y) / (1 + PQ_C3 * y), PQ_M2));
    }

    /**
     * @brief Decodes a PQ signal to linear light.
     * @param[in] value The PQ signal in the 0-1 range
     * @return The linear value, 1 being 203 cd/m².
     * @note Not constexpr: std::pow is constexpr only since C++26.
     */
    static float decode(const float value)
    {
        const double power = std::pow(static_cast<double>(value), 1 / PQ_M2);
        const double ratio = std::max(power - PQ_C1, 0.0) / (PQ_C2 - PQ_C3 * power);
        return static_cast<float>(PQ_PEAK_NITS / REFERENCE_WHITE_NITS * std::pow(ratio, 1 / PQ_M1));
    }

    /**
     * @brief Brings a signal value into the encoded range.
     * @param[in] value The signal value
     * @return @p value clamped to the 0-1 range
     */
    static constexpr float clamp(const float value) { return colors::clamp(value); }
};

/**
 * @brief The Hybrid Log-Gamma OETF of ITU-R BT.2100 and ARIB STD-B67, the
 *        relative HDR transfer function of broadcast.
 *
 * Linear 1.0 encodes to 75%, the BT.2408 reference white, so the signal 1.0
 * is linear 3.77. Only the OETF and its inverse are applied: the linear
 * values are scene light, and no OOTF maps them to the light of a display
 * with a given peak. Channels are clamped to the 0-1 range.
 */
struct HLG
{
    /** @brief Prefix of the color space name. */
    static constexpr std::string_view PREFIX = "HLG";

    /** @brief HLG is an HDR signal. */
    static constexpr bool IS_HDR = true;

    /**
     * @brief Encodes linear light into an HLG signal with the OETF.
     * @param[in] value The linear value, 0 or greater, 1 being reference white
     * @return The HLG signal; values from linear 3.77 up give 1 or more.
     * @note Not constexpr: std::sqrt and std::log are constexpr only since C++26.
     */
    static float encode(const float value)
    {
        const double scene = value * HLG_REFERENCE_WHITE;
        return static_cast<float>(scene <= 1.0 / 12 ? std::sqrt(3 * scene)
                                                    : HLG_A * std::log(12 * scene - HLG_B) + HLG_C);
    }

    /**
     * @brief Decodes an HLG signal to linear light with the inverse OETF.
     * @param[in] value The HLG signal in the 0-1 range
     * @return The linear value, 1 being reference white.
     * @note Not constexpr: std::exp is constexpr only since C++26.
     */
    static float decode(const float value)
    {
        const double signal = value;
        const double scene =
            signal <= 0.5 ? signal * signal / 3 : (std::exp((signal - HLG_C) / HLG_A) + HLG_B) / 12;
        return static_cast<float>(scene / HLG_REFERENCE_WHITE);
    }

    /**
     * @brief Brings a signal value into the encoded range.
     * @param[in] value The signal value
     * @return @p value clamped to the 0-1 range
     */
    static constexpr float clamp(const float value) { return colors::clamp(value); }
};
} // namespace aest::colors::tf
