# Colors {#colors}

`aest.colors` provides color types, conversions and color operations in the
`aest::colors` namespace.

## Color types

Every color has float channels and alpha, is built on aest::colors::ColorBase
and is checked by the aest::colors::ColorType concept.

aest::colors::BasicRGB is an RGB space made of a gamut tag from
aest::colors::gamut (primaries and white point) and a transfer function tag
from aest::colors::tf. Encoded spaces clamp their channels to [0, 1]; linear
ones (tf::Linear) clamp at 0 only, so they hold HDR values, and have the
arithmetic operators. The aliases name the usual spaces:

| Alias                      | Gamut            | Transfer   |
|----------------------------|------------------|------------|
| aest::colors::SRGB         | gamut::SRGB      | tf::SRGB   |
| aest::colors::LinSRGB      | gamut::SRGB      | tf::Linear |
| aest::colors::DisplayP3    | gamut::DisplayP3 | tf::SRGB   |
| aest::colors::LinDisplayP3 | gamut::DisplayP3 | tf::Linear |
| aest::colors::LinRec2020   | gamut::Rec2020   | tf::Linear |
| aest::colors::ACEScg       | gamut::AP1       | tf::Linear |

SRGB is the encoding of hex codes, bytes and CSS; convert to a linear space
for color math. aest::colors::RGBSpace, aest::colors::LinearRGBSpace and
aest::colors::BoundedRGBSpace tell the kinds apart.

- aest::colors::HSL, aest::colors::HSV — hue, saturation and lightness or
  value, over SRGB.
- aest::colors::XYZ — CIE 1931 XYZ relative to D65, independent of any
  display's primaries: the root of the conversion tree. Y is the relative
  luminance, and the channels are not clamped.
- aest::colors::Premultiplied — a linear color with premultiplied alpha, such
  as `Premultiplied<LinSRGB>`, for compositing.
- `from_hex()`, `from_rgba()`, `from_argb()` and `from_bytes()` build an SRGB
  color from packed or 8-bit values; `to_rgba()`, `to_argb()` and
  `hex_string()` pack it back, rounding each channel to the nearest byte.
  They are members of SRGB only, the space hex codes and bytes are written
  in, as in `SRGB::from_hex(0xFF7F50)`.
- aest::colors::ColorBase::opacity() — copies a color with a new alpha.
- `std::format("{}", color)` prints the color space and its channels, as in
  `SRGB(1.000, 0.000, 0.500, 1.000)`; a float spec such as `{:.1f}` applies to
  each channel.
- Named CSS colors in aest::colors::css, such as aest::colors::css::teal.

## Conversions

aest::colors::convert() converts between any two color spaces, keeping
alpha. Each linear RGB space converts to XYZ with a matrix derived at compile
time from the chromaticities of its gamut, so sRGB matches IEC 61966-2-1;
ACEScg, whose white point is not D65, is adapted to it with the Bradford
transform. A color outside the target gamut clips at 0, so P3 red loses its
negative green and blue in LinSRGB. Pairs with an
aest::colors::Converter specialization convert directly; the others go
through the conversion tree declared by aest::colors::ParentSpace (see
@ref colors_architecture). Converting a color to its own type returns it
unchanged. aest::colors::ConvertibleTo checks that a conversion exists.

## Arithmetic

The linear spaces, LinSRGB and the other linear RGB spaces and XYZ, have
`+`, `-` and `*` between two colors, and their compound forms, through
aest::colors::ColorArithmetic. Every operator combines all four
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
below: `over<LinDisplayP3>(fg, bg)` keeps P3 colors that LinSRGB would clip.

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

- aest::colors::luminance() — the relative luminance of a color, the Y of
  its XYZ, ignoring alpha: for LinSRGB its channels weighted with the
  Rec. 709 coefficients, about 0.2126, 0.7152 and 0.0722. White gives 1,
  black 0, and HDR colors more than 1.
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
