// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), the unit tests of
// crates/rc-formats/src/shrub.rs: ISC License, Copyright (c) 2026 ReRAC contributors.
//
// Shrubs: the packet walker's VU1 rules (stop bits, slots, texture state),
// malformed packets, ad-gif and billboard registers, the billboard quad,
// fading, wind sway, instances.

#include "assets/geometry/shrub.h"

#include <cmath>
#include <cstring>

#include "assets/geometry/tfrag.h"
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

template <typename T>
void append(std::vector<u8>& v, const T& value) {
    const auto* p = reinterpret_cast<const u8*>(&value);
    v.insert(v.end(), p, p + sizeof(T));
}

void words(std::vector<u8>& v, std::initializer_list<u32> ws) {
    for (const u32 w : ws) {
        append(v, w);
    }
}

ShrubGifTag tag(u32 nloop, bool eop, u64 prim, s32 slot) {
    return {
        u64{nloop} | u64{eop} << 15 | u64{1} << 46 | prim << 47 | u64{3} << 60,
        0x412,
        slot,
    };
}

ShrubAdGifs adgif(s32 slot, s32 tex) {
    ShrubAdGifs a;
    std::memcpy(a.tex1.pad.data() + 3, &slot, 4);
    a.tex0.data_lo = tex;
    return a;
}

using Vert = std::pair<s16, u16>;  // (slot, normal and stop)

