// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/tfrag_light.rs:
// ISC License, Copyright (c) 2026 ReRAC contributors.
//
// RAC1 tfrag vertex lighting: the game's LightTfrags (NTSC-U boot 0x234f98,
// level01 0x2a8e40), bit for bit (ReRAC docs/plan/tfrag_lighting.md). The EE
// runs it in VU0 macro mode and carries colours as the float 65536 + c / 128
// (bits 0x47800000 + c), so the colour byte is the low byte of the float's
// bits after the accumulations and every add truncates to that 1/128 grid;
// the pass therefore runs on ps2_float.h's arithmetic.
//
// Per vertex: base + A.rgb * max(dA, dA * wA) + B.rgb * max(dB, dB * wB), where
// d is the dot product of the light's direction with the stored (negated)
// normal, then the point lights in the tfrag's list.

#pragma once

#include <optional>
#include <vector>

#include "assets/geometry/lighting.h"
#include "assets/geometry/tfrag.h"

namespace openrac::assets::rac1 {

// The normal the pass keeps in scratchpad: -(cos az cos el, sin az cos el,
// sin el), w = sin el.
ps2::V4 tfrag_stored_normal(const NormalTable& table, u8 azimuth, u8 elevation);

// The directional pass for one vertex.
std::array<u8, 4> light_tfrag_vertex(
    const LightBank& bank, const NormalTable& table, const TfragLight& light
);

// The point lights of one tfrag: the slot bank, the header's nibble list
// (+0x36; low nibble first, 0xf ends it, at most four) and the integer origin
// quadword at light_ofs.
struct TfragPointLights {
    const PointLightBank* bank = nullptr;
    u16 list = 0xffff;
    std::array<s32, 4> origin{};
};

// LightTfrags for one tfrag: its new RGBA array, which the game writes back
// over the stored one for VU1 to read. Entries 0..vert_count are computed;
// the padding up to rgba_size * 4 keeps its stored values (the game leaves
// scratchpad leftovers there that no vertex reads).
std::vector<TfragRgba> light_tfrag(
    const Tfrag& tfrag,
    const LightBank& bank,
    const NormalTable& table,
    const std::optional<TfragPointLights>& points = std::nullopt
);

// One point light's contribution to one vertex at raw position `local`
// (relative to `origin_floats`, the tfrag origin as itof12(origin << 2));
// empty when the light does not reach it.
std::optional<std::array<u8, 4>> light_tfrag_point(
    const PointLight& light,
    const std::array<u32, 3>& origin_floats,
    const std::array<s16, 3>& local,
    const ps2::V4& stored_normal,
    const std::array<u8, 4>& rgba
);

}  // namespace openrac::assets::rac1
