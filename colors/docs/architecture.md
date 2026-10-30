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
  scaling by a scalar, and only
  LinSRGB inherits it: adding gamma-encoded values or hues has no
  physical meaning.

### Families of color spaces

| Family  | Spaces    | Channels                                                    |
|---------|-----------|-------------------------------------------------------------|
| Encoded | SRGB      | sRGB with its transfer function: hex codes, bytes, CSS      |
| Linear  | LinSRGB   | Proportional to light: blending and lighting math belong here |
| Derived | HSL, HSV  | Cylindrical views of sRGB, for picking and adjusting colors  |

### Ranges

Each constructor brings its channels into the range of its space, so a
value of a color type is always valid for that space.

| Space     | Channels                                                     | Alpha |
|-----------|--------------------------------------------------------------|-------|
| SRGB      | Clamped to [0, 1]                                            | [0, 1] |
| LinSRGB   | Clamped at 0, unbounded above: values over 1 are HDR light   | [0, 1] |
| HSL, HSV  | Hue wrapped to [0, 360); saturation, lightness/value clamped to [0, 1] | [0, 1] |

Channels must be finite: NaN and infinities are a precondition violation,
not a value the constructors repair.

### Alpha

Alpha is straight, not premultiplied: the color channels do not depend on
it. Conversions keep it unchanged. Operations that move a color, such as
lighten(), keep it too, and so do the scalar operators, which scale only
the light; those that combine colors, mix() and the LinSRGB operators
between two colors, combine it like the other channels.

## Conversion tree

aest::colors::convert() reaches any color space from any other. The spaces
form a tree, declared by aest::colors::ParentSpace, and each space provides
aest::colors::Converter specializations to and from its parent:

```
LinSRGB                root
└── SRGB               sRGB transfer function
    ├── HSL            hue, saturation, lightness
    └── HSV            hue, saturation, value
```

A conversion takes one of two routes:

1. **Direct.** If a Converter specialization exists between the two spaces,
   it is used. Besides the parent edges, these are shortcuts that are more
   precise or faster than the path through the tree, such as HSL ↔ HSV,
   which keeps the hue exactly.
2. **Through the tree.** Otherwise the color goes up, parent by parent, to
   the closest ancestor the two spaces share, and down from it to the target.
   HSL → LinSRGB goes HSL → SRGB → LinSRGB; LinSRGB → HSV goes
   LinSRGB → SRGB → HSV.

The route is chosen at compile time. aest::colors::ConvertibleTo tells
whether one exists, and the operations are constrained on it, so a space
outside the tree is rejected when the call is resolved.

Each step goes through a constructor, so it clamps to the range of the
intermediate space. HDR light is lost wherever a path crosses SRGB: LinSRGB
values above 1 clip to 1 there. lighten() of a LinSRGB color works in HSL,
so it clips them too, while mix() and the arithmetic operators stay in
LinSRGB and keep them. Every hop also costs some float precision: a direct
shortcut is worth adding when a pair is converted often.

## Operations

Operations are free functions, such as lighten(), mix() and euclidean().
Each is written once, generically, and works in a *working space*:

1. The inputs are converted to the working space.
2. The operation runs there, through a trait specialized for that space:
   aest::colors::Lightness, aest::colors::Interpolation or
   aest::colors::Euclidean.
3. The result is converted back to the input's type.

The working space is a template parameter with a default, picked either
with a trailing aest::colors::in tag or explicitly:
`lighten(color, 0.2f, in<HSL>)` is `lighten<HSL>(color, 0.2f)`. A concept on
the trait (`LightnessSpace`, `InterpolationSpace`, `EuclideanSpace`) rejects
spaces where the operation has no meaning, such as Euclidean distance over a
hue angle.

The defaults are part of the API: they never change silently, because the
same call would start giving different colors.

## Compile-time evaluation

The library targets C++20, where `<cmath>` is not `constexpr`. Code paths
that need it run only at run time; the rest is `constexpr` on GCC, Clang
and MSVC.

| Compile time                                                | Run time only                  |
|-------------------------------------------------------------|--------------------------------|
| Construction, hex and byte packing, opacity(), `==`         | SRGB ↔ LinSRGB (`std::pow`)    |
| SRGB ↔ HSL ↔ HSV conversions                                | euclidean() (`std::sqrt`)      |
| lighten(), darken() and mix() when no step needs `<cmath>`  | Anything through LinSRGB       |

A function marked `constexpr` whose path reaches a run-time step simply
cannot be used in a constant expression for those arguments: mix() of two
LinSRGB colors is a constant expression, while mix() of two SRGB colors,
which converts through LinSRGB, is not.
