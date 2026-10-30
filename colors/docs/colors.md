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
| aest::colors::Rec2100PQ    | gamut::Rec2020   | tf::PQ     |
| aest::colors::Rec2100HLG   | gamut::Rec2020   | tf::HLG    |

SRGB is the encoding of hex codes, bytes and CSS; convert to a linear space
for color math. aest::colors::RGBSpace, aest::colors::LinearRGBSpace and
aest::colors::BoundedRGBSpace tell the kinds apart.

Rec2100PQ and Rec2100HLG are the HDR signals of Rec. 2100. Linear 1.0 is the
BT.2408 reference white, aest::colors::REFERENCE_WHITE_NITS, 203 cd/m²: PQ encodes it to 58%
and reaches 10000 cd/m² at linear 49.26; HLG encodes it to 75% and reaches
its peak at linear 3.77, without an OOTF. For a luminance in cd/m², divide
by REFERENCE_WHITE_NITS first, as in `tf::PQ::encode(1000.f / REFERENCE_WHITE_NITS)`.

- aest::colors::BasicHSL, aest::colors::BasicHSV — hue, saturation and
  lightness or value, over an encoded RGB space: `BasicHSL<DisplayP3>` reaches
  the P3 colors that HSL over sRGB clips. aest::colors::HSL and
  aest::colors::HSV are the ones over SRGB, the HSL of CSS. A hue is relative
  to the primaries, so the same color has different hues over different
  gamuts. Linear spaces are rejected, since the formulas need channels in
  [0, 1].
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
the first template argument; when omitted it is the
aest::colors::LinearSpaceOf the background, linear light in the background's
gamut, so P3 colors composite without clipping to sRGB.

## Operations

Operations are free functions. Each works in a default color space, fixed so
results never change silently; another space is picked as the first
template argument, as in `lighten<HSL>(color, 0.2f)`, or with a trailing
aest::colors::in tag, as in `lighten(color, 0.2f, in<HSL>)`, which is the
same call. The result has the input's type. A space supports an operation by
specializing the operation's trait.

| Operation                                       | Does                                                                                          | Default space                     | Trait                       |
|-------------------------------------------------|-----------------------------------------------------------------------------------------------|-----------------------------------|-----------------------------|
| aest::colors::lighten(), aest::colors::darken() | Move lightness a fraction of the way toward white or black, keeping hue, saturation and alpha | HSL                               | aest::colors::Lightness     |
| aest::colors::mix()                             | Blend two colors, alpha included: 0 gives the first, 1 the second                             | Linear light in the input's gamut | aest::colors::Interpolation |
| aest::colors::euclidean()                       | Distance between two colors, ignoring alpha                                                   | SRGB                              | aest::colors::Euclidean     |

mix() blends in the aest::colors::LinearSpaceOf its color type: LinSRGB for
SRGB and HSL, LinDisplayP3 for DisplayP3 and `BasicHSL<DisplayP3>`, XYZ for
XYZ. Linear blending gives the same result in every linear gamut, so this
only avoids clipping wide-gamut colors to sRGB. Both colors have one type:
mix() of two different types is deleted, so convert one into the other's
type first, into the wider gamut:

```cpp
const DisplayP3 blend = mix(convert<DisplayP3>(srgbColor), p3Color, 0.5f);
```

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

## Tonemapping

The operators in aest::colors::tonemap compress HDR linear light into the
[0, 1] range of a display. Each takes and returns a color of any linear RGB
space and keeps alpha.

| Operator | Does |
|----------|------|
| aest::colors::tonemap::reinhard() | `c / (1 + c)` per channel: never clips, reaches white only at infinity |
| aest::colors::tonemap::reinhard_extended() | `c (1 + c / w²) / (1 + c)` per channel: maps the white `w` to 1 |
| aest::colors::tonemap::aces_fitted() | Stephen Hill's fit of the ACES RRT and sRGB ODT, in LinSRGB: filmic contrast, desaturated highlights, clamped to [0, 1] |

```cpp
const LinSRGB hdr{4.f, 2.f, 0.5f};
const SRGB pixel = convert<SRGB>(tonemap::aces_fitted(hdr));
```

## Packed formats

Functions to pack a color into the pixel formats of GPUs and back, with the
bit layouts of Vulkan and Direct3D. Each `pack_*()` takes a color and
returns the stored bits; each `unpack_*<Color>()` takes the bits and the
color type to read them as. They are all `constexpr`.

| Functions | Vulkan format | Layout | Takes |
|-----------|---------------|--------|-------|
| aest::colors::pack_rgba8(), aest::colors::unpack_rgba8() | `R8G8B8A8_UNORM`, `R8G8B8A8_SRGB` | 4 bytes R, G, B, A, in memory order | Any RGB space |
| aest::colors::pack_rgb10a2(), aest::colors::unpack_rgb10a2() | `A2B10G10R10_UNORM_PACK32` | R bits 0-9, G 10-19, B 20-29, A 30-31 | Any RGB space |
| aest::colors::pack_rgba16f(), aest::colors::unpack_rgba16f() | `R16G16B16A16_SFLOAT` | 4 halves R, G, B, A, in memory order | Linear RGB |
| aest::colors::pack_r11g11b10f(), aest::colors::unpack_r11g11b10f() | `B10G11R11_UFLOAT_PACK32` | R bits 0-10, G 11-21, B 22-31, no alpha | Linear RGB |
| aest::colors::pack_rgb9e5(), aest::colors::unpack_rgb9e5() | `E5B9G9R9_UFLOAT_PACK32` | R, G, B mantissas in bits 0-26, shared exponent 27-31, no alpha | Linear RGB |

The unorm formats clamp each channel to [0, 1] and store the color's own
encoding: pack SRGB for an `_SRGB` format, a linear space for `_UNORM` data.
The float formats clamp to their largest finite value, so a color never
becomes infinite, and round to nearest even. Every format also takes the
premultiplied form of a linear space, `Premultiplied<LinSRGB>`.
aest::colors::PackableRGB and aest::colors::PackableLinearRGB name the
accepted types.

aest::colors::to_half() and aest::colors::from_half() convert single floats
to and from IEEE 754 halves, with IEEE rounding, infinities and NaN.

## Design

- @subpage colors_architecture — the type model, the conversion tree, how
  operations pick a working space, and what runs at compile time.
- @subpage colors_extending — adding a color space or an operation.
- @subpage colors_rendering — using the library in a rendering pipeline:
  linear light, HDR values and compositing.
