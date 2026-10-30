/**
 * @copyright Copyright (c) 2026 Davide Pierotti, MIT License (see LICENSE.md)
 * @author Davide Pierotti
 * @file colors.test.cpp
 * @brief Unit tests for `aest.colors`.
 */

#include <algorithm>
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include <cstdint>
#include <format>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>

import aest.colors;

using aest::colors::HSL;
using aest::colors::HSV;
using aest::colors::LinSRGB;
using aest::colors::SRGB;
using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;

TEST_CASE("from_hex unpacks channels", "[colors]")
{
    STATIC_REQUIRE(SRGB::from_hex(0x12'34'56) == SRGB::from_bytes(0x12, 0x34, 0x56));
    STATIC_REQUIRE(SRGB::from_argb(0xFF'00'00'00) == SRGB{});
}

TEST_CASE("hex_string formats as #RRGGBB", "[colors]")
{
    CHECK(SRGB{}.hex_string() == "#000000");
    CHECK(SRGB::from_bytes(0xAB, 0x0C, 0xFF).hex_string() == "#AB0CFF");
}

TEST_CASE("std::format prints the color space and channels", "[colors]")
{
    CHECK(std::format("{}", SRGB{1.f, 0.f, 0.5f, 0.25f}) == "SRGB(1.000, 0.000, 0.500, 0.250)");
    CHECK(std::format("{}", HSL{-90.f, 1.f, 0.5f}) == "HSL(270.000, 1.000, 0.500, 1.000)");
}

TEST_CASE("std::format applies the spec to each channel", "[colors]")
{
    const SRGB color{1.f, 0.f, 0.5f, 0.75f};
    CHECK(std::format("{:.1f}", color) == "SRGB(1.0, 0.0, 0.5, 0.8)");
    CHECK(std::format("{:5.2f}", color) == "SRGB( 1.00,  0.00,  0.50,  0.75)");
}

TEST_CASE("std::format rejects specs that are invalid for a float", "[colors]")
{
    const SRGB color;
    CHECK_THROWS_AS(std::vformat("{:s}", std::make_format_args(color)), std::format_error);
}

TEST_CASE("hex_string round-trips from_hex", "[colors]")
{
    CHECK(SRGB::from_hex(0xC0'FF'EE).hex_string() == "#C0FFEE");
}

TEST_CASE("Byte conversions round to the nearest value", "[colors]")
{
    const SRGB color{0.5f, 0.2f, 0.999f, 0.6f};
    CHECK(color.hex_string() == "#8033FF");
    CHECK(color.to_rgba() == 0x80'33'FF'99);
    CHECK(color.to_argb() == 0x99'80'33'FF);
    CHECK(color.hex_string() == std::format("#{:06X}", color.to_rgba() >> 8));
}

TEST_CASE("Hue is wrapped to [0, 360)", "[colors]")
{
    STATIC_REQUIRE(HSL{-120.f, 1.f, 0.5f}.hue() == 240.f);
    CHECK(HSL{720.f, 1.f, 0.5f}.hue() == 0.f);
    CHECK(HSV{370.f, 1.f, 1.f}.hue() == 10.f);
    CHECK(HSV{-360.f, 1.f, 1.f}.hue() == 0.f);
    CHECK(HSV{-1e-6f, 1.f, 1.f}.hue() < 360.f);
    STATIC_REQUIRE(HSV{-1e-6f, 1.f, 1.f}.hue() < 360.f);
    STATIC_REQUIRE(HSV{370.f, 1.f, 1.f}.hue() == 10.f);
}

TEST_CASE("Negative hues convert like their wrapped equivalent", "[colors]")
{
    const auto fromHsl = aest::colors::convert<SRGB>(HSL{-120.f, 1.f, 0.5f});
    CHECK_THAT(fromHsl.red(), WithinAbs(0., 1e-6));
    CHECK_THAT(fromHsl.green(), WithinAbs(0., 1e-6));
    CHECK_THAT(fromHsl.blue(), WithinAbs(1., 1e-6));

    const auto fromHsv = aest::colors::convert<SRGB>(HSV{-60.f, 1.f, 1.f});
    CHECK(fromHsv == aest::colors::convert<SRGB>(HSV{300.f, 1.f, 1.f}));
}

TEST_CASE("SRGB with more blue than green converts to a positive hue", "[colors]")
{
    const SRGB rose{1.f, 0.f, 0.5f};
    CHECK_THAT(aest::colors::convert<HSL>(rose).hue(), WithinAbs(330., 1e-4));
    CHECK_THAT(aest::colors::convert<HSV>(rose).hue(), WithinAbs(330., 1e-4));
}

TEST_CASE("SRGB converts to LinSRGB with the sRGB transfer function", "[colors]")
{
    const auto linear = aest::colors::convert<LinSRGB>(SRGB{0.5f, 0.02f, 1.f, 0.25f});
    CHECK_THAT(linear.red(), WithinAbs(0.214041, 1e-06));   // ((0.5 + 0.055) / 1.055)^2.4
    CHECK_THAT(linear.green(), WithinAbs(0.001548, 1e-06)); // 0.02 / 12.92
    CHECK(linear.blue() == 1.f);
    CHECK(linear.alpha() == 0.25f);
}

TEST_CASE("LinSRGB converts to SRGB with the sRGB transfer function", "[colors]")
{
    const auto srgb = aest::colors::convert<SRGB>(LinSRGB{0.5f, 0.002f, 0.f, 0.75f});
    CHECK_THAT(srgb.red(), WithinAbs(0.735357, 1e-06));  // 1.055 * 0.5^(1 / 2.4) - 0.055
    CHECK_THAT(srgb.green(), WithinAbs(0.02584, 1e-06)); // 0.002 * 12.92
    CHECK(srgb.blue() == 0.f);
    CHECK(srgb.alpha() == 0.75f);
}

TEST_CASE("SRGB round-trips through LinSRGB", "[colors]")
{
    for (std::int32_t value = 0; value <= 255; ++value)
    {
        const auto byte  = static_cast<std::uint8_t>(value);
        const SRGB color = SRGB::from_bytes(byte, byte, byte);
        const SRGB back  = aest::colors::convert<SRGB>(aest::colors::convert<LinSRGB>(color));
        CHECK_THAT(back.red(), WithinAbs(color.red(), 1e-06));
    }
}

namespace
{
/** @brief Checks the channels of @p color against the expected RGB values. */
void check_rgb(const SRGB& color, const double red, const double green, const double blue)
{
    CHECK_THAT(color.red(), WithinAbs(red, 1e-6));
    CHECK_THAT(color.green(), WithinAbs(green, 1e-6));
    CHECK_THAT(color.blue(), WithinAbs(blue, 1e-6));
}

/** @brief The six fully saturated hues, at 60 degree steps, as {hue, r, g, b}. */
constexpr float SECTORS[6][4] = {
    {0.f, 1.f, 0.f, 0.f},   {60.f, 1.f, 1.f, 0.f},  {120.f, 0.f, 1.f, 0.f},
    {180.f, 0.f, 1.f, 1.f}, {240.f, 0.f, 0.f, 1.f}, {300.f, 1.f, 0.f, 1.f},
};
} // namespace

TEST_CASE("HSL converts to SRGB", "[colors][conversions]")
{
    using aest::colors::convert;

    for (const auto& [hue, r, g, b] : SECTORS)
    {
        CAPTURE(hue);
        check_rgb(convert<SRGB>(HSL{hue, 1.f, 0.5f}), r, g, b);
    }
    check_rgb(convert<SRGB>(HSL{330.f, 1.f, 0.5f}), 1., 0., 0.5);
    check_rgb(convert<SRGB>(HSL{210.f, 0.5f, 0.5f}), 0.25, 0.5, 0.75);
    check_rgb(convert<SRGB>(HSL{123.f, 0.f, 0.3f}), 0.3, 0.3, 0.3);
    check_rgb(convert<SRGB>(HSL{0.f, 1.f, 1.f}), 1., 1., 1.);
    check_rgb(convert<SRGB>(HSL{0.f, 1.f, 0.f}), 0., 0., 0.);
    CHECK(convert<SRGB>(HSL{0.f, 1.f, 0.5f, 0.25f}).alpha() == 0.25f);
}

TEST_CASE("HSV converts to SRGB", "[colors][conversions]")
{
    using aest::colors::convert;

    for (const auto& [hue, r, g, b] : SECTORS)
    {
        CAPTURE(hue);
        check_rgb(convert<SRGB>(HSV{hue, 1.f, 1.f}), r, g, b);
    }
    check_rgb(convert<SRGB>(HSV{330.f, 1.f, 1.f}), 1., 0., 0.5);
    check_rgb(convert<SRGB>(HSV{210.f, 2.f / 3.f, 0.75f}), 0.25, 0.5, 0.75);
    check_rgb(convert<SRGB>(HSV{123.f, 0.f, 0.3f}), 0.3, 0.3, 0.3);
    check_rgb(convert<SRGB>(HSV{0.f, 1.f, 0.f}), 0., 0., 0.);
    CHECK(convert<SRGB>(HSV{0.f, 1.f, 1.f, 0.25f}).alpha() == 0.25f);
}

TEST_CASE("SRGB converts to HSL", "[colors][conversions]")
{
    using aest::colors::convert;

    for (const auto& [hue, r, g, b] : SECTORS)
    {
        CAPTURE(hue);
        const HSL hsl = convert<HSL>(SRGB{r, g, b});
        CHECK_THAT(hsl.hue(), WithinAbs(hue, 1e-4));
        CHECK_THAT(hsl.saturation(), WithinAbs(1., 1e-6));
        CHECK_THAT(hsl.lightness(), WithinAbs(0.5, 1e-6));
    }

    const HSL blueish = convert<HSL>(SRGB{0.25f, 0.5f, 0.75f, 0.25f});
    CHECK_THAT(blueish.hue(), WithinAbs(210., 1e-4));
    CHECK_THAT(blueish.saturation(), WithinAbs(0.5, 1e-6));
    CHECK_THAT(blueish.lightness(), WithinAbs(0.5, 1e-6));
    CHECK(blueish.alpha() == 0.25f);

    // Grays have no hue and no saturation
    for (const float value : {0.f, 0.3f, 1.f})
    {
        CAPTURE(value);
        const HSL gray = convert<HSL>(SRGB{value, value, value});
        CHECK(gray.hue() == 0.f);
        CHECK(gray.saturation() == 0.f);
        CHECK_THAT(gray.lightness(), WithinAbs(value, 1e-6));
    }
}

TEST_CASE("SRGB converts to HSV", "[colors][conversions]")
{
    using aest::colors::convert;

    for (const auto& [hue, r, g, b] : SECTORS)
    {
        CAPTURE(hue);
        const HSV hsv = convert<HSV>(SRGB{r, g, b});
        CHECK_THAT(hsv.hue(), WithinAbs(hue, 1e-4));
        CHECK_THAT(hsv.saturation(), WithinAbs(1., 1e-6));
        CHECK_THAT(hsv.brightness(), WithinAbs(1., 1e-6));
    }

    const HSV blueish = convert<HSV>(SRGB{0.25f, 0.5f, 0.75f, 0.25f});
    CHECK_THAT(blueish.hue(), WithinAbs(210., 1e-4));
    CHECK_THAT(blueish.saturation(), WithinAbs(2. / 3., 1e-6));
    CHECK_THAT(blueish.brightness(), WithinAbs(0.75, 1e-6));
    CHECK(blueish.alpha() == 0.25f);

    // Grays have no hue and no saturation
    for (const float value : {0.f, 0.3f, 1.f})
    {
        CAPTURE(value);
        const HSV gray = convert<HSV>(SRGB{value, value, value});
        CHECK(gray.hue() == 0.f);
        CHECK(gray.saturation() == 0.f);
        CHECK_THAT(gray.brightness(), WithinAbs(value, 1e-6));
    }
}

TEST_CASE("SRGB round-trips through HSL and HSV", "[colors][conversions]")
{
    using aest::colors::convert;

    for (std::int32_t r = 0; r <= 8; ++r)
        for (std::int32_t g = 0; g <= 8; ++g)
            for (std::int32_t b = 0; b <= 8; ++b)
            {
                const SRGB color{r / 8.f, g / 8.f, b / 8.f};
                CAPTURE(color.red(), color.green(), color.blue());

                const SRGB viaHsl = convert<SRGB>(convert<HSL>(color));
                CHECK_THAT(viaHsl.red(), WithinAbs(color.red(), 1e-06));
                CHECK_THAT(viaHsl.green(), WithinAbs(color.green(), 1e-06));
                CHECK_THAT(viaHsl.blue(), WithinAbs(color.blue(), 2e-06));

                const SRGB viaHsv = convert<SRGB>(convert<HSV>(color));
                CHECK_THAT(viaHsv.red(), WithinAbs(color.red(), 1e-06));
                CHECK_THAT(viaHsv.green(), WithinAbs(color.green(), 1e-06));
                CHECK_THAT(viaHsv.blue(), WithinAbs(color.blue(), 2e-06));
            }
}

TEST_CASE("Converting to the same type returns the color unchanged", "[colors][conversions]")
{
    using aest::colors::convert;

    STATIC_REQUIRE(std::is_same_v<decltype(convert<SRGB>(SRGB{})), SRGB>);
    STATIC_REQUIRE(convert<SRGB>(SRGB{0.1f, 0.2f, 0.3f, 0.4f}) == SRGB{0.1f, 0.2f, 0.3f, 0.4f});
    STATIC_REQUIRE(convert<LinSRGB>(LinSRGB{0.1f, 0.2f, 0.3f, 0.4f})
                   == LinSRGB{0.1f, 0.2f, 0.3f, 0.4f});
    STATIC_REQUIRE(convert<HSL>(HSL{210.f, 0.5f, 0.25f, 0.4f}) == HSL{210.f, 0.5f, 0.25f, 0.4f});
    STATIC_REQUIRE(convert<HSV>(HSV{210.f, 0.5f, 0.25f, 0.4f}) == HSV{210.f, 0.5f, 0.25f, 0.4f});
}

TEST_CASE("HSL converts to HSV", "[colors][conversions]")
{
    using aest::colors::convert;

    const HSV blueish = convert<HSV>(HSL{210.f, 0.5f, 0.5f, 0.25f});
    CHECK(blueish.hue() == 210.f);
    CHECK_THAT(blueish.saturation(), WithinAbs(2. / 3., 1e-6));
    CHECK_THAT(blueish.brightness(), WithinAbs(0.75, 1e-6));
    CHECK(blueish.alpha() == 0.25f);

    const HSV red = convert<HSV>(HSL{0.f, 1.f, 0.5f});
    CHECK(red.saturation() == 1.f);
    CHECK(red.brightness() == 1.f);

    const HSV white = convert<HSV>(HSL{0.f, 1.f, 1.f});
    CHECK(white.saturation() == 0.f);
    CHECK(white.brightness() == 1.f);

    const HSV black = convert<HSV>(HSL{0.f, 1.f, 0.f});
    CHECK(black.saturation() == 0.f);
    CHECK(black.brightness() == 0.f);
}

TEST_CASE("HSV converts to HSL", "[colors][conversions]")
{
    using aest::colors::convert;

    const HSL blueish = convert<HSL>(HSV{210.f, 2.f / 3.f, 0.75f, 0.25f});
    CHECK(blueish.hue() == 210.f);
    CHECK_THAT(blueish.saturation(), WithinAbs(0.5, 1e-6));
    CHECK_THAT(blueish.lightness(), WithinAbs(0.5, 1e-6));
    CHECK(blueish.alpha() == 0.25f);

    const HSL red = convert<HSL>(HSV{0.f, 1.f, 1.f});
    CHECK(red.saturation() == 1.f);
    CHECK(red.lightness() == 0.5f);

    const HSL white = convert<HSL>(HSV{0.f, 0.f, 1.f});
    CHECK(white.saturation() == 0.f);
    CHECK(white.lightness() == 1.f);

    const HSL black = convert<HSL>(HSV{0.f, 1.f, 0.f});
    CHECK(black.saturation() == 0.f);
    CHECK(black.lightness() == 0.f);
}

TEST_CASE("HSL and HSV convert like going through SRGB", "[colors][conversions]")
{
    using aest::colors::convert;

    for (std::int32_t r = 0; r <= 8; ++r)
        for (std::int32_t g = 0; g <= 8; ++g)
            for (std::int32_t b = 0; b <= 8; ++b)
            {
                const SRGB color{r / 8.f, g / 8.f, b / 8.f};
                CAPTURE(color.red(), color.green(), color.blue());

                const HSL hsl = convert<HSL>(color);
                const HSV hsv = convert<HSV>(color);

                const HSV toHsv = convert<HSV>(hsl);
                CHECK_THAT(toHsv.hue(), WithinAbs(hsv.hue(), 1e-4));
                CHECK_THAT(toHsv.saturation(), WithinAbs(hsv.saturation(), 1e-06));
                CHECK_THAT(toHsv.brightness(), WithinAbs(hsv.brightness(), 1e-06));

                const HSL toHsl = convert<HSL>(hsv);
                CHECK_THAT(toHsl.hue(), WithinAbs(hsl.hue(), 1e-4));
                CHECK_THAT(toHsl.saturation(), WithinAbs(hsl.saturation(), 1e-06));
                CHECK_THAT(toHsl.lightness(), WithinAbs(hsl.lightness(), 1e-06));
            }
}

TEST_CASE("Hue-based conversions are constant expressions", "[colors][conversions]")
{
    using aest::colors::convert;

    STATIC_REQUIRE(convert<SRGB>(HSL{210.f, 0.5f, 0.5f}) == SRGB{0.25f, 0.5f, 0.75f});
    STATIC_REQUIRE(convert<SRGB>(HSV{210.f, 0.5f, 0.75f}) == SRGB{0.375f, 0.5625f, 0.75f});
    STATIC_REQUIRE(convert<HSL>(SRGB{0.25f, 0.5f, 0.75f}) == HSL{210.f, 0.5f, 0.5f});
    STATIC_REQUIRE(convert<HSV>(SRGB{1.f, 0.f, 0.5f}) == HSV{330.f, 1.f, 1.f});
    STATIC_REQUIRE(convert<HSV>(HSL{210.f, 0.5f, 0.5f}).hue() == 210.f);
    STATIC_REQUIRE(convert<HSL>(HSV{210.f, 0.5f, 0.75f}).hue() == 210.f);
}

TEST_CASE("Conversions chain through the conversion tree", "[colors][conversions]")
{
    using aest::colors::convert;

    const HSL hsl{210.f, 0.5f, 0.5f, 0.25f};
    const LinSRGB linear = convert<LinSRGB>(hsl);
    CHECK(linear == convert<LinSRGB>(convert<SRGB>(hsl)));
    CHECK(linear.alpha() == 0.25f);

    const HSV hsv = convert<HSV>(linear);
    CHECK(hsv == convert<HSV>(convert<SRGB>(linear)));
    CHECK_THAT(hsv.hue(), WithinAbs(210., 1e-3));
}

namespace
{
/** @brief A color space outside the conversion tree, with no converters. */
struct Gray
{
    static constexpr std::string_view TYPE = "Gray"; ///< Color space name.

    /**
     * @brief Construct a gray from channel values; only the first one is kept.
     * @param[in] value Gray level
     * @param[in] unused1 Ignored
     * @param[in] unused2 Ignored
     * @param[in] alphaValue Alpha channel value
     */
    constexpr Gray(const float value,
                   [[maybe_unused]] const float unused1,
                   [[maybe_unused]] const float unused2,
                   const float alphaValue = 1.f)
        : _value(value)
        , _alpha(alphaValue)
    {}

    /** @brief Alpha channel. @return The alpha value. */
    [[nodiscard]] constexpr float alpha() const { return _alpha; }
    /** @brief First channel. @return The gray level. */
    [[nodiscard]] constexpr float ch1() const { return _value; }
    /** @brief Second channel. @return The gray level. */
    [[nodiscard]] constexpr float ch2() const { return _value; }
    /** @brief Third channel. @return The gray level. */
    [[nodiscard]] constexpr float ch3() const { return _value; }

    float _value; ///< Gray level
    float _alpha; ///< Alpha channel value
};
} // namespace

TEST_CASE("ConvertibleTo follows converters and the conversion tree", "[colors][conversions]")
{
    using aest::colors::ConvertibleTo;

    STATIC_REQUIRE(aest::colors::ColorType<Gray>);
    STATIC_REQUIRE(ConvertibleTo<HSV, HSL>);
    STATIC_REQUIRE(ConvertibleTo<LinSRGB, HSL>);
    STATIC_REQUIRE(ConvertibleTo<HSV, LinSRGB>);
    STATIC_REQUIRE(ConvertibleTo<Gray, Gray>);
    STATIC_REQUIRE_FALSE(ConvertibleTo<SRGB, Gray>);
    STATIC_REQUIRE_FALSE(ConvertibleTo<Gray, HSL>);
}

TEST_CASE("HSL and HSV clamp saturation, lightness and value", "[colors]")
{
    STATIC_REQUIRE(HSL{0.f, 2.f, -1.f} == HSL{0.f, 1.f, 0.f});
    STATIC_REQUIRE(HSV{0.f, -1.f, 2.f} == HSV{0.f, 0.f, 1.f});
    STATIC_REQUIRE(HSL{} == HSL{0.f, 0.f, 0.f, 1.f});
    STATIC_REQUIRE(HSV{} == HSV{0.f, 0.f, 0.f, 1.f});
}

TEST_CASE("opacity replaces alpha and keeps the channels", "[colors]")
{
    constexpr SRGB color{0.25f, 0.5f, 0.75f};
    STATIC_REQUIRE(color.opacity(0.4f) == SRGB{0.25f, 0.5f, 0.75f, 0.4f});
    STATIC_REQUIRE(color.opacity(2.f).alpha() == 1.f);
    STATIC_REQUIRE(color.opacity(-1.f).alpha() == 0.f);

    // The result has the same type, so non-RGB channels such as hue survive
    constexpr HSL hsl{300.f, 0.5f, 0.25f};
    STATIC_REQUIRE(hsl.opacity(0.5f) == HSL{300.f, 0.5f, 0.25f, 0.5f});
}

namespace
{
template <typename T>
concept Addable = requires(T color) { color + color; };
} // namespace

TEST_CASE("Only LinSRGB has arithmetic operators", "[colors]")
{
    STATIC_REQUIRE(Addable<LinSRGB>);
    STATIC_REQUIRE_FALSE(Addable<SRGB>);
    STATIC_REQUIRE_FALSE(Addable<aest::colors::HSL>);
    STATIC_REQUIRE_FALSE(Addable<aest::colors::HSV>);
}

TEST_CASE("LinSRGB arithmetic is channel-wise and clamped at 0", "[colors]")
{
    constexpr LinSRGB a{0.5f, 0.25f, 0.75f, 1.f};
    constexpr LinSRGB b{0.25f, 0.25f, 0.5f, 0.5f};

    STATIC_REQUIRE(a - b == LinSRGB{0.25f, 0.f, 0.25f, 0.5f});
    STATIC_REQUIRE(b - a == LinSRGB{0.f, 0.f, 0.f, 0.f});
    STATIC_REQUIRE(a + b == LinSRGB{0.75f, 0.5f, 1.25f, 1.f});
    STATIC_REQUIRE((a + b).blue() == 1.25f);
    STATIC_REQUIRE(a * b == LinSRGB{0.125f, 0.0625f, 0.375f, 0.5f});

    LinSRGB c = a;
    c -= b;
    CHECK(c == a - b);
    c += b;
    CHECK(c == a);
    c *= b;
    CHECK(c == a * b);
}

TEST_CASE("LinSRGB holds HDR values", "[colors]")
{
    constexpr LinSRGB bright{4.f, 1.5f, -0.5f, 2.f};
    STATIC_REQUIRE(bright.red() == 4.f);
    STATIC_REQUIRE(bright.green() == 1.5f);
    STATIC_REQUIRE(bright.blue() == 0.f);
    STATIC_REQUIRE(bright.alpha() == 1.f);

    constexpr LinSRGB sum = LinSRGB{0.75f, 0.75f, 0.75f} + LinSRGB{0.75f, 0.75f, 0.75f};
    STATIC_REQUIRE(sum == LinSRGB{1.5f, 1.5f, 1.5f, 1.f});
}

TEST_CASE("HDR LinSRGB values clip to 1 in SRGB", "[colors]")
{
    const SRGB srgb = aest::colors::convert<SRGB>(LinSRGB{4.f, 1.f, 0.5f, 0.5f});
    CHECK(srgb.red() == 1.f);
    CHECK(srgb.green() == 1.f);
    CHECK_THAT(srgb.blue(), WithinAbs(0.735357, 1e-06));
    CHECK(srgb.alpha() == 0.5f);
}

TEST_CASE("mix keeps HDR values in LinSRGB", "[colors][operations]")
{
    constexpr LinSRGB dim{2.f, 0.5f, 0.f};
    constexpr LinSRGB bright{4.f, 1.5f, 8.f};
    STATIC_REQUIRE(aest::colors::mix(dim, bright, 0.5f) == LinSRGB{3.f, 1.f, 4.f});
}

namespace
{
template <typename T>
concept ScalableByFloat = requires(T color, float scalar) { color * scalar; };
} // namespace

TEST_CASE("LinSRGB arithmetic combines alpha like the other channels", "[colors]")
{
    constexpr LinSRGB halfTransparent{0.25f, 0.25f, 0.25f, 0.5f};
    STATIC_REQUIRE((halfTransparent + halfTransparent).alpha() == 1.f);
    STATIC_REQUIRE((halfTransparent - halfTransparent).alpha() == 0.f);
    STATIC_REQUIRE((halfTransparent * halfTransparent).alpha() == 0.25f);
}

TEST_CASE("LinSRGB scales by a scalar, keeping alpha", "[colors]")
{
    STATIC_REQUIRE(ScalableByFloat<LinSRGB>);
    STATIC_REQUIRE_FALSE(ScalableByFloat<SRGB>);

    constexpr LinSRGB color{0.5f, 1.f, 0.25f, 0.5f};
    STATIC_REQUIRE(color * 2.f == LinSRGB{1.f, 2.f, 0.5f, 0.5f});
    STATIC_REQUIRE(2.f * color == color * 2.f);
    STATIC_REQUIRE(color / 4.f == LinSRGB{0.125f, 0.25f, 0.0625f, 0.5f});
    STATIC_REQUIRE(color * -1.f == LinSRGB{0.f, 0.f, 0.f, 0.5f});

    LinSRGB scaled = color;
    scaled *= 4.f;
    CHECK(scaled == LinSRGB{2.f, 4.f, 1.f, 0.5f});
    scaled /= 2.f;
    CHECK(scaled == LinSRGB{1.f, 2.f, 0.5f, 0.5f});
}

TEST_CASE("lighten moves HSL lightness toward white", "[colors][operations]")
{
    using aest::colors::lighten;

    const SRGB gray = lighten(SRGB{}, 0.5f);
    CHECK_THAT(gray.red(), WithinAbs(0.5, 1e-6));
    CHECK_THAT(gray.green(), WithinAbs(0.5, 1e-6));
    CHECK_THAT(gray.blue(), WithinAbs(0.5, 1e-6));

    // Orange is HSL(30, 1, 0.5); lightness 0.5 -> 0.75 keeps hue, saturation and alpha
    const SRGB light = lighten(SRGB{1.f, 0.5f, 0.f, 0.5f}, 0.5f);
    CHECK_THAT(light.red(), WithinAbs(1., 1e-6));
    CHECK_THAT(light.green(), WithinAbs(0.75, 1e-6));
    CHECK_THAT(light.blue(), WithinAbs(0.5, 1e-6));
    CHECK(light.alpha() == 0.5f);
}

TEST_CASE("lighten clamps the amount to [0, 1]", "[colors][operations]")
{
    using aest::colors::lighten;

    constexpr SRGB coral = SRGB::from_hex(0xFF'7F'50);
    CHECK(lighten(coral, 0.f).to_rgba() == coral.to_rgba());
    CHECK(lighten(coral, -1.f).to_rgba() == coral.to_rgba());
    CHECK(lighten(coral, 1.f).to_rgba() == 0xFF'FF'FF'FF);
    CHECK(lighten(coral, 2.f).to_rgba() == 0xFF'FF'FF'FF);
}

TEST_CASE("darken moves HSL lightness toward black", "[colors][operations]")
{
    using aest::colors::darken;

    // Orange is HSL(30, 1, 0.5); lightness 0.5 -> 0.25 keeps hue, saturation and alpha
    const SRGB dark = darken(SRGB{1.f, 0.5f, 0.f, 0.5f}, 0.5f);
    CHECK_THAT(dark.red(), WithinAbs(0.5, 1e-6));
    CHECK_THAT(dark.green(), WithinAbs(0.25, 1e-6));
    CHECK_THAT(dark.blue(), WithinAbs(0., 1e-6));
    CHECK(dark.alpha() == 0.5f);

    constexpr SRGB coral = SRGB::from_hex(0xFF'7F'50);
    CHECK(darken(coral, 0.f).to_rgba() == coral.to_rgba());
    CHECK(darken(coral, -1.f).to_rgba() == coral.to_rgba());
    CHECK(darken(coral, 1.f).to_rgba() == 0x00'00'00'FF);
    CHECK(darken(coral, 2.f).to_rgba() == 0x00'00'00'FF);
}

namespace
{
template <typename Space>
concept CanLightenIn = requires(SRGB color) { aest::colors::lighten<Space>(color, 0.5f); };
} // namespace

TEST_CASE("The working space is chosen with in<Space>", "[colors][operations]")
{
    using aest::colors::darken;
    using aest::colors::in;
    using aest::colors::lighten;

    constexpr SRGB coral = SRGB::from_hex(0xFF'7F'50);
    CHECK(lighten<HSL>(coral, 0.3f) == lighten(coral, 0.3f));
    CHECK(darken<HSL>(coral, 0.3f) == darken(coral, 0.3f));

    // The result has the input's type, with no conversion when it is the working space
    const HSL hsl{200.f, 0.5f, 0.4f, 0.75f};
    STATIC_REQUIRE(std::is_same_v<decltype(lighten(hsl, 0.5f)), HSL>);
    const HSL light = lighten(hsl, 0.5f);
    CHECK(light.hue() == 200.f);
    CHECK(light.saturation() == 0.5f);
    CHECK_THAT(light.lightness(), WithinAbs(0.7, 1e-6));
    CHECK(light.alpha() == 0.75f);

    STATIC_REQUIRE(CanLightenIn<HSL>);
    STATIC_REQUIRE_FALSE(CanLightenIn<SRGB>);
}

TEST_CASE("Operations convert through the conversion tree", "[colors][operations]")
{
    using aest::colors::convert;
    using aest::colors::euclidean;
    using aest::colors::in;
    using aest::colors::lighten;
    using aest::colors::mix;

    const LinSRGB linear{0.2f, 0.4f, 0.6f, 0.5f};
    const LinSRGB light    = lighten(linear, 0.5f);
    const LinSRGB expected = convert<LinSRGB>(lighten(convert<SRGB>(linear), 0.5f));
    CHECK_THAT(light.red(), WithinAbs(expected.red(), 1e-6));
    CHECK_THAT(light.green(), WithinAbs(expected.green(), 1e-6));
    CHECK_THAT(light.blue(), WithinAbs(expected.blue(), 1e-6));
    CHECK(light.alpha() == 0.5f);

    // Mixing HSL colors blends their light in LinSRGB
    const HSL red{0.f, 1.f, 0.5f};
    const HSL blue{240.f, 1.f, 0.5f};
    const SRGB blended = convert<SRGB>(mix(red, blue, 0.5f));
    const SRGB viaRgb  = mix(convert<SRGB>(red), convert<SRGB>(blue), 0.5f);
    CHECK_THAT(blended.red(), WithinAbs(viaRgb.red(), 1e-06));
    CHECK_THAT(blended.green(), WithinAbs(viaRgb.green(), 1e-06));
    CHECK_THAT(blended.blue(), WithinAbs(viaRgb.blue(), 1e-06));

    // Red and green are a full channel apart in each of two channels
    CHECK_THAT(euclidean(red, HSV{120.f, 1.f, 1.f}, in<LinSRGB>), WithinAbs(1.41421356, 1e-06));
}

TEST_CASE("mix blends in LinSRGB by default", "[colors][operations]")
{
    using aest::colors::in;
    using aest::colors::mix;

    const SRGB black{0.f, 0.f, 0.f};
    const SRGB white{1.f, 1.f, 1.f};

    // Half the light of white, encoded: 1.055 * 0.5^(1 / 2.4) - 0.055
    const SRGB linearMidpoint = mix(black, white, 0.5f);
    CHECK_THAT(linearMidpoint.red(), WithinAbs(0.735357, 1e-06));
    CHECK_THAT(linearMidpoint.green(), WithinAbs(0.735357, 1e-06));
    CHECK_THAT(linearMidpoint.blue(), WithinAbs(0.735357, 1e-06));
    CHECK(mix<LinSRGB>(black, white, 0.5f) == linearMidpoint);

    // in<SRGB> averages the encoded values, like CSS color-mix(in srgb, ...)
    CHECK(mix<SRGB>(black, white, 0.5f) == SRGB{0.5f, 0.5f, 0.5f});
}

TEST_CASE("mix goes from the first color to the second", "[colors][operations]")
{
    using aest::colors::mix;

    constexpr LinSRGB from{0.25f, 0.5f, 1.f, 0.f};
    constexpr LinSRGB to{0.75f, 0.f, 0.f, 1.f};

    STATIC_REQUIRE(mix(from, to, 0.f) == from);
    STATIC_REQUIRE(mix(from, to, 1.f) == to);
    STATIC_REQUIRE(mix(from, to, 0.25f) == LinSRGB{0.375f, 0.375f, 0.75f, 0.25f});

    // The amount is clamped to [0, 1]
    STATIC_REQUIRE(mix(from, to, -1.f) == from);
    STATIC_REQUIRE(mix(from, to, 2.f) == to);
}

namespace
{
template <typename Space>
concept CanMixIn = requires(SRGB color) { aest::colors::mix<Space>(color, color, 0.5f); };
} // namespace

TEST_CASE("mix returns the input type and needs an interpolation space", "[colors][operations]")
{
    using aest::colors::mix;

    const HSL red{0.f, 1.f, 0.5f};
    const HSL blue{240.f, 1.f, 0.5f};
    STATIC_REQUIRE(std::is_same_v<decltype(mix(red, blue, 0.5f)), HSL>);

    STATIC_REQUIRE(CanMixIn<LinSRGB>);
    STATIC_REQUIRE(CanMixIn<SRGB>);
    STATIC_REQUIRE_FALSE(CanMixIn<HSL>);
    STATIC_REQUIRE_FALSE(CanMixIn<HSV>);
}

TEST_CASE("euclidean measures in SRGB by default", "[colors][distance]")
{
    using aest::colors::euclidean;
    using aest::colors::in;

    const SRGB black{0.f, 0.f, 0.f};
    const SRGB white{1.f, 1.f, 1.f};
    const SRGB gray{0.5f, 0.5f, 0.5f};

    CHECK_THAT(euclidean(black, white), WithinAbs(1.732051, 1e-6)); // sqrt(3)
    CHECK_THAT(euclidean(black, gray), WithinAbs(0.866025, 1e-6));  // sqrt(3) * 0.5
    CHECK(euclidean(black, gray, in<SRGB>) == euclidean(black, gray));

    // In linear light the encoded 0.5 gray is only 0.214 bright: sqrt(3) * 0.214041
    CHECK_THAT(euclidean(black, white, in<LinSRGB>), WithinAbs(1.732051, 1e-6));
    CHECK_THAT(euclidean(black, gray, in<LinSRGB>), WithinAbs(0.370730, 1e-06));
}

TEST_CASE("euclidean is symmetric and ignores alpha", "[colors][distance]")
{
    using aest::colors::euclidean;

    const SRGB coral = SRGB::from_hex(0xFF'7F'50);
    const SRGB teal  = SRGB::from_hex(0x00'80'80);

    CHECK(euclidean(coral, coral) == 0.f);
    CHECK(euclidean(coral, teal) == euclidean(teal, coral));
    CHECK(euclidean(coral, teal) == euclidean(coral.opacity(0.f), teal));
}

namespace
{
template <typename Space>
concept CanMeasureIn =
    requires(SRGB color) { aest::colors::euclidean(color, color, aest::colors::in<Space>); };
} // namespace

TEST_CASE("euclidean accepts mixed color types and needs a Euclidean space", "[colors][distance]")
{
    using aest::colors::euclidean;

    CHECK_THAT(euclidean(HSL{0.f, 1.f, 0.5f}, SRGB{1.f, 0.f, 0.f}), WithinAbs(0., 1e-6));
    CHECK_THAT(euclidean(HSV{120.f, 1.f, 1.f}, SRGB{1.f, 0.f, 0.f}), WithinAbs(1.414214, 1e-6));

    STATIC_REQUIRE(CanMeasureIn<SRGB>);
    STATIC_REQUIRE(CanMeasureIn<LinSRGB>);
    STATIC_REQUIRE_FALSE(CanMeasureIn<HSL>);
    STATIC_REQUIRE_FALSE(CanMeasureIn<HSV>);
}

TEST_CASE("Named CSS colors live in the css namespace", "[colors]")
{
    namespace css = aest::colors::css;
    STATIC_REQUIRE(css::coral == SRGB::from_hex(0xFF'7F'50));
    STATIC_REQUIRE(css::midnightBlue == SRGB::from_hex(0x19'19'70));
}

namespace
{
using PremulLin = aest::colors::Premultiplied<LinSRGB>;

template <typename T>
concept Subtractable = requires(T color) { color - color; };
} // namespace

TEST_CASE("Premultiplied names its space and clamps like it", "[colors][compositing]")
{
    STATIC_REQUIRE(PremulLin::TYPE == "Premultiplied<LinSRGB>");
    STATIC_REQUIRE(aest::colors::ColorType<PremulLin>);
    STATIC_REQUIRE(PremulLin{} == PremulLin{0.f, 0.f, 0.f, 1.f});
    STATIC_REQUIRE(PremulLin{-1.f, 2.f, 0.5f, 3.f} == PremulLin{0.f, 2.f, 0.5f, 1.f});
    CHECK(std::format("{:.2f}", PremulLin{0.25f, 0.5f, 0.f, 0.5f})
          == "Premultiplied<LinSRGB>(0.25, 0.50, 0.00, 0.50)");

    STATIC_REQUIRE(aest::colors::LinearSpace<LinSRGB>);
    STATIC_REQUIRE_FALSE(aest::colors::LinearSpace<SRGB>);
    STATIC_REQUIRE_FALSE(aest::colors::LinearSpace<HSL>);
}

TEST_CASE("Premultiplied converts to and from straight alpha", "[colors][compositing]")
{
    using aest::colors::convert;

    constexpr LinSRGB straight{0.5f, 1.f, 0.25f, 0.5f};
    constexpr PremulLin premultiplied{0.25f, 0.5f, 0.125f, 0.5f};
    STATIC_REQUIRE(convert<PremulLin>(straight) == premultiplied);
    STATIC_REQUIRE(convert<LinSRGB>(premultiplied) == straight);

    // Fully transparent colors lose their light, so they come back as transparent black
    STATIC_REQUIRE(convert<LinSRGB>(convert<PremulLin>(LinSRGB{1.f, 1.f, 1.f, 0.f}))
                   == LinSRGB{0.f, 0.f, 0.f, 0.f});

    // Other spaces convert through the tree
    STATIC_REQUIRE(aest::colors::ConvertibleTo<PremulLin, HSL>);
    STATIC_REQUIRE(aest::colors::ConvertibleTo<HSV, PremulLin>);
    const SRGB back = convert<SRGB>(convert<PremulLin>(SRGB{1.f, 0.5f, 0.f, 0.25f}));
    CHECK_THAT(back.green(), WithinAbs(0.5, 1e-06));
    CHECK(back.alpha() == 0.25f);
}

TEST_CASE("Premultiplied arithmetic includes alpha", "[colors][compositing]")
{
    constexpr PremulLin a{0.25f, 0.5f, 0.f, 0.5f};
    constexpr PremulLin b{0.5f, 0.25f, 0.25f, 0.25f};
    STATIC_REQUIRE(a + b == PremulLin{0.75f, 0.75f, 0.25f, 0.75f});
    STATIC_REQUIRE(a * 0.5f == PremulLin{0.125f, 0.25f, 0.f, 0.25f});
    STATIC_REQUIRE(0.5f * a == a * 0.5f);
    STATIC_REQUIRE_FALSE(Subtractable<PremulLin>);

    PremulLin c = a;
    c += b;
    CHECK(c == a + b);
    c *= 2.f;
    CHECK(c == PremulLin{1.5f, 1.5f, 0.5f, 1.f});
}

TEST_CASE("over composites source-over in premultiplied linear light", "[colors][compositing]")
{
    using aest::colors::over;

    constexpr LinSRGB blue{0.f, 0.f, 1.f};
    constexpr LinSRGB halfRed{1.f, 0.f, 0.f, 0.5f};

    STATIC_REQUIRE(over(halfRed, blue) == LinSRGB{0.5f, 0.f, 0.5f, 1.f});
    STATIC_REQUIRE(over(LinSRGB{0.f, 1.f, 0.f}, blue) == LinSRGB{0.f, 1.f, 0.f});
    STATIC_REQUIRE(over(LinSRGB{0.f, 1.f, 0.f, 0.f}, blue) == blue);

    // Alpha combines as a + b (1 - a); the light is weighted by coverage
    constexpr LinSRGB halfBlue{0.f, 0.f, 1.f, 0.5f};
    constexpr LinSRGB both = over(halfRed, halfBlue);
    STATIC_REQUIRE(both.alpha() == 0.75f);
    STATIC_REQUIRE(both == LinSRGB{0.5f / 0.75f, 0.f, 0.25f / 0.75f, 0.75f});

    // The result has the background's type, and works in linear light
    const SRGB onWhite = over(SRGB{0.f, 0.f, 0.f, 0.5f}, SRGB{1.f, 1.f, 1.f});
    CHECK_THAT(onWhite.red(), WithinAbs(0.735357, 1e-06)); // linear 0.5, encoded
    CHECK(onWhite.alpha() == 1.f);
    STATIC_REQUIRE(std::is_same_v<decltype(over(halfRed, SRGB{})), SRGB>);

    // A premultiplied background accumulates layers without leaving premultiplied form
    constexpr PremulLin frame = over(halfRed, PremulLin{0.f, 0.f, 1.f});
    STATIC_REQUIRE(frame == PremulLin{0.5f, 0.f, 0.5f, 1.f});
}

TEST_CASE("luminance is the Y of XYZ", "[colors][operations]")
{
    using aest::colors::luminance;

    STATIC_REQUIRE(luminance(LinSRGB{1.f, 1.f, 1.f}) == 1.f);
    STATIC_REQUIRE(luminance(LinSRGB{0.f, 0.f, 0.f}) == 0.f);
    STATIC_REQUIRE(luminance(LinSRGB{2.f, 2.f, 2.f}) == 2.f);

    // For sRGB, the Rec. 709 coefficients, derived from the primaries
    CHECK_THAT(luminance(LinSRGB{1.f, 0.f, 0.f, 0.25f}), WithinAbs(0.2126, 1e-4));
    CHECK_THAT(luminance(LinSRGB{0.f, 1.f, 0.f}), WithinAbs(0.7152, 1e-4));
    CHECK_THAT(luminance(LinSRGB{0.f, 0.f, 1.f}), WithinAbs(0.0722, 1e-4));

    // Other gamuts weight their own primaries
    CHECK_THAT(luminance(aest::colors::LinRec2020{1.f, 0.f, 0.f}), WithinAbs(0.262700, 1e-6));
    CHECK_THAT(luminance(aest::colors::LinDisplayP3{1.f, 1.f, 1.f}), WithinAbs(1., 1e-6));

    // Other spaces are linearized first
    CHECK_THAT(luminance(SRGB::from_hex(0x80'80'80)), WithinAbs(0.2158605, 1e-06));
    CHECK_THAT(luminance(HSL{0.f, 0.f, 1.f}), WithinAbs(1., 1e-6));
}

TEST_CASE("contrast_ratio follows WCAG 2.x", "[colors][operations]")
{
    using aest::colors::contrast_ratio;

    const SRGB white = SRGB::from_hex(0xFF'FF'FF);
    const SRGB black = SRGB::from_hex(0x00'00'00);
    CHECK_THAT(contrast_ratio(black, white), WithinAbs(21., 1e-05));
    CHECK(contrast_ratio(white, black) == contrast_ratio(black, white));
    CHECK(contrast_ratio(white, white) == 1.f);

    // #777777 on white just fails the 4.5:1 AA threshold
    CHECK_THAT(contrast_ratio(SRGB::from_hex(0x77'77'77), white), WithinAbs(4.478089, 1e-06));

    // Mixed types, and HDR colors clip to white as a display shows them
    CHECK_THAT(contrast_ratio(HSL{0.f, 0.f, 0.f}, LinSRGB{4.f, 4.f, 4.f}), WithinAbs(21., 1e-05));
}

namespace
{
using aest::colors::XYZ;

/**
 * @brief Checks the channels of @p color against expected XYZ values.
 * @param[in] color The color to check
 * @param[in] x Expected X
 * @param[in] y Expected Y
 * @param[in] z Expected Z
 * @param[in] margin Tolerance of each channel
 */
void check_xyz(
    const XYZ& color, const double x, const double y, const double z, const double margin)
{
    CHECK_THAT(color.x(), WithinAbs(x, margin));
    CHECK_THAT(color.y(), WithinAbs(y, margin));
    CHECK_THAT(color.z(), WithinAbs(z, margin));
}
} // namespace

TEST_CASE("XYZ is unclamped and linear", "[colors][xyz]")
{
    constexpr XYZ color{-0.25f, 2.f, 0.5f, 0.75f};
    STATIC_REQUIRE(color.x() == -0.25f);
    STATIC_REQUIRE(color.y() == 2.f);
    STATIC_REQUIRE(color.z() == 0.5f);
    STATIC_REQUIRE(XYZ{} == XYZ{0.f, 0.f, 0.f, 1.f});
    STATIC_REQUIRE(color * 2.f == XYZ{-0.5f, 4.f, 1.f, 0.75f});
    STATIC_REQUIRE(aest::colors::LinearSpace<XYZ>);
}

TEST_CASE("LinSRGB converts to XYZ with the IEC 61966-2-1 matrix", "[colors][xyz]")
{
    using aest::colors::convert;

    // The columns of the matrix derived from the sRGB chromaticities match the
    // four-decimal table of IEC 61966-2-1
    check_xyz(convert<XYZ>(LinSRGB{1.f, 0.f, 0.f}), 0.4124, 0.2126, 0.0193, 1e-4);
    check_xyz(convert<XYZ>(LinSRGB{0.f, 1.f, 0.f}), 0.3576, 0.7152, 0.1192, 1e-4);
    check_xyz(convert<XYZ>(LinSRGB{0.f, 0.f, 1.f}), 0.1805, 0.0722, 0.9505, 1e-4);

    // White maps to the D65 white point at Y = 1
    check_xyz(convert<XYZ>(LinSRGB{1.f, 1.f, 1.f}), 0.950456, 1., 1.089058, 1e-6);

    STATIC_REQUIRE(convert<XYZ>(LinSRGB{0.f, 0.f, 0.f, 0.5f}) == XYZ{0.f, 0.f, 0.f, 0.5f});
    STATIC_REQUIRE(convert<XYZ>(LinSRGB{1.f, 1.f, 1.f}).y() > 0.99999f);
}

TEST_CASE("XYZ round-trips through LinSRGB, HDR values included", "[colors][xyz]")
{
    using aest::colors::convert;

    for (const LinSRGB color : {LinSRGB{0.25f, 0.5f, 0.75f, 0.5f}, LinSRGB{4.f, 0.f, 1.5f}})
    {
        const LinSRGB back = convert<LinSRGB>(convert<XYZ>(color));
        CHECK_THAT(back.red(), WithinAbs(color.red(), 1e-06));
        CHECK_THAT(back.green(), WithinAbs(color.green(), 1e-06));
        CHECK_THAT(back.blue(), WithinAbs(color.blue(), 1e-06));
        CHECK(back.alpha() == color.alpha());
    }
}

TEST_CASE("XYZ outside the sRGB gamut clips at 0 in LinSRGB", "[colors][xyz]")
{
    // Pure Y has no sRGB mix: red and blue would be negative
    const LinSRGB clipped = aest::colors::convert<LinSRGB>(XYZ{0.f, 1.f, 0.f});
    CHECK(clipped.red() == 0.f);
    CHECK_THAT(clipped.green(), WithinAbs(1.8759675, 1e-06));
    CHECK(clipped.blue() == 0.f);
}

TEST_CASE("XYZ is the root of the conversion tree", "[colors][xyz]")
{
    using aest::colors::convert;
    using aest::colors::ConvertibleTo;

    STATIC_REQUIRE(ConvertibleTo<XYZ, HSL>);
    STATIC_REQUIRE(ConvertibleTo<HSV, XYZ>);
    STATIC_REQUIRE(ConvertibleTo<XYZ, aest::colors::Premultiplied<LinSRGB>>);
    STATIC_REQUIRE_FALSE(ConvertibleTo<XYZ, Gray>);

    // HSL and HSV still convert through SRGB, without a round-trip through XYZ
    STATIC_REQUIRE(convert<HSV>(HSL{210.f, 0.5f, 0.5f}).hue() == 210.f);

    const XYZ white = convert<XYZ>(SRGB::from_hex(0xFF'FF'FF));
    CHECK_THAT(white.y(), WithinAbs(1., 1e-6));
    CHECK(std::format("{:.2f}", XYZ{0.5f, 1.f, 0.25f}) == "XYZ(0.50, 1.00, 0.25, 1.00)");
}

namespace
{
using aest::colors::ACEScg;
using aest::colors::DisplayP3;
using aest::colors::LinDisplayP3;
using aest::colors::LinRec2020;

template <typename T>
concept HasFromHex = requires { T::from_hex(0u); };
} // namespace

TEST_CASE("BasicRGB names each space after its gamut and transfer", "[colors][gamut]")
{
    STATIC_REQUIRE(SRGB::TYPE == "SRGB");
    STATIC_REQUIRE(LinSRGB::TYPE == "LinSRGB");
    STATIC_REQUIRE(DisplayP3::TYPE == "DisplayP3");
    STATIC_REQUIRE(LinDisplayP3::TYPE == "LinDisplayP3");
    STATIC_REQUIRE(LinRec2020::TYPE == "LinRec2020");
    STATIC_REQUIRE(ACEScg::TYPE == "ACEScg");
    CHECK(std::format("{:.1f}", DisplayP3{1.f, 0.5f, 0.f}) == "DisplayP3(1.0, 0.5, 0.0, 1.0)");
}

TEST_CASE("BasicRGB clamps and adds arithmetic by transfer function", "[colors][gamut]")
{
    using aest::colors::BoundedRGBSpace;
    using aest::colors::LinearRGBSpace;
    using aest::colors::RGBSpace;

    STATIC_REQUIRE(DisplayP3{2.f, -1.f, 0.5f} == DisplayP3{1.f, 0.f, 0.5f});
    STATIC_REQUIRE(LinDisplayP3{2.f, -1.f, 0.5f} == LinDisplayP3{2.f, 0.f, 0.5f});

    STATIC_REQUIRE(Addable<LinDisplayP3>);
    STATIC_REQUIRE(Addable<ACEScg>);
    STATIC_REQUIRE_FALSE(Addable<DisplayP3>);
    STATIC_REQUIRE(ScalableByFloat<LinRec2020>);

    STATIC_REQUIRE(RGBSpace<DisplayP3>);
    STATIC_REQUIRE_FALSE(RGBSpace<HSL>);
    STATIC_REQUIRE_FALSE(RGBSpace<XYZ>);
    STATIC_REQUIRE(LinearRGBSpace<ACEScg>);
    STATIC_REQUIRE_FALSE(LinearRGBSpace<SRGB>);
    STATIC_REQUIRE(BoundedRGBSpace<DisplayP3>);
    STATIC_REQUIRE_FALSE(BoundedRGBSpace<LinSRGB>);

    // Hex codes and bytes stay with SRGB, the space they are written in
    STATIC_REQUIRE(HasFromHex<SRGB>);
    STATIC_REQUIRE_FALSE(HasFromHex<DisplayP3>);
    STATIC_REQUIRE_FALSE(HasFromHex<LinSRGB>);
}

TEST_CASE("Gamut matrices are derived from the chromaticities", "[colors][gamut]")
{
    using aest::colors::convert;

    check_xyz(convert<XYZ>(LinDisplayP3{1.f, 0.f, 0.f}), 0.486571, 0.228975, 0., 1e-6);
    check_xyz(convert<XYZ>(LinRec2020{1.f, 0.f, 0.f}), 0.636958, 0.262700, 0., 1e-6);

    // Every gamut maps its white to the D65 white, ACEScg through Bradford adaptation
    check_xyz(convert<XYZ>(LinDisplayP3{1.f, 1.f, 1.f}), 0.950456, 1., 1.089058, 1e-6);
    check_xyz(convert<XYZ>(ACEScg{1.f, 1.f, 1.f}), 0.950456, 1., 1.089058, 1e-6);

    // The usual ACEScg to linear sRGB matrix, first row
    const LinSRGB red = convert<LinSRGB>(ACEScg{1.f, 0.f, 0.f});
    CHECK_THAT(red.red(), WithinAbs(1.705051, 1e-06));
    STATIC_REQUIRE(convert<XYZ>(LinDisplayP3{0.f, 0.f, 0.f}) == XYZ{});
}

TEST_CASE("Wide gamuts round-trip, encoded and linear", "[colors][gamut]")
{
    using aest::colors::convert;

    const DisplayP3 p3{0.25f, 0.5f, 0.75f, 0.5f};
    const DisplayP3 p3Back = convert<DisplayP3>(convert<XYZ>(p3));
    CHECK_THAT(p3Back.red(), WithinAbs(p3.red(), 1e-06));
    CHECK_THAT(p3Back.green(), WithinAbs(p3.green(), 1e-06));
    CHECK_THAT(p3Back.blue(), WithinAbs(p3.blue(), 1e-06));
    CHECK(p3Back.alpha() == 0.5f);

    // Every sRGB color fits in the wider gamuts, so it comes back unchanged
    const SRGB srgb = SRGB::from_hex(0x33'99'CC);
    for (const SRGB back :
         {convert<SRGB>(convert<DisplayP3>(srgb)), convert<SRGB>(convert<LinRec2020>(srgb)),
          convert<SRGB>(convert<ACEScg>(srgb))})
    {
        CHECK_THAT(back.red(), WithinAbs(srgb.red(), 1e-06));
        CHECK_THAT(back.green(), WithinAbs(srgb.green(), 1e-06));
        CHECK_THAT(back.blue(), WithinAbs(srgb.blue(), 1e-06));
    }

    // Converting within one gamut skips XYZ
    const LinDisplayP3 linear = convert<LinDisplayP3>(p3);
    CHECK_THAT(linear.red(), WithinAbs(0.050876, 1e-06));
}

TEST_CASE("Colors outside the sRGB gamut clip at 0", "[colors][gamut]")
{
    using aest::colors::convert;

    // P3 red needs negative green and blue in sRGB, and more than 1 red
    const LinSRGB linear = convert<LinSRGB>(LinDisplayP3{1.f, 0.f, 0.f});
    CHECK_THAT(linear.red(), WithinAbs(1.224940, 1e-06));
    CHECK(linear.green() == 0.f);
    CHECK(linear.blue() == 0.f);

    const SRGB encoded = convert<SRGB>(DisplayP3{1.f, 0.f, 0.f});
    CHECK(encoded == SRGB{1.f, 0.f, 0.f});
}

TEST_CASE("Operations work on every RGB gamut", "[colors][gamut]")
{
    using aest::colors::mix;

    // mix in LinDisplayP3 keeps the P3 red that LinSRGB would clip
    const DisplayP3 red{1.f, 0.f, 0.f};
    CHECK(mix<LinDisplayP3>(red, red, 0.5f) == red);
    CHECK(mix<DisplayP3>(red, DisplayP3{0.f, 0.f, 1.f}, 0.5f) == DisplayP3{0.5f, 0.f, 0.5f});
    CHECK_THAT(aest::colors::euclidean(LinDisplayP3{1.f, 0.f, 0.f}, LinDisplayP3{},
                                       aest::colors::in<LinDisplayP3>),
               WithinAbs(1., 1e-6));

    using PremulP3 = aest::colors::Premultiplied<LinDisplayP3>;
    STATIC_REQUIRE(PremulP3::TYPE == "Premultiplied<LinDisplayP3>");
    STATIC_REQUIRE(aest::colors::over<LinDisplayP3>(LinDisplayP3{1.f, 0.f, 0.f, 0.5f},
                                                    LinDisplayP3{0.f, 0.f, 1.f})
                   == LinDisplayP3{0.5f, 0.f, 0.5f});
}

namespace
{
/** @brief The Adobe RGB (1998) gamut, defined outside the library. */
struct AdobeRGBGamut
{
    static constexpr std::string_view NAME            = "AdobeRGB";        ///< Color space name.
    static constexpr aest::colors::Chromaticity RED   = {0.64, 0.33};      ///< Red primary.
    static constexpr aest::colors::Chromaticity GREEN = {0.21, 0.71};      ///< Green primary.
    static constexpr aest::colors::Chromaticity BLUE  = {0.15, 0.06};      ///< Blue primary.
    static constexpr aest::colors::Chromaticity WHITE = aest::colors::D65; ///< White point.
};

/** @brief A pure 2.2 gamma, defined outside the library. */
struct Gamma22
{
    static constexpr std::string_view PREFIX = "Gamma22"; ///< Name prefix.

    /** @brief Encode. @param[in] value Linear value. @return Encoded value. */
    static float encode(const float value) { return std::pow(value, 1.f / 2.2f); }

    /** @brief Decode. @param[in] value Encoded value. @return Linear value. */
    static float decode(const float value) { return std::pow(value, 2.2f); }

    /** @brief Clamp. @param[in] value Channel value. @return Value in [0, 1]. */
    static constexpr float clamp(const float value) { return std::clamp(value, 0.f, 1.f); }
};

using LinAdobeRGB = aest::colors::BasicRGB<AdobeRGBGamut, aest::colors::tf::Linear>;
using AdobeRGB    = aest::colors::BasicRGB<AdobeRGBGamut, Gamma22>;
} // namespace

TEST_CASE("Gamuts and transfer functions defined outside the library join the tree",
          "[colors][gamut]")
{
    using aest::colors::convert;

    STATIC_REQUIRE(LinAdobeRGB::TYPE == "LinAdobeRGB");
    STATIC_REQUIRE(AdobeRGB::TYPE == "Gamma22AdobeRGB");
    STATIC_REQUIRE(aest::colors::ConvertibleTo<HSL, AdobeRGB>);

    // The red column of the published Adobe RGB (1998) matrix
    check_xyz(convert<XYZ>(LinAdobeRGB{1.f, 0.f, 0.f}), 0.5767, 0.2973, 0.0270, 1e-4);

    const AdobeRGB gray = convert<AdobeRGB>(LinAdobeRGB{0.5f, 0.5f, 0.5f});
    CHECK_THAT(gray.red(), WithinAbs(0.729740, 1e-06)); // 0.5^(1 / 2.2)

    const SRGB srgb = SRGB::from_hex(0x33'99'CC);
    const SRGB back = convert<SRGB>(convert<AdobeRGB>(srgb));
    CHECK_THAT(back.green(), WithinAbs(srgb.green(), 1e-06));
}

namespace
{
template <typename RGB>
concept HasHSL = requires { typename aest::colors::BasicHSL<RGB>; };

template <typename RGB>
concept HasHSV = requires { typename aest::colors::BasicHSV<RGB>; };

using HSLP3 = aest::colors::BasicHSL<DisplayP3>;
using HSVP3 = aest::colors::BasicHSV<DisplayP3>;
} // namespace

TEST_CASE("HSL and HSV are views of any encoded RGB space", "[colors][cylindrical]")
{
    STATIC_REQUIRE(std::is_same_v<HSL, aest::colors::BasicHSL<SRGB>>);
    STATIC_REQUIRE(HSL::TYPE == "HSL");
    STATIC_REQUIRE(HSV::TYPE == "HSV");
    STATIC_REQUIRE(HSLP3::TYPE == "HSL<DisplayP3>");
    STATIC_REQUIRE(HSVP3::TYPE == "HSV<DisplayP3>");
    CHECK(std::format("{:.1f}", HSLP3{120.f, 1.f, 0.5f}) == "HSL<DisplayP3>(120.0, 1.0, 0.5, 1.0)");

    // The formulas need channels in [0, 1]: linear spaces are rejected
    STATIC_REQUIRE(HasHSL<DisplayP3>);
    STATIC_REQUIRE(HasHSV<SRGB>);
    STATIC_REQUIRE_FALSE(HasHSL<LinSRGB>);
    STATIC_REQUIRE_FALSE(HasHSV<ACEScg>);
    STATIC_REQUIRE_FALSE(HasHSL<XYZ>);
}

TEST_CASE("HSL over DisplayP3 converts through DisplayP3", "[colors][cylindrical]")
{
    using aest::colors::convert;

    // Same formulas as over SRGB, applied to the P3 channels
    STATIC_REQUIRE(convert<DisplayP3>(HSLP3{0.f, 1.f, 0.5f}) == DisplayP3{1.f, 0.f, 0.f});
    STATIC_REQUIRE(convert<HSVP3>(DisplayP3{1.f, 0.f, 0.5f}) == HSVP3{330.f, 1.f, 1.f});

    // HSL <-> HSV over one RGB space keeps the hue exactly
    STATIC_REQUIRE(convert<HSVP3>(HSLP3{210.f, 0.5f, 0.5f}).hue() == 210.f);

    // P3 red is outside sRGB: HSL over P3 keeps it, HSL over sRGB clips it
    const HSLP3 p3Red{0.f, 1.f, 0.5f};
    const DisplayP3 back = convert<DisplayP3>(convert<XYZ>(p3Red));
    CHECK_THAT(back.red(), WithinAbs(1., 1e-06));
    CHECK_THAT(back.green(), WithinAbs(0., 1e-06));
    const DisplayP3 viaSrgb = convert<DisplayP3>(convert<HSL>(p3Red));
    CHECK(viaSrgb.green() > 0.1f);

    // Between gamuts, the path goes through XYZ. A hue is relative to the
    // primaries, so the same color has another hue in the HSL of another gamut
    const HSL fromP3 = convert<HSL>(HSLP3{210.f, 0.5f, 0.5f, 0.25f});
    CHECK(fromP3.alpha() == 0.25f);
    CHECK_THAT(fromP3.hue(), WithinAbs(204.460031, 1e-3));
    CHECK_THAT(fromP3.saturation(), WithinAbs(0.72279826, 1e-06));
    CHECK_THAT(fromP3.lightness(), WithinAbs(0.44760169, 1e-06));
}

TEST_CASE("lighten works in the HSL of any RGB space", "[colors][cylindrical]")
{
    using aest::colors::lighten;

    const DisplayP3 red{1.f, 0.f, 0.f};
    const DisplayP3 light = lighten<HSLP3>(red, 0.5f);
    CHECK(light == DisplayP3{1.f, 0.5f, 0.5f});
}

namespace
{
template <typename From, typename To>
concept MixableTogether = requires(From from, To to) { aest::colors::mix(from, to, 0.5f); };
} // namespace

TEST_CASE("mix follows the gamut of its input by default", "[colors][operations]")
{
    using aest::colors::convert;
    using aest::colors::mix;

    // P3 red and green blend in LinDisplayP3: in LinSRGB, P3 red would clip
    const DisplayP3 red{1.f, 0.f, 0.f};
    const DisplayP3 green{0.f, 1.f, 0.f};
    const DisplayP3 blend = mix(red, green, 0.5f);
    CHECK_THAT(blend.red(), WithinAbs(0.735357, 1e-06)); // linear 0.5, encoded
    CHECK_THAT(blend.green(), WithinAbs(0.735357, 1e-06));
    CHECK(blend.blue() == 0.f);
    CHECK(blend == mix<LinDisplayP3>(red, green, 0.5f));
    CHECK(mix(red, red, 0.5f) == red);

    // HSL over P3 follows P3 too, and SRGB still blends in LinSRGB
    const HSLP3 hslRed = convert<HSLP3>(red);
    CHECK_THAT(convert<DisplayP3>(mix(hslRed, hslRed, 0.5f)).green(), WithinAbs(0., 1e-06));
    CHECK(mix(SRGB{1.f, 0.f, 0.f}, SRGB{0.f, 0.f, 1.f}, 0.5f)
          == mix<LinSRGB>(SRGB{1.f, 0.f, 0.f}, SRGB{0.f, 0.f, 1.f}, 0.5f));

    // XYZ blends in XYZ, so colors outside every gamut survive
    STATIC_REQUIRE(mix(XYZ{0.f, 1.f, 0.f}, XYZ{0.f, 0.f, 1.f}, 0.5f) == XYZ{0.f, 0.5f, 0.5f});

    // An explicit space still wins
    CHECK(mix<DisplayP3>(red, green, 0.5f) == DisplayP3{0.5f, 0.5f, 0.f});
    CHECK(mix(red, green, 0.5f, aest::colors::in<DisplayP3>) == DisplayP3{0.5f, 0.5f, 0.f});
}

TEST_CASE("mix rejects colors of two types", "[colors][operations]")
{
    using aest::colors::convert;
    using aest::colors::mix;

    STATIC_REQUIRE(MixableTogether<SRGB, SRGB>);
    STATIC_REQUIRE_FALSE(MixableTogether<SRGB, DisplayP3>);
    STATIC_REQUIRE_FALSE(MixableTogether<SRGB, HSL>);

    // Converting into the wider gamut first keeps the P3 color
    const SRGB srgbBlue{0.f, 0.f, 1.f};
    const DisplayP3 p3Red{1.f, 0.f, 0.f};
    const DisplayP3 blend = mix(convert<DisplayP3>(srgbBlue), p3Red, 1.f);
    CHECK(blend == p3Red);
}

TEST_CASE("over follows the gamut of its background by default", "[colors][compositing]")
{
    using aest::colors::over;

    constexpr LinDisplayP3 halfRed{1.f, 0.f, 0.f, 0.5f};
    constexpr LinDisplayP3 blue{0.f, 0.f, 1.f};
    STATIC_REQUIRE(over(halfRed, blue) == LinDisplayP3{0.5f, 0.f, 0.5f});

    // P3 red over a P3 background keeps its red, which LinSRGB would clip
    const DisplayP3 onBlue = over(DisplayP3{1.f, 0.f, 0.f}, DisplayP3{0.f, 0.f, 1.f});
    CHECK(onBlue == DisplayP3{1.f, 0.f, 0.f});
}

namespace
{
using aest::colors::Rec2100HLG;
using aest::colors::Rec2100PQ;
} // namespace

TEST_CASE("tf::PQ follows SMPTE ST 2084 with linear 1.0 at 203 cd/m2", "[colors][transfer]")
{
    using aest::colors::REFERENCE_WHITE_NITS;
    using aest::colors::tf::PQ;

    // Absolute luminances become linear values through the reference white
    CHECK(PQ::encode(0.f) < 1e-6f);
    CHECK_THAT(PQ::encode(10000.f / REFERENCE_WHITE_NITS), WithinAbs(1., 1e-6));
    CHECK_THAT(PQ::encode(100.f / REFERENCE_WHITE_NITS), WithinAbs(0.50807842, 1e-06));
    CHECK_THAT(PQ::encode(1.f), WithinAbs(0.580689, 1e-06)); // BT.2408: reference white at 58%
    CHECK_THAT(PQ::encode(1000.f / REFERENCE_WHITE_NITS), WithinAbs(0.751827, 1e-06));

    for (const float linear : {0.001f, 0.01f, 1.f, 4.f, 20.f})
        CHECK_THAT(PQ::decode(PQ::encode(linear)), WithinRel(linear, 1e-6f));
}

TEST_CASE("Rec2100PQ puts linear 1.0 at 203 cd/m2", "[colors][transfer]")
{
    using aest::colors::convert;

    STATIC_REQUIRE(Rec2100PQ::TYPE == "Rec2100PQ");
    STATIC_REQUIRE(Rec2100HLG::TYPE == "Rec2100HLG");
    STATIC_REQUIRE(aest::colors::HDRTransfer<aest::colors::tf::PQ>);
    STATIC_REQUIRE_FALSE(aest::colors::HDRTransfer<aest::colors::tf::SRGB>);

    const Rec2100PQ white = convert<Rec2100PQ>(LinRec2020{1.f, 1.f, 1.f});
    CHECK_THAT(white.red(), WithinAbs(0.580689, 1e-06));

    // sRGB white is the reference white too, so it lands on the same signal
    const Rec2100PQ srgbWhite = convert<Rec2100PQ>(SRGB::from_hex(0xFF'FF'FF));
    CHECK_THAT(srgbWhite.green(), WithinAbs(0.580689, 1e-06));

    // HDR values round-trip: four times reference white is 812 cd/m2
    const LinRec2020 bright{4.f, 2.f, 0.5f};
    const Rec2100PQ encoded = convert<Rec2100PQ>(bright);
    CHECK_THAT(encoded.red(), WithinAbs(0.729145, 1e-06));
    const LinRec2020 back = convert<LinRec2020>(encoded);
    CHECK_THAT(back.red(), WithinRel(4.f, 1e-6f));
    CHECK_THAT(back.green(), WithinRel(2.f, 1e-6f));
    CHECK_THAT(back.blue(), WithinRel(0.5f, 1e-6f));

    // The signal tops out at 10000 cd/m2, linear 49.26
    CHECK(convert<Rec2100PQ>(LinRec2020{100.f, 0.f, 0.f}).red() == 1.f);
}

TEST_CASE("Rec2100HLG puts linear 1.0 at 75%", "[colors][transfer]")
{
    using aest::colors::convert;

    const Rec2100HLG white = convert<Rec2100HLG>(LinRec2020{1.f, 1.f, 1.f});
    CHECK_THAT(white.red(), WithinAbs(0.75, 1e-06));

    // The two segments of the OETF meet at scene light 1/12, signal 0.5
    CHECK_THAT(aest::colors::tf::HLG::encode(1.f / 12 / 0.264962560f), WithinAbs(0.5, 1e-06));
    CHECK_THAT(aest::colors::tf::HLG::encode(2.f), WithinAbs(0.882541, 1e-06));

    for (const float linear : {0.f, 0.1f, 1.f, 3.f})
        CHECK_THAT(aest::colors::tf::HLG::decode(aest::colors::tf::HLG::encode(linear)),
                   WithinAbs(linear, 1e-6));
    CHECK(convert<Rec2100HLG>(LinRec2020{4.f, 0.f, 0.f}).red() == 1.f);
}

TEST_CASE("HDR signals join the tree but not HSL", "[colors][transfer]")
{
    using aest::colors::convert;

    STATIC_REQUIRE_FALSE(aest::colors::BoundedRGBSpace<Rec2100PQ>);
    STATIC_REQUIRE_FALSE(HasHSL<Rec2100PQ>);
    STATIC_REQUIRE_FALSE(HasHSV<Rec2100HLG>);
    STATIC_REQUIRE(aest::colors::ConvertibleTo<Rec2100HLG, Rec2100PQ>);

    // PQ to HLG through their shared linear space, and mix in linear light
    const Rec2100HLG hlg = convert<Rec2100HLG>(convert<Rec2100PQ>(LinRec2020{1.f, 1.f, 1.f}));
    CHECK_THAT(hlg.blue(), WithinAbs(0.75, 1e-06));
    const Rec2100PQ pq{0.58f, 0.58f, 0.58f};
    CHECK_THAT(aest::colors::mix(pq, pq, 0.5f).red(), WithinAbs(0.58, 1e-06));
}

namespace
{
template <typename T>
concept Tonemappable = requires(T color) { aest::colors::tonemap::reinhard(color); };
} // namespace

TEST_CASE("reinhard maps each channel into [0, 1)", "[colors][tonemap]")
{
    using aest::colors::tonemap::reinhard;

    STATIC_REQUIRE(reinhard(LinSRGB{0.f, 1.f, 3.f, 0.5f}) == LinSRGB{0.f, 0.5f, 0.75f, 0.5f});
    STATIC_REQUIRE(reinhard(LinDisplayP3{1000.f, 1000.f, 1000.f}).red() < 1.f);

    STATIC_REQUIRE(Tonemappable<ACEScg>);
    STATIC_REQUIRE_FALSE(Tonemappable<SRGB>);
    STATIC_REQUIRE_FALSE(Tonemappable<XYZ>);
}

TEST_CASE("reinhard_extended maps white to 1", "[colors][tonemap]")
{
    using aest::colors::tonemap::reinhard_extended;

    STATIC_REQUIRE(reinhard_extended(LinSRGB{4.f, 0.f, 1.f, 0.25f}, 4.f)
                   == LinSRGB{1.f, 0.f, 0.53125f, 0.25f}); // 1 (1 + 1 / 16) / 2
    STATIC_REQUIRE(reinhard_extended(LinSRGB{8.f, 8.f, 8.f}, 4.f).red() > 1.f);
}

TEST_CASE("aces_fitted follows the fit by Stephen Hill", "[colors][tonemap]")
{
    using aest::colors::tonemap::aces_fitted;

    // Middle gray, reference white and a bright highlight, from the reference code
    CHECK_THAT(aces_fitted(LinSRGB{0.18f, 0.18f, 0.18f}).red(), WithinAbs(0.10559125, 1e-6));
    CHECK_THAT(aces_fitted(LinSRGB{1.f, 1.f, 1.f}).red(), WithinAbs(0.61911543, 1e-6));
    CHECK_THAT(aces_fitted(LinSRGB{4.f, 4.f, 4.f}).blue(), WithinAbs(0.90900468, 1e-6));

    const LinSRGB orange = aces_fitted(LinSRGB{1.f, 0.5f, 0.25f, 0.75f});
    CHECK_THAT(orange.red(), WithinAbs(0.63499038, 1e-6));
    CHECK_THAT(orange.green(), WithinAbs(0.38460008, 1e-6));
    CHECK_THAT(orange.blue(), WithinAbs(0.20316036, 1e-6));
    CHECK(orange.alpha() == 0.75f);

    // Black stays black and highlights saturate at 1
    STATIC_REQUIRE(aces_fitted(LinSRGB{0.f, 0.f, 0.f}) == LinSRGB{0.f, 0.f, 0.f});
    STATIC_REQUIRE(aces_fitted(LinSRGB{100.f, 100.f, 100.f}) == LinSRGB{1.f, 1.f, 1.f});

    // Other linear spaces go through LinSRGB and come back in their own type
    const ACEScg gray = aces_fitted(aest::colors::convert<ACEScg>(LinSRGB{1.f, 1.f, 1.f}));
    CHECK_THAT(aest::colors::convert<LinSRGB>(gray).green(), WithinAbs(0.61911543, 1e-6));
}

namespace
{
using aest::colors::from_half;
using aest::colors::to_half;

template <typename T>
concept UnormPackable = requires(T color) { aest::colors::pack_rgba8(color); };

template <typename T>
concept FloatPackable = requires(T color) { aest::colors::pack_rgba16f(color); };
} // namespace

TEST_CASE("to_half rounds to nearest even", "[colors][packed]")
{
    STATIC_REQUIRE(to_half(1.f) == 0x3C00);
    STATIC_REQUIRE(to_half(-2.f) == 0xC000);
    STATIC_REQUIRE(to_half(0.1f) == 0x2E66);
    STATIC_REQUIRE(to_half(0.f) == 0x0000);
    STATIC_REQUIRE(to_half(-0.f) == 0x8000);

    // Ties go to the even mantissa
    STATIC_REQUIRE(to_half(1.f + 0x1p-11f) == 0x3C00);
    STATIC_REQUIRE(to_half(1.f + 0x3p-11f) == 0x3C02);

    // Largest finite, and rounding past it to infinity
    STATIC_REQUIRE(to_half(65504.f) == 0x7BFF);
    STATIC_REQUIRE(to_half(65519.f) == 0x7BFF);
    STATIC_REQUIRE(to_half(65520.f) == 0x7C00);

    // Subnormals: the smallest, a tie to even at half of it, and 1.5 of it
    STATIC_REQUIRE(to_half(0x1p-24f) == 0x0001);
    STATIC_REQUIRE(to_half(0x1p-25f) == 0x0000);
    STATIC_REQUIRE(to_half(0x3p-25f) == 0x0002);
    STATIC_REQUIRE(to_half(0x1p-14f) == 0x0400); // smallest normal

    // Infinities keep their sign, NaN becomes the quiet NaN
    STATIC_REQUIRE(to_half(std::numeric_limits<float>::infinity()) == 0x7C00);
    STATIC_REQUIRE(to_half(-std::numeric_limits<float>::infinity()) == 0xFC00);
    STATIC_REQUIRE(to_half(std::numeric_limits<float>::quiet_NaN()) == 0x7E00);
}

TEST_CASE("from_half is exact and inverts to_half", "[colors][packed]")
{
    STATIC_REQUIRE(from_half(0x3C00) == 1.f);
    STATIC_REQUIRE(from_half(0x7BFF) == 65504.f);
    STATIC_REQUIRE(from_half(0x0001) == 0x1p-24f);
    STATIC_REQUIRE(from_half(0x8000) == 0.f);
    CHECK(std::isinf(from_half(0xFC00)));
    CHECK(from_half(0xFC00) < 0.f);
    CHECK(std::isnan(from_half(0x7E00)));

    // Every half that is not NaN survives the round trip bit for bit
    for (std::uint32_t bits = 0; bits <= 0xFFFF; ++bits)
    {
        const auto half = static_cast<std::uint16_t>(bits);
        if ((half & 0x7C00) == 0x7C00 && (half & 0x03FF) != 0)
            continue;
        REQUIRE(to_half(from_half(half)) == half);
    }
}

TEST_CASE("RGBA8 and RGB10A2 store unorm channels", "[colors][packed]")
{
    using aest::colors::pack_rgb10a2;
    using aest::colors::pack_rgba8;
    using aest::colors::unpack_rgb10a2;
    using aest::colors::unpack_rgba8;
    using Bytes = std::array<std::uint8_t, 4>;

    // Bytes in memory order, the color's own encoding stored as is
    constexpr SRGB color = SRGB::from_rgba(0x33'66'99'80);
    STATIC_REQUIRE(pack_rgba8(color) == Bytes{0x33, 0x66, 0x99, 0x80});
    STATIC_REQUIRE(unpack_rgba8<SRGB>(Bytes{0x33, 0x66, 0x99, 0x80}) == color);
    STATIC_REQUIRE(pack_rgba8(LinSRGB{2.f, 0.5f, 0.f}) == Bytes{0xFF, 0x80, 0x00, 0xFF});

    // Red in the low bits, alpha in the top two
    STATIC_REQUIRE(pack_rgb10a2(SRGB{1.f, 0.f, 0.f}) == 0xC000'03FFu);
    STATIC_REQUIRE(pack_rgb10a2(SRGB{0.f, 1.f, 0.f, 0.f}) == 0x000F'FC00u);
    STATIC_REQUIRE(pack_rgb10a2(SRGB{0.f, 0.f, 1.f, 0.f}) == 0x3FF0'0000u);
    STATIC_REQUIRE(pack_rgb10a2(SRGB{0.5f, 0.5f, 0.5f, 0.5f})
                   == (512u | 512u << 10 | 512u << 20 | 2u << 30));

    // Round trips within half a step of each format
    const LinDisplayP3 linear{0.3f, 0.6f, 0.9f, 0.4f};
    const LinDisplayP3 back8 = unpack_rgba8<LinDisplayP3>(pack_rgba8(linear));
    CHECK_THAT(back8.green(), WithinAbs(0.6, 0.5 / 255));
    const LinDisplayP3 back10 = unpack_rgb10a2<LinDisplayP3>(pack_rgb10a2(linear));
    CHECK_THAT(back10.blue(), WithinAbs(0.9, 0.5 / 1023));
    CHECK(back10.alpha() == 1.f / 3.f);

    STATIC_REQUIRE(UnormPackable<DisplayP3>);
    STATIC_REQUIRE(UnormPackable<aest::colors::Premultiplied<LinSRGB>>);
    STATIC_REQUIRE_FALSE(UnormPackable<HSL>);
    STATIC_REQUIRE_FALSE(UnormPackable<XYZ>);
}

TEST_CASE("RGBA16F stores halves and never overflows to infinity", "[colors][packed]")
{
    using aest::colors::pack_rgba16f;
    using aest::colors::unpack_rgba16f;
    using Halves = std::array<std::uint16_t, 4>;

    STATIC_REQUIRE(pack_rgba16f(LinSRGB{1.f, 0.5f, 70000.f, 0.25f})
                   == Halves{0x3C00, 0x3800, 0x7BFF, 0x3400});
    STATIC_REQUIRE(unpack_rgba16f<LinSRGB>(Halves{0x3C00, 0x3800, 0x4400, 0x3400})
                   == LinSRGB{1.f, 0.5f, 4.f, 0.25f});

    using PremulLinear = aest::colors::Premultiplied<LinSRGB>;
    STATIC_REQUIRE(pack_rgba16f(PremulLinear{0.5f, 0.25f, 0.f, 0.5f})
                   == Halves{0x3800, 0x3400, 0x0000, 0x3800});

    STATIC_REQUIRE(FloatPackable<ACEScg>);
    STATIC_REQUIRE_FALSE(FloatPackable<SRGB>);
}

TEST_CASE("R11G11B10F stores unsigned small floats", "[colors][packed]")
{
    using aest::colors::pack_r11g11b10f;
    using aest::colors::unpack_r11g11b10f;

    // 1.0 is exponent 15 with an empty mantissa, in each field
    STATIC_REQUIRE(pack_r11g11b10f(LinSRGB{1.f, 1.f, 1.f})
                   == (0x3C0u | 0x3C0u << 11 | 0x1E0u << 22));
    STATIC_REQUIRE(pack_r11g11b10f(LinSRGB{0.f, 0.f, 0.f}) == 0u);

    // Clamped to the largest finite value of each field, never infinity
    constexpr LinSRGB clamped =
        unpack_r11g11b10f<LinSRGB>(pack_r11g11b10f(LinSRGB{1e6f, 1e6f, 1e6f}));
    STATIC_REQUIRE(clamped == LinSRGB{65024.f, 65024.f, 64512.f});

    // Round trips within half a step: 6 and 5 mantissa bits, alpha dropped
    for (const float value : {0.001f, 0.18f, 1.f, 3.7f, 200.f})
    {
        const LinSRGB back =
            unpack_r11g11b10f<LinSRGB>(pack_r11g11b10f(LinSRGB{value, value, value, 0.5f}));
        CHECK_THAT(back.red(), WithinRel(value, 0x1p-7f));
        CHECK_THAT(back.blue(), WithinRel(value, 0x1p-6f));
        CHECK(back.alpha() == 1.f);
    }
}

TEST_CASE("RGB9E5 shares one exponent", "[colors][packed]")
{
    using aest::colors::pack_rgb9e5;
    using aest::colors::unpack_rgb9e5;

    // White: mantissas of 256 under the exponent of 1.0
    STATIC_REQUIRE(pack_rgb9e5(LinSRGB{1.f, 1.f, 1.f})
                   == (256u | 256u << 9 | 256u << 18 | 16u << 27));
    STATIC_REQUIRE(pack_rgb9e5(LinSRGB{0.f, 0.f, 0.f}) == 0u);

    // A largest channel that rounds up to 512 moves to the next exponent
    STATIC_REQUIRE(pack_rgb9e5(LinSRGB{0.9995f, 0.f, 0.f}) == (256u | 16u << 27));

    // The largest value fits, anything above clamps to it
    constexpr LinSRGB top = unpack_rgb9e5<LinSRGB>(pack_rgb9e5(LinSRGB{1e6f, 0.f, 0.f}));
    STATIC_REQUIRE(top.red() == 65408.f);

    // Channels keep precision relative to the largest one
    const LinSRGB color{4.f, 1.f, 0.25f, 0.5f};
    const LinSRGB back = unpack_rgb9e5<LinSRGB>(pack_rgb9e5(color));
    CHECK_THAT(back.red(), WithinAbs(4., 0x1p-7));
    CHECK_THAT(back.green(), WithinAbs(1., 0x1p-7));
    CHECK_THAT(back.blue(), WithinAbs(0.25, 0x1p-7));
    CHECK(back.alpha() == 1.f);
}

namespace
{
using aest::colors::OKLab;
using aest::colors::OKLCH;

/**
 * @brief Checks the channels of @p color against expected OKLab values.
 * @param[in] color The color to check
 * @param[in] l Expected lightness
 * @param[in] a Expected a
 * @param[in] b Expected b
 */
void check_oklab(const OKLab& color, const double l, const double a, const double b)
{
    CHECK_THAT(color.lightness(), WithinAbs(l, 1e-6));
    CHECK_THAT(color.a(), WithinAbs(a, 1e-6));
    CHECK_THAT(color.b(), WithinAbs(b, 1e-6));
}
} // namespace

TEST_CASE("OKLab matches the CSS Color 4 reference", "[colors][oklab]")
{
    using aest::colors::convert;

    STATIC_REQUIRE(OKLab::TYPE == "OKLab");
    check_oklab(convert<OKLab>(LinSRGB{1.f, 1.f, 1.f}), 1., 0., 0.);
    check_oklab(convert<OKLab>(LinSRGB{1.f, 0.f, 0.f}), 0.62795536, 0.22486307, 0.12584628);
    check_oklab(convert<OKLab>(LinSRGB{0.f, 1.f, 0.f}), 0.86643962, -0.23388758, 0.17949845);
    check_oklab(convert<OKLab>(SRGB::from_hex(0x00'00'FF)), 0.45201372, -0.03245698, -0.31152817);
    check_oklab(convert<OKLab>(SRGB::from_hex(0xFF'FF'00)), 0.96798273, -0.07136908, 0.19856972);

    // Grays have no chroma, and alpha passes through
    const OKLab gray = convert<OKLab>(SRGB{0.5f, 0.5f, 0.5f, 0.25f});
    CHECK_THAT(gray.a(), WithinAbs(0., 1e-6));
    CHECK_THAT(gray.b(), WithinAbs(0., 1e-6));
    CHECK(gray.alpha() == 0.25f);

    // Unclamped: HDR light is brighter than white
    CHECK(convert<OKLab>(LinSRGB{4.f, 4.f, 4.f}).lightness() > 1.f);
}

TEST_CASE("OKLCH is the polar form of OKLab", "[colors][oklab]")
{
    using aest::colors::convert;

    STATIC_REQUIRE(OKLCH::TYPE == "OKLCH");
    STATIC_REQUIRE(OKLCH{0.5f, -1.f, 370.f} == OKLCH{0.5f, 0.f, 10.f});

    const OKLCH red = convert<OKLCH>(LinSRGB{1.f, 0.f, 0.f});
    CHECK_THAT(red.lightness(), WithinAbs(0.62795536, 1e-6));
    CHECK_THAT(red.chroma(), WithinAbs(0.25768330, 1e-6));
    CHECK_THAT(red.hue(), WithinAbs(29.233880, 1e-4));

    // Blue's hue is past 180 degrees, so it comes out wrapped, not negative
    CHECK_THAT(convert<OKLCH>(LinSRGB{0.f, 0.f, 1.f}).hue(), WithinAbs(264.052023, 1e-4));
}

TEST_CASE("OKLab and OKLCH round-trip through the tree", "[colors][oklab]")
{
    using aest::colors::convert;

    for (const SRGB color :
         {SRGB::from_hex(0x33'99'CC), SRGB::from_hex(0xE0'40'10), SRGB{0.f, 0.f, 0.f}})
    {
        const SRGB viaLab = convert<SRGB>(convert<OKLab>(color));
        const SRGB viaLch = convert<SRGB>(convert<OKLCH>(color));
        CHECK_THAT(viaLab.red(), WithinAbs(color.red(), 1e-6));
        CHECK_THAT(viaLab.blue(), WithinAbs(color.blue(), 1e-6));
        CHECK_THAT(viaLch.green(), WithinAbs(color.green(), 1e-6));
    }

    // Wide-gamut and HDR colors survive too, since OKLab is unclamped
    const LinDisplayP3 p3Red{1.f, 0.f, 0.f};
    const LinDisplayP3 back = convert<LinDisplayP3>(convert<OKLCH>(p3Red));
    CHECK_THAT(back.red(), WithinAbs(1., 1e-6));
    CHECK_THAT(back.green(), WithinAbs(0., 1e-6));

    STATIC_REQUIRE(aest::colors::ConvertibleTo<OKLCH, HSV>);
}

TEST_CASE("Operations work in OKLab and OKLCH", "[colors][oklab]")
{
    using aest::colors::convert;
    using aest::colors::euclidean;
    using aest::colors::lighten;
    using aest::colors::mix;

    // ΔE_OK: black to white is 1
    CHECK_THAT(euclidean<OKLab>(SRGB{0.f, 0.f, 0.f}, SRGB{1.f, 1.f, 1.f}), WithinAbs(1., 1e-6));

    // lighten in OKLCH moves the perceived lightness, keeping chroma and hue
    const OKLCH lch{0.5f, 0.1f, 200.f};
    const OKLCH light = lighten<OKLCH>(lch, 0.5f);
    STATIC_REQUIRE(lighten<OKLCH>(OKLCH{0.5f, 0.1f, 200.f}, 0.5f) == OKLCH{0.75f, 0.1f, 200.f});
    CHECK(light.chroma() == lch.chroma());

    // mix in OKLab: the midpoint of black and white is perceived middle gray
    const SRGB middle = mix<OKLab>(SRGB{0.f, 0.f, 0.f}, SRGB{1.f, 1.f, 1.f}, 0.5f);
    CHECK_THAT(convert<OKLab>(middle).lightness(), WithinAbs(0.5, 1e-6));

    // Linear operations default to LinSRGB for OKLab colors
    STATIC_REQUIRE(std::is_same_v<aest::colors::LinearSpaceOf<OKLCH>::type, LinSRGB>);
}
