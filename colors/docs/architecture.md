# Architecture {#colors_architecture}

How `aest.colors` is put together: the color type model, the conversion
tree, the way operations pick a working space, and which parts run at
compile time. @ref colors_extending shows how to add to it.

## Type model

Every color is four `float`s: three channels and alpha. The channels mean
different things in each color space, so each space is its own type and
the compiler keeps them apart: an SRGB value cannot be passed where an HSL
one is expected, and the conversion between them is always explicit.

- aest::colors::ColorBase holds the channels and alpha, and gives every space
  the generic observers `ch1()`, `ch2()`, `ch3()` and `alpha()`, opacity()
  and `==`. It is a CRTP base, so these return and compare the derived type.
- Each space adds named accessors for its channels (`red()`, `hue()`, ...)
  and a constructor taking `(ch1, ch2, ch3, alpha)` that enforces the range
  of the space.
- aest::colors::ColorType is the concept the rest of the library is written
  against: the four observers, a `TYPE` name for formatting, and the
  four-channel constructor. A type outside the library can satisfy it.
- Mixins add behavior only where it is meaningful.
  aest::colors::ColorArithmetic gives `+`, `-` and `*` between colors and
  scaling by a scalar, and only the linear RGB spaces and XYZ inherit it:
  adding gamma-encoded values or hues has no physical meaning. The spaces
  that inherit it are the aest::colors::LinearSpace ones.
- aest::colors::BasicRGB is one class template for every RGB space, over a
  gamut tag and a transfer function tag (see below). The transfer function
  picks the clamping in the constructor, and BasicRGB inherits
  ColorArithmetic only with tf::Linear.
- aest::colors::Premultiplied wraps a linear space with premultiplied alpha.
  Its own operators, `+` and scaling by a scalar, apply to all four
  channels, which is how premultiplied colors add up and fade.

### Families of color spaces

| Family     | Spaces                                                        | Channels                                                                        |
|------------|---------------------------------------------------------------|---------------------------------------------------------------------------------|
| Encoded    | SRGB, DisplayP3, Rec2100PQ, Rec2100HLG                        | RGB with a transfer function: what displays, files and CSS store                |
| Linear     | LinSRGB, LinDisplayP3, LinRec2020, ACEScg, `Premultiplied<S>` | Proportional to light: blending, lighting and compositing belong here           |
| Derived    | HSL, HSV, `BasicHSL<S>`, `BasicHSV<S>`                        | Cylindrical views of an encoded RGB space, for picking and adjusting colors     |
| Absolute   | XYZ                                                           | CIE 1931 tristimulus values, independent of any display: links the gamuts       |
| Perceptual | OKLab, OKLCH                                                  | Spaced as the eye sees: color differences, perceptual gradients and adjustments |

### Ranges

Each constructor brings its channels into the range of its space, so a
value of a color type is always valid for that space.

| Space              | Channels                                                               | Alpha  |
|--------------------|------------------------------------------------------------------------|--------|
| Encoded RGB        | Clamped to [0, 1]                                                      | [0, 1] |
| Linear RGB         | Clamped at 0, unbounded above: values over 1 are HDR light             | [0, 1] |
| `Premultiplied<S>` | Clamped like S                                                         | [0, 1] |
| HSL, HSV           | Hue wrapped to [0, 360); saturation, lightness/value clamped to [0, 1] | [0, 1] |
| XYZ                | Unclamped: colors outside a gamut clip only when converted to it       | [0, 1] |
| OKLab              | Unclamped                                                              | [0, 1] |
| OKLCH              | Lightness unclamped; chroma clamped at 0; hue wrapped to [0, 360)      | [0, 1] |

Channels must be finite: NaN and infinities are a precondition violation,
not a value the constructors repair.

### RGB spaces

An RGB space is fixed by two independent choices, each a tag type:

- the **gamut**, in aest::colors::gamut: the chromaticities of the three
  primaries and of the white point, checked by aest::colors::RGBGamut. The
  matrix to XYZ is derived from them at compile time, so the library holds
  no hand-copied matrices; a white point other than D65 is adapted to D65
  with the Bradford transform, so XYZ has one white.
