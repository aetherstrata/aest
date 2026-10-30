/**
 * @copyright Copyright (c) 2026 Davide Pierotti, MIT License (see LICENSE.md)
 * @author Davide Pierotti
 * @file Color.cppm
 * @brief Partition `aest.colors:color`: color types, their shared base and
 *        the ColorType concept.
 */
module;
#include <algorithm>
#include <bit>
#include <concepts>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>
export module aest.colors:color;

namespace aest::colors
{
/** @brief Maximum value for an 8-bit color channel. */
inline constexpr std::uint8_t BYTE_MAX = 255;

/**
 * @brief Clamp a floating-point value to the 0-1 range.
 * @param[in] value The value to clamp
 * @return The clamped value
 */
constexpr float clamp(const float value)
{
    return std::clamp(value, 0.f, 1.f);
}

/**
 * @brief Convert an 8-bit color channel value to a float in the 0-1 range.
 * @param[in] value The 8-bit color channel value
 * @return The corresponding float value in the 0-1 range
 */
constexpr float to_float(const std::uint8_t value)
{
    return static_cast<float>(value) / BYTE_MAX;
}

/**
 * @brief Convert a float in the 0-1 range to a 8-bit color channel value.
 * @param[in] value The color value in the 0-1 range.
 * @return The nearest 8-bit representation.
 */
constexpr std::uint8_t to_byte(const float value)
{
    return static_cast<std::uint8_t>(value * BYTE_MAX + 0.5f);
}

/**
 * @brief Extract one byte of a 32-bit word.
 * @param[in] word The word to read
 * @param[in] index Position of the byte, 0 being the least significant
 * @return The byte at @p index.
 */
constexpr std::uint8_t byte_at(const std::uint32_t word, const std::uint32_t index)
{
    return static_cast<std::uint8_t>(word >> index * 8);
}

/**
 * @brief Pack four bytes into a 32-bit word, most significant first.
 * @param[in] byte3 The most significant byte
 * @param[in] byte2 The second byte
 * @param[in] byte1 The third byte
 * @param[in] byte0 The least significant byte
 * @return The word with @p byte3 in the top byte and @p byte0 in the bottom one.
 */
constexpr std::uint32_t pack(const std::uint8_t byte3,
                             const std::uint8_t byte2,
                             const std::uint8_t byte1,
                             const std::uint8_t byte0)
{
    return std::uint32_t{byte3} << 24 | std::uint32_t{byte2} << 16 | std::uint32_t{byte1} << 8
         | byte0;
}

/**
 * @brief Wrap a hue angle to the [0, 360) range.
 * @param[in] hue The hue in degrees, any finite value whose whole turns fit a
 *            std::int64_t.
 * @return The equivalent hue in the [0, 360) range.
 */
constexpr float wrap_hue(const float hue)
{
    // Degrees in a full turn of the hue circle
    constexpr double fullTurn = 360.;

    // std::fmod is constexpr only since C++23. The quotient is truncated in double,
    // where it cannot round up to the next whole turn, and the subtraction is exact
    const auto turns = static_cast<std::int64_t>(hue / fullTurn);
    auto wrapped     = static_cast<float>(hue - static_cast<double>(turns) * fullTurn);

    // The remainder keeps the sign of hue, and a tiny negative one + 360 rounds to 360
    if (wrapped < 0)
        wrapped += static_cast<float>(fullTurn);

    return wrapped == fullTurn ? 0.f : wrapped;
}

/**
 * @brief Base class for color representations.
 * @tparam Derived The derived color class
 */
export template <typename Derived>
struct ColorBase
{
    /// Lets the derived color reach the private constructor and channels.
    friend Derived;

private:
    float _a;   ///< Alpha channel value in the 0-1 range.
    float _ch1; ///< First channel value
    float _ch2; ///< Second channel value
    float _ch3; ///< Third channel value

