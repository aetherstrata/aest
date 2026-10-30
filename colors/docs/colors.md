# Colors {#colors}

`aest.colors` provides color types, conversions and color operations in the
`aest::colors` namespace.

## Color types

- aest::colors::SRGB, aest::colors::LinSRGB, aest::colors::HSL,
  aest::colors::HSV — colors with float channels and alpha, built on
  aest::colors::ColorBase and checked by the aest::colors::ColorType concept.
  SRGB holds gamma-encoded sRGB (hex codes, bytes, CSS); LinSRGB holds the
  same primaries without the gamma, so its channels are proportional to light.
- aest::colors::SRGB::from_hex(), aest::colors::SRGB::from_rgba(),
  aest::colors::SRGB::from_argb(), aest::colors::SRGB::from_bytes() — build an
  SRGB color from packed or 8-bit values; aest::colors::SRGB::to_rgba(),
  aest::colors::SRGB::to_argb() and aest::colors::SRGB::hex_string() pack it
  back, rounding each channel to the nearest byte.
- aest::colors::ColorBase::opacity() — copies a color with a new alpha.
- `std::format("{}", color)` prints the color space and its channels, as in
  `SRGB(1.000, 0.000, 0.500, 1.000)`; a float spec such as `{:.1f}` applies to
  each channel.
- Named CSS colors in aest::colors::css, such as aest::colors::css::teal.

## Conversions

aest::colors::convert() converts between any two of SRGB, LinSRGB, HSL and
HSV, keeping alpha. Pairs with an aest::colors::Converter specialization
convert directly; the others go through the conversion tree declared by
aest::colors::ParentSpace (see @ref colors_architecture). Converting a color
to its own type returns it unchanged. aest::colors::ConvertibleTo checks that
a conversion exists.

## Arithmetic

LinSRGB has `+`, `-` and `*` between two colors, and their compound forms,
through aest::colors::ColorArithmetic. Every operator combines all four
channels, alpha included, and clamps the result like the constructor does:
the color channels at 0 only, so sums can exceed 1 and hold HDR light, and
alpha to [0, 1]. There are no scalar
operators: scale by a gray color instead, as in `color *= {0.5f, 0.5f, 0.5f}`
(alpha defaults to 1).

## Operations

Operations are free functions. Each works in a default color space, fixed so
results never change silently; another space is picked with a trailing
aest::colors::in tag or as the first template argument, so
`lighten(color, 0.2f, in<HSL>)` and `lighten<HSL>(color, 0.2f)` are the same
call. The result has the input's type. A space supports an operation by
specializing the operation's trait.

| Operation | Does | Default space | Trait |
|-----------|------|---------------|-------|
| aest::colors::lighten(), aest::colors::darken() | Move lightness a fraction of the way toward white or black, keeping hue, saturation and alpha | HSL | aest::colors::Lightness |
| aest::colors::mix() | Blend two colors, alpha included: 0 gives the first, 1 the second | LinSRGB | aest::colors::Interpolation |
| aest::colors::euclidean() | Distance between two colors, ignoring alpha | SRGB | aest::colors::Euclidean |

## Design

- @subpage colors_architecture — the type model, the conversion tree, how
  operations pick a working space, and what runs at compile time.
- @subpage colors_extending — adding a color space or an operation.
