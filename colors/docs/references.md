# References {#colors_references}

The standards `aest.colors` implements. The documentation and the source
comments link here rather than to the standards themselves, so each link is
kept in one place.

## Color spaces {#colors_ref_spaces}

| Color space                                         | Specification(s)                                                                                                                                                                                     |
|-----------------------------------------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| aest::colors::SRGB, aest::colors::LinSRGB           | [IEC 61966-2-1](https://webstore.iec.ch/en/publication/6169), [ICC registry](https://registry.color.org/rgb-registry/files/sRGB.pdf), [W3C](https://www.w3.org/Graphics/Color/sRGB.html)             |
| aest::colors::DisplayP3, aest::colors::LinDisplayP3 | [SMPTE EG 432-1](https://doi.org/10.5594/SMPTE.EG432-1.2010), the P3 primaries; with D65 white and the sRGB curve as in [CSS Color 4](https://www.w3.org/TR/css-color-4/)                            |
| aest::colors::Rec709                                | [ITU-R BT.709](https://www.itu.int/rec/R-REC-BT.709)                                                                                                                                                 |
| aest::colors::Rec2020, aest::colors::LinRec2020     | [ITU-R BT.2020](https://www.itu.int/rec/R-REC-BT.2020)                                                                                                                                               |
| aest::colors::Rec2100PQ, aest::colors::Rec2100HLG   | [ITU-R BT.2100](https://www.itu.int/rec/R-REC-BT.2100)                                                                                                                                               |
| aest::colors::ACEScg                                | [SMPTE ST 2065-1](https://doi.org/10.5594/SMPTE.ST2065-1.2021), the ACES primaries, and the [ACEScg specification](https://docs.acescentral.com/specifications/acescg/), AP1                         |
| aest::colors::HSL, aest::colors::HSV                | [CSS Color 4](https://www.w3.org/TR/css-color-4/) for HSL; A. R. Smith, [Color gamut transform pairs](https://doi.org/10.1145/965139.807361) (1978), for HSV                                         |
| aest::colors::XYZ                                   | [CIE 015:2018](https://doi.org/10.25039/TR.015.2018), Colorimetry, 4th edition                                                                                                                       |
| aest::colors::CIELab, aest::colors::CIELCH          | [ISO/CIE 11664-4:2019](https://cie.co.at/publications/colorimetry-part-4-cie-1976-lab-colour-space-1), with the D50 white and Bradford matrices of [CSS Color 4](https://www.w3.org/TR/css-color-4/) |
| aest::colors::OKLab, aest::colors::OKLCH            | B. Ottosson, [A perceptual color space for image processing](https://bottosson.github.io/posts/oklab/) (2020), with the matrices recomputed by [CSS Color 4](https://www.w3.org/TR/css-color-4/)     |

The Bradford transform that adapts white points is described, with its
matrix, in B. Lindbloom's
[Chromatic Adaptation](http://www.brucelindbloom.com/index.html?Eqn_ChromAdapt.html).

## Transfer functions and HDR {#colors_ref_transfer}

| Function or constant               | Specification                                                                                                                              |
|------------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------|
| aest::colors::tf::SRGB             | [IEC 61966-2-1](https://webstore.iec.ch/en/publication/6169)                                                                               |
| aest::colors::tf::Rec709           | [ITU-R BT.709](https://www.itu.int/rec/R-REC-BT.709), with the constants of [ITU-R BT.2020](https://www.itu.int/rec/R-REC-BT.2020)         |
| aest::colors::tf::PQ               | [SMPTE ST 2084](https://doi.org/10.5594/SMPTE.ST2084.2014) and [ITU-R BT.2100](https://www.itu.int/rec/R-REC-BT.2100)                      |
| aest::colors::tf::HLG              | [ARIB STD-B67](https://www.arib.or.jp/english/std_tr/broadcasting/std-b67.html) and [ITU-R BT.2100](https://www.itu.int/rec/R-REC-BT.2100) |
| aest::colors::REFERENCE_WHITE_NITS | [ITU-R BT.2408](https://www.itu.int/pub/R-REP-BT.2408), the 203 cd/m² reference white                                                      |
| Display decoding                   | [ITU-R BT.1886](https://www.itu.int/rec/R-REC-BT.1886), the display EOTF the library leaves out                                            |

## Operations {#colors_ref_operations}

| Function                                                  | Specification                                                                                                                                                               |
|-----------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| aest::colors::mix() with an aest::colors::HueMode         | [CSS Color 4](https://www.w3.org/TR/css-color-4/), hue interpolation                                                                                                        |
| aest::colors::luminance(), aest::colors::contrast_ratio() | [WCAG 2.2](https://www.w3.org/TR/WCAG22/), relative luminance and contrast ratio                                                                                            |
| aest::colors::over(), aest::colors::Premultiplied         | T. Porter and T. Duff, [Compositing Digital Images](https://doi.org/10.1145/964965.808606) (1984); [Compositing and Blending Level 1](https://www.w3.org/TR/compositing-1/) |
| aest::colors::invert(), named colors in aest::colors::css | [CSS Color 4](https://www.w3.org/TR/css-color-4/)                                                                                                                           |

## Tonemapping {#colors_ref_tonemap}

| Operator                                                                      | Source                                                                                                                                           |
|-------------------------------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------|
| aest::colors::tonemap::reinhard(), aest::colors::tonemap::reinhard_extended() | E. Reinhard et al., [Photographic Tone Reproduction for Digital Images](https://doi.org/10.1145/566654.566575) (2002)                            |
| aest::colors::tonemap::aces_fitted()                                          | S. Hill's fit of the ACES RRT and sRGB ODT, as published in [BakingLab](https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl) |

## Packed formats {#colors_ref_packed}

| Format                                                                           | Specification                                                                                                                                       |
|----------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------|
| Bit layouts of every `pack_*()` function                                         | [Vulkan specification](https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html), formats                                                  |
| 11-, 10-bit and shared-exponent floats                                           | [Khronos Data Format Specification 1.3](https://registry.khronos.org/DataFormat/specs/1.3/dataformat.1.3.html), unsigned and shared-exponent floats |
| aest::colors::to_half(), aest::colors::from_half(), aest::colors::pack_rgba16f() | [IEEE 754-2019](https://doi.org/10.1109/IEEESTD.2019.8766229), binary16                                                                             |