    /**
     * @brief Construct a color from raw channel values.
     * @param[in] alpha Alpha channel value, clamped to the 0-1 range
     * @param[in] channel1 First channel value, stored as is
     * @param[in] channel2 Second channel value, stored as is
     * @param[in] channel3 Third channel value, stored as is
     */
    explicit constexpr ColorBase(const float alpha,
                                 const float channel1,
                                 const float channel2,
                                 const float channel3)
        : _a(clamp(alpha))
        , _ch1(channel1)
        , _ch2(channel2)
        , _ch3(channel3)
    {}

public:
    /// @name Observers
    /// @{
    /**
     * @brief Get the alpha channel value in the 0-1 range.
     * @return The alpha channel value
     */
    [[nodiscard]] constexpr float alpha() const noexcept { return _a; }

    /**
     * @brief Get the first channel value
     * @return The first channel value
     * @note There are no guarantees on the value range.
     *       Refer to the derived class for more information.
     */
    [[nodiscard]] constexpr float ch1() const noexcept { return _ch1; }

    /**
     * @brief Get the second channel value
     * @return The second channel value
     * @note There are no guarantees on the value range.
     *       Refer to the derived class for more information.
     */
    [[nodiscard]] constexpr float ch2() const noexcept { return _ch2; }

    /**
     * @brief Get the third channel value
     * @return The third channel value
     * @note There are no guarantees on the value range.
     *       Refer to the derived class for more information.
     */
    [[nodiscard]] constexpr float ch3() const noexcept { return _ch3; }

    /// @}
    /// @name Modifiers
    /// @{
    /**
     * @brief Returns a new color with the same channels and the specified alpha value.
     * @param[in] alpha The new alpha value for the returned color, in the 0-1 range.
     * @return a new color with the desired opacity.
     */
    [[nodiscard]] constexpr Derived opacity(const float alpha) const
    {
        return Derived{_ch1, _ch2, _ch3, alpha};
    }

    /// @}
    /// @name Identity operators
    /// @{
    /**
     * @brief Compare two colors of same type by checking each channel for equality.
     * @param[in] color1 First color
     * @param[in] color2 Second color
     * @return <c>true</c> if all channels have the same value, <c>false</c> otherwise.
     */
    friend constexpr bool operator==(const Derived color1, const Derived color2)
    {
        return color1._ch1 == color2._ch1 && color1._ch2 == color2._ch2
            && color1._ch3 == color2._ch3 && color1._a == color2._a;
    }

    ///@}
};

/**
 * @brief Mixin adding channel-wise arithmetic operators to a color type.
 *
 * Inherit it only in color spaces where adding and multiplying channel values
 * is meaningful, such as LinSRGB. Every operator combines both operands on
 * all four channels, alpha included: two half-transparent colors add up to an
 * opaque one, and `a - a` is fully transparent. The results are built through
 * the derived type's `(ch1, ch2, ch3, alpha)` constructor, so they are clamped
 * like any other value of that type.
 *
 * There are no scalar operators: multiplying by a scalar would scale the
 * alpha with the light. Scale by a gray color instead, choosing the alpha
 * explicitly, as in `color *= {0.5f, 0.5f, 0.5f}` (alpha defaults to 1), or
 * use the named operations such as lighten() and darken().
 *
 * @tparam Derived The derived color class
 */
export template <typename Derived>
struct ColorArithmetic
{
    ///@name Arithmetic operators
    ///@{
    /**
     * @brief Adds two colors together, channel by channel.
     * @param[in] lhs The left-hand color.
     * @param[in] rhs The right-hand color.
     * @return A new Color containing the component-wise sum.
     */
    friend constexpr Derived operator+(const Derived& lhs, const Derived& rhs) noexcept
    {
        return Derived{lhs.ch1() + rhs.ch1(), lhs.ch2() + rhs.ch2(), lhs.ch3() + rhs.ch3(),
                       lhs.alpha() + rhs.alpha()};
    }

