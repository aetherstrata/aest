/**
 * @copyright Copyright (c) 2026 Davide Pierotti, MIT License (see LICENSE.md)
 * @author Davide Pierotti
 * @file Color.cppm
 * @brief Partition `aest.colors:color`: color types, their shared base and
 *        the ColorType concept.
 */
module;
#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
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
 * @brief Clamp a floating-point value to be non-negative, leaving it unbounded above.
 * @param[in] value The value to clamp
 * @return The value, or 0 when it is negative
 */
constexpr float non_negative(const float value)
{
    return std::max(value, 0.f);
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
 * @brief Concatenate names into a character array, for a color space name
 *        built at compile time.
 * @tparam Size The total length of @p parts
 * @param[in] parts The names to join, in order
 * @return The characters of @p parts, without a terminator.
 */
template <std::size_t Size>
constexpr std::array<char, Size> join_names(const std::initializer_list<std::string_view> parts)
{
    std::array<char, Size> chars{};
    auto out = chars.begin();
    for (const std::string_view part : parts)
        out = std::ranges::copy(part, out).out;
    return chars;
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
 * like any other value of that type: LinSRGB channels stop at 0 but not at 1,
 * and alpha stays in the 0-1 range.
 *
 * The scalar operators are the exception: `color * s` and `color / s` scale
 * the light, the three color channels, and keep alpha, as exposure or a
 * light's intensity would.
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

    ///@name Scalar operators
    ///@{
    /**
     * @brief Scales the color channels of a color, keeping its alpha.
     * @param[in] color The color to scale.
     * @param[in] scalar The factor applied to each color channel.
     * @return A new Color with the scaled channels and the alpha of @p color.
     * @pre @p scalar is finite.
     */
    friend constexpr Derived operator*(const Derived& color, const float scalar) noexcept
    {
        return Derived{color.ch1() * scalar, color.ch2() * scalar, color.ch3() * scalar,
                       color.alpha()};
    }

    /**
     * @brief Scales the color channels of a color, keeping its alpha.
     * @param[in] scalar The factor applied to each color channel.
     * @param[in] color The color to scale.
     * @return A new Color with the scaled channels and the alpha of @p color.
     * @pre @p scalar is finite.
     */
    friend constexpr Derived operator*(const float scalar, const Derived& color) noexcept
    {
        return color * scalar;
    }

    /**
     * @brief Divides the color channels of a color, keeping its alpha.
     * @param[in] color The color to divide.
     * @param[in] scalar The divisor of each color channel.
     * @return A new Color with the divided channels and the alpha of @p color.
     * @pre @p scalar is finite and not 0.
     */
    friend constexpr Derived operator/(const Derived& color, const float scalar) noexcept
    {
        return Derived{color.ch1() / scalar, color.ch2() / scalar, color.ch3() / scalar,
                       color.alpha()};
    }

    /**
     * @brief Scales the color channels of this color in place, keeping its alpha.
     * @param[in] scalar The factor applied to each color channel.
     * @return Reference to this color after modification.
     * @pre @p scalar is finite.
     */
    constexpr Derived& operator*=(const float scalar) noexcept { return self() = self() * scalar; }

    /**
     * @brief Divides the color channels of this color in place, keeping its alpha.
     * @param[in] scalar The divisor of each color channel.
     * @return Reference to this color after modification.
     * @pre @p scalar is finite and not 0.
     */
    constexpr Derived& operator/=(const float scalar) noexcept { return self() = self() / scalar; }
    ///@}

private:
    /**
     * @brief Access this object as the derived color type.
     * @return Reference to the derived color.
     */
    constexpr Derived& self() noexcept { return *static_cast<Derived*>(this); }
};

/**
 * @brief A color in the CIE 1931 XYZ color space, relative to the D65 white
 * point.
 *
 * XYZ describes any color the eye can see, independently of a display's
 * primaries, and links the RGB gamuts: it is the root of the conversion tree.
 * Y is the relative luminance, so the white of LinSRGB is
 * `(0.9505, 1, 1.0891)`. Channels are not clamped: XYZ itself has no bounds,
 * and colors outside a gamut clip only when converted to it. Like LinSRGB, it
 * is linear in light, so it has the arithmetic operators.
 */
export class XYZ final : public ColorBase<XYZ>, public ColorArithmetic<XYZ>
{
public:
    /** @brief Color space name, used by the std::formatter specialization. */
    static constexpr std::string_view TYPE = "XYZ";

    /** @brief Default constructor initializes the color to opaque black (0, 0, 0, 1). */
    constexpr XYZ()
        : ColorBase(1.0f, 0.0f, 0.0f, 0.0f)
    {}

    /**
     * @brief Construct a color from XYZ tristimulus values and alpha.
     * @param[in] x X tristimulus value, stored as is
     * @param[in] y Y tristimulus value, the relative luminance, stored as is
     * @param[in] z Z tristimulus value, stored as is
     * @param[in] a Alpha channel value (default 1.0)
     * @pre Every channel is finite: NaN and infinities are undefined behavior.
     */
    constexpr XYZ(const float x, const float y, const float z, const float a = 1.0f)
        : ColorBase(a, x, y, z)
    {}

    ///@name Color Channels
    ///@{
    /**
     * @brief Get the X tristimulus value.
     * @return The X value
     */
    [[nodiscard]] constexpr float x() const noexcept { return _ch1; }

    /**
     * @brief Get the Y tristimulus value, the relative luminance.
     * @return The Y value
     */
    [[nodiscard]] constexpr float y() const noexcept { return _ch2; }

    /**
     * @brief Get the Z tristimulus value.
     * @return The Z value
     */
    [[nodiscard]] constexpr float z() const noexcept { return _ch3; }
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
