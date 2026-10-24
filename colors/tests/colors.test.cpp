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
#include <type_traits>

import aest.colors;

using aest::colors::HSL;
using aest::colors::HSV;
using aest::colors::LinearRGB;
using aest::colors::RGB;
using Catch::Matchers::WithinAbs;

TEST_CASE("from_hex unpacks channels", "[colors]")
{
    STATIC_REQUIRE(RGB::from_hex(0x12'34'56) == RGB::from_bytes(0x12, 0x34, 0x56));
    STATIC_REQUIRE(RGB::from_argb(0xFF'00'00'00) == RGB{});
}

TEST_CASE("hex_string formats as #RRGGBB", "[colors]")
{
    CHECK(RGB{}.hex_string() == "#000000");
    CHECK(RGB::from_bytes(0xAB, 0x0C, 0xFF).hex_string() == "#AB0CFF");
}

TEST_CASE("std::format prints the color space and channels", "[colors]")
{
    CHECK(std::format("{}", RGB{1.f, 0.f, 0.5f, 0.25f}) == "RGB(1.000, 0.000, 0.500, 0.250)");
    CHECK(std::format("{}", HSL{-90.f, 1.f, 0.5f}) == "HSL(270.000, 1.000, 0.500, 1.000)");
}

TEST_CASE("std::format applies the spec to each channel", "[colors]")
{
    const RGB color{1.f, 0.f, 0.5f, 0.75f};
    CHECK(std::format("{:.1f}", color) == "RGB(1.0, 0.0, 0.5, 0.8)");
    CHECK(std::format("{:5.2f}", color) == "RGB( 1.00,  0.00,  0.50,  0.75)");
}

TEST_CASE("std::format rejects specs that are invalid for a float", "[colors]")
{
    const RGB color;
    CHECK_THROWS_AS(std::vformat("{:s}", std::make_format_args(color)), std::format_error);
}

TEST_CASE("hex_string round-trips from_hex", "[colors]")
{
    CHECK(RGB::from_hex(0xC0'FF'EE).hex_string() == "#C0FFEE");
}

TEST_CASE("Byte conversions round to the nearest value", "[colors]")
{
    const RGB color{0.5f, 0.2f, 0.999f, 0.6f};
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
    const auto fromHsl = aest::colors::convert<RGB>(HSL{-120.f, 1.f, 0.5f});
    CHECK_THAT(fromHsl.red(), WithinAbs(0., 1e-6));
    CHECK_THAT(fromHsl.green(), WithinAbs(0., 1e-6));
    CHECK_THAT(fromHsl.blue(), WithinAbs(1., 1e-6));

    const auto fromHsv = aest::colors::convert<RGB>(HSV{-60.f, 1.f, 1.f});
    CHECK(fromHsv == aest::colors::convert<RGB>(HSV{300.f, 1.f, 1.f}));
}

TEST_CASE("RGB with more blue than green converts to a positive hue", "[colors]")
{
    const RGB rose{1.f, 0.f, 0.5f};
    CHECK_THAT(aest::colors::convert<HSL>(rose).hue(), WithinAbs(330., 1e-4));
    CHECK_THAT(aest::colors::convert<HSV>(rose).hue(), WithinAbs(330., 1e-4));
}

TEST_CASE("RGB converts to LinearRGB with the sRGB transfer function", "[colors]")
{
    const auto linear = aest::colors::convert<LinearRGB>(RGB{0.5f, 0.02f, 1.f, 0.25f});
    CHECK_THAT(linear.red(), WithinAbs(0.214041, 1e-5));   // ((0.5 + 0.055) / 1.055)^2.4
    CHECK_THAT(linear.green(), WithinAbs(0.001548, 1e-5)); // 0.02 / 12.92
    CHECK(linear.blue() == 1.f);
    CHECK(linear.alpha() == 0.25f);
}

TEST_CASE("LinearRGB converts to RGB with the sRGB transfer function", "[colors]")
{
    const auto srgb = aest::colors::convert<RGB>(LinearRGB{0.5f, 0.002f, 0.f, 0.75f});
    CHECK_THAT(srgb.red(), WithinAbs(0.735357, 1e-5));  // 1.055 * 0.5^(1 / 2.4) - 0.055
    CHECK_THAT(srgb.green(), WithinAbs(0.02584, 1e-5)); // 0.002 * 12.92
    CHECK(srgb.blue() == 0.f);
    CHECK(srgb.alpha() == 0.75f);
}

TEST_CASE("RGB round-trips through LinearRGB", "[colors]")
{
    for (std::int32_t value = 0; value <= 255; ++value)
    {
        const auto byte = static_cast<std::uint8_t>(value);
        const RGB color = RGB::from_bytes(byte, byte, byte);
        const RGB back  = aest::colors::convert<RGB>(aest::colors::convert<LinearRGB>(color));
        CHECK_THAT(back.red(), WithinAbs(color.red(), 1e-5));
    }
}

namespace
{
/** @brief Checks the channels of @p color against the expected RGB values. */
void check_rgb(const RGB& color, const double red, const double green, const double blue)
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

TEST_CASE("HSL converts to RGB", "[colors][conversions]")
{
    using aest::colors::convert;

    for (const auto& [hue, r, g, b] : SECTORS)
    {
        CAPTURE(hue);
        check_rgb(convert<RGB>(HSL{hue, 1.f, 0.5f}), r, g, b);
    }
    check_rgb(convert<RGB>(HSL{330.f, 1.f, 0.5f}), 1., 0., 0.5);
    check_rgb(convert<RGB>(HSL{210.f, 0.5f, 0.5f}), 0.25, 0.5, 0.75);
    check_rgb(convert<RGB>(HSL{123.f, 0.f, 0.3f}), 0.3, 0.3, 0.3);
    check_rgb(convert<RGB>(HSL{0.f, 1.f, 1.f}), 1., 1., 1.);
    check_rgb(convert<RGB>(HSL{0.f, 1.f, 0.f}), 0., 0., 0.);
    CHECK(convert<RGB>(HSL{0.f, 1.f, 0.5f, 0.25f}).alpha() == 0.25f);
}

TEST_CASE("HSV converts to RGB", "[colors][conversions]")
{
    using aest::colors::convert;

    for (const auto& [hue, r, g, b] : SECTORS)
    {
        CAPTURE(hue);
        check_rgb(convert<RGB>(HSV{hue, 1.f, 1.f}), r, g, b);
    }
    check_rgb(convert<RGB>(HSV{330.f, 1.f, 1.f}), 1., 0., 0.5);
    check_rgb(convert<RGB>(HSV{210.f, 2.f / 3.f, 0.75f}), 0.25, 0.5, 0.75);
    check_rgb(convert<RGB>(HSV{123.f, 0.f, 0.3f}), 0.3, 0.3, 0.3);
    check_rgb(convert<RGB>(HSV{0.f, 1.f, 0.f}), 0., 0., 0.);
    CHECK(convert<RGB>(HSV{0.f, 1.f, 1.f, 0.25f}).alpha() == 0.25f);
}

TEST_CASE("RGB converts to HSL", "[colors][conversions]")
{
    using aest::colors::convert;

    for (const auto& [hue, r, g, b] : SECTORS)
    {
        CAPTURE(hue);
        const HSL hsl = convert<HSL>(RGB{r, g, b});
        CHECK_THAT(hsl.hue(), WithinAbs(hue, 1e-4));
        CHECK_THAT(hsl.saturation(), WithinAbs(1., 1e-6));
        CHECK_THAT(hsl.lightness(), WithinAbs(0.5, 1e-6));
    }

    const HSL blueish = convert<HSL>(RGB{0.25f, 0.5f, 0.75f, 0.25f});
    CHECK_THAT(blueish.hue(), WithinAbs(210., 1e-4));
    CHECK_THAT(blueish.saturation(), WithinAbs(0.5, 1e-6));
    CHECK_THAT(blueish.lightness(), WithinAbs(0.5, 1e-6));
    CHECK(blueish.alpha() == 0.25f);

    // Grays have no hue and no saturation
    for (const float value : {0.f, 0.3f, 1.f})
    {
        CAPTURE(value);
        const HSL gray = convert<HSL>(RGB{value, value, value});
        CHECK(gray.hue() == 0.f);
        CHECK(gray.saturation() == 0.f);
        CHECK_THAT(gray.lightness(), WithinAbs(value, 1e-6));
    }
}

TEST_CASE("RGB converts to HSV", "[colors][conversions]")
{
    using aest::colors::convert;

    for (const auto& [hue, r, g, b] : SECTORS)
    {
        CAPTURE(hue);
        const HSV hsv = convert<HSV>(RGB{r, g, b});
        CHECK_THAT(hsv.hue(), WithinAbs(hue, 1e-4));
        CHECK_THAT(hsv.saturation(), WithinAbs(1., 1e-6));
        CHECK_THAT(hsv.brightness(), WithinAbs(1., 1e-6));
    }

    const HSV blueish = convert<HSV>(RGB{0.25f, 0.5f, 0.75f, 0.25f});
    CHECK_THAT(blueish.hue(), WithinAbs(210., 1e-4));
    CHECK_THAT(blueish.saturation(), WithinAbs(2. / 3., 1e-6));
    CHECK_THAT(blueish.brightness(), WithinAbs(0.75, 1e-6));
    CHECK(blueish.alpha() == 0.25f);

    // Grays have no hue and no saturation
    for (const float value : {0.f, 0.3f, 1.f})
    {
        CAPTURE(value);
        const HSV gray = convert<HSV>(RGB{value, value, value});
        CHECK(gray.hue() == 0.f);
        CHECK(gray.saturation() == 0.f);
        CHECK_THAT(gray.brightness(), WithinAbs(value, 1e-6));
    }
}

TEST_CASE("RGB round-trips through HSL and HSV", "[colors][conversions]")
{
    using aest::colors::convert;

    for (std::int32_t r = 0; r <= 8; ++r)
        for (std::int32_t g = 0; g <= 8; ++g)
            for (std::int32_t b = 0; b <= 8; ++b)
            {
                const RGB color{r / 8.f, g / 8.f, b / 8.f};
                CAPTURE(color.red(), color.green(), color.blue());

                const RGB viaHsl = convert<RGB>(convert<HSL>(color));
                CHECK_THAT(viaHsl.red(), WithinAbs(color.red(), 1e-5));
                CHECK_THAT(viaHsl.green(), WithinAbs(color.green(), 1e-5));
                CHECK_THAT(viaHsl.blue(), WithinAbs(color.blue(), 1e-5));

                const RGB viaHsv = convert<RGB>(convert<HSV>(color));
                CHECK_THAT(viaHsv.red(), WithinAbs(color.red(), 1e-5));
                CHECK_THAT(viaHsv.green(), WithinAbs(color.green(), 1e-5));
                CHECK_THAT(viaHsv.blue(), WithinAbs(color.blue(), 1e-5));
            }
}

TEST_CASE("Converting to the same type returns the color unchanged", "[colors][conversions]")
{
    using aest::colors::convert;

    STATIC_REQUIRE(std::is_same_v<decltype(convert<RGB>(RGB{})), RGB>);
    STATIC_REQUIRE(convert<RGB>(RGB{0.1f, 0.2f, 0.3f, 0.4f}) == RGB{0.1f, 0.2f, 0.3f, 0.4f});
    STATIC_REQUIRE(convert<LinearRGB>(LinearRGB{0.1f, 0.2f, 0.3f, 0.4f})
                   == LinearRGB{0.1f, 0.2f, 0.3f, 0.4f});
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

TEST_CASE("HSL and HSV convert like going through RGB", "[colors][conversions]")
{
    using aest::colors::convert;

    for (std::int32_t r = 0; r <= 8; ++r)
        for (std::int32_t g = 0; g <= 8; ++g)
            for (std::int32_t b = 0; b <= 8; ++b)
            {
                const RGB color{r / 8.f, g / 8.f, b / 8.f};
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

    STATIC_REQUIRE(convert<RGB>(HSL{210.f, 0.5f, 0.5f}) == RGB{0.25f, 0.5f, 0.75f});
    STATIC_REQUIRE(convert<RGB>(HSV{210.f, 0.5f, 0.75f}) == RGB{0.375f, 0.5625f, 0.75f});
    STATIC_REQUIRE(convert<HSL>(RGB{0.25f, 0.5f, 0.75f}) == HSL{210.f, 0.5f, 0.5f});
    STATIC_REQUIRE(convert<HSV>(RGB{1.f, 0.f, 0.5f}) == HSV{330.f, 1.f, 1.f});
    STATIC_REQUIRE(convert<HSV>(HSL{210.f, 0.5f, 0.5f}).hue() == 210.f);
    STATIC_REQUIRE(convert<HSL>(HSV{210.f, 0.5f, 0.75f}).hue() == 210.f);
}

TEST_CASE("opacity replaces alpha and keeps the channels", "[colors]")
{
    constexpr RGB color{0.25f, 0.5f, 0.75f};
    STATIC_REQUIRE(color.opacity(0.4f) == RGB{0.25f, 0.5f, 0.75f, 0.4f});
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

TEST_CASE("Only LinearRGB has arithmetic operators", "[colors]")
{
    STATIC_REQUIRE(Addable<LinearRGB>);
    STATIC_REQUIRE_FALSE(Addable<RGB>);
    STATIC_REQUIRE_FALSE(Addable<aest::colors::HSL>);
    STATIC_REQUIRE_FALSE(Addable<aest::colors::HSV>);
}

TEST_CASE("LinearRGB arithmetic is channel-wise and clamped", "[colors]")
{
    constexpr LinearRGB a{0.5f, 0.25f, 0.75f, 1.f};
    constexpr LinearRGB b{0.25f, 0.25f, 0.5f, 0.5f};

    STATIC_REQUIRE(a - b == LinearRGB{0.25f, 0.f, 0.25f, 0.5f});
    STATIC_REQUIRE(b - a == LinearRGB{0.f, 0.f, 0.f, 0.f});
    STATIC_REQUIRE(a + b == LinearRGB{0.75f, 0.5f, 1.f, 1.f});
    STATIC_REQUIRE(a * b == LinearRGB{0.125f, 0.0625f, 0.375f, 0.5f});

    LinearRGB c = a;
    c -= b;
    CHECK(c == a - b);
    c += b;
    CHECK(c == a);
    c *= b;
    CHECK(c == a * b);
}

namespace
{
template <typename T>
concept ScalableByFloat = requires(T color, float scalar) { color * scalar; };
} // namespace

TEST_CASE("LinearRGB arithmetic combines alpha like the other channels", "[colors]")
{
    constexpr LinearRGB halfTransparent{0.25f, 0.25f, 0.25f, 0.5f};
    STATIC_REQUIRE((halfTransparent + halfTransparent).alpha() == 1.f);
    STATIC_REQUIRE((halfTransparent - halfTransparent).alpha() == 0.f);
    STATIC_REQUIRE((halfTransparent * halfTransparent).alpha() == 0.25f);
}

TEST_CASE("LinearRGB scales by a gray color, not by a scalar", "[colors]")
{
    STATIC_REQUIRE_FALSE(ScalableByFloat<LinearRGB>);

    LinearRGB color{0.5f, 1.f, 0.25f, 0.5f};
    color *= {0.5f, 0.5f, 0.5f}; // alpha defaults to 1, so opacity is kept
    CHECK(color == LinearRGB{0.25f, 0.5f, 0.125f, 0.5f});
}

TEST_CASE("lighten moves HSL lightness toward white", "[colors][operations]")
{
    using aest::colors::lighten;

    const RGB gray = lighten(RGB{}, 0.5f);
    CHECK_THAT(gray.red(), WithinAbs(0.5, 1e-6));
    CHECK_THAT(gray.green(), WithinAbs(0.5, 1e-6));
    CHECK_THAT(gray.blue(), WithinAbs(0.5, 1e-6));

    // Orange is HSL(30, 1, 0.5); lightness 0.5 -> 0.75 keeps hue, saturation and alpha
    const RGB light = lighten(RGB{1.f, 0.5f, 0.f, 0.5f}, 0.5f);
    CHECK_THAT(light.red(), WithinAbs(1., 1e-6));
    CHECK_THAT(light.green(), WithinAbs(0.75, 1e-6));
    CHECK_THAT(light.blue(), WithinAbs(0.5, 1e-6));
    CHECK(light.alpha() == 0.5f);
}

TEST_CASE("lighten clamps the amount to [0, 1]", "[colors][operations]")
{
    using aest::colors::lighten;

    constexpr RGB coral = RGB::from_hex(0xFF'7F'50);
    CHECK(lighten(coral, 0.f).to_rgba() == coral.to_rgba());
    CHECK(lighten(coral, -1.f).to_rgba() == coral.to_rgba());
    CHECK(lighten(coral, 1.f).to_rgba() == 0xFF'FF'FF'FF);
    CHECK(lighten(coral, 2.f).to_rgba() == 0xFF'FF'FF'FF);
}

TEST_CASE("darken moves HSL lightness toward black", "[colors][operations]")
{
    using aest::colors::darken;

    // Orange is HSL(30, 1, 0.5); lightness 0.5 -> 0.25 keeps hue, saturation and alpha
    const RGB dark = darken(RGB{1.f, 0.5f, 0.f, 0.5f}, 0.5f);
    CHECK_THAT(dark.red(), WithinAbs(0.5, 1e-6));
    CHECK_THAT(dark.green(), WithinAbs(0.25, 1e-6));
    CHECK_THAT(dark.blue(), WithinAbs(0., 1e-6));
    CHECK(dark.alpha() == 0.5f);

    constexpr RGB coral = RGB::from_hex(0xFF'7F'50);
    CHECK(darken(coral, 0.f).to_rgba() == coral.to_rgba());
    CHECK(darken(coral, -1.f).to_rgba() == coral.to_rgba());
    CHECK(darken(coral, 1.f).to_rgba() == 0x00'00'00'FF);
    CHECK(darken(coral, 2.f).to_rgba() == 0x00'00'00'FF);
}

namespace
{
template <typename Space>
concept CanLightenIn = requires(RGB color) { aest::colors::lighten<Space>(color, 0.5f); };
} // namespace

TEST_CASE("The working space is chosen with in<Space>", "[colors][operations]")
{
    using aest::colors::darken;
    using aest::colors::in;
    using aest::colors::lighten;

    constexpr RGB coral = RGB::from_hex(0xFF'7F'50);
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
    STATIC_REQUIRE_FALSE(CanLightenIn<RGB>);
}

TEST_CASE("mix blends in LinearRGB by default", "[colors][operations]")
{
    using aest::colors::in;
    using aest::colors::mix;

    const RGB black{0.f, 0.f, 0.f};
    const RGB white{1.f, 1.f, 1.f};

    // Half the light of white, encoded: 1.055 * 0.5^(1 / 2.4) - 0.055
    const RGB linearMidpoint = mix(black, white, 0.5f);
    CHECK_THAT(linearMidpoint.red(), WithinAbs(0.735357, 1e-5));
    CHECK_THAT(linearMidpoint.green(), WithinAbs(0.735357, 1e-5));
    CHECK_THAT(linearMidpoint.blue(), WithinAbs(0.735357, 1e-5));
    CHECK(mix<LinearRGB>(black, white, 0.5f) == linearMidpoint);

    // in<RGB> averages the encoded values, like CSS color-mix(in srgb, ...)
    CHECK(mix<RGB>(black, white, 0.5f) == RGB{0.5f, 0.5f, 0.5f});
}

TEST_CASE("mix goes from the first color to the second", "[colors][operations]")
{
    using aest::colors::mix;

    constexpr LinearRGB from{0.25f, 0.5f, 1.f, 0.f};
    constexpr LinearRGB to{0.75f, 0.f, 0.f, 1.f};

    STATIC_REQUIRE(mix(from, to, 0.f) == from);
    STATIC_REQUIRE(mix(from, to, 1.f) == to);
    STATIC_REQUIRE(mix(from, to, 0.25f) == LinearRGB{0.375f, 0.375f, 0.75f, 0.25f});

    // The amount is clamped to [0, 1]
    STATIC_REQUIRE(mix(from, to, -1.f) == from);
    STATIC_REQUIRE(mix(from, to, 2.f) == to);
}

namespace
{
template <typename Space>
concept CanMixIn = requires(RGB color) { aest::colors::mix<Space>(color, color, 0.5f); };
} // namespace

TEST_CASE("mix returns the input type and needs an interpolation space", "[colors][operations]")
{
    using aest::colors::mix;

    const HSL red{0.f, 1.f, 0.5f};
    const HSL blue{240.f, 1.f, 0.5f};
    STATIC_REQUIRE(std::is_same_v<decltype(mix(red, blue, 0.5f)), HSL>);

    STATIC_REQUIRE(CanMixIn<LinearRGB>);
    STATIC_REQUIRE(CanMixIn<RGB>);
    STATIC_REQUIRE_FALSE(CanMixIn<HSL>);
    STATIC_REQUIRE_FALSE(CanMixIn<HSV>);
}

TEST_CASE("euclidean measures in RGB by default", "[colors][distance]")
{
    using aest::colors::euclidean;
    using aest::colors::in;

    const RGB black{0.f, 0.f, 0.f};
    const RGB white{1.f, 1.f, 1.f};
    const RGB gray{0.5f, 0.5f, 0.5f};

    CHECK_THAT(euclidean(black, white), WithinAbs(1.732051, 1e-6)); // sqrt(3)
    CHECK_THAT(euclidean(black, gray), WithinAbs(0.866025, 1e-6));  // sqrt(3) * 0.5
    CHECK(euclidean(black, gray, in<RGB>) == euclidean(black, gray));

    // In linear light the encoded 0.5 gray is only 0.214 bright: sqrt(3) * 0.214041
    CHECK_THAT(euclidean(black, white, in<LinearRGB>), WithinAbs(1.732051, 1e-6));
    CHECK_THAT(euclidean(black, gray, in<LinearRGB>), WithinAbs(0.370730, 1e-5));
}

TEST_CASE("euclidean is symmetric and ignores alpha", "[colors][distance]")
{
    using aest::colors::euclidean;

    const RGB coral = RGB::from_hex(0xFF'7F'50);
    const RGB teal  = RGB::from_hex(0x00'80'80);

    CHECK(euclidean(coral, coral) == 0.f);
    CHECK(euclidean(coral, teal) == euclidean(teal, coral));
    CHECK(euclidean(coral, teal) == euclidean(coral.opacity(0.f), teal));
}

namespace
{
template <typename Space>
concept CanMeasureIn =
    requires(RGB color) { aest::colors::euclidean(color, color, aest::colors::in<Space>); };
} // namespace

TEST_CASE("euclidean accepts mixed color types and needs a Euclidean space", "[colors][distance]")
{
    using aest::colors::euclidean;

    CHECK_THAT(euclidean(HSL{0.f, 1.f, 0.5f}, RGB{1.f, 0.f, 0.f}), WithinAbs(0., 1e-6));
    CHECK_THAT(euclidean(HSV{120.f, 1.f, 1.f}, RGB{1.f, 0.f, 0.f}), WithinAbs(1.414214, 1e-6));

    STATIC_REQUIRE(CanMeasureIn<RGB>);
    STATIC_REQUIRE(CanMeasureIn<LinearRGB>);
    STATIC_REQUIRE_FALSE(CanMeasureIn<HSL>);
    STATIC_REQUIRE_FALSE(CanMeasureIn<HSV>);
}

TEST_CASE("Named CSS colors live in the css namespace", "[colors]")
{
    namespace css = aest::colors::css;
    STATIC_REQUIRE(css::coral == RGB::from_hex(0xFF'7F'50));
    STATIC_REQUIRE(css::midnightBlue == RGB::from_hex(0x19'19'70));
}