    /**
     * @brief Subtracts a color from another, channel by channel.
     * @param[in] lhs The color to subtract from.
     * @param[in] rhs The color to subtract.
     * @return A new Color containing the component-wise difference.
     */
    friend constexpr Derived operator-(const Derived& lhs, const Derived& rhs) noexcept
    {
        return Derived{lhs.ch1() - rhs.ch1(), lhs.ch2() - rhs.ch2(), lhs.ch3() - rhs.ch3(),
                       lhs.alpha() - rhs.alpha()};
    }

    /**
     * @brief Multiplies two colors, channel by channel.
     * @param[in] lhs The left-hand color.
     * @param[in] rhs The right-hand color.
     * @return A new Color containing the component-wise product.
     */
    friend constexpr Derived operator*(const Derived& lhs, const Derived& rhs) noexcept
    {
        return Derived{lhs.ch1() * rhs.ch1(), lhs.ch2() * rhs.ch2(), lhs.ch3() * rhs.ch3(),
                       lhs.alpha() * rhs.alpha()};
    }

    /**
     * @brief Adds another color to this color in place.
     * @param[in] other The color to add.
     * @return Reference to this color after modification.
     */
    constexpr Derived& operator+=(const Derived& other) noexcept { return self() = self() + other; }

    /**
     * @brief Subtracts another color from this color in place.
     * @param[in] other The color to subtract.
     * @return Reference to this color after modification.
     */
    constexpr Derived& operator-=(const Derived& other) noexcept { return self() = self() - other; }

    /**
     * @brief Multiplies this color by another color in place.
     * @param[in] other The color to multiply by.
     * @return Reference to this color after modification.
     */
    constexpr Derived& operator*=(const Derived& other) noexcept { return self() = self() * other; }
    ///@}

private:
    /**
     * @brief Access this object as the derived color type.
     * @return Reference to the derived color.
     */
    constexpr Derived& self() noexcept { return *static_cast<Derived*>(this); }
};

/**
 * @brief A color in the sRGB color space, with gamma-encoded channels.
 *
 * This is the encoding of hex codes, 8-bit channels and CSS colors.
 * Convert it to LinSRGB with convert() before blending or other color math.
 */
export class SRGB final : public ColorBase<SRGB>
{
public:
    /** @brief Color space name, used by the std::formatter specialization. */
    static constexpr std::string_view TYPE = "SRGB";

    /** @brief Default constructor initializes the color to opaque black (0, 0, 0, 1). */
    constexpr SRGB()
        : ColorBase(1.0f, 0.0f, 0.0f, 0.0f)
    {}

    /**
     * @brief Construct a color from floating-point RGBA values.
     * @param[in] r Red channel value
     * @param[in] g Green channel value
     * @param[in] b Blue channel value
     * @param[in] a Alpha channel value (default 1.0)
     * @pre Every channel is finite: NaN and infinities are undefined behavior.
     */
    constexpr SRGB(const float r, const float g, const float b, const float a = 1.0f)
        : ColorBase(a, clamp(r), clamp(g), clamp(b))
    {}

    ///@name Color Channels
    ///@{
    /**
     * @brief Get the red channel value in the 0-1 range.
     * @return The red channel value
     */
    [[nodiscard]] constexpr float red() const noexcept { return _ch1; }

    /**
     * @brief Get the green channel value in the 0-1 range.
     * @return The green channel value
     */
    [[nodiscard]] constexpr float green() const noexcept { return _ch2; }

    /**
     * @brief Get the blue channel value in the 0-1 range.
     * @return The blue channel value
     */
    [[nodiscard]] constexpr float blue() const noexcept { return _ch3; }
    ///@}

    /**
     * @brief Create the string representation for this color in `#RRGGBB` format.
     * @return A new string containing the hex color value, without alpha.
     */
    [[nodiscard]] std::string hex_string() const
    {
        return std::format("#{:02X}{:02X}{:02X}", to_byte(_ch1), to_byte(_ch2), to_byte(_ch3));
    }

