# Rendering {#colors_rendering}

How to use `aest.colors` in a rendering pipeline: which space to do the math
in, how HDR values are kept, and how to composite. @ref colors_architecture
explains the types these sections rely on.

## Blend in linear light

SRGB stores gamma-encoded values: they are spaced for how the eye sees
brightness, not for how light adds up. Averaging two encoded values gives a
midpoint that is too dark, so blending, lighting and filtering belong in
LinSRGB, where channels are proportional to light.

```cpp
using namespace aest::colors;

const SRGB a = css::red;
const SRGB b = css::lime;
const SRGB physical = mix(a, b, 0.5f);           // blends in LinSRGB
const SRGB encoded  = mix<SRGB>(a, b, 0.5f);     // darker midpoint, like CSS
```

## Scene-referred and display-referred values

LinSRGB is *scene-referred*: its channels measure light, so they are clamped
at 0 but not at 1. A value of 4 is a light four times brighter than the
brightest color SRGB can encode, and the arithmetic operators keep it:

```cpp
LinSRGB light = convert<LinSRGB>(SRGB::from_hex(0xFFCC88));
light *= 4.f;                      // exposure: scales the light, keeps alpha
const LinSRGB lit = light * LinSRGB{0.25f, 0.5f, 0.5f}; // surface albedo
```

SRGB is *display-referred*: [0, 1] is all the display can show. Converting
to it clips every channel above 1, so keep colors in LinSRGB for as long as
they may be brighter than white, and convert at the end. Any operation whose
path crosses SRGB clips too; lighten() and darken() work in OKLCH and mix()
and the operators in linear light, so they keep HDR values.

## Compositing

aest::colors::over() puts one color over another with Porter-Duff
source-over. It works in `Premultiplied<LinSRGB>`, where each channel is
already multiplied by alpha: a premultiplied color is the light it adds plus
the fraction of the background it covers, so compositing is one
multiply-add, and the result is right for any pair of alphas.

```cpp
const SRGB background = css::navy;
const SRGB glass{1.f, 1.f, 1.f, 0.25f};
const SRGB result = over(glass, background); // SRGB, the background's type
```

When compositing many layers, keep the frame in premultiplied form, as a
GPU blend unit does, and convert it back to straight alpha once at the end:

```cpp
using Layer = Premultiplied<LinSRGB>;

Layer frame = convert<Layer>(LinSRGB{0.f, 0.f, 0.f});
for (const LinSRGB& layer : layers)            // back to front
    frame = over(layer, frame);
const SRGB pixel = convert<SRGB>(frame);
```

A premultiplied color with alpha 0 converts back to transparent black: its
light was multiplied away.

## Tonemapping

A scene can be brighter than a display: LinSRGB values above 1 clip to
white when converted to SRGB, flattening highlights. A tonemapping operator
from aest::colors::tonemap compresses them into [0, 1] first, so the
typical pipeline is:

```
decode textures   SRGB -> LinSRGB (or a wider linear space)
light and blend   LinSRGB, values above 1 allowed
composite         Premultiplied<LinSRGB>, over()
expose            color * exposure
tonemap           tonemap::aces_fitted(), reinhard() or reinhard_extended()
encode            LinSRGB -> SRGB, then pack for the framebuffer
```

```cpp
LinSRGB frame = convert<LinSRGB>(accumulated) * exposure;
const SRGB pixel = convert<SRGB>(tonemap::aces_fitted(frame));
```

reinhard() never clips but never reaches white; reinhard_extended() reaches
white at a chosen value, such as the brightest light of the scene;
aces_fitted() adds the contrast and highlight desaturation of film. Since
the fit is defined on sRGB, it returns colors inside the sRGB gamut.

## Choosing a packed format

The last step writes the pixel in the framebuffer's format; the
`pack_*()` functions produce its bits.

| Data | Format | Why |
|------|--------|-----|
| Display-referred color, tonemapped | `pack_rgba8(SRGB)` into `R8G8B8A8_SRGB` | The encoding spends the 256 levels where the eye sees steps |
| Display-referred, 10-bit or HDR10 output | `pack_rgb10a2()` of SRGB or Rec2100PQ | 1024 levels per channel; alpha only 2 bits |
| HDR scene light, with alpha | `pack_rgba16f()` into `R16G16B16A16_SFLOAT` | Linear values up to 65504 at 11-bit precision, premultiplied alpha included |
| HDR scene light, opaque | `pack_r11g11b10f()` | Half the size of RGBA16F; 6-bit and 5-bit mantissas, so slight banding and hue shifts |
| HDR light probes, environment maps | `pack_rgb9e5()` | 9-bit mantissas under one exponent: precise for the brightest channel, coarse for much dimmer ones |
| Linear non-color data: masks, roughness | `pack_rgba8(LinSRGB)` into `R8G8B8A8_UNORM` | Stored as is, without the sRGB curve |

Never store linear light in 8-bit unorm: 256 evenly spaced levels band
visibly in the shadows, which is what the sRGB encoding is for.

## HDR output

HDR displays take a signal relative to an absolute luminance (PQ) or to the
display's peak (HLG), in the Rec. 2020 gamut. The library keeps linear 1.0
as the reference white everywhere, 203 cd/m² after ITU-R BT.2408, so a scene
lit for SDR keeps its brightness on an HDR display, and highlights above 1
use the headroom:

```cpp
LinRec2020 scene = convert<LinRec2020>(sceneInLinSRGB); // same light, wider gamut
const Rec2100PQ hdr10 = convert<Rec2100PQ>(scene);     // 1.0 -> 58%, 4.0 -> 73%
const Rec2100HLG hlg  = convert<Rec2100HLG>(scene);    // 1.0 -> 75%
```

PQ is absolute: linear 49.26 is the 10000 cd/m² top of the signal. A
pipeline that works in cd/m² divides by aest::colors::REFERENCE_WHITE_NITS to
get linear values, as in `tf::PQ::encode(nits / REFERENCE_WHITE_NITS)`.
HLG is relative: the library applies its OETF only, so the linear values are
scene light and the display applies the OOTF for its own peak. Neither
signal tonemaps: values beyond the signal's range clip, so tonemap first
when the scene is brighter than the display.
