// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), the unit tests of
// crates/rc-formats/src/texture.rs, particle_tex.rs and vif.rs: ISC License, Copyright
// (c) 2026 ReRAC contributors.
//
// Textures (CLUT order, alpha, indexed decode), particle textures and VIF
// code lists, on synthetic data.

#include "assets/geometry/particle_textures.h"
#include "assets/geometry/texture.h"
#include "assets/geometry/vif.h"
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

std::vector<u8> words(std::initializer_list<u32> ws) {
    ByteWriter w;
    for (const u32 x : ws) {
        w.put(x);
    }
    return w.bytes();
}

std::vector<u8> concat(std::initializer_list<std::vector<u8>> parts) {
    std::vector<u8> out;
    for (const auto& p : parts) {
        out.insert(out.end(), p.begin(), p.end());
    }
    return out;
}

void clut_order_and_alpha() {
    // Within each 32-entry group the stored blocks 0, 1, 2, 3 are linear 0, 2, 1, 3.
    for (u32 g = 0; g < 8; ++g) {
        const u32 b = g * 32;
        for (u32 k = 0; k < 8; ++k) {
            CHECK(clut_index(b + k) == b + k);
            CHECK(clut_index(b + 8 + k) == b + 16 + k);
            CHECK(clut_index(b + 16 + k) == b + 8 + k);
            CHECK(clut_index(b + 24 + k) == b + 24 + k);
        }
    }
    for (u32 i = 0; i < 256; ++i) {
        CHECK(clut_index(clut_index(i)) == i);
    }
    CHECK(scale_alpha(0x00) == 0x00);
    CHECK(scale_alpha(0x01) == 0x02);
    CHECK(scale_alpha(0x40) == 0x80);
    CHECK(scale_alpha(0x7f) == 0xfe);
    CHECK(scale_alpha(0x80) == 0xff);
    CHECK(scale_alpha(0x81) == 0xff);
    CHECK(scale_alpha(0xff) == 0xff);
}

void indexed_decode() {
    // Stored CLUT entry j = (j, j, j, 0x80 for even j, else 0x10).
    std::vector<u8> clut;
    for (u32 j = 0; j < 256; ++j) {
        const auto v = static_cast<u8>(j);
        clut.insert(clut.end(), {v, v, v, static_cast<u8>(j % 2 == 0 ? 0x80 : 0x10)});
    }
    const std::vector<u8> pixels = {0, 8, 16, 25};
    const RgbaImage t = decode_indexed8(pixels, 2, 2, clut);
    CHECK(t.width == 2 && t.height == 2);
    CHECK((
        t.rgba == std::vector<u8>{0, 0, 0, 0xff, 16, 16, 16, 0xff, 8, 8, 8, 0xff, 25, 25, 25, 0x20}
    ));
    CHECK(throws([&] { decode_indexed8(std::vector<u8>(3), 2, 2, clut); }));
    CHECK(throws([&] { decode_indexed8(std::vector<u8>(4), 2, 2, ByteView(clut.data(), 1020)); }));
}

void particle_defs() {
    // data offset 0x20; types: 0 null, 1 at blob 0 (3 frames), 2 at blob 3 (2 frames), 3 null.
    PartDefs d;
    d.header = {4, 5, 0x20, 5};
    d.offsets = {0, 0x20, 0x23, 0};
    d.blob = {0, 1, 2, 4, 3};
    CHECK(d.start(0) == 0u);
    CHECK(d.start(2) == 3u);
    CHECK(d.first_frame(2) == 4);
    CHECK((d.frames(1) == std::vector<u8>{0, 1, 2}));
    CHECK((d.frames(0) == std::vector<u8>{0, 1, 2}));  // null types share the blob's start
    CHECK((d.frames(2) == std::vector<u8>{4, 3}));
    CHECK(!d.start(9));

    std::vector<u8> index(0x10, 0);
    ByteWriter w;
    for (const s32 v : {2, 7, 0x18, 3, 0, 0x19}) {
        w.put(v);
    }
    index.insert(index.end(), w.bytes().begin(), w.bytes().end());
    index.insert(index.end(), {9, 8, 7});
    const PartDefs p = parse_part_defs(index, 0x10);
    CHECK((p.header == std::array<s32, 4>{2, 7, 0x18, 3}));
    CHECK((p.offsets == std::vector<s32>{0, 0x19}));
    CHECK((p.blob == std::vector<u8>{9, 8, 7}));
    CHECK(p.first_frame(1) == 8);
    CHECK(throws([&] { parse_part_defs(ByteView(index.data(), index.size() - 1), 0x10); }));
    CHECK(throws([&] { parse_part_defs(index, 0); }));
}