- the **transfer function**, in aest::colors::tf: how a channel value
  encodes linear light, and the range it is clamped to, checked by
  aest::colors::TransferFunction. tf::Linear is the identity; tf::SRGB is
  the curve of IEC 61966-2-1, which Display P3 shares; tf::PQ and tf::HLG
  are the HDR signals of Rec. 2100. HDR tags set `IS_HDR`
  (aest::colors::HDRTransfer): their signal is in [0, 1] but not relative
  to the display's white, so aest::colors::BoundedRGBSpace, and with it HSL
  and HSV, leaves them out.

`BasicRGB<Gamut, Transfer>` combines them, and the aliases name the usual
pairs. aest::colors::RGB_NAME builds the `TYPE` from the tags, as the transfer
prefix then the gamut name (`Lin` + `SRGB`), and is specialized where a space
has its own name, as ACEScg does.

| Gamut            | tf::SRGB  | tf::Linear   | tf::PQ    | tf::HLG    |
|------------------|-----------|--------------|-----------|------------|
| gamut::SRGB      | SRGB      | LinSRGB      |           |            |
| gamut::DisplayP3 | DisplayP3 | LinDisplayP3 |           |            |
| gamut::Rec2020   |           | LinRec2020   | Rec2100PQ | Rec2100HLG |
| gamut::AP1       |           | ACEScg       |           |            |

Linear 1.0 is the reference white everywhere: the brightest SDR white, and
203 cd/m² (aest::colors::REFERENCE_WHITE_NITS) for PQ, after ITU-R BT.2408.
So an SDR color and an HDR one convert into each other without a scale
factor, and SRGB white lands on the PQ and HLG reference white.

### Alpha

Alpha is straight in every space but aest::colors::Premultiplied: the color
channels do not depend on it. Conversions keep it unchanged; between a
linear space and its premultiplied form they multiply or divide the
channels by it, and a premultiplied color with alpha 0 comes back as
transparent black, since its light is lost. Operations that move a color, such as
lighten(), keep it too, and so do the scalar operators, which scale only
the light; those that combine colors, mix() and the linear-space operators
between two colors, combine it like the other channels. aest::colors::over()
composites in premultiplied linear light, where coverage and light add up
correctly, and returns straight alpha in the background's type.

## Conversion tree

aest::colors::convert() reaches any color space from any other. The spaces
form a tree, declared by aest::colors::ParentSpace, and each space provides
aest::colors::Converter specializations to and from its parent:

```
XYZ                                  root, CIE 1931 relative to D65
├── LinSRGB                          matrix of gamut::SRGB
│   ├── Premultiplied<LinSRGB>       channels times alpha
│   └── SRGB                         tf::SRGB
│       ├── HSL                      hue, saturation, lightness
│       └── HSV                      hue, saturation, value
├── LinDisplayP3                     matrix of gamut::DisplayP3
│   └── DisplayP3                    tf::SRGB
│       ├── HSL<DisplayP3>           hue, saturation, lightness
│       └── HSV<DisplayP3>           hue, saturation, value
├── LinRec2020                       matrix of gamut::Rec2020
│   ├── Rec2100PQ                    tf::PQ
│   └── Rec2100HLG                   tf::HLG
├── ACEScg                           matrix of gamut::AP1, Bradford to D65
└── OKLab                            LMS matrix, cube root, OKLab matrix
    └── OKLCH                        polar form: chroma and hue
```

Every `BasicRGB<G, tf::Linear>` is a child of XYZ, and every other
`BasicRGB<G, T>` a child of `BasicRGB<G, tf::Linear>`: the links and their
converters are partial specializations, so a new gamut or transfer
function joins the tree without code of its own. Likewise `BasicHSL<S>` and
`BasicHSV<S>` are children of the encoded space `S`, with the HSL ↔ HSV
shortcut between the two over the same `S`.

A conversion takes one of two routes:

1. **Direct.** If a Converter specialization exists between the two spaces,
   it is used. Besides the parent edges, these are shortcuts that are more
   precise or faster than the path through the tree, such as HSL ↔ HSV,
   which keeps the hue exactly.
2. **Through the tree.** Otherwise the color goes up, parent by parent, to
   the closest ancestor the two spaces share, and down from it to the target.
   HSL → LinSRGB goes HSL → SRGB → LinSRGB; DisplayP3 → SRGB goes
   DisplayP3 → LinDisplayP3 → XYZ → LinSRGB → SRGB.

