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
path crosses SRGB clips too: lighten() works in HSL, a child of SRGB, while
mix() and the operators stay in LinSRGB.

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