    /**
     * @brief Construct a color from 8-bit RGBA values.
     * @param[in] r Red channel value
     * @param[in] g Green channel value
     * @param[in] b Blue channel value
     * @param[in] a Alpha channel value (default 255)
     * @return The color with each channel scaled to the 0-1 range.
     */
    static constexpr SRGB from_bytes(const std::uint8_t r,
                                     const std::uint8_t g,
                                     const std::uint8_t b,
                                     const std::uint8_t a = 255) noexcept
    {
        return {to_float(r), to_float(g), to_float(b), to_float(a)};
    }

    /**
     * @brief Create a color from a 32-bit integer in @c 0xRRGGBB format.
     * @param[in] hex The color as an HTML color code
     * @return The color, fully opaque.
     */
    static constexpr SRGB from_hex(const std::uint32_t hex) noexcept
    {
        // Shift the hex color code up one byte and fill with 0xFF
        return from_rgba(hex << 8 | BYTE_MAX);
    }

    /**
     * @brief Create a color from a 32-bit integer in @c 0xRRGGBBAA format.
     * @param[in] color The color as a 32-bit integer
     * @return The unpacked color.
     */
    static constexpr SRGB from_rgba(const std::uint32_t color) noexcept
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
    {
        return pack(to_byte(_ch1), to_byte(_ch2), to_byte(_ch3), to_byte(_a));
    }

    /**
     * @brief Create a color from a 32-bit integer in @c 0xAARRGGBB format.
     * @param[in] color The color as a 32-bit integer
     * @return The unpacked color.
     */
    static constexpr SRGB from_argb(const std::uint32_t color) noexcept
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
    {
        // RRGGBBAA -> AARRGGBB
        return std::rotr(to_rgba(), 8);
    }
};

/**
 * @brief A color in the linear sRGB color space: sRGB primaries without the gamma
 * encoding, so channel values are proportional to light intensity.
 *
 * Blending, averaging and other color math are physically correct only in
 * this space. Convert from and to SRGB with convert().
 */
export class LinSRGB final : public ColorBase<LinSRGB>, public ColorArithmetic<LinSRGB>
{
public:
    /** @brief Color space name, used by the std::formatter specialization. */
    static constexpr std::string_view TYPE = "LinSRGB";

    /** @brief Default constructor initializes the color to opaque black (0, 0, 0, 1). */
    constexpr LinSRGB()
        : ColorBase(1.0f, 0.0f, 0.0f, 0.0f)
    {}

    /**
     * @brief Construct a color from linear floating-point RGBA values.
     * @param[in] r Red channel value
     * @param[in] g Green channel value
     * @param[in] b Blue channel value
     * @param[in] a Alpha channel value (default 1.0)
     * @pre Every channel is finite: NaN and infinities are undefined behavior.
     */
    constexpr LinSRGB(const float r, const float g, const float b, const float a = 1.0f)
        : ColorBase(a, clamp(r), clamp(g), clamp(b))
    {}

    ///@name Color Channels
    ///@{
    /**
     * @brief Get the linear red channel value in the 0-1 range.
     * @return The red channel value
     */
    [[nodiscard]] constexpr float red() const noexcept { return _ch1; }

    /**
     * @brief Get the linear green channel value in the 0-1 range.
     * @return The green channel value
     */
    [[nodiscard]] constexpr float green() const noexcept { return _ch2; }

    /**
     * @brief Get the linear blue channel value in the 0-1 range.
     * @return The blue channel value
     */
    [[nodiscard]] constexpr float blue() const noexcept { return _ch3; }
    ///@}
};

/**
 * @brief A color as hue, saturation and lightness, over sRGB.
 *
 * The hue is wrapped to the [0, 360) range; saturation and lightness are
 * clamped to the 0-1 range. Convert to any other space with convert().
 */
export class HSL final : public ColorBase<HSL>
{
public:
    /** @brief Color space name, used by the std::formatter specialization. */
    static constexpr std::string_view TYPE = "HSL";

