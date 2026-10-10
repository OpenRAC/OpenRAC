// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Precomputed occlusion on a hand-made grid: the Z -> Y -> X tree and its
// cells, the octant override, the gameplay mappings and how objects resolve
// to visibility bits.

#include "assets/world/occlusion.h"

#include "tests/check.h"

using namespace openrac::assets;

namespace {

// Cells (30, 20, 10) -> mask 0 and (32, 20, 10) -> mask 1; (31, 20, 10) is
// empty (0xffff), row y = 21 and layer z = 11 have no node.
std::vector<u8> grid_block() {
    ByteWriter w;
    w.put<s32>(0x40);      // masks offset
    w.put<u16>(10);        // z base
    w.put<u16>(2);         // z slots
    w.put<u16>(0x10 / 4);  // z 10 -> the y node at 0x10
    w.put<u16>(0);         // z 11: none
    w.resize(0x10);
    w.put<u16>(20);  // y base
    w.put<u16>(2);
    w.put<u16>(0x18 / 4);  // y 20 -> the x node at 0x18
    w.put<u16>(0);
    w.put<u16>(30);  // x base
    w.put<u16>(3);
    w.put<u16>(0);
    w.put<u16>(0xffff);
    w.put<u16>(1);
    w.resize(0x40);
    OcclusionMask a{};
    a[0] = 0x01;
    OcclusionMask b{};
    b[1] = 0x04;
    w.put(a);
    w.put(b);
    return w.bytes();
}

void grid() {
    const auto block = grid_block();
    const OcclusionGrid g = read_occlusion_grid(Game::Rac1, block);
    CHECK((g.cells == std::vector<OcclusionCell>{{30, 20, 10, 0}, {32, 20, 10, 1}}));
    CHECK(g.masks.size() == 2 && g.masks[1][1] == 0x04);
    CHECK(g.lookup(30, 20, 10) == u16{0});
    CHECK(g.lookup(32, 20, 10) == u16{1});
    CHECK(!g.lookup(31, 20, 10));
    CHECK(!g.lookup(30, 21, 10));
    CHECK(!g.lookup(30, 20, 11));
    CHECK(!g.lookup(29, 20, 10));
    // Cells are 4 units: world (121.5, 83, 40.9) is cell (30, 20, 10).
    const auto cell = g.cell_for({121.5f, 83.0f, 40.9f});
    CHECK(cell && cell->mask == 0);
    CHECK((occlusion_cell_coords({-1.0f, 7.9f, 8.0f}) == std::array<s32, 3>{0, 1, 2})
    );  // truncation
    std::vector<u8> bad = block;
    bad[0] = 4;  // a mask offset inside the header
    bool refused = false;
    try {
        read_occlusion_grid(Game::Rac1, bad);
    } catch (const AssetError&) {
        refused = true;
    }
    CHECK(refused);
}

void octants() {
    ByteWriter w;
    for (const f32 c : {10.0f, 20.0f, 30.0f, 0.0f}) {
        w.put(c);
    }
    for (u8 i = 0; i < 8; ++i) {
        OcclusionMask m{};
        m[0] = i;
        w.put(m);
    }
    const OcclusionOctants o = read_occlusion_octants(Game::Rac1, w.bytes());
    // (x > cx) * 4 + (y > cy) * 2 + (z > cz), strictly.
    CHECK(o.mask_for({11, 0, 0})[0] == 4);
    CHECK(o.mask_for({0, 21, 31})[0] == 3);
    CHECK(o.mask_for({10, 20, 30})[0] == 0);
}

OcclusionMappings mappings() {
    ByteWriter w;
    w.put<s32>(2);  // tfrags
    w.put<s32>(2);  // ties
    w.put<s32>(1);  // mobys
    w.resize(0x10);
    for (const std::array<s32, 2>& r :
         {std::array<s32, 2>{3, 0}, {9, 1}, {17, 500}, {18, 501}, {40, 77}}) {
        w.put(r[0]);  // bit
        w.put(r[1]);  // key
    }
    const auto m = read_occlusion_mappings(Game::Rac1, w.bytes());
    CHECK(m.tfrag.size() == 2 && m.tie.size() == 2 && m.moby.size() == 1);
    return m;
}

void bits() {
    const OcclusionBits b = OcclusionBits::from_bit(10);
    CHECK(b.byte() == 1 && b.bit() == 4);
    OcclusionMask frame{};
    CHECK(!b.visible(frame));
    frame[1] = 4;
    CHECK(b.visible(frame));
    CHECK(!OcclusionBits::always().visible(frame));
    CHECK(OcclusionBits::always().visible(with_always_bit(frame)));
}

void resolving() {
    const OcclusionMappings m = mappings();
    LevelOcclusion out;
    // Tfrags whose keys still match take their bits; any mismatch makes the
    // whole table stale and every tfrag always drawn.
    const std::vector<u8> keys = {0, 1};
    resolve_tfrag_occlusion(m, keys, out);
    CHECK(!out.tfrag_out_of_date && out.tfrag[1] == OcclusionBits::from_bit(9));
    const std::vector<u8> stale = {0, 2};
    resolve_tfrag_occlusion(m, stale, out);
    CHECK(out.tfrag_out_of_date && out.tfrag[0] == OcclusionBits::always());

    // Ties in the same order match by position; otherwise by key.
    const std::vector<s32> in_order = {500, 501};
    resolve_tie_occlusion(m, in_order, out);
    CHECK(out.ties_positional && out.tie[1] == OcclusionBits::from_bit(18));
    const std::vector<s32> shuffled = {501, 600, 500};
    resolve_tie_occlusion(m, shuffled, out);
    CHECK(!out.ties_positional && out.ties_not_found == 1);
    CHECK(out.tie[0] == OcclusionBits::from_bit(18) && out.tie[1] == OcclusionBits::always());

    // Mobys by their spawn id, unless their instance opts out.
    MobyInstance culled;
    culled.spawn_id = 77;
    MobyInstance opted_out = culled;
    opted_out.occlusion = 1;
    MobyInstance unknown;
    unknown.spawn_id = 5;
    const std::vector<MobyInstance> mobys = {culled, opted_out, unknown};
    resolve_moby_occlusion(m, mobys, out);
    CHECK(out.moby[0] == OcclusionBits::from_bit(40));
    CHECK(out.moby[1] == OcclusionBits::always() && out.moby[2] == OcclusionBits::always());
    CHECK(out.mobys_not_found == 1);

    // Without mappings everything is always drawn.
    const LevelOcclusion none = resolve_level_occlusion(nullptr, keys, in_order, mobys);
    CHECK(none.tfrag.size() == 2 && none.tie.size() == 2 && none.moby.size() == 3);
    CHECK(none.moby[0] == OcclusionBits::always());

    OcclusionMask frame{};
    frame[5] = 1;  // bit 40
    CHECK((visible_objects(out.moby, frame) == std::vector<u32>{0}));
}

}  // namespace

int main() {
    grid();
    octants();
    bits();
    resolving();
    return openrac::test::result();
}
