/**
 * @copyright Copyright (c) 2026 Davide Pierotti, MIT License (see LICENSE.md)
 * @author Davide Pierotti
 * @file Transfer.cppm
 * @brief Partition `aest.colors:transfer`: the transfer functions that encode
 *        linear light into RGB channel values.
 */
module;
#include <cmath>
#include <concepts>
#include <string_view>
export module aest.colors:transfer;

import :color;

namespace aest::colors
{
/** @brief Exponent of the sRGB transfer function. */
constexpr float SRGB_GAMMA = 2.4f;

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
} // namespace aest::colors::tf
