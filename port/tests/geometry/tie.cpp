// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), the unit tests of
// crates/rc-formats/src/tie.rs: ISC License, Copyright (c) 2026 ReRAC contributors.
//
// Ties, the instanced scenery: the packet walker's VU1 rules, strips and
// winding, dinky vertex phases, malformed packets, and the instances.

#include "assets/geometry/tie.h"

#include <cstring>

#include "tests/check.h"

using namespace openrac::assets;
using namespace openrac::assets::rac1;

namespace {

template <typename F>
bool throws(F&& f) {
    try {
        f();
    } catch (const AssetError&) {
        return true;
    }
    return false;
}

void pad16(std::vector<u8>& v) {
    while (v.size() % 16 != 0) {
        v.push_back(0);
    }
}

template <typename T>
void append(std::vector<u8>& v, const T& value) {
    const auto* p = reinterpret_cast<const u8*>(&value);
    v.insert(v.end(), p, p + sizeof(T));
}

// A one-LOD class blob around a packet.
std::vector<u8> class_blob(TiePacketHeader ph, const std::vector<u8>& data, std::size_t ad_gifs) {
    const std::size_t table = 0x80;
    const std::size_t pdata = table + 0x10;
    const std::size_t normals = pdata + data.size();
    const std::size_t adg = normals + 0x200;
    TieClassHeader h;
    h.scale = 1.0f;
    h.o_class = 7;
    h.packets = {static_cast<s32>(table), 0, 0};
    h.packet_count = {1, 0, 0};
    h.normals = static_cast<u32>(normals);
    h.ad_gif_ofs = static_cast<u32>(adg);
    h.texture_count = static_cast<u8>(ad_gifs);
    std::vector<u8> b;
    append(b, h);
    ph.data = 0x10;
    append(b, ph);
    b.insert(b.end(), data.begin(), data.end());
    for (s16 i = 0; i < 64; ++i) {
        append(b, std::array<s16, 4>{i, 0, 0, 0});
    }
    b.resize(b.size() + ad_gifs * 0x50, 0);
    return b;
}

// Ad-gif 0 at slot 0, strip A (3 vertices) at 6, ad-gif 1 at 16, strip B (4)
// at 22. Slots A: 7, 10, 13; B: 23, 26, 29, 32. Four dinky vertices
// (single-only, marker index 0) and two fat ones (single phase index 0,
// double phase index 1, which also writes slot 26).
std::pair<TiePacketHeader, std::vector<u8>> sample() {
    std::vector<u8> d;
    for (const s32 x : {16, 0, 0, 0}) {
        append(d, x);  // ad_gif_dest
    }
    for (const s32 x : {0x50, 0, 0, 0}) {
        append(d, x);  // ad_gif_src: block 0 -> ad-gif 1, block 1 -> ad-gif 0
    }
    d.insert(d.end(), {1, 0, 6, 2, 29, 0, 7, 23, 12, 12, 4, 2});  // the unpack header
    d.insert(d.end(), {3, 0, 6, 0, 4, 0, 22, 0});                 // strips
    pad16(d);
    const std::size_t vert_ofs = d.size() / 16;
    const std::array<u16, 4> dinky_slots = {29, 13, 32, 10};
    for (std::size_t i = 0; i < 4; ++i) {
        append(
            d, TieDinkyVertex{static_cast<s16>(i), 100, 200, dinky_slots[i], 4096, -4096, 0x1000, 0}
        );
    }
    append(d, TieFatVertex{1, 2, 3, 7, 10, 11, 12, 0, 0, 0, 0x1000, 0});
    append(d, TieFatVertex{-1, -2, -3, 23, 20, 21, 22, 0, 0, 0, 0x1000, 26});
    pad16(d);
    const std::size_t vert_size = d.size() / 16 - vert_ofs;
    const std::size_t color_ofs = d.size() / 16;
    const std::array<u8, 12> colors = {5, 6, 7, 8, 9, 10, 11, 0xff, 12, 13, 14, 0xff};
    d.insert(d.end(), colors.begin(), colors.end());
    pad16(d);
    for (const u8 c : colors) {
        d.push_back(c == 0xff ? c : static_cast<u8>(c + 0x40));
    }
    pad16(d);
    const std::size_t slot_ofs = d.size() / 16;
    d.insert(d.end(), {7, 3, 3, 0xfc, 3, 3, 3, 0xf6});
    pad16(d);
    TiePacketHeader ph;
    ph.shader_count = 2;
    ph.ad_gif_qwc = 10;
    ph.control_count = 5;
    ph.control_size = 2;
    ph.vert_ofs = static_cast<u8>(vert_ofs);
    ph.vert_size = static_cast<u8>(vert_size);
    ph.color_ofs = static_cast<u8>(color_ofs);
    ph.color_count = 3;
    ph.slot_table_ofs = static_cast<u8>(slot_ofs);
    ph.slot_table_size = 1;
    ph.strip_count = 2;
    ph.strip_vertex_count = 7;
    return {ph, d};
}

void walker_follows_vu1_rules() {
    const auto [ph, d] = sample();
    const TieClass tc = parse_tie_class(class_blob(ph, d, 2));
    CHECK(tc.header_ext.empty());
    CHECK((tc.normal(3) == std::array<f32, 3>{3.0f / 32767.0f, 0, 0}));
    const TiePacket& pk = tc.lods[0][0];
    // Dinky 0..4, then fat 0..2; only fat 1 is in a double-write phase.
    std::vector<std::pair<u16, u16>> slots;
    std::vector<u8> colors;
    for (const TieVertex& v : pk.vertices) {
        slots.emplace_back(v.gs_slot, v.gs_slot_2);
        colors.push_back(v.color);
    }
    CHECK(
        (slots
         == std::vector<std::pair<u16, u16>>{{29, 0}, {13, 0}, {32, 0}, {10, 0}, {7, 0}, {23, 26}})
    );
    CHECK((colors == std::vector<u8>{5, 6, 7, 8, 9, 12}));
    CHECK((pk.vertices[4].morph_colors == std::array<u8, 2>{10, 11}));
    CHECK((pk.vertices[0].morph_colors == std::array<u8, 2>{5, 5}));
    CHECK((pk.vertices[5].position == std::array<s16, 3>{20, 21, 22}));
    CHECK((pk.vertices[5].morph_delta == std::array<s16, 3>{-1, -2, -3}) && pk.vertices[5].fat);
    CHECK((pk.vertices[1].uv() == std::array<f32, 2>{1.0f, -1.0f}));
    // Strip A uses ad-gif 1 (block 0's source is 0x50), strip B ad-gif 0.
    CHECK((pk.draws == std::vector<TieDraw>{{1, 0, {4, 3, 1}}, {0, 0, {5, 5, 0, 2}}}));
    CHECK((tie_triangles(pk) == std::vector<TieTriangle>{{4, 3, 1, 1}, {5, 5, 0, 0}, {2, 0, 5, 0}})
    );
}

void winding_flips_parity() {
    TiePacket p;
    p.draws = {{3, 1, {0, 1, 2, 3}}};
    CHECK((tie_triangles(p) == std::vector<TieTriangle>{{2, 1, 0, 3}, {1, 2, 3, 3}}));
}

void dinky_double_phase() {
    // 8 dinky, no fat: the single marker at index 0 makes 0..3 single and
    // 4..7 double; the double marker at index 5 (8 - 3).
    TiePacket pk;
    pk.unpack.dinky_single_end = 40;
    pk.unpack.dinky_double_end = 45;
    pk.unpack.no_fat = 1;
    pk.colors.assign(8, 0);
    for (u16 i = 0; i < 8; ++i) {
        TieDinkyVertex v;
        v.gs_slot = static_cast<u16>(40 + i);
        v.gs_slot_2 = static_cast<u16>(100 + i);
        pk.dinky.push_back(v);
    }
    resolve_tie_vertices(pk);
    std::vector<u16> second;
    for (const TieVertex& v : pk.vertices) {
        second.push_back(v.gs_slot_2);
    }
    CHECK((second == std::vector<u16>{0, 0, 0, 0, 104, 105, 106, 107}));
    // A double marker not at D - 3 would run past the dinky vertices.
    pk.unpack.dinky_double_end = 44;
    CHECK(throws([&] { resolve_tie_vertices(pk); }));
    // Single-only: the marker must be D - 4.
    pk.unpack.dinky_single_only = 1;
    pk.unpack.dinky_single_end = 44;
    resolve_tie_vertices(pk);
    bool none = true;
    for (const TieVertex& v : pk.vertices) {
        none = none && v.gs_slot_2 == 0;
    }
    CHECK(none);
    pk.unpack.dinky_single_end = 45;
    CHECK(throws([&] { resolve_tie_vertices(pk); }));
}

void malformed_packets() {
    const auto [ph, d] = sample();
    std::vector<u8> bad = d;
    bad[0x2c] = 4;  // a strip whose slots are not all written
    CHECK(throws([&] { parse_tie_class(class_blob(ph, bad, 2)); }));
    bad = d;
    const std::array<u8, 16> never_ends = {16, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0};
    std::memcpy(bad.data(), never_ends.data(), 16);  // ad_gif_dest[3] > 0
    CHECK(throws([&] { parse_tie_class(class_blob(ph, bad, 2)); }));
    TiePacketHeader one = ph;
    one.shader_count = 1;
    CHECK(throws([&] { parse_tie_class(class_blob(one, d, 2)); }));
    bad = d;
    bad[0x20 + 6] = 99;  // a missing marker
    CHECK(throws([&] { parse_tie_class(class_blob(ph, bad, 2)); }));
}

void instances() {
    TieInstance inst{};
    inst.matrix = {{{2, 0, 0, 0}, {0, 3, 0, 0}, {0, 0, 4, 0}, {10, 20, 30, 0.01f}}};
    CHECK((inst.world_matrix()[3] == std::array<f32, 4>{10, 20, 30, 1}));
    CHECK((inst.transform_point({1, 1, 1}) == std::array<f32, 3>{12, 23, 34}));
    inst.ambient_rgbas[5] = 0x8000 | (31 << 10) | (1 << 5) | 2;
    CHECK((inst.ambient_rgba(5) == std::array<u8, 4>{16, 8, 248, 0x80}));
    std::vector<u8> section(0x10, 0);
    section[0] = 1;
    append(section, inst);
    std::vector<u8> gameplay(0x40, 0);
    const u32 at = 0x40;
    std::memcpy(gameplay.data() + kGameplayTieInstances, &at, 4);
    gameplay.insert(gameplay.end(), section.begin(), section.end());
    const auto got = parse_tie_instances(gameplay);
    CHECK(got.size() == 1 && std::memcmp(&got[0], &inst, sizeof inst) == 0);
}

}  // namespace

int main() {
    walker_follows_vu1_rules();
    winding_flips_parity();
    dinky_double_phase();
    malformed_packets();
    instances();
    return openrac::test::result();
}