// A packet's VIF list: the header block (V4_32 at 0), then parts 1 and 2
// (V4_16) at the header's addresses.
std::vector<u8> packet(
    const std::vector<ShrubGifTag>& tags,
    const std::vector<ShrubAdGifs>& ads,
    const std::vector<Vert>& verts,
    std::optional<s32> vertex_offset = std::nullopt
) {
    const s32 vo = vertex_offset.value_or(static_cast<s32>(1 + tags.size() + 4 * ads.size()));
    const auto n = static_cast<u32>(verts.size());
    std::vector<u8> v;
    words(v, {0x0100'0404, 0, 0x0500'0000});
    const auto hq = static_cast<u32>(1 + tags.size() + 4 * ads.size());
    words(v, {0x6c00'8000 | hq << 16});
    append(
        v,
        ShrubPacketHeader{
            static_cast<s32>(ads.size()), static_cast<s32>(tags.size()), static_cast<s32>(n), vo
        }
    );
    for (const auto& t : tags) {
        append(v, t);
    }
    for (const auto& a : ads) {
        append(v, a);
    }
    words(v, {0x0500'0000, 0x6d00'8000 | n << 16 | static_cast<u32>(vo)});
    for (std::size_t i = 0; i < verts.size(); ++i) {
        append(v, ShrubVertexPart1{static_cast<s16>(i), 10, 20, verts[i].first});
    }
    words(v, {0x0500'0000, 0x6d00'8000 | n << 16 | (static_cast<u32>(vo) + n)});
    for (std::size_t i = 0; i < verts.size(); ++i) {
        append(
            v,
            ShrubVertexPart2{4096, static_cast<s16>(-static_cast<s16>(i)), 0x1000, verts[i].second}
        );
    }
    return v;
}

std::vector<u8> class_blob(const std::vector<std::vector<u8>>& packets, bool billboard) {
    std::vector<ShrubPacketEntry> table;
    std::vector<u8> data;
    const std::size_t first = 0x40 + 8 * packets.size();
    for (const auto& p : packets) {
        table.push_back({static_cast<s32>(first + data.size()), static_cast<s32>(p.size())});
        data.insert(data.end(), p.begin(), p.end());
        while (data.size() % 16 != 0) {
            data.push_back(0);
        }
    }
    const std::size_t bb = first + data.size();
    const std::size_t normals = bb + (billboard ? 0x40 : 0);
    ShrubClassHeader h;
    h.scale = 2.0f;
    h.o_class = 5;
    h.packet_count = static_cast<s16>(packets.size());
    h.normals_offset = static_cast<s32>(normals);
    h.billboard_offset = billboard ? static_cast<s32>(bb) : 0;
    std::vector<u8> b;
    append(b, h);
    for (const auto& e : table) {
        append(b, e);
    }
    b.insert(b.end(), data.begin(), data.end());
    if (billboard) {
        ShrubBillboard bill;
        bill.fade_distance = 40;
        bill.width = 2;
        bill.height = 3;
        bill.z_ofs = 1;
        append(b, bill);
    }
    for (s16 i = 0; i < 24; ++i) {
        append(b, std::array<s16, 4>{i, 0, 0, 0});
    }
    return b;
}

// The ad-gif at 0, strip A (3 vertices) at 5, strip B (4, EOP) at 15; 8
// vertices: 7 real and a padding one that rewrites vertex 6's slot. The stop
// bit on vertex 4 (8 - 4).
std::vector<u8> sample() {
    std::vector<Vert> verts =
        {{6, 3}, {9, 1}, {12, 2}, {16, 7}, {19, 8}, {22, 9}, {25, 10}, {25, 11}};
    verts[4].second |= 0x8000;
    return packet({tag(3, false, 4, 5), tag(4, true, 4, 15)}, {adgif(0, 2)}, verts);
}

void walker() {
    const ShrubClass sc = parse_shrub_class(class_blob({sample()}, true));
    CHECK((sc.normal(3) == std::array<f32, 3>{3.0f / 32767.0f, 0, 0}));
    CHECK(sc.billboard && sc.billboard->height == 3.0f);
    const ShrubPacket& pk = sc.packets[0];
    CHECK(pk.vertices.size() == 8);
    CHECK(pk.vertices[4].stop == 1 && pk.vertices[4].normal == 8 && pk.vertices[3].normal == 7);
    CHECK((pk.vertices[2].uv() == std::array<f32, 2>{1.0f, -2.0f / 4096.0f}));
    CHECK(
        (pk.vertices[1].class_position(2.0f)
         == std::array<f32, 3>{1.0f / 512.0f, 20.0f / 1024.0f, 40.0f / 1024.0f})
    );
    // The padding vertex 7 wins slot 25 over vertex 6.
    CHECK((pk.draws == std::vector<ShrubDraw>{{2, 4, {0, 1, 2}}, {2, 4, {3, 4, 5, 7}}}));
    CHECK((
        shrub_triangles(pk) == std::vector<ShrubTriangle>{{0, 1, 2, 2}, {3, 4, 5, 2}, {7, 5, 4, 2}}
    ));
}

void stop_bits() {
    std::vector<Vert> base;
    for (s16 i = 0; i < 6; ++i) {
        base.emplace_back(static_cast<s16>(6 + 3 * i), 0);
    }
    auto run = [&](const std::vector<Vert>& v) {
        s32 texture = -1;
        return read_shrub_packet(
            packet({tag(6, true, 4, 5)}, {adgif(0, 1)}, v), ShrubPacketEntry{}, texture
        );
    };
    // A stop bit on vertex 0 or 1 is never tested: the one on vertex 2 ends the loop.
    std::vector<Vert> v = base;
    v[1].second = 0x8000;
    v[2].second = 0x8000;
    const ShrubPacket pk = run(v);
    CHECK(pk.vertices.size() == 6);
    CHECK((pk.draws[0].vertices == std::vector<u16>{0, 1, 2, 3, 4, 5}));
    v = base;
    v[0].second = 0x8000;  // none from index 2 on: VU1 reads past the tables
    CHECK(throws([&] { run(v); }));
    v = base;
    v[3].second = 0x8000;  // 7 vertices processed; the 7th unwritten
    CHECK(throws([&] { run(v); }));
}

void texture_state() {
    std::vector<Vert> verts;
    for (s16 i = 0; i < 6; ++i) {
        verts.emplace_back(static_cast<s16>(1 + 3 * i), i == 2 ? 0x8000 : 0);
    }
    // The second packet's ad-gif block lies after its EOP tag: never read.
    const auto p2 = packet({tag(6, true, 4, 0)}, {adgif(0x40, 9)}, verts);
    const ShrubClass sc = parse_shrub_class(class_blob({sample(), p2}, false));
    CHECK(sc.packets[1].draws[0].texture == 2);
    CHECK(!sc.billboard);
    // Alone, it draws before any texture was set.
    CHECK(throws([&] { parse_shrub_class(class_blob({p2}, false)); }));
}

void malformed() {
    s32 texture = -1;
    CHECK(!throws([&] { read_shrub_packet(sample(), ShrubPacketEntry{}, texture); }));
    std::vector<Vert> verts =
        {{6, 3}, {9, 1}, {12, 2}, {16, 7}, {19, 8}, {22, 9}, {25, 10}, {25, 11}};
    verts[4].second |= 0x8000;
    // A strip longer than the vertices written.
    CHECK(throws([&] {
        read_shrub_packet(
            packet({tag(3, false, 4, 5), tag(5, true, 4, 15)}, {adgif(0, 2)}, verts),
            ShrubPacketEntry{},
            texture
        );
    }));
    // A vertex overwriting the ad-gif block.
    std::vector<Vert> over = verts;
    over[0].first = 2;
    CHECK(throws([&] {
        read_shrub_packet(
            packet({tag(3, false, 4, 5), tag(4, true, 4, 15)}, {adgif(0, 2)}, over),
            ShrubPacketEntry{},
            texture
        );
    }));
    // Triangle fans are not a shrub primitive.
    CHECK(throws([&] {
        read_shrub_packet(
            packet({tag(3, false, 5, 5), tag(4, true, 4, 15)}, {adgif(0, 2)}, verts),
            ShrubPacketEntry{},
            texture
        );
    }));
    // Tables are read by address: elsewhere is fine where the unpacks wrote...
    CHECK(!throws([&] {
        read_shrub_packet(
            packet({tag(3, false, 4, 5), tag(4, true, 4, 15)}, {adgif(0, 2)}, verts, 40),
            ShrubPacketEntry{},
            texture
        );
    }));
    // ...but not where nothing was written.
    std::vector<u8> bad = sample();
    const s32 fifty = 50;
    std::memcpy(bad.data() + 4 * 4 + 12, &fifty, 4);
    CHECK(throws([&] { read_shrub_packet(bad, ShrubPacketEntry{}, texture); }));
}

void ad_gif_rule() {
    const CoreTextureEntry e{0x1000, 128, 64, 4, 231, 2879, 2878};
    const std::array<std::array<s32, 4>, 3> cases = {
        {{-110, 4, 0, 0}, {-142, 4, 0, 1}, {-126, 2, 1, 1}}
    };
    for (const auto& [k, hi, wms, wmt] : cases) {
        ShrubAdGifs s;
        s.tex1.data_lo = k & 0xffff;
        s.tex1.data_hi = hi;
        s.clamp.data_lo = wms;
        s.clamp.data_hi = wmt;
        s.tex0.data_lo = 3;  // the class slot; the table index (7) is the class entry's
        const ShrubGsRegisters r = s.gs_registers(7, e, 0x2'0000);
        TfragAdGifs t;
        t.tex0.data_lo = 7;
        t.tex1 = s.tex1;
        t.clamp = s.clamp;
        const TfragGsRegisters tr = t.gs_registers(e, 0x2'0000);
        CHECK(
            r.tex0 == tr.tex0 && r.tex1 == tr.tex1 && r.clamp == tr.clamp && r.miptbp1 == tr.miptbp1
        );
        CHECK(s.lod_k_raw() == static_cast<s16>(k));
    }
}

void billboards() {
    const CoreBillboardInfo info{32, 32, 3, 3061, 3065, 3069, 3070, 0};
    ShrubBillboard b;
    b.fade_distance = 15;
    b.width = 27724.238f;
    b.height = 27724.238f;
    b.tex1.data_lo = 0xff56;
    b.tex1.data_hi = 4;
    const ShrubBillboardRegisters r = b.gs_registers(info, 0);
    CHECK(r.mxl() == 2 && r.lod_k() == -170.0f / 16.0f);
    CHECK((r.tex1 & 0xffff'ffff) == (2 << 2 | 4 << 6 | 0x20));
    CHECK((r.tex0 & 0x3fff) == 3065);
    CHECK(((r.tex0 >> 14) & 0x3f) == 1);
    CHECK(
        ((r.tex0 >> 26) & 0xf) == 5 && ((r.tex0 >> 30) & 0xf) == 5
        && ((r.tex0 >> 37) & 0x3fff) == 3061
    );
    CHECK(
        (r.miptbp1 & 0x3fff) == 3069 && ((r.miptbp1 >> 20) & 0x3fff) == 3070
        && ((r.miptbp1 >> 40) & 0x3fff) == 0
    );
    // An instance of class scale 0.036935188: a 1-unit-wide quad from the origin up.
    std::array<std::array<f32, 4>, 4> m{};
    m[0][0] = 1;
    m[1][1] = 1;
    m[2][2] = 2;
    const u32 packed = packed_column_lengths(m);
    CHECK(packed == (0x1000u | 0x2000u << 16));
    const auto ext = billboard_extent(b, 0.036935188f, packed);
    CHECK(std::abs(ext[0] - 1) < 1e-5f && std::abs(ext[1] - 2) < 1e-5f && ext[2] == 0);
    // The camera due -y, level: the quad spans x = +-0.5, z = 0..2.
    auto c = billboard_corners({10, 20, 5}, {10, 0, 5}, ext);
    CHECK(std::abs(c[0][0] - 9.5f) < 1e-5f && std::abs(c[0][2] - 7) < 1e-5f);
    CHECK(std::abs(c[3][0] - 10.5f) < 1e-5f && std::abs(c[3][2] - 5) < 1e-5f);
    // From 45 degrees above, the width shrinks by cos 45.
    c = billboard_corners({0, 10, 0}, {0, 0, 10}, ext);
    CHECK(std::abs((c[1][0] - c[0][0]) - static_cast<f32>(std::sqrt(0.5))) < 1e-5f);
    // A column length of exactly 16 spills into bit 16.
    m[0][0] = 16;
    m[1][1] = 16;
    m[2][2] = 17;
    CHECK(packed_column_lengths(m) == 0x10000);
}

void fading() {
    CHECK((shrub_fade(10, 32, std::nullopt) == ShrubFade{0x80, std::nullopt}));
    CHECK((shrub_fade(28, 32, std::nullopt) == ShrubFade{64, std::nullopt}));
    CHECK((shrub_fade(10, 200, 15) == ShrubFade{0x80, std::nullopt}));
    CHECK((shrub_fade(17, 200, 15) == ShrubFade{96, 32}));
    CHECK((shrub_fade(23, 200, 15) == ShrubFade{std::nullopt, 0x80}));
    CHECK((shrub_fade(196, 200, 15) == ShrubFade{std::nullopt, 32}));
    CHECK((shrub_fade(1, 24, 0) == ShrubFade{std::nullopt, 0x80}));
}

void sway() {
    // The table is a negated sine.
    CHECK(sway_table(0) == 0 && sway_table(20) == -59 && sway_table(64) == -127);
    CHECK(sway_table(128) == 0 && sway_table(192) == 127);
    CHECK(!wind_sway(0, 0x100000, 5, {1, 1, 0}));
    CHECK(!wind_sway(2, 0x100000, 5, {70, 40, 0}));  // 4900 + 1600 > 6000
    const auto s = wind_sway(2, 0, 0, {0, 0, 0});
    CHECK(s && std::abs((*s)[0] - (0.08f - 0.02f * 127.0f / 128.0f)) < 1e-6f && (*s)[1] == 0);
    const auto half = wind_sway(1, 0, 0, {0, 0, 0});
    CHECK(half && s && std::abs((*half)[0] - (*s)[0] * 0.5f) < 1e-7f);
    // Attenuation is linear in the squared distance.
    const auto far = wind_sway(2, 0, 0, {0, 0, std::sqrt(3000.0f)});
    CHECK(far && s && std::abs((*far)[0] - (*s)[0] * 0.5f) < 1e-4f);
}

void instances() {
    ShrubInstance inst;
    inst.o_class = 3;
    inst.draw_distance = 64;
    inst.colour = {10, 20, 30};
    inst.dir_lights = 0x4021;
    inst.matrix = {{{2, 0, 0, 0}, {0, 3, 0, 0}, {0, 0, 4, 0}, {10, 20, 30, 0.01f}}};
    CHECK((inst.world_matrix()[3] == std::array<f32, 4>{10, 20, 30, 1}));
    CHECK((inst.transform_point({1, 1, 1}) == std::array<f32, 3>{12, 23, 34}));
    CHECK((inst.ambient_rgba() == std::array<u8, 4>{10, 20, 30, 0x80}));
    CHECK((inst.light_sets() == ShrubInstance::LightSets{1, 2, 0x40}));
    std::vector<u8> gp(0x40, 0);
    const u32 classes_at = 0x40;
    std::memcpy(gp.data() + kGameplayShrubClasses, &classes_at, 4);
    gp.insert(gp.end(), {1, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    const auto at = static_cast<u32>(gp.size());
    std::memcpy(gp.data() + kGameplayShrubInstances, &at, 4);
    gp.insert(gp.end(), {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    append(gp, inst);
    CHECK((parse_shrub_class_list(gp) == std::vector<s32>{3}));
    const auto got = parse_shrub_instances(gp);
    CHECK(got.size() == 1 && std::memcmp(&got[0], &inst, sizeof inst) == 0);
}

}  // namespace

int main() {
    walker();
    stop_bits();
    texture_state();
    malformed();
    ad_gif_rule();
    billboards();
    fading();
    sway();
    instances();
    return openrac::test::result();
}
