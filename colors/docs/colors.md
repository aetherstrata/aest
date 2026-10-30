# Colors {#colors}

`aest.colors` provides color types, conversions and color operations in the
`aest::colors` namespace.

## Color types

- aest::colors::SRGB, aest::colors::LinSRGB, aest::colors::HSL,
  aest::colors::HSV — colors with float channels and alpha, built on
  aest::colors::ColorBase and checked by the aest::colors::ColorType concept.
  SRGB holds gamma-encoded sRGB (hex codes, bytes, CSS); LinSRGB holds the
  same primaries without the gamma, so its channels are proportional to light
  and can exceed 1 for HDR values.
- aest::colors::XYZ — CIE 1931 XYZ relative to D65, independent of any
  display's primaries: the root of the conversion tree. Y is the relative
  luminance, and the channels are not clamped.
- aest::colors::Premultiplied — a linear color with premultiplied alpha, such
  as `Premultiplied<LinSRGB>`, for compositing.
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

aest::colors::convert() converts between any two of SRGB, LinSRGB, HSL,
HSV, XYZ and `Premultiplied<LinSRGB>`, keeping alpha. LinSRGB ↔ XYZ uses a
matrix derived at compile time from the sRGB chromaticities, so it matches
IEC 61966-2-1; XYZ colors outside the sRGB gamut clip at 0 in LinSRGB. Pairs with an
aest::colors::Converter specialization convert directly; the others go
through the conversion tree declared by aest::colors::ParentSpace (see
@ref colors_architecture). Converting a color to its own type returns it
unchanged. aest::colors::ConvertibleTo checks that a conversion exists.

## Arithmetic

LinSRGB has `+`, `-` and `*` between two colors, and their compound forms,
through aest::colors::ColorArithmetic. Every operator combines all four
channels, alpha included, and clamps the result like the constructor does:
the color channels at 0 only, so sums can exceed 1 and hold HDR light, and
alpha to [0, 1]. The scalar operators `color * s`, `s * color`,
`color / s`, `*=` and `/=` scale the three color channels and keep alpha, as
exposure or a light's intensity would.

Premultiplied colors have `+` and scaling by a scalar too, but both apply to
all four channels: that is how premultiplied light and coverage add up and
fade.

## Compositing

aest::colors::over() composites a color over another with Porter-Duff
source-over: it converts both to premultiplied linear light, computes
`foreground + background * (1 - foreground alpha)`, and returns the result in
the background's type. Its working space, an aest::colors::LinearSpace, is
the first template argument, LinSRGB when omitted, as for the operations
below.

## Operations

Operations are free functions. Each works in a default color space, fixed so
results never change silently; another space is picked as the first
template argument, as in `lighten<HSL>(color, 0.2f)`, or with a trailing
aest::colors::in tag, as in `lighten(color, 0.2f, in<HSL>)`, which is the
same call. The result has the input's type. A space supports an operation by
specializing the operation's trait.

| Operation                                       | Does                                                                                          | Default space | Trait                       |
|-------------------------------------------------|-----------------------------------------------------------------------------------------------|---------------|-----------------------------|
| aest::colors::lighten(), aest::colors::darken() | Move lightness a fraction of the way toward white or black, keeping hue, saturation and alpha | HSL           | aest::colors::Lightness     |
| aest::colors::mix()                             | Blend two colors, alpha included: 0 gives the first, 1 the second                             | LinSRGB       | aest::colors::Interpolation |
| aest::colors::euclidean()                       | Distance between two colors, ignoring alpha                                                   | SRGB          | aest::colors::Euclidean     |

## Luminance and contrast

- aest::colors::luminance() — the relative luminance of a color: its LinSRGB
  channels weighted with the Rec. 709 / sRGB coefficients, ignoring alpha.
  White gives 1, black 0, and HDR colors more than 1.
- aest::colors::contrast_ratio() — the WCAG 2.x contrast ratio of two colors,
  from 1 to 21, in either order. Both are measured as a display shows them,
  through SRGB, so HDR values clip to white. Alpha is ignored: composite a
  translucent color onto its background with aest::colors::over() first.

```cpp
const SRGB text = SRGB::from_hex(0x777777);
const bool readable = contrast_ratio(text, css::white) >= 4.5f; // false: 4.48
```

## Design

- @subpage colors_architecture — the type model, the conversion tree, how
  operations pick a working space, and what runs at compile time.
- @subpage colors_extending — adding a color space or an operation.
- @subpage colors_rendering — using the library in a rendering pipeline:
  linear light, HDR values and compositing.