    /** @brief Default constructor initializes the color to opaque black (0, 0, 0, 1). */
    constexpr HSL()
        : ColorBase(1.0f, 0.0f, 0.0f, 0.0f)
    {}

    /**
     * @brief Construct a color from floating-point HSLA values.
     * @param[in] h Hue in degrees, wrapped to the 0-360 range
     * @param[in] s Saturation value
     * @param[in] l Lightness value
     * @param[in] a Alpha channel value (default 1.0)
     * @pre Every channel is finite: NaN and infinities are undefined behavior.
     */
    constexpr HSL(const float h, const float s, const float l, const float a = 1.0f)
        : ColorBase(a, wrap_hue(h), clamp(s), clamp(l))
    {}

    ///@name Color Channels
    ///@{
    /**
     * @brief Get the hue value in the 0-360 range.
     * @return The color hue
     */
    [[nodiscard]] constexpr float hue() const noexcept { return _ch1; }

    /**
     * @brief Get the saturation value in the 0-1 range.
     * @return The color saturation
     */
    [[nodiscard]] constexpr float saturation() const noexcept { return _ch2; }

    /**
     * @brief Get the lightness value in the 0-1 range.
     * @return The color lightness
     */
    [[nodiscard]] constexpr float lightness() const noexcept { return _ch3; }
    ///@}
};

/**
 * @brief A color as hue, saturation and value (brightness), over sRGB.
 *
 * The hue is wrapped to the [0, 360) range; saturation and value are
 * clamped to the 0-1 range. Convert to any other space with convert().
 */
export class HSV final : public ColorBase<HSV>
{
public:
    /** @brief Color space name, used by the std::formatter specialization. */
    static constexpr std::string_view TYPE = "HSV";

    /** @brief Default constructor initializes the color to opaque black (0, 0, 0, 1). */
    constexpr HSV()
        : ColorBase(1.0f, 0.0f, 0.0f, 0.0f)
    {}

    /**
     * @brief Construct a color from floating-point HSVA values.
     * @param[in] h Hue in degrees, wrapped to the 0-360 range
     * @param[in] s Saturation value
     * @param[in] v Brightness value
     * @param[in] a Alpha channel value (default 1.0)
     * @pre Every channel is finite: NaN and infinities are undefined behavior.
     */
    constexpr HSV(const float h, const float s, const float v, const float a = 1.0f)
        : ColorBase(a, wrap_hue(h), clamp(s), clamp(v))
    {}

    ///@name Color Channels
    ///@{
    /**
     * @brief Get the hue value in the 0-360 range.
     * @return The color hue
     */
    [[nodiscard]] constexpr float hue() const noexcept { return _ch1; }

    /**
     * @brief Get the saturation value in the 0-1 range.
     * @return The color saturation
     */
    [[nodiscard]] constexpr float saturation() const noexcept { return _ch2; }

    /**
     * @brief Get the brightness value in the 0-1 range.
     * @return The color brightness
     */
    [[nodiscard]] constexpr float brightness() const noexcept { return _ch3; }
    ///@}
};

/**
 * @brief Concept to check if a type is a color
 * @tparam T the type to check
 *
 * A type satisfies the ColorType concept if it has the following members:
 * - float alpha() const
 * - float ch1() const
 * - float ch2() const
 * - float ch3() const
 * - a static TYPE convertible to std::string_view
 * - a constructor taking `(ch1, ch2, ch3, alpha)`, used by the operations
 *   to build results
 */
export template <typename T>
concept ColorType = requires(const T& color) {
    { color.alpha() } -> std::same_as<float>;
    { color.ch1() } -> std::same_as<float>;
    { color.ch2() } -> std::same_as<float>;
    { color.ch3() } -> std::same_as<float>;
    { T::TYPE } -> std::convertible_to<std::string_view>;
} && std::constructible_from<T, float, float, float, float>;
} // namespace aest::colors
