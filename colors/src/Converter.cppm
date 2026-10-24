/**
 * @copyright Copyright (c) 2026 Davide Pierotti, MIT License (see LICENSE.md)
 * @author Davide Pierotti
 * @file Converter.cppm
 * @brief Partition `aest.colors:converter`: conversions between color spaces.
 */
module;
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <type_traits>
export module aest.colors:converter;

import :color;

/** @brief Exponent of the sRGB transfer function. */
static constexpr float SRGB_GAMMA = 2.4f;

/**
 * @brief Always `false`, but dependent on @p T so a `static_assert` using it
 *        only fires when the enclosing template is instantiated.
 * @tparam T Any type, used only to make the value dependent
 * @note Needed because the library targets C++20: a plain
 *       `static_assert(false)` in a template is only allowed from C++23
 *       (P2593). Before that it makes the template ill-formed even if it is
 *       never instantiated, and only some compilers accept it as extension.
 */
template <typename T>
constexpr bool ALWAYS_FALSE = false;

export namespace aest::colors
{
/**
 * @brief Template struct for converting colors between different color spaces.
 *
 * Specializations of this struct should implement the static convert method
 * for specific color space conversions.
 *
 * @tparam ColorOut The desired output color space.
 * @tparam ColorIn The input color space.
 */
template <ColorType ColorOut, ColorType ColorIn>
struct Converter
{
    /**
     * @brief Converts a color from one color space to another.
     *
     * @param[in] color The input color of type ColorIn to be converted.
     * @return The converted color of type ColorOut.
     */
    static constexpr ColorOut convert([[maybe_unused]] const ColorIn& color)
    {
        static_assert(ALWAYS_FALSE<ColorOut>,
                      "Conversion not implemented between the specified color types. "
                      "Please implement a specialization of the Converter template. ");
    }
};

/**
 * @brief Convert a color from one color space to another.
 * @tparam ColorOut The desired color space
 * @tparam ColorIn The current color space
 * @param[in] color The color to convert
 * @return A new color representing the input value in another color space
 */
template <ColorType ColorOut, ColorType ColorIn>
constexpr ColorOut convert(const ColorIn& color)
{
    return Converter<ColorOut, ColorIn>::convert(color);
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

/** @brief Converts HSL to RGB. */
template <>
struct Converter<RGB, HSL>
{
    /**
     * @brief Converts @p color to RGB.
     * @param[in] color The color to convert
     * @return The same color in RGB, with the same alpha
     */
    static constexpr RGB convert(const HSL& color)
    {
        const float h = color.hue();
        const float c = 2 * std::min(color.lightness(), 1 - color.lightness()) * color.saturation();
        // Distance from the middle of the current pair of 60 degree sectors;
        // std::fmod and std::fabs are constexpr only since C++23
        const float sector   = h / 60.0f;
        const float distance = sector - 2 * static_cast<std::int32_t>(sector / 2) - 1;
        const float x        = c * (1 - (distance < 0 ? -distance : distance));
        const float m        = color.lightness() - c / 2;

        float r, g, b;
        if (h < 60)
        {
            r = c;
            g = x;
            b = 0;
        }
        else if (h < 120)
        {
            r = x;
            g = c;
            b = 0;
        }
        else if (h < 180)
        {
            r = 0;
            g = c;
            b = x;
        }
        else if (h < 240)
        {
            r = 0;
            g = x;
            b = c;
        }
        else if (h < 300)
        {
            r = x;
            g = 0;
            b = c;
        }
        else
        {
            r = c;
            g = 0;
            b = x;
        }

        return RGB{r + m, g + m, b + m, color.alpha()};
    }
};

/** @brief Converts RGB to HSL. */
template <>
struct Converter<HSL, RGB>
{
    /**
     * @brief Converts @p color to HSL.
     * @param[in] color The color to convert
     * @return The same color in HSL, with the same alpha
     */
    static constexpr HSL convert(const RGB& color)
    {
        const float r = color.red();
        const float g = color.green();
        const float b = color.blue();

        const float max   = std::max({r, g, b});
        const float min   = std::min({r, g, b});
        const float delta = max - min;

        float h = 0;
        if (delta != 0)
        {
            if (max == r)
            {
                // In [-60, 60]: the color constructor wraps negative hues
                h = 60.0f * ((g - b) / delta);
            }
            else if (max == g)
            {
                h = 60.0f * ((b - r) / delta + 2);
            }
            else if (max == b)
            {
                h = 60.0f * ((r - g) / delta + 4);
            }
        }

        const float l = std::midpoint(max, min);
        const float s = delta == 0 ? 0 : delta / (2 * std::min(l, 1 - l));

        return HSL{h, s, l, color.alpha()};
    }
};

/** @brief Converts RGB to HSV. */
template <>
struct Converter<HSV, RGB>
{
    /**
     * @brief Converts @p color to HSV.
     * @param[in] color The color to convert
     * @return The same color in HSV, with the same alpha
     */
    static constexpr HSV convert(const RGB& color)
    {
        const float r = color.red();
        const float g = color.green();
        const float b = color.blue();

        const float max   = std::max({r, g, b});
        const float min   = std::min({r, g, b});
        const float delta = max - min;

        float h = 0;
        if (delta != 0)
        {
            if (max == r)
            {
                // In [-60, 60]: the color constructor wraps negative hues
                h = 60.0f * ((g - b) / delta);
            }
            else if (max == g)
            {
                h = 60.0f * ((b - r) / delta + 2);
            }
            else if (max == b)
            {
                h = 60.0f * ((r - g) / delta + 4);
            }
        }

        const float s = max == 0.0f ? 0.0f : delta / max;
        const float v = max;

        return HSV{h, s, v, color.alpha()};
    }
};

/** @brief Converts HSV to RGB. */
template <>
struct Converter<RGB, HSV>
{
    /**
     * @brief Converts @p color to RGB.
     * @param[in] color The color to convert
     * @return The same color in RGB, with the same alpha
     */
    static constexpr RGB convert(const HSV& color)
    {
        const float h = color.hue();

        const float c = color.brightness() * color.saturation();
        // Distance from the middle of the current pair of 60 degree sectors;
        // std::fmod and std::fabs are constexpr only since C++23
        const float sector   = h / 60.0f;
        const float distance = sector - 2 * static_cast<std::int32_t>(sector / 2) - 1;
        const float x        = c * (1 - (distance < 0 ? -distance : distance));
        const float m        = color.brightness() - c;

        float r, g, b;
        if (h < 60)
        {
            r = c;
            g = x;
            b = 0;
        }
        else if (h < 120)
        {
            r = x;
            g = c;
            b = 0;
        }
        else if (h < 180)
        {
            r = 0;
            g = c;
            b = x;
        }
        else if (h < 240)
        {
            r = 0;
            g = x;
            b = c;
        }
        else if (h < 300)
        {
            r = x;
            g = 0;
            b = c;
        }
        else
        {
            r = c;
            g = 0;
            b = x;
        }

        return RGB{r + m, g + m, b + m, color.alpha()};
    }
};

/** @brief Converts HSL to HSV. */
template <>
struct Converter<HSV, HSL>
{
    /**
     * @brief Converts @p color to HSV, keeping the hue.
     * @param[in] color The color to convert
     * @return The same color in HSV, with the same alpha
     */
    static constexpr HSV convert(const HSL& color)
    {
        const float l = color.lightness();
        const float v = l + color.saturation() * std::min(l, 1 - l);
        const float s = v == 0 ? 0 : 2 * (1 - l / v);

        return HSV{color.hue(), s, v, color.alpha()};
    }
};

/** @brief Converts HSV to HSL. */
template <>
struct Converter<HSL, HSV>
{
    /**
     * @brief Converts @p color to HSL, keeping the hue.
     * @param[in] color The color to convert
     * @return The same color in HSL, with the same alpha
     */
    static constexpr HSL convert(const HSV& color)
    {
        const float v = color.brightness();
        const float l = v * (1 - color.saturation() / 2);
        const float s = l == 0 || l == 1 ? 0 : (v - l) / std::min(l, 1 - l);

        return HSL{color.hue(), s, l, color.alpha()};
    }
};

/** @brief Converts sRGB to linear sRGB by removing the gamma encoding. */
template <>
struct Converter<LinearRGB, RGB>
{
    /**
     * @brief Convert a color channel value from sRGB to linear space.
     * @param[in] value The sRGB value in the 0-1 range.
     * @return The linear value in the 0-1 range.
     * @note This function is implemented according to the sRGB standard,
     * as defined by W3C in "https://www.w3.org/Graphics/Color/sRGB.html".
     * @note Not constexpr: std::pow is constexpr only since C++26.
     */
    static float srgb_to_linear(const float value)
    {
        if (value == 1)
            return 1;

        return value <= 0.04045f ? value / 12.92f : std::pow((value + 0.055f) / 1.055f, SRGB_GAMMA);
    }

    /**
     * @brief Applies the inverse sRGB transfer function to each color channel.
     * @param[in] color The color to convert
     * @return The same color in linear sRGB, with the same alpha
     */
    static LinearRGB convert(const RGB& color)
    {
        return LinearRGB{srgb_to_linear(color.red()), srgb_to_linear(color.green()),
                         srgb_to_linear(color.blue()), color.alpha()};
    }
};

/** @brief Converts linear sRGB to sRGB by applying the gamma encoding. */
template <>
struct Converter<RGB, LinearRGB>
{
    /**
     * @brief Convert a color channel value from linear space to sRGB.
     * @param[in] value The linear value in the 0-1 range.
     * @return The sRGB value in the 0-1 range.
     * @note This function is implemented according to the sRGB standard, as defined
     *       by W3C in "https://www.w3.org/Graphics/Color/sRGB.html".
     * @note Not constexpr: std::pow is constexpr only since C++26.
     */
    static float linear_to_srgb(const float value)
    {
        if (value == 1.f)
            return 1.f;

        return value <= 0.0031308f ? 12.92f * value
                                   : 1.055f * std::pow(value, 1.0f / SRGB_GAMMA) - 0.055f;
    }

    /**
     * @brief Applies the sRGB transfer function to each color channel.
     * @param[in] color The color to convert
     * @return The same color in sRGB, with the same alpha
     */
    static RGB convert(const LinearRGB& color)
    {
        return RGB{linear_to_srgb(color.red()), linear_to_srgb(color.green()),
                   linear_to_srgb(color.blue()), color.alpha()};
    }
};

} // namespace aest::colors
