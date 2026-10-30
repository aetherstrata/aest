/**
 * @copyright Copyright (c) 2026 Davide Pierotti, MIT License (see LICENSE.md)
 * @author Davide Pierotti
 * @file Compositing.cppm
 * @brief Partition `aest.colors:compositing`: premultiplied alpha and
 *        Porter-Duff compositing.
 */
module;
#include <array>
#include <concepts>
#include <string_view>
export module aest.colors:compositing;

import :color;
import :converter;
import :gamut;
import :space;

export namespace aest::colors
{
/**
 * @brief A color space whose channels are proportional to light, so that
 *        adding and scaling them is physically meaningful.
 *
 * The spaces that inherit ColorArithmetic, such as LinSRGB.
 *
 * @tparam T The type to check
 */
template <typename T>
concept LinearSpace = ColorType<T> && std::derived_from<T, ColorArithmetic<T>>;

/**
 * @brief A color of a linear space with premultiplied alpha: each color
 *        channel is already multiplied by alpha.
 *
 * This is the form compositing works in: a premultiplied color is the light
 * it adds plus the fraction of the background it covers, so blending is a
 * sum. Its channels are clamped like those of @p Space; alpha is clamped to
 * the 0-1 range. Convert from and to @p Space with convert(): converting a
 * color whose alpha is 0 back to @p Space gives transparent black.
 *
 * @tparam Space The linear color space of the channels, such as LinSRGB
 */
template <LinearSpace Space>
class Premultiplied final : public ColorBase<Premultiplied<Space>>
{
    /** @brief Beginning of the space name, before the name of @p Space. */
    static constexpr std::string_view TYPE_PREFIX = "Premultiplied<";

    /** @brief The space name, `Premultiplied<` + `Space::TYPE` + `>`. */
    static constexpr auto TYPE_CHARS =
        join_names<TYPE_PREFIX.size() + Space::TYPE.size() + 1>({TYPE_PREFIX, Space::TYPE, ">"});

public:
    /** @brief Color space name, used by the std::formatter specialization. */
    static constexpr std::string_view TYPE{TYPE_CHARS.data(), TYPE_CHARS.size()};

    /** @brief Default constructor initializes the color to opaque black (0, 0, 0, 1). */
    constexpr Premultiplied()
        : Premultiplied(Space{0.0f, 0.0f, 0.0f, 1.0f})
    {}

    /**
     * @brief Construct a color from premultiplied channel values.
     * @param[in] ch1 First channel, already multiplied by @p a
     * @param[in] ch2 Second channel, already multiplied by @p a
     * @param[in] ch3 Third channel, already multiplied by @p a
     * @param[in] a Alpha channel value (default 1.0)
     * @pre Every channel is finite: NaN and infinities are undefined behavior.
     */
    constexpr Premultiplied(const float ch1, const float ch2, const float ch3, const float a = 1.0f)
        : Premultiplied(Space{ch1, ch2, ch3, a})
    {}

    ///@name Arithmetic operators
    ///@{
    /**
     * @brief Adds two colors, alpha included, as when one adds its light and
     *        coverage on top of the other.
     * @param[in] lhs The left-hand color.
     * @param[in] rhs The right-hand color.
     * @return A new color containing the component-wise sum.
     */
    friend constexpr Premultiplied operator+(const Premultiplied& lhs,
                                             const Premultiplied& rhs) noexcept
    {
        return Premultiplied{lhs.ch1() + rhs.ch1(), lhs.ch2() + rhs.ch2(), lhs.ch3() + rhs.ch3(),
                             lhs.alpha() + rhs.alpha()};
    }

    /**
     * @brief Scales every channel of a color, alpha included, as when fading it.
     * @param[in] color The color to scale.
     * @param[in] scalar The factor applied to each channel.
     * @return A new color with every channel scaled.
     * @pre @p scalar is finite.
     */
    friend constexpr Premultiplied operator*(const Premultiplied& color,
                                             const float scalar) noexcept
    {
        return Premultiplied{color.ch1() * scalar, color.ch2() * scalar, color.ch3() * scalar,
                             color.alpha() * scalar};
    }

