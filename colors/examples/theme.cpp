/**
 * @copyright Copyright (c) 2026 Davide Pierotti, MIT License (see LICENSE.md)
 * @author Davide Pierotti
 * @file theme.cpp
 * @brief UI theme from one brand color with `aest.colors`: interaction
 *        states, a muted and an accent variant, and readable text on each.
 */

#include <format>
#include <iostream>
#include <string>

import aest.colors;

namespace
{
/** @brief The WCAG 2.x AA contrast minimum for body text. */
constexpr float AA_CONTRAST = 4.5f;

/**
 * @brief Print a theme color with the text color that reads best on it.
 *
 * Black or white text is picked by the higher contrast ratio, and flagged
 * when even that one falls short of WCAG AA.
 *
 * @param[in] name The role of the color in the theme
 * @param[in] color The background color
 */
void print_role(const char* name, const aest::colors::SRGB& color)
{
    using namespace aest::colors;

    const float onWhite  = contrast_ratio(color, css::white);
    const float onBlack  = contrast_ratio(color, css::black);
    const bool useWhite  = onWhite >= onBlack;
    const float contrast = useWhite ? onWhite : onBlack;

    std::cout << std::format("{:<8} {}  text {}  {:5.2f}:1 {}\n", name, hex_string(color),
                             useWhite ? "white" : "black", contrast,
                             contrast >= AA_CONTRAST ? "AA" : "fails AA");
}
} // namespace

int main()
{
    using namespace aest::colors;

    const SRGB brand = from_hex(0x3B'82'F6);

    // The operations work in OKLCH: equal amounts look like equal steps on
    // any hue, and the hue stays put
    print_role("brand", brand);
    print_role("hover", lighten(brand, 0.15f));
    print_role("pressed", darken(brand, 0.25f));
    print_role("disabled", desaturate(brand, 0.7f));
    print_role("accent", complement(brand));

    // A tinted surface for cards and a dark one for the footer, both of the
    // brand's hue
    print_role("surface", lighten(desaturate(brand, 0.8f), 0.9f));
    print_role("footer", darken(desaturate(brand, 0.5f), 0.75f));
}
