/**
 * @copyright Copyright (c) 2026 Davide Pierotti, MIT License (see LICENSE.md)
 * @author Davide Pierotti
 * @file hdr.cpp
 * @brief HDR pipeline with `aest.colors`: light in linear sRGB, tonemap,
 *        then pack for an SDR and an HDR framebuffer.
 */

#include <array>
#include <cstdint>
#include <cstdio>

import aest.colors;

int main()
{
    using namespace aest::colors;

    // An sRGB texture, decoded to linear light and lit by a light 6 times
    // brighter than reference white
    const LinSRGB albedo = convert<LinSRGB>(SRGB::from_hex(0xE0'A0'60));
    const LinSRGB lit    = albedo * 6.f;

    // SDR: tonemap into [0, 1], then encode and pack for R8G8B8A8_SRGB
    const SRGB sdr                         = convert<SRGB>(tonemap::aces_fitted(lit));
    const std::array<std::uint8_t, 4> rgba = pack_rgba8(sdr);
    std::printf("SDR R8G8B8A8_SRGB: %02X %02X %02X %02X\n", rgba[0], rgba[1], rgba[2], rgba[3]);

    // HDR: keep the scene light in a half-float target, and encode HDR10 for
    // the display in A2B10G10R10 with the PQ curve
    const std::array<std::uint16_t, 4> halves = pack_rgba16f(lit);
    std::printf("HDR R16G16B16A16_SFLOAT: %04X %04X %04X %04X\n", halves[0], halves[1], halves[2],
                halves[3]);
    const std::uint32_t hdr10 = pack_rgb10a2(convert<Rec2100PQ>(lit));
    std::printf("HDR10 A2B10G10R10_UNORM: %08X\n", hdr10);
}