The route is chosen at compile time. aest::colors::ConvertibleTo tells
whether one exists, and the operations are constrained on it, so a space
outside the tree is rejected when the call is resolved.

Each step goes through a constructor, so it clamps to the range of the
intermediate space. HDR light is lost wherever a path crosses SRGB: LinSRGB
values above 1 clip to 1 there. Colors outside a gamut lose their negative
channels where a path enters its linear space from XYZ, as P3 red does in
LinSRGB. lighten() and darken() work in OKLCH, which is unclamped, so the
step to it keeps HDR light and wide gamuts; the result clips only if it
falls outside the input's own range when converted back. mix() and the
arithmetic operators stay in linear light and keep HDR values. Every hop also costs some float precision: a direct
shortcut is worth adding when a pair is converted often.

## Operations

Operations are free functions, such as lighten(), mix() and euclidean().
Each is written once, generically, and works in a *working space*:

1. The inputs are converted to the working space.
2. The operation runs there, through a trait specialized for that space:
   aest::colors::Lightness, aest::colors::Saturation, aest::colors::Hue,
   aest::colors::Interpolation or aest::colors::Euclidean.
3. The result is converted back to the input's type.

The working space is a template parameter with a default, picked either
explicitly or with a trailing aest::colors::in tag:
`lighten<HSL>(color, 0.2f)` is `lighten(color, 0.2f, in<HSL>)`. A concept on
the trait (`LightnessSpace`, `SaturationSpace`, `HueSpace`,
`InterpolationSpace`, `EuclideanSpace`) rejects
spaces where the operation has no meaning, such as Euclidean distance over a
hue angle.

The defaults are part of the API: they never change silently, because the
same call would start giving different colors. The operations that judge
colors by eye default to the perceptual spaces: lighten() and darken() to
OKLCH, euclidean() to OKLab. The aest::colors::Lightness trait passes the
end a color moves toward, white or black, to `set`, so OKLCH and OKLab can
shrink chroma on the way and reach that end exactly; HSL ignores it. mix() and over() default to
`void`, which aest::colors::LinearSpaceOf resolves to linear light in the
input's own gamut, so a default never clips a wide-gamut color to sRGB. A
default template argument cannot name a later parameter, so `void` stands in
for "the input's" while keeping the space the first template argument.
Operations that combine two colors take them as one type; mix() deletes the
overload for two types rather than pick one of them for the result, with
or without a HueMode. The overload with a HueMode is constrained on
aest::colors::HueInterpolationSpace, the spaces whose Interpolation takes
the mode, so asking for a hue arc in a space without a hue fails at the
call.

## Compile-time evaluation

The library targets C++20, where `<cmath>` is not `constexpr`. Code paths
that need it run only at run time; the rest is `constexpr` on GCC, Clang
and MSVC.

| Compile time                                                                                                                  | Run time only                                                       |
|-------------------------------------------------------------------------------------------------------------------------------|---------------------------------------------------------------------|
| Construction, hex and byte packing, opacity(), `==`                                                                           | tf::SRGB, tf::PQ and tf::HLG (`std::pow`, `std::log`)               |
| Encoded RGB ↔ HSL ↔ HSV conversions                                                                                           | euclidean() (`std::sqrt`)                                           |
| lighten(), darken(), saturate(), desaturate(), grayscale(), rotate_hue(), complement() and mix() when no step needs `<cmath>` | Anything through an encoding                                        |
| Premultiplied conversions, over() of linear colors                                                                            | contrast_ratio() (`std::pow`)                                       |
| luminance() of linear colors                                                                                                  |                                                                     |
| OKLab → XYZ                                                                                                                   | OKLab ← XYZ (`std::cbrt`), OKLab ↔ OKLCH (`std::atan2`, `std::cos`) |
| Linear RGB ↔ XYZ (matrices derived at compile time)                                                                           |                                                                     |
| Tonemapping operators                                                                                                         |                                                                     |
| invert()                                                                                                                      |                                                                     |
| Packing and unpacking, to_half() and from_half()                                                                              |                                                                     |

A function marked `constexpr` whose path reaches a run-time step simply
cannot be used in a constant expression for those arguments: mix() of two
LinSRGB colors is a constant expression, while mix() of two SRGB colors,
which converts through LinSRGB, is not.
