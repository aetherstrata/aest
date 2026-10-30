/**
 * @copyright Copyright (c) 2026 Davide Pierotti, MIT License (see LICENSE.md)
 * @author Davide Pierotti
 * @file colors.test.cpp
 * @brief Unit tests for `aest.colors`.
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <type_traits>

import aest.colors;

using aest::colors::HSL;
using aest::colors::HSV;
using aest::colors::LinSRGB;
using aest::colors::SRGB;
using Catch::Matchers::WithinAbs;

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
    CHECK_THAT(linear.red(), WithinAbs(0.214041, 1e-5));   // ((0.5 + 0.055) / 1.055)^2.4
    CHECK_THAT(linear.green(), WithinAbs(0.001548, 1e-5)); // 0.02 / 12.92
    CHECK(linear.blue() == 1.f);
    CHECK(linear.alpha() == 0.25f);
}

TEST_CASE("LinSRGB converts to SRGB with the sRGB transfer function", "[colors]")
{
    const auto srgb = aest::colors::convert<SRGB>(LinSRGB{0.5f, 0.002f, 0.f, 0.75f});
    CHECK_THAT(srgb.red(), WithinAbs(0.735357, 1e-5));  // 1.055 * 0.5^(1 / 2.4) - 0.055
    CHECK_THAT(srgb.green(), WithinAbs(0.02584, 1e-5)); // 0.002 * 12.92
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
        CHECK_THAT(back.red(), WithinAbs(color.red(), 1e-5));
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
                CHECK_THAT(viaHsl.red(), WithinAbs(color.red(), 1e-5));
                CHECK_THAT(viaHsl.green(), WithinAbs(color.green(), 1e-5));
                CHECK_THAT(viaHsl.blue(), WithinAbs(color.blue(), 1e-5));

                const SRGB viaHsv = convert<SRGB>(convert<HSV>(color));
                CHECK_THAT(viaHsv.red(), WithinAbs(color.red(), 1e-5));
                CHECK_THAT(viaHsv.green(), WithinAbs(color.green(), 1e-5));
                CHECK_THAT(viaHsv.blue(), WithinAbs(color.blue(), 1e-5));
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
                CHECK_THAT(toHsv.saturation(), WithinAbs(hsv.saturation(), 1e-5));
                CHECK_THAT(toHsv.brightness(), WithinAbs(hsv.brightness(), 1e-5));

                const HSL toHsl = convert<HSL>(hsv);
                CHECK_THAT(toHsl.hue(), WithinAbs(hsl.hue(), 1e-4));
                CHECK_THAT(toHsl.saturation(), WithinAbs(hsl.saturation(), 1e-5));
                CHECK_THAT(toHsl.lightness(), WithinAbs(hsl.lightness(), 1e-5));
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
    CHECK_THAT(srgb.blue(), WithinAbs(0.735357, 1e-5));
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
    CHECK_THAT(blended.red(), WithinAbs(viaRgb.red(), 1e-5));
    CHECK_THAT(blended.green(), WithinAbs(viaRgb.green(), 1e-5));
    CHECK_THAT(blended.blue(), WithinAbs(viaRgb.blue(), 1e-5));

    // Red and green are a full channel apart in each of two channels
    CHECK_THAT(euclidean(red, HSV{120.f, 1.f, 1.f}, in<LinSRGB>), WithinAbs(1.414214, 1e-5));
}

TEST_CASE("mix blends in LinSRGB by default", "[colors][operations]")
{
    using aest::colors::in;
    using aest::colors::mix;

    const SRGB black{0.f, 0.f, 0.f};
    const SRGB white{1.f, 1.f, 1.f};

    // Half the light of white, encoded: 1.055 * 0.5^(1 / 2.4) - 0.055
    const SRGB linearMidpoint = mix(black, white, 0.5f);
    CHECK_THAT(linearMidpoint.red(), WithinAbs(0.735357, 1e-5));
    CHECK_THAT(linearMidpoint.green(), WithinAbs(0.735357, 1e-5));
    CHECK_THAT(linearMidpoint.blue(), WithinAbs(0.735357, 1e-5));
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
    CHECK_THAT(euclidean(black, gray, in<LinSRGB>), WithinAbs(0.370730, 1e-5));
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
    CHECK_THAT(back.green(), WithinAbs(0.5, 1e-5));
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
    CHECK_THAT(onWhite.red(), WithinAbs(0.735357, 1e-5)); // linear 0.5, encoded
    CHECK(onWhite.alpha() == 1.f);
    STATIC_REQUIRE(std::is_same_v<decltype(over(halfRed, SRGB{})), SRGB>);

    // A premultiplied background accumulates layers without leaving premultiplied form
    constexpr PremulLin frame = over(halfRed, PremulLin{0.f, 0.f, 1.f});
    STATIC_REQUIRE(frame == PremulLin{0.5f, 0.f, 0.5f, 1.f});
}

TEST_CASE("luminance weights linear light with the Rec. 709 coefficients", "[colors][operations]")
{
    using aest::colors::luminance;

    STATIC_REQUIRE(luminance(LinSRGB{1.f, 1.f, 1.f}) == 1.f);
    STATIC_REQUIRE(luminance(LinSRGB{0.f, 0.f, 0.f}) == 0.f);
    STATIC_REQUIRE(luminance(LinSRGB{1.f, 0.f, 0.f, 0.25f}) == 0.2126f);
    STATIC_REQUIRE(luminance(LinSRGB{0.f, 1.f, 0.f}) == 0.7152f);
    STATIC_REQUIRE(luminance(LinSRGB{0.f, 0.f, 1.f}) == 0.0722f);
    STATIC_REQUIRE(luminance(LinSRGB{2.f, 2.f, 2.f}) == 2.f);

    // Other spaces are linearized first
    CHECK_THAT(luminance(SRGB::from_hex(0x80'80'80)), WithinAbs(0.215861, 1e-5));
    CHECK_THAT(luminance(HSL{0.f, 0.f, 1.f}), WithinAbs(1., 1e-6));
}

TEST_CASE("contrast_ratio follows WCAG 2.x", "[colors][operations]")
{
    using aest::colors::contrast_ratio;

    const SRGB white = SRGB::from_hex(0xFF'FF'FF);
    const SRGB black = SRGB::from_hex(0x00'00'00);
    CHECK_THAT(contrast_ratio(black, white), WithinAbs(21., 1e-5));
    CHECK(contrast_ratio(white, black) == contrast_ratio(black, white));
    CHECK(contrast_ratio(white, white) == 1.f);

    // #777777 on white just fails the 4.5:1 AA threshold
    CHECK_THAT(contrast_ratio(SRGB::from_hex(0x77'77'77), white), WithinAbs(4.478089, 1e-4));

    // Mixed types, and HDR colors clip to white as a display shows them
    CHECK_THAT(contrast_ratio(HSL{0.f, 0.f, 0.f}, LinSRGB{4.f, 4.f, 4.f}), WithinAbs(21., 1e-5));
}
