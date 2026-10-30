/**
 * @copyright Copyright (c) 2026 Davide Pierotti, MIT License (see LICENSE.md)
 * @author Davide Pierotti
 * @file Cylindrical.cppm
 * @brief Partition `aest.colors:cylindrical`: HSL and HSV, cylindrical views
 *        of an encoded RGB space.
 */
module;
#include <concepts>
#include <string_view>
export module aest.colors:cylindrical;

import :color;
import :gamut;

export namespace aest::colors
{
/**
 * @brief A color as hue, saturation and lightness, over the encoded RGB
 *        space @p RGB.
 *
 * HSL rearranges the channels of @p RGB, so it covers the same gamut: over
 * DisplayP3 it reaches the P3 colors that HSL over SRGB clips. The hue is
 * wrapped to the [0, 360) range; saturation and lightness are clamped to the
 * 0-1 range. Convert to any other space with convert().
 *
 * @tparam RGB The encoded RGB space, such as SRGB: its channels must be in
 *         the 0-1 range, so linear spaces are rejected.
 */
template <BoundedRGBSpace RGB>
class BasicHSL final : public ColorBase<BasicHSL<RGB>>
{
    /** @brief Name of the model, the whole space name over SRGB. */
    static constexpr std::string_view MODEL = "HSL";

    /** @brief The space name over another RGB space, `HSL<` + `RGB::TYPE` + `>`. */
    static constexpr auto QUALIFIED_CHARS =
        join_names<MODEL.size() + RGB::TYPE.size() + 2>({MODEL, "<", RGB::TYPE, ">"});

public:
    /** @brief Color space name, used by the std::formatter specialization. */
    static constexpr std::string_view TYPE =
        std::same_as<RGB, SRGB> ? MODEL
                                : std::string_view{QUALIFIED_CHARS.data(), QUALIFIED_CHARS.size()};

    /** @brief Default constructor initializes the color to opaque black (0, 0, 0, 1). */
    constexpr BasicHSL()
        : ColorBase<BasicHSL>(1.0f, 0.0f, 0.0f, 0.0f)
    {}

    /**
     * @brief Construct a color from floating-point HSLA values.
     * @param[in] h Hue in degrees, wrapped to the 0-360 range
     * @param[in] s Saturation value
     * @param[in] l Lightness value
     * @param[in] a Alpha channel value (default 1.0)
     * @pre Every channel is finite: NaN and infinities are undefined behavior.
     */
    constexpr BasicHSL(const float h, const float s, const float l, const float a = 1.0f)
        : ColorBase<BasicHSL>(a, wrap_hue(h), clamp(s), clamp(l))
    {}

    ///@name Color Channels
    ///@{
    /**
     * @brief Get the hue value in the 0-360 range.
     * @return The color hue
     */
    [[nodiscard]] constexpr float hue() const noexcept { return this->_ch1; }

    /**
     * @brief Get the saturation value in the 0-1 range.
     * @return The color saturation
     */
    [[nodiscard]] constexpr float saturation() const noexcept { return this->_ch2; }

    /**
     * @brief Get the lightness value in the 0-1 range.
     * @return The color lightness
     */
    [[nodiscard]] constexpr float lightness() const noexcept { return this->_ch3; }
    ///@}
};

/**
 * @brief A color as hue, saturation and value (brightness), over the encoded
 *        RGB space @p RGB.
 *
 * HSV rearranges the channels of @p RGB, so it covers the same gamut. The hue
 * is wrapped to the [0, 360) range; saturation and value are clamped to the
 * 0-1 range. Convert to any other space with convert().
 *
 * @tparam RGB The encoded RGB space, such as SRGB: its channels must be in
 *         the 0-1 range, so linear spaces are rejected.
 */
template <BoundedRGBSpace RGB>
class BasicHSV final : public ColorBase<BasicHSV<RGB>>
{
    /** @brief Name of the model, the whole space name over SRGB. */
    static constexpr std::string_view MODEL = "HSV";

    /** @brief The space name over another RGB space, `HSV<` + `RGB::TYPE` + `>`. */
    static constexpr auto QUALIFIED_CHARS =
        join_names<MODEL.size() + RGB::TYPE.size() + 2>({MODEL, "<", RGB::TYPE, ">"});

public:
    /** @brief Color space name, used by the std::formatter specialization. */
    static constexpr std::string_view TYPE =
        std::same_as<RGB, SRGB> ? MODEL
                                : std::string_view{QUALIFIED_CHARS.data(), QUALIFIED_CHARS.size()};

    /** @brief Default constructor initializes the color to opaque black (0, 0, 0, 1). */
    constexpr BasicHSV()
        : ColorBase<BasicHSV>(1.0f, 0.0f, 0.0f, 0.0f)
    {}

    /**
     * @brief Construct a color from floating-point HSVA values.
     * @param[in] h Hue in degrees, wrapped to the 0-360 range
     * @param[in] s Saturation value
     * @param[in] v Brightness value
     * @param[in] a Alpha channel value (default 1.0)
     * @pre Every channel is finite: NaN and infinities are undefined behavior.
     */
    constexpr BasicHSV(const float h, const float s, const float v, const float a = 1.0f)
        : ColorBase<BasicHSV>(a, wrap_hue(h), clamp(s), clamp(v))
    {}

    ///@name Color Channels
    ///@{
    /**
     * @brief Get the hue value in the 0-360 range.
     * @return The color hue
     */
    [[nodiscard]] constexpr float hue() const noexcept { return this->_ch1; }

    /**
     * @brief Get the saturation value in the 0-1 range.
     * @return The color saturation
     */
    [[nodiscard]] constexpr float saturation() const noexcept { return this->_ch2; }

    /**
     * @brief Get the brightness value in the 0-1 range.
     * @return The color brightness
     */
    [[nodiscard]] constexpr float brightness() const noexcept { return this->_ch3; }
    ///@}
};

/** @brief HSL over sRGB, the HSL of CSS. */
using HSL = BasicHSL<SRGB>;

/** @brief HSV over sRGB. */
using HSV = BasicHSV<SRGB>;
} // namespace aest::colors