void particle_words_and_bank() {
    const PartTextureEntry e{0x800, 0, 0xc00, 32};
    const auto [lo, hi] = e.runtime_words(0x0010'0000);
    CHECK(lo >> 4 == 0x0010'0800 && (lo & 0xf) == 0);
    CHECK(hi >> 4 == 0x0010'0c00 && (hi & 0xf) == 5);

    std::vector<u8> bank(1024 + 4, 0);
    // Stored CLUT entry 8 is linear entry 16 (CSM1).
    bank[8 * 4 + 0] = 10;
    bank[8 * 4 + 1] = 20;
    bank[8 * 4 + 2] = 30;
    bank[8 * 4 + 3] = 0x40;
    bank[1024] = 16;
    bank[1026] = 16;
    const RgbaImage t = decode_bank_texture(bank, 0, 1024, 2, 2);
    CHECK(t.rgba[0] == 10 && t.rgba[1] == 20 && t.rgba[2] == 30 && t.rgba[3] == 0x80);
    CHECK(throws([&] { decode_bank_texture(bank, 0, 1024, 4, 4); }));
}

void vif_codes() {
    // UNPACK V4_16 USN, FLG, address 0x12, NUM 5, the interrupt bit set.
    const auto one = concat({words({0xed05'c012}), std::vector<u8>(40)});
    const auto ps = vif::parse(one);
    CHECK(ps.size() == 1);
    if (ps.size() == 1) {
        const vif::Code& p = ps[0];
        CHECK(p.cmd == 0x6d && p.num == 5 && p.imm == 0xc012 && p.offset == 0);
        CHECK(p.is_unpack());
        CHECK(p.vn() == 3 && p.vl() == 1 && p.usn() && p.addr() == 0x12 && p.count() == 5);
        CHECK(p.element_size() == 8 && p.data.size() == 40);
    }

    // V3_16 x 3 = 18 bytes padded to 20; V4_8 NUM 0 = 256 x 4 bytes; V4_32 x 1.
    const auto sizes = concat(
        {words({0x6903'8000}),
         std::vector<u8>(20),
         words({0x6e00'0000}),
         std::vector<u8>(1024),
         words({0x6c01'0000}),
         std::vector<u8>(16)}
    );
    const auto s = vif::parse(sizes);
    CHECK(s.size() == 3);
    if (s.size() == 3) {
        CHECK(s[0].vn() == 2 && s[0].vl() == 1 && s[0].element_size() == 6 && !s[0].usn());
        CHECK(s[0].data.size() == 20);
        CHECK(
            s[1].count() == 256 && s[1].element_size() == 4 && s[1].data.size() == 1024
            && s[1].offset == 24
        );
        CHECK(
            s[2].vn() == 3 && s[2].vl() == 0 && s[2].element_size() == 16 && s[2].data.size() == 16
        );
    }

    const auto others = concat(
        {words({0x0100'0102, 0x0500'0001}),
         words({0x3000'0000, 1, 2, 3, 4}),
         words({0x2000'0000, 0xffff'ffff}),
         words({0x4a02'0000}),
         std::vector<u8>(16),
         words({0x5000'0001}),
         std::vector<u8>(16),
         words({0x1400'0000})}
    );
    const auto o = vif::parse(others);
    std::vector<u8> cmds;
    std::vector<std::size_t> lengths;
    for (const vif::Code& c : o) {
        cmds.push_back(c.cmd);
        lengths.push_back(c.data.size());
    }
    CHECK((
        cmds
        == std::vector<
            u8>{vif::kStcycl, vif::kStmod, vif::kStrow, vif::kStmask, vif::kMpg, vif::kDirect, vif::kMscal}
    ));
    CHECK((lengths == std::vector<std::size_t>{0, 0, 16, 4, 16, 16, 0}));
    if (o.size() == 7) {
        CHECK(!o[2].is_unpack() && o[2].data.u32_at(4) == 2 && o[1].imm == 1);
    }

    CHECK(throws([&] { vif::parse(words({0x3000'0000, 1, 2})); }));
    // Fewer than four trailing bytes are not a code word.
    CHECK(vif::parse(std::vector<u8>(6)).size() == 1);
}

}  // namespace

int main() {
    clut_order_and_alpha();
    indexed_decode();
    particle_defs();
    particle_words_and_bank();
    vif_codes();
    return openrac::test::result();
}
