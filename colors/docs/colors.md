# Colors {#colors}

`aest.colors` provides color types, conversions and color operations in the
`aest::colors` namespace.

## Color types

- aest::colors::RGB, aest::colors::LinearRGB, aest::colors::HSL,
  aest::colors::HSV — colors with float channels and alpha, built on
  aest::colors::ColorBase and checked by the aest::colors::ColorType concept.
  RGB holds gamma-encoded sRGB (hex codes, bytes, CSS); LinearRGB holds the
  same primaries without the gamma, so its channels are proportional to light.
- aest::colors::RGB::from_hex(), aest::colors::RGB::from_rgba(),
  aest::colors::RGB::from_argb(), aest::colors::RGB::from_bytes() — build an
  RGB color from packed or 8-bit values; aest::colors::RGB::to_rgba(),
  aest::colors::RGB::to_argb() and aest::colors::RGB::hex_string() pack it
  back, rounding each channel to the nearest byte.
- aest::colors::ColorBase::opacity() — copies a color with a new alpha.
- `std::format("{}", color)` prints the color space and its channels, as in
  `RGB(1.000, 0.000, 0.500, 1.000)`; a float spec such as `{:.1f}` applies to
  each channel.
- Named CSS colors in aest::colors::css, such as aest::colors::css::teal.

## Conversions

aest::colors::convert() converts between RGB, LinearRGB, HSL and HSV through
aest::colors::Converter specializations, keeping alpha. Converting a color to
its own type returns it unchanged; a pair with no specialization fails to
compile.

## Arithmetic

LinearRGB has `+`, `-` and `*` between two colors, and their compound forms,
through aest::colors::ColorArithmetic. Every operator combines all four
channels, alpha included, and clamps the result. There are no scalar
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
| aest::colors::mix() | Blend two colors, alpha included: 0 gives the first, 1 the second | LinearRGB | aest::colors::Interpolation |
| aest::colors::euclidean() | Distance between two colors, ignoring alpha | RGB | aest::colors::Euclidean |