    /**
     * @brief Scales every channel of a color, alpha included, as when fading it.
     * @param[in] scalar The factor applied to each channel.
     * @param[in] color The color to scale.
     * @return A new color with every channel scaled.
     * @pre @p scalar is finite.
     */
    friend constexpr Premultiplied operator*(const float scalar,
                                             const Premultiplied& color) noexcept
    {
        return color * scalar;
    }

    /**
     * @brief Adds another color to this color in place.
     * @param[in] other The color to add.
     * @return Reference to this color after modification.
     */
    constexpr Premultiplied& operator+=(const Premultiplied& other) noexcept
    {
        return *this = *this + other;
    }

    /**
     * @brief Scales every channel of this color in place, alpha included.
     * @param[in] scalar The factor applied to each channel.
     * @return Reference to this color after modification.
     * @pre @p scalar is finite.
     */
    constexpr Premultiplied& operator*=(const float scalar) noexcept
    {
        return *this = *this * scalar;
    }
    ///@}

private:
    /**
     * @brief Construct a color from channels already clamped by @p Space.
     * @param[in] clamped The premultiplied channels, held in a @p Space value
     */
    explicit constexpr Premultiplied(const Space& clamped)
        : ColorBase<Premultiplied>(clamped.alpha(), clamped.ch1(), clamped.ch2(), clamped.ch3())
    {}
};

/**
 * @brief A premultiplied color converts through its straight-alpha space.
 * @tparam Space The linear color space of the channels
 */
template <LinearSpace Space>
struct ParentSpace<Premultiplied<Space>>
{
    /** @brief The parent space. */
    using type = Space;
};

/**
 * @brief Converts straight alpha to premultiplied alpha.
 * @tparam Space The linear color space of the channels
 */
template <LinearSpace Space>
struct Converter<Premultiplied<Space>, Space>
{
    /**
     * @brief Multiplies each color channel of @p color by its alpha.
     * @param[in] color The color to convert
     * @return The same color with premultiplied alpha
     */
    static constexpr Premultiplied<Space> convert(const Space& color)
    {
        const float alpha = color.alpha();
        return Premultiplied<Space>{color.ch1() * alpha, color.ch2() * alpha, color.ch3() * alpha,
                                    alpha};
    }
};

/**
 * @brief Converts premultiplied alpha to straight alpha.
 * @tparam Space The linear color space of the channels
 */
template <LinearSpace Space>
struct Converter<Space, Premultiplied<Space>>
{
    /**
     * @brief Divides each color channel of @p color by its alpha.
     * @param[in] color The color to convert
     * @return The same color with straight alpha; transparent black when
     *         the alpha of @p color is 0
     */
    static constexpr Space convert(const Premultiplied<Space>& color)
    {
        const float alpha = color.alpha();
        if (alpha == 0)
            return Space{0.0f, 0.0f, 0.0f, 0.0f};

        return Space{color.ch1() / alpha, color.ch2() / alpha, color.ch3() / alpha, alpha};
    }
};

/**
 * @brief Composites a color over another: Porter-Duff source-over.
 *
 * Both colors are converted to premultiplied @p Space, where the result is
 * `foreground + background * (1 - foreground alpha)`, then converted to the
 * type of @p background. Alpha combines the same way, so a half-transparent
 * color over another gives alpha 0.75.
 *
 * @tparam Space The linear color space the colors are composited in
 * @tparam Foreground Color type of @p foreground
 * @tparam Background Color type of @p background and of the result
 * @param[in] foreground The color on top
 * @param[in] background The color underneath
 * @param[in] space The working space, e.g. `in<LinSRGB>`; LinSRGB when omitted
 * @return The composited color, in the type of @p background.
 */
template <LinearSpace Space = LinSRGB, ColorType Foreground, ColorType Background>
    requires ConvertibleTo<Premultiplied<Space>, Foreground>
          && ConvertibleTo<Premultiplied<Space>, Background>
          && ConvertibleTo<Background, Premultiplied<Space>>
constexpr Background over(const Foreground& foreground,
                          const Background& background,
                          [[maybe_unused]] In<Space> space = {})
{
    using Working = Premultiplied<Space>;

    const Working front = convert<Working>(foreground);
    return convert<Background>(front + convert<Working>(background) * (1 - front.alpha()));
}
} // namespace aest::colors
