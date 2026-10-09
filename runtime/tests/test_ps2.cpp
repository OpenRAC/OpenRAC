// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Tests of the hardware model. The expected values are from public hardware
// documentation (the memory layouts, the packet formats, the equations) and
// from arithmetic, never from a game.

#include <cstdio>
#include <functional>
#include <set>
#include <string>
#include <vector>

#include "check.h"
#include "dma.h"
#include "gif.h"
#include "gs.h"
#include "memory.h"
#include "vif.h"

using namespace ps2;

namespace {

// --- helpers -----------------------------------------------------------------

constexpr u32 kOffset = 2048;  // window origin at the centre of the coordinate space

struct Machine {
    Gs gs;
    Gif gif{gs};
    Vif1 vif{gif};

    // A 64-pixel-wide 32-bit colour buffer at page 0 and a 32-bit Z buffer at page 8.
    void target(u32 psm = PSMCT32, u32 zpsm = 0) {
        gs.write(gsreg::PRMODECONT, 1);
        gs.write(gsreg::FRAME_1, 0 | (u64{1} << 16) | (u64{psm} << 24));
        gs.write(gsreg::ZBUF_1, 8 | (u64{zpsm} << 24));
        gs.write(gsreg::XYOFFSET_1, (kOffset * 16) | (u64{kOffset * 16} << 32));
        gs.write(gsreg::SCISSOR_1, (u64{63} << 16) | (u64{63} << 48));
        gs.write(gsreg::TEST_1, 0);
        gs.write(gsreg::COLCLAMP, 1);
    }

    void colour(u32 r, u32 g, u32 b, u32 a, float q = 1.0f) {
        gs.write(gsreg::RGBAQ, r | (g << 8) | (b << 16) | (u64{a} << 24) | (u64{as_u32(q)} << 32));
    }

    // Window coordinates in pixels, with four fractional bits available.
    void xyz(float x, float y, u32 z = 0, bool kick = true) {
        u64 fx = static_cast<u64>((x + kOffset) * 16.0f),
            fy = static_cast<u64>((y + kOffset) * 16.0f);
        gs.write(kick ? gsreg::XYZ2 : gsreg::XYZ3, fx | (fy << 16) | (u64{z} << 32));
    }

    void uv(float u, float v) {
        gs.write(gsreg::UV, static_cast<u64>(u * 16.0f) | (static_cast<u64>(v * 16.0f) << 16));
    }

    void sprite(float x0, float y0, float x1, float y1, u32 z = 0) {
        xyz(x0, y0, z);
        xyz(x1, y1, z);
    }

    u32 at(u32 x, u32 y, u32 psm = PSMCT32) const { return gs.memory.read(psm, 0, 1, x, y); }

    // Send pixels to a buffer as a host to local transfer.
    void upload(
        u32 block,
        u32 bw,
        u32 psm,
        u32 w,
        u32 h,
        const std::vector<u8>& bytes,
        std::size_t chunk = 0
    ) {
        gs.write(gsreg::BITBLTBUF, (u64{block} << 32) | (u64{bw} << 48) | (u64{psm} << 56));
        gs.write(gsreg::TRXPOS, 0);
        gs.write(gsreg::TRXREG, w | (u64{h} << 32));
        gs.write(gsreg::TRXDIR, 0);
        if (chunk == 0) {
            gs.transfer_in(bytes.data(), bytes.size());
        } else {
            for (std::size_t i = 0; i < bytes.size(); i += chunk) {
                gs.transfer_in(bytes.data() + i, std::min(chunk, bytes.size() - i));
            }
        }
    }
};

u64 tex0(
    u32 tbp,
    u32 tbw,
    u32 psm,
    u32 tw,
    u32 th,
    bool tcc,
    u32 tfx,
    u32 cbp = 0,
    u32 cpsm = 0,
    u32 csa = 0,
    u32 cld = 0
) {
    return tbp | (u64{tbw} << 14) | (u64{psm} << 20) | (u64{tw} << 26) | (u64{th} << 30)
           | (u64{tcc} << 34) | (u64{tfx} << 35) | (u64{cbp} << 37) | (u64{cpsm} << 51)
           | (u64{csa} << 56) | (u64{cld} << 61);
}

constexpr u64 kPrimTriangle = 3, kPrimStrip = 4, kPrimFan = 5, kPrimSprite = 6;
constexpr u64 kIip = 1 << 3, kTme = 1 << 4, kFge = 1 << 5, kAbe = 1 << 6, kFst = 1 << 8;

std::vector<u8> words(std::initializer_list<u32> list) {
    std::vector<u8> out(list.size() * 4);
    std::size_t i = 0;
    for (u32 w : list) {
        store<u32>(&out[i], w);
        i += 4;
    }
    return out;
}

// --- GS local memory ---------------------------------------------------------

void test_memory_layout() {
    // 32-bit: pixels inside a column, columns inside a block, blocks inside a page.
    CHECK_EQ(GsMemory::address32(0, 1, 0, 0), 0u);
    CHECK_EQ(GsMemory::address32(0, 1, 1, 0), 1u);
    CHECK_EQ(GsMemory::address32(0, 1, 0, 1), 2u);
    CHECK_EQ(GsMemory::address32(0, 1, 2, 0), 4u);
    CHECK_EQ(GsMemory::address32(0, 1, 0, 2), 16u);
    CHECK_EQ(GsMemory::address32(0, 1, 8, 0), 1u * 64);
    CHECK_EQ(GsMemory::address32(0, 1, 0, 8), 2u * 64);
    CHECK_EQ(GsMemory::address32(0, 1, 16, 0), 4u * 64);
    CHECK_EQ(GsMemory::address32(0, 1, 0, 16), 8u * 64);
    CHECK_EQ(GsMemory::address32(0, 1, 32, 0), 16u * 64);
    // The next page along and the next page down, in a buffer two pages wide.
    CHECK_EQ(GsMemory::address32(0, 2, 64, 0), 32u * 64);
    CHECK_EQ(GsMemory::address32(0, 2, 0, 32), 64u * 64);
    // A block pointer is added as it is.
    CHECK_EQ(GsMemory::address32(100, 1, 0, 0), 100u * 64);
    // Z buffers store their blocks in a different order.
    CHECK_EQ(GsMemory::address32(0, 1, 0, 0, true), 24u * 64);
    CHECK_EQ(GsMemory::address32(0, 1, 32, 16, true), 0u * 64);

    // 16-bit: two pixels to a word, eight columns apart.
    CHECK_EQ(GsMemory::address16(0, 1, 1, 0), 2u);
    CHECK_EQ(GsMemory::address16(0, 1, 8, 0), 1u);
    CHECK_EQ(GsMemory::address16(0, 1, 0, 1), 4u);
    CHECK_EQ(GsMemory::address16(0, 1, 0, 2), 32u);
    CHECK_EQ(GsMemory::address16(0, 1, 16, 0), 2u * 128);
    CHECK_EQ(GsMemory::address16(0, 1, 0, 8), 1u * 128);
    CHECK_EQ(GsMemory::address16(0, 1, 32, 0), 8u * 128);
    CHECK_EQ(GsMemory::address16(0, 1, 0, 32), 16u * 128);
    CHECK_EQ(GsMemory::address16s(0, 1, 32, 0), 16u * 128);
    CHECK_EQ(GsMemory::address16s(0, 1, 0, 16), 8u * 128);
    CHECK_EQ(GsMemory::address16s(0, 1, 0, 32), 4u * 128);

    // 8-bit: the second half of a column's rows swaps its two groups of words,
    // and odd columns do the opposite.
    CHECK_EQ(GsMemory::address8(0, 2, 1, 0), 4u);
    CHECK_EQ(GsMemory::address8(0, 2, 8, 0), 2u);
    CHECK_EQ(GsMemory::address8(0, 2, 0, 1), 8u);
    CHECK_EQ(GsMemory::address8(0, 2, 0, 2), 33u);
    CHECK_EQ(GsMemory::address8(0, 2, 4, 2), 1u);
    CHECK_EQ(GsMemory::address8(0, 2, 0, 4), 96u);
    CHECK_EQ(GsMemory::address8(0, 2, 0, 6), 65u);
    CHECK_EQ(GsMemory::address8(0, 2, 16, 0), 1u * 256);
    CHECK_EQ(GsMemory::address8(0, 2, 0, 16), 2u * 256);

    // 4-bit.
    CHECK_EQ(GsMemory::address4(0, 2, 1, 0), 8u);
    CHECK_EQ(GsMemory::address4(0, 2, 8, 0), 2u);
    CHECK_EQ(GsMemory::address4(0, 2, 0, 1), 16u);
    CHECK_EQ(GsMemory::address4(0, 2, 0, 2), 65u);
    CHECK_EQ(GsMemory::address4(0, 2, 0, 4), 192u);
    CHECK_EQ(GsMemory::address4(0, 2, 32, 0), 2u * 512);
    CHECK_EQ(GsMemory::address4(0, 2, 0, 16), 1u * 512);
}

void test_memory_is_one_to_one() {
    // Over one page, every pixel of a format has a storage unit of its own.
    struct Case {
        const char* name;
        u32 w, h, units;
        std::function<u32(u32, u32)> address;
    };

    const Case cases[] = {
        {"32", 64, 32, 2048, [](u32 x, u32 y) { return GsMemory::address32(0, 1, x, y); }},
        {"32z", 64, 32, 2048, [](u32 x, u32 y) { return GsMemory::address32(0, 1, x, y, true); }},
        {"16", 64, 64, 4096, [](u32 x, u32 y) { return GsMemory::address16(0, 1, x, y); }},
        {"16s", 64, 64, 4096, [](u32 x, u32 y) { return GsMemory::address16s(0, 1, x, y); }},
        {"16z", 64, 64, 4096, [](u32 x, u32 y) { return GsMemory::address16(0, 1, x, y, true); }},
        {"16sz", 64, 64, 4096, [](u32 x, u32 y) { return GsMemory::address16s(0, 1, x, y, true); }},
        {"8", 128, 64, 8192, [](u32 x, u32 y) { return GsMemory::address8(0, 2, x, y); }},
        {"4", 128, 128, 16384, [](u32 x, u32 y) { return GsMemory::address4(0, 2, x, y); }},
    };
    for (const Case& c : cases) {
        std::set<u32> seen;
        u32 highest = 0;
        for (u32 y = 0; y < c.h; y++) {
            for (u32 x = 0; x < c.w; x++) {
                u32 a = c.address(x, y);
                seen.insert(a);
                highest = std::max(highest, a);
            }
        }
        CHECK_EQ(seen.size(), std::size_t{c.w} * c.h);
        CHECK_EQ(highest, c.units - 1);
    }
}

void test_memory_tables() {
    // The table form of the layout gives the same place as the formulas, for
    // every format, at several buffer positions and widths.
    struct Case {
        u32 psm;
        std::function<u32(u32, u32, u32, u32)> address;
    };

    const Case cases[] = {
        {PSMCT32, [](u32 bp, u32 bw, u32 x, u32 y) { return GsMemory::address32(bp, bw, x, y); }},
        {PSMZ32,
         [](u32 bp, u32 bw, u32 x, u32 y) { return GsMemory::address32(bp, bw, x, y, true); }},
        {PSMCT16, [](u32 bp, u32 bw, u32 x, u32 y) { return GsMemory::address16(bp, bw, x, y); }},
        {PSMZ16,
         [](u32 bp, u32 bw, u32 x, u32 y) { return GsMemory::address16(bp, bw, x, y, true); }},
        {PSMCT16S, [](u32 bp, u32 bw, u32 x, u32 y) { return GsMemory::address16s(bp, bw, x, y); }},
        {PSMZ16S,
         [](u32 bp, u32 bw, u32 x, u32 y) { return GsMemory::address16s(bp, bw, x, y, true); }},
        {PSMT8, [](u32 bp, u32 bw, u32 x, u32 y) { return GsMemory::address8(bp, bw, x, y); }},
        {PSMT4, [](u32 bp, u32 bw, u32 x, u32 y) { return GsMemory::address4(bp, bw, x, y); }},
    };
    for (const Case& c : cases) {
        const GsMemory::Layout& l = GsMemory::layout(c.psm);
        bool same = true;
        for (u32 bw : {2u, 8u, 10u}) {
            for (u32 bp : {0u, 4160u, 16000u}) {
                for (u32 y = 0; y < 300; y += 1) {
                    for (u32 x = 0; x < 700; x += 3) {
                        same = same && GsMemory::index(l, bp, bw, x, y) == c.address(bp, bw, x, y);
                    }
                }
            }
        }
        CHECK(same);
    }
}

void test_memory_formats() {
    GsMemory m;
    const u32 formats[] =
        {PSMCT32, PSMCT24, PSMCT16, PSMCT16S, PSMT8, PSMT4, PSMZ32, PSMZ24, PSMZ16, PSMZ16S};
    for (u32 psm : formats) {
        unsigned bits_per_pixel = transfer_bits(psm);
        u32 mask = bits_per_pixel == 32 ? 0xFFFFFFFFu : (1u << bits_per_pixel) - 1;
        for (u32 y = 0; y < 40; y++) {
            for (u32 x = 0; x < 70; x++) {
                m.write(psm, 64, 2, x, y, (x * 2654435761u + y * 40503u) & mask);
            }
        }
        bool same = true;
        for (u32 y = 0; y < 40; y++) {
            for (u32 x = 0; x < 70; x++) {
                same = same && m.read(psm, 64, 2, x, y) == ((x * 2654435761u + y * 40503u) & mask);
            }
        }
        CHECK(same);
    }
    // The formats that live in the top byte of a 32-bit pixel leave the rest alone.
    m.write(PSMCT32, 0, 1, 3, 3, 0x11223344);
    m.write(PSMT8H, 0, 1, 3, 3, 0xAB);
    CHECK_EQ(m.read(PSMCT32, 0, 1, 3, 3), 0xAB223344u);
    m.write(PSMT4HL, 0, 1, 3, 3, 0x5);
    CHECK_EQ(m.read(PSMCT32, 0, 1, 3, 3), 0xA5223344u);
    m.write(PSMT4HH, 0, 1, 3, 3, 0xC);
    CHECK_EQ(m.read(PSMCT32, 0, 1, 3, 3), 0xC5223344u);
    CHECK_EQ(m.read(PSMT4HH, 0, 1, 3, 3), 0xCu);
    m.write(PSMCT24, 0, 1, 3, 3, 0xFFFFFFFF);
    CHECK_EQ(m.read(PSMCT32, 0, 1, 3, 3), 0xC5FFFFFFu);
}

// --- drawing -----------------------------------------------------------------

void test_sprite_coverage() {
    Machine m;
    m.target();
    m.gs.write(gsreg::PRIM, kPrimSprite);
    m.colour(1, 2, 3, 4);
    m.sprite(10, 5, 20, 8);
    CHECK_EQ(m.at(10, 5), 0x04030201u);
    CHECK_EQ(m.at(19, 7), 0x04030201u);
    CHECK_EQ(m.at(9, 5), 0u);
    CHECK_EQ(m.at(20, 5), 0u);
    CHECK_EQ(m.at(10, 4), 0u);
    CHECK_EQ(m.at(10, 8), 0u);
    CHECK_EQ(m.gs.stats.pixels, 30u);

    // Corners in the other order draw the same pixels.
    m.colour(9, 9, 9, 9);
    m.sprite(20, 8, 10, 5);
    CHECK_EQ(m.at(10, 5), 0x09090909u);
    CHECK_EQ(m.at(20, 8), 0u);

    // The scissor rectangle is inclusive.
    m.gs.write(gsreg::SCISSOR_1, u64{12} | (u64{13} << 16) | (u64{6} << 32) | (u64{6} << 48));
    m.colour(7, 7, 7, 7);
    m.sprite(0, 0, 64, 64);
    CHECK_EQ(m.at(12, 6), 0x07070707u);
    CHECK_EQ(m.at(13, 6), 0x07070707u);
    CHECK_EQ(m.at(11, 6), 0x09090909u);
    CHECK_EQ(m.at(14, 6), 0x09090909u);
    CHECK_EQ(m.at(12, 7), 0x09090909u);
}

void test_triangles_share_edges() {
    // Two triangles that share an edge, drawn additively: every pixel of the
    // square they make is touched exactly once, in every winding.
    for (int winding = 0; winding < 2; winding++) {
        Machine m;
        m.target();
        m.gs.write(
            gsreg::ALPHA_1, 0x0 | (2 << 2) | (2 << 4) | (1 << 6) | (u64{0x80} << 32)
        );  // Cs * FIX + Cd
        m.gs.write(gsreg::PRIM, kPrimTriangle | kAbe);
        m.colour(0x10, 0x10, 0x10, 0x80);
        const float a[2] = {3, 2}, b[2] = {19, 2}, c[2] = {3, 18}, d[2] = {19, 18};
        if (winding == 0) {
            m.xyz(a[0], a[1]);
            m.xyz(b[0], b[1]);
            m.xyz(c[0], c[1]);
            m.xyz(b[0], b[1]);
            m.xyz(d[0], d[1]);
            m.xyz(c[0], c[1]);
        } else {
            m.xyz(c[0], c[1]);
            m.xyz(b[0], b[1]);
            m.xyz(a[0], a[1]);
            m.xyz(c[0], c[1]);
            m.xyz(d[0], d[1]);
            m.xyz(b[0], b[1]);
        }
        bool once = true;
        for (u32 y = 0; y < 24; y++) {
            for (u32 x = 0; x < 24; x++) {
                bool inside = x >= 3 && x < 19 && y >= 2 && y < 18;
                once = once && (m.at(x, y) & 0xFF) == (inside ? 0x10u : 0u);
            }
        }
        CHECK(once);
        CHECK_EQ(m.gs.stats.pixels, 256u);
    }
}

void test_strip_and_fan() {
    // A strip of four vertices and a fan of four vertices both make the same square.
    for (u64 prim : {kPrimStrip, kPrimFan}) {
        Machine m;
        m.target();
        m.gs.write(gsreg::PRIM, prim);
        m.colour(0xFF, 0, 0, 0x80);
        if (prim == kPrimStrip) {
            m.xyz(4, 4);
            m.xyz(12, 4);
            m.xyz(4, 12);
            m.xyz(12, 12);
        } else {
            m.xyz(4, 4);
            m.xyz(12, 4);
            m.xyz(12, 12);
            m.xyz(4, 12);
        }
        CHECK_EQ(m.gs.stats.primitives, 2u);
        CHECK_EQ(m.gs.stats.pixels, 64u);
        CHECK_EQ(m.at(4, 4) & 0xFF, 0xFFu);
        CHECK_EQ(m.at(11, 11) & 0xFF, 0xFFu);
        CHECK_EQ(m.at(12, 12), 0u);
    }
    // A vertex written without a kick leaves its triangle out of a strip.
    Machine m;
    m.target();
    m.gs.write(gsreg::PRIM, kPrimStrip);
    m.colour(0xFF, 0, 0, 0x80);
    m.xyz(4, 4);
    m.xyz(12, 4);
    m.xyz(4, 12, 0, false);
    m.xyz(12, 12);
    CHECK_EQ(m.gs.stats.primitives, 1u);
    CHECK_EQ(m.at(5, 4), 0u);              // in the skipped triangle
    CHECK_EQ(m.at(11, 10) & 0xFF, 0xFFu);  // in the drawn one
}

void test_gouraud() {
    Machine m;
    m.target();
    m.gs.write(gsreg::PRIM, kPrimTriangle | kIip);
    m.colour(0, 0, 0, 0x80);
    m.xyz(0, 0);
    m.colour(160, 0, 0, 0x80);
    m.xyz(16, 0);
    m.colour(0, 160, 0, 0x80);
    m.xyz(0, 16);
    CHECK_EQ(m.at(0, 0), 0x80000000u);
    CHECK_EQ(m.at(8, 0), 0x80000050u);  // halfway to the red corner
    CHECK_EQ(m.at(0, 8), 0x80005000u);  // halfway to the green one
    CHECK_EQ(m.at(4, 4), 0x80002828u);
    // Without shading the last vertex's colour covers the whole triangle.
    m.gs.write(gsreg::PRIM, kPrimTriangle);
    m.colour(1, 1, 1, 1);
    m.xyz(32, 0);
    m.colour(2, 2, 2, 2);
    m.xyz(48, 0);
    m.colour(3, 3, 3, 3);
    m.xyz(32, 16);
    CHECK_EQ(m.at(33, 1), 0x03030303u);
}

void test_depth() {
    Machine m;
    m.target();
    m.gs.write(gsreg::TEST_1, (u64{1} << 16) | (u64{2} << 17));  // greater or equal
    m.gs.write(gsreg::PRIM, kPrimSprite);
    m.colour(1, 0, 0, 0);
    m.sprite(0, 0, 8, 8, 1000);
    m.colour(2, 0, 0, 0);
    m.sprite(0, 0, 8, 8, 999);  // behind: rejected
    CHECK_EQ(m.at(1, 1), 1u);
    m.colour(3, 0, 0, 0);
    m.sprite(0, 0, 8, 8, 1000);  // equal: passes
    CHECK_EQ(m.at(1, 1), 3u);
    m.gs.write(gsreg::TEST_1, (u64{1} << 16) | (u64{3} << 17));  // greater
    m.colour(4, 0, 0, 0);
    m.sprite(0, 0, 8, 8, 1000);
    CHECK_EQ(m.at(1, 1), 3u);
    m.sprite(0, 0, 8, 8, 1001);
    CHECK_EQ(m.at(1, 1), 4u);
    CHECK_EQ(m.gs.memory.read(PSMZ32, 8 * 32, 1, 1, 1), 1001u);
    // With the Z mask set the colour is written and the depth is not.
    m.gs.write(gsreg::ZBUF_1, 8 | (u64{1} << 32));
    m.colour(5, 0, 0, 0);
    m.sprite(0, 0, 8, 8, 5000);
    CHECK_EQ(m.at(1, 1), 5u);
    CHECK_EQ(m.gs.memory.read(PSMZ32, 8 * 32, 1, 1, 1), 1001u);
    // With the depth test off, nothing is written to the Z buffer.
    m.gs.write(gsreg::ZBUF_1, 8);
    m.gs.write(gsreg::TEST_1, 0);
    m.colour(6, 0, 0, 0);
    m.sprite(0, 0, 8, 8, 9000);
    CHECK_EQ(m.at(1, 1), 6u);
    CHECK_EQ(m.gs.memory.read(PSMZ32, 8 * 32, 1, 1, 1), 1001u);
    // A triangle's depth is interpolated across it.
    Machine t;
    t.target();
    t.gs.write(gsreg::TEST_1, (u64{1} << 16) | (u64{1} << 17));
    t.gs.write(gsreg::PRIM, kPrimTriangle);
    t.colour(1, 1, 1, 1);
    t.xyz(0, 0, 0);
    t.xyz(16, 0, 1600);
    t.xyz(0, 16, 0);
    CHECK_EQ(t.gs.memory.read(PSMZ32, 8 * 32, 1, 4, 0), 400u);
    CHECK_EQ(t.gs.memory.read(PSMZ32, 8 * 32, 1, 8, 4), 800u);
}

void test_blend_and_masks() {
    Machine m;
    m.target();
    m.gs.write(gsreg::PRIM, kPrimSprite);
    m.colour(100, 100, 100, 0x80);
    m.sprite(0, 0, 8, 8);
    // (Cs - Cd) * As + Cd with As a quarter: a quarter of the way to Cs.
    m.gs.write(gsreg::ALPHA_1, 0 | (1 << 2) | (0 << 4) | (1 << 6));
    m.gs.write(gsreg::PRIM, kPrimSprite | kAbe);
    m.colour(200, 60, 100, 0x20);
    m.sprite(0, 0, 8, 8);
    CHECK_EQ(m.at(0, 0), 0x20000000u | 125 | (90 << 8) | (100 << 16));
    // Cs + Cd saturates with COLCLAMP on and wraps with it off.
    m.gs.write(gsreg::ALPHA_1, 0 | (2 << 2) | (2 << 4) | (1 << 6) | (u64{0x80} << 32));
    m.colour(200, 0, 0, 0);
    m.sprite(0, 0, 8, 8);
    CHECK_EQ(m.at(0, 0) & 0xFF, 255u);
    m.gs.write(gsreg::COLCLAMP, 0);
    m.sprite(0, 0, 8, 8);
    CHECK_EQ(m.at(0, 0) & 0xFF, (255u + 200u) & 0xFF);
    m.gs.write(gsreg::COLCLAMP, 1);

    // The frame mask protects the bits that are set in it.
    m.gs.write(gsreg::PRIM, kPrimSprite);
    m.gs.write(gsreg::FRAME_1, 0 | (u64{1} << 16) | (u64{0x00FF00FFu} << 32));
    m.colour(0x11, 0x22, 0x33, 0x44);
    m.sprite(16, 0, 24, 8);
    CHECK_EQ(m.at(16, 0), 0x44002200u);
    // FBA forces the top alpha bit on.
    m.gs.write(gsreg::FRAME_1, 0 | (u64{1} << 16));
    m.gs.write(gsreg::FBA_1, 1);
    m.sprite(16, 0, 24, 8);
    CHECK_EQ(m.at(16, 0), 0xC4332211u);
    m.gs.write(gsreg::FBA_1, 0);

    // Destination alpha test: only pixels whose stored top alpha bit matches.
    m.gs.write(gsreg::TEST_1, (u64{1} << 14) | (u64{1} << 15));
    m.colour(9, 9, 9, 9);
    m.sprite(0, 0, 24, 8);
    CHECK_EQ(m.at(16, 0), 0x09090909u);  // was 0xC4...: bit set, passes
    CHECK((m.at(0, 0) >> 24) == 0);      // was alpha 0: fails, unchanged
    CHECK(m.at(0, 0) != 0x09090909u);
}

void test_alpha_test() {
    Machine m;
    m.target();
    auto test_reg = [](u32 atst, u32 aref, u32 afail) {
        return u64{1} | (u64{atst} << 1) | (u64{aref} << 4) | (u64{afail} << 12) | (u64{1} << 16)
               | (u64{1} << 17);
    };
    auto z = [&](u32 x) {
        return m.gs.memory.read(PSMZ32, 8 * 32, 1, x, 0);
    };
    m.gs.write(gsreg::PRIM, kPrimSprite);
    m.colour(1, 1, 1, 0x10);
    // Fails "greater or equal 0x80"; what happens next depends on AFAIL.
    m.gs.write(gsreg::TEST_1, test_reg(5, 0x80, 0));
    m.sprite(0, 0, 8, 8, 77);
    CHECK_EQ(m.at(0, 0), 0u);
    CHECK_EQ(z(0), 0u);
    m.gs.write(gsreg::TEST_1, test_reg(5, 0x80, 1));
    m.sprite(8, 0, 16, 8, 77);
    CHECK_EQ(m.at(8, 0), 0x10010101u);
    CHECK_EQ(z(8), 0u);
    m.gs.write(gsreg::TEST_1, test_reg(5, 0x80, 2));
    m.sprite(16, 0, 24, 8, 77);
    CHECK_EQ(m.at(16, 0), 0u);
    CHECK_EQ(z(16), 77u);
    m.gs.write(gsreg::TEST_1, test_reg(5, 0x80, 3));
    m.sprite(24, 0, 32, 8, 77);
    CHECK_EQ(m.at(24, 0), 0x00010101u);
    CHECK_EQ(z(24), 0u);
    // And a pixel that passes is written whole.
    m.gs.write(gsreg::TEST_1, test_reg(2, 0x80, 0));
    m.sprite(32, 0, 40, 8, 77);
    CHECK_EQ(m.at(32, 0), 0x10010101u);
    CHECK_EQ(z(32), 77u);
}

void test_16_bit_target() {
    Machine m;
    m.target(PSMCT16);
    m.gs.write(gsreg::PRIM, kPrimSprite);
    m.colour(0xFF, 0x80, 0x08, 0x80);
    m.sprite(0, 0, 8, 8);
    CHECK_EQ(m.at(0, 0, PSMCT16), 0x8000u | 31 | (16 << 5) | (1 << 10));
    // Blending reads the pixel back as 8 bits a channel, alpha 0x80 or 0.
    m.gs.write(gsreg::ALPHA_1, 1 | (2 << 2) | (1 << 4) | (2 << 6));  // Cd * Ad
    m.gs.write(gsreg::PRIM, kPrimSprite | kAbe);
    m.colour(0, 0, 0, 0);
    m.sprite(0, 0, 8, 8);
    CHECK_EQ(m.at(0, 0, PSMCT16), 31u | (16 << 5) | (1 << 10));
}

// --- textures and transfers --------------------------------------------------

void test_transfer_round_trip() {
    for (u32 psm : {PSMCT32, PSMCT24, PSMCT16, PSMT8, PSMT4}) {
        Machine m;
        unsigned bpp = transfer_bits(psm);
        const u32 w = 13, h = 7;  // odd sizes, so pixels straddle the chunks
        std::vector<u8> bytes((w * h * bpp + 7) / 8);
        for (std::size_t i = 0; i < bytes.size(); i++) {
            bytes[i] = static_cast<u8>(i * 37 + 11);
        }
        m.upload(320, 2, psm, w, h, bytes, 5);
        // Pixel (x, y) is the (y * w + x)-th value of the stream.
        auto stream = [&](u32 n) {
            u64 v = 0;
            std::size_t bit = std::size_t{n} * bpp;
            for (unsigned k = 0; k < (bpp + 7) / 8 + 1 && bit / 8 + k < bytes.size(); k++) {
                v |= static_cast<u64>(bytes[bit / 8 + k]) << (k * 8);
            }
            return static_cast<u32>((v >> (bit % 8)) & ((u64{1} << bpp) - 1));
        };
        bool same = true;
        for (u32 y = 0; y < h; y++) {
            for (u32 x = 0; x < w; x++) {
                same = same && m.gs.memory.read(psm, 320, 2, x, y) == stream(y * w + x);
            }
        }
        CHECK(same);

        // And back out: the same bytes, padded to whole quadwords.
        m.gs.write(gsreg::BITBLTBUF, u64{320} | (u64{2} << 16) | (u64{psm} << 24));
        m.gs.write(gsreg::TRXPOS, 0);
        m.gs.write(gsreg::TRXREG, w | (u64{h} << 32));
        m.gs.write(gsreg::TRXDIR, 1);
        std::vector<u8> back(bytes.size() + 32);
        std::size_t got = m.gs.transfer_out(back.data(), back.size());
        CHECK(got >= bytes.size() && got % 16 == 0);
        if (bpp == 4 && (w * h) % 2) {
            bytes.back() &= 0x0F;  // the unused half of the last byte
        }
        back.resize(bytes.size());
        CHECK(back == bytes);
        CHECK_EQ(m.gs.transfer_out(back.data(), back.size()), std::size_t{0});
    }
}

void test_local_copy() {
    Machine m;
    for (u32 y = 0; y < 4; y++) {
        for (u32 x = 0; x < 4; x++) {
            m.gs.memory.write(PSMCT32, 0, 1, x, y, 0x100 + y * 4 + x);
        }
    }
    m.gs.write(gsreg::BITBLTBUF, u64{0} | (u64{1} << 16) | (u64{64} << 32) | (u64{1} << 48));
    m.gs.write(gsreg::TRXPOS, u64{1} | (u64{1} << 16) | (u64{10} << 32) | (u64{20} << 48));
    m.gs.write(gsreg::TRXREG, 2 | (u64{3} << 32));
    m.gs.write(gsreg::TRXDIR, 2);
    CHECK_EQ(m.gs.memory.read(PSMCT32, 64, 1, 10, 20), 0x105u);
    CHECK_EQ(m.gs.memory.read(PSMCT32, 64, 1, 11, 22), 0x10Eu);
    CHECK_EQ(m.gs.memory.read(PSMCT32, 64, 1, 12, 20), 0u);
}

void test_texture_direct_colour() {
    Machine m;
    m.target();
    // A 4 x 4 texture of 32-bit pixels at block 512.
    std::vector<u8> pixels(16 * 4);
    for (u32 i = 0; i < 16; i++) {
        store<u32>(&pixels[i * 4], 0x80000000u | (i * 16) | ((255 - i * 16) << 8));
    }
    m.upload(512, 1, PSMCT32, 4, 4, pixels);
    m.gs.write(gsreg::TEX0_1, tex0(512, 1, PSMCT32, 2, 2, true, 1));  // decal, texture alpha
    m.gs.write(gsreg::TEX1_1, 0);
    m.gs.write(gsreg::CLAMP_1, 0);
    m.gs.write(gsreg::PRIM, kPrimSprite | kTme | kFst);
    m.colour(0x80, 0x80, 0x80, 0x80);
    // One texel per pixel.
    m.uv(0, 0);
    m.xyz(8, 8);
    m.uv(4, 4);
    m.xyz(12, 12);
    bool same = true;
    for (u32 i = 0; i < 16; i++) {
        same = same && m.at(8 + (i & 3), 8 + (i >> 2)) == load<u32>(&pixels[i * 4]);
    }
    CHECK(same);

    // Modulate: 0x80 is one, so a half-bright vertex colour halves the texel.
    m.gs.write(gsreg::TEX0_1, tex0(512, 1, PSMCT32, 2, 2, true, 0));
    m.colour(0x40, 0x80, 0xFF, 0x40);
    m.uv(1, 0);
    m.xyz(20, 8);
    m.uv(2, 1);
    m.xyz(21, 9);
    CHECK_EQ(m.at(20, 8), (u32{0x40} << 24) | (16 / 2) | (239u << 8));

    // Repeat wraps, clamp holds the edge.
    m.gs.write(gsreg::TEX0_1, tex0(512, 1, PSMCT32, 2, 2, true, 1));
    m.colour(0x80, 0x80, 0x80, 0x80);
    m.uv(4, 0);
    m.xyz(30, 8);
    m.uv(8, 1);
    m.xyz(34, 9);
    CHECK_EQ(m.at(30, 8), load<u32>(&pixels[0]));
    CHECK_EQ(m.at(33, 8), load<u32>(&pixels[3 * 4]));
    m.gs.write(gsreg::CLAMP_1, 1 | (1 << 2));
    m.uv(4, 0);
    m.xyz(30, 20);
    m.uv(8, 1);
    m.xyz(34, 21);
    CHECK_EQ(m.at(30, 20), load<u32>(&pixels[3 * 4]));
    CHECK_EQ(m.at(33, 20), load<u32>(&pixels[3 * 4]));
    m.gs.write(gsreg::CLAMP_1, 0);

    // Bilinear: halfway between two texels is their average.
    m.gs.write(gsreg::TEX1_1, (u64{1} << 5) | (u64{1} << 6));
    m.uv(1.0f, 0.5f);
    m.xyz(40, 8);
    m.uv(2.0f, 1.5f);
    m.xyz(41, 9);
    CHECK_EQ(m.at(40, 8) & 0xFF, 8u);  // texels 0 and 1 have red 0 and 16

    // Perspective: with Q halving across a triangle, the midpoint on screen is
    // a third of the way along the texture.
    m.gs.write(gsreg::TEX1_1, 0);
    m.gs.write(gsreg::TEX0_1, tex0(512, 1, PSMCT32, 2, 2, true, 1));
    m.gs.write(gsreg::CLAMP_1, 1 | (1 << 2));
    m.gs.write(gsreg::PRIM, kPrimTriangle | kTme);
    auto st = [&](float s, float t, float q) {
        m.gs.write(gsreg::ST, as_u32(s * q) | (u64{as_u32(t * q)} << 32));
        m.colour(0x80, 0x80, 0x80, 0x80, q);
    };
    st(0.0f, 0.0f, 1.0f);
    m.xyz(0, 40);
    st(1.0f, 0.0f, 0.5f);
    m.xyz(48, 40);
    st(0.0f, 1.0f, 1.0f);
    m.xyz(0, 60);
    // At x = 24 on the top edge: s/q = (0.5 * 0.5) / 0.75 = 1/3, texel 1 of 4.
    CHECK_EQ(m.at(24, 40), load<u32>(&pixels[1 * 4]));
    // At x = 12: s/q = (0.25 * 0.5) / 0.875 = 1/7, texel 0.
    CHECK_EQ(m.at(12, 40), load<u32>(&pixels[0]));
}

void test_texture_indexed() {
    Machine m;
    m.target();
    // An 8-bit texture, 16 x 16, whose texel (x, y) is index y * 16 + x.
    std::vector<u8> indices(256);
    for (u32 i = 0; i < 256; i++) {
        indices[i] = static_cast<u8>(i);
    }
    m.upload(512, 2, PSMT8, 16, 16, indices);
    // A 32-bit table whose entry i is the colour i, i, i with alpha 0x80,
    // stored as the GS stores tables: entries 8-15 and 16-23 of each 32 swapped.
    std::vector<u8> table(256 * 4);
    for (u32 i = 0; i < 256; i++) {
        u32 position = (i & 0xE7) | ((i & 0x08) << 1) | ((i & 0x10) >> 1);
        store<u32>(&table[position * 4], 0x80000000u | i | (i << 8) | (i << 16));
    }
    m.upload(600, 1, PSMCT32, 16, 16, table);
    m.gs.write(gsreg::TEX0_1, tex0(512, 2, PSMT8, 4, 4, true, 1, 600, PSMCT32, 0, 1));
    m.gs.write(gsreg::PRIM, kPrimSprite | kTme | kFst);
    m.colour(0x80, 0x80, 0x80, 0x80);
    m.uv(0, 0);
    m.xyz(0, 0);
    m.uv(16, 16);
    m.xyz(16, 16);
    bool same = true;
    for (u32 i = 0; i < 256; i++) {
        same = same && m.at(i & 15, i >> 4) == (0x80000000u | i | (i << 8) | (i << 16));
    }
    CHECK(same);

    // The table is a copy: changing memory does not reach the next primitive
    // until TEX0 is written again with a load.
    std::vector<u8> black(256 * 4, 0);
    m.upload(600, 1, PSMCT32, 16, 16, black);
    m.uv(5, 0);
    m.xyz(32, 0);
    m.uv(6, 1);
    m.xyz(33, 1);
    CHECK_EQ(m.at(32, 0), 0x80050505u);
    m.gs.write(gsreg::TEX0_1, tex0(512, 2, PSMT8, 4, 4, true, 1, 600, PSMCT32, 0, 1));
    m.uv(5, 0);
    m.xyz(32, 0);
    m.uv(6, 1);
    m.xyz(33, 1);
    CHECK_EQ(m.at(32, 0), 0u);

    // A 4-bit texture with a 16-bit table: 16 entries, eight to a row, loaded
    // at the table offset CSA selects; the alpha comes from TEXA.
    Machine n;
    n.target();
    std::vector<u8> nibbles(8);  // 4 x 4 pixels: indices 0..15
    for (u32 i = 0; i < 8; i++) {
        nibbles[i] = static_cast<u8>((i * 2) | ((i * 2 + 1) << 4));
    }
    n.upload(512, 2, PSMT4, 4, 4, nibbles);
    std::vector<u8> table16(16 * 2);
    for (u32 i = 0; i < 16; i++) {
        store<u16>(&table16[i * 2], static_cast<u16>(i | (i << 5) | ((i & 1) << 15)));
    }
    n.upload(600, 1, PSMCT16, 8, 2, table16);
    n.gs.write(gsreg::TEXA, 0x30 | (u64{0x70} << 32));
    n.gs.write(gsreg::TEX0_1, tex0(512, 2, PSMT4, 2, 2, true, 1, 600, PSMCT16, 3, 1));
    n.gs.write(gsreg::PRIM, kPrimSprite | kTme | kFst);
    n.colour(0x80, 0x80, 0x80, 0x80);
    n.uv(0, 0);
    n.xyz(0, 0);
    n.uv(4, 4);
    n.xyz(4, 4);
    CHECK_EQ(n.at(2, 0), (u32{0x30} << 24) | (2 << 3) | (2 << 11));  // entry 2: top bit clear, TA0
    CHECK_EQ(n.at(3, 1), (u32{0x70} << 24) | (7 << 3) | (7 << 11));  // entry 7: top bit set, TA1
    CHECK_EQ(n.at(1, 3), (u32{0x70} << 24) | (13 << 3) | (13 << 11));
}

void test_fog() {
    Machine m;
    m.target();
    m.gs.write(gsreg::FOGCOL, 0x000000FF);  // red fog
    m.gs.write(gsreg::PRIM, kPrimSprite | kFge);
    m.colour(0, 0xFF, 0, 0x80);
    auto fogged = [&](u32 x, u32 f) {
        u64 fx = (kOffset + x) * 16, fy = kOffset * 16;
        m.gs.write(gsreg::XYZF2, fx | (fy << 16) | (u64{f} << 56));
        m.gs.write(gsreg::XYZF2, (fx + 16) | ((fy + 16) << 16) | (u64{f} << 56));
    };
    fogged(0, 0);     // all fog
    fogged(1, 0x80);  // half
    fogged(2, 0xFF);  // (almost) none
    CHECK_EQ(m.at(0, 0) & 0xFFFFFF, 0x0000FFu);
    // colour = fog colour + (colour - fog colour) * F / 256, rounded down.
    CHECK_EQ(m.at(1, 0) & 0xFFFFFF, 0x007F7Fu);
    CHECK_EQ(m.at(2, 0) & 0xFFFFFF, 0x00FE00u);
}

void test_display() {
    Machine m;
    m.target();
    m.gs.memory.write(PSMCT32, 0, 1, 5, 3, 0x00332211);
    Image image;
    CHECK(!m.gs.display(image));
    m.gs.write_privileged(gspriv::PMODE, 1);
    m.gs.write_privileged(gspriv::DISPFB1, 0 | (u64{1} << 9));
    m.gs.write_privileged(
        gspriv::DISPLAY1, (u64{3} << 23) | (u64{64 * 4 - 1} << 32) | (u64{31} << 44)
    );
    CHECK(m.gs.display(image));
    CHECK_EQ(image.width, 64);
    CHECK_EQ(image.height, 32);
    CHECK_EQ(image.pixels[3 * 64 + 5], 0xFF332211u);
    CHECK_EQ(m.gs.read_privileged(gspriv::PMODE), u64{1});
}

// --- GIF ---------------------------------------------------------------------

std::vector<u8> quads(std::initializer_list<u64> list) {
    std::vector<u8> out(list.size() * 8);
    std::size_t i = 0;
    for (u64 q : list) {
        store<u64>(&out[i], q);
        i += 8;
    }
    return out;
}

u64 gif_tag(u32 loops, bool eop, u32 flg, u32 nreg, bool pre = false, u32 prim = 0) {
    return loops | (u64{eop} << 15) | (u64{pre} << 46) | (u64{prim} << 47) | (u64{flg} << 58)
           | (u64{nreg} << 60);
}

void test_gif_packed() {
    Machine m;
    m.target();
    const u64 x0 = (kOffset + 2) * 16, y0 = (kOffset + 3) * 16, x1 = (kOffset + 6) * 16,
              y1 = (kOffset + 5) * 16;
    std::vector<u8> packet = quads({
        // A+D: the alpha register, by address.
        gif_tag(1, false, 0, 1),
        0xE,
        0x44,
        gsreg::ALPHA_1,
        // A sprite: PRIM from the tag, then RGBAQ and two XYZ2 per loop.
        gif_tag(1, true, 0, 3, true, kPrimSprite),
        0x551,
        0x11 | (u64{0x22} << 32),
        0x33 | (u64{0x44} << 32),
        x0 | (y0 << 32),
        123,
        x1 | (y1 << 32),
        123,
    });
    CHECK(m.gif.idle(2));
    m.gif.write(2, packet.data(), 3);
    CHECK(!m.gif.idle(2));
    m.gif.write(2, packet.data() + 48, packet.size() / 16 - 3);  // the rest, in a second piece
    CHECK(m.gif.idle(2));
    CHECK_EQ(m.at(2, 3), 0x44332211u);
    CHECK_EQ(m.at(5, 4), 0x44332211u);
    CHECK_EQ(m.at(6, 4), 0u);
    CHECK_EQ(m.gs.stats.pixels, 8u);

    // The ADC bit of a packed XYZ2 turns it into a vertex without a kick, and a
    // packed XYZF2 carries Z in bits 4-27 and fog in bits 36-43 of its top half.
    Machine n;
    n.target();
    n.gs.write(gsreg::TEST_1, (u64{1} << 16) | (u64{1} << 17));
    std::vector<u8> strip = quads({
        gif_tag(4, true, 0, 1, true, kPrimStrip),
        0x5,
        (kOffset + 0) * 16 | (u64{(kOffset + 0) * 16} << 32),
        0,
        (kOffset + 8) * 16 | (u64{(kOffset + 0) * 16} << 32),
        0,
        (kOffset + 0) * 16 | (u64{(kOffset + 8) * 16} << 32),
        u64{1} << 47,
        (kOffset + 8) * 16 | (u64{(kOffset + 8) * 16} << 32),
        0,
    });
    n.gif.write(1, strip.data(), strip.size() / 16);
    CHECK_EQ(n.gs.stats.primitives, 1u);
    std::vector<u8> fogged = quads({
        gif_tag(2, true, 0, 1, true, kPrimSprite),
        0x4,
        (kOffset + 20) * 16 | (u64{(kOffset + 0) * 16} << 32),
        (u64{0xABCDEF} << 4) | (u64{0x55} << 36),
        (kOffset + 21) * 16 | (u64{(kOffset + 1) * 16} << 32),
        (u64{0xABCDEF} << 4) | (u64{0x55} << 36),
    });
    n.gif.write(1, fogged.data(), fogged.size() / 16);
    CHECK_EQ(n.gs.memory.read(PSMZ32, 8 * 32, 1, 20, 0), 0xABCDEFu);
}

void test_gif_reglist_and_image() {
    Machine m;
    m.target();
    const u64 x0 = (kOffset + 1) * 16, y0 = (kOffset + 1) * 16, x1 = (kOffset + 3) * 16,
              y1 = (kOffset + 2) * 16;
    // REGLIST: 64 bits per register, three registers, so one half is padding.
    std::vector<u8> list = quads({
        gif_tag(1, true, 1, 3),
        0x551,
        u64{0x04030201},
        x0 | (y0 << 16),
        x1 | (y1 << 16),
        0xDEADBEEF,
    });
    m.gs.write(gsreg::PRIM, kPrimSprite);
    m.gif.write(3, list.data(), list.size() / 16);
    CHECK(m.gif.idle(3));
    CHECK_EQ(m.at(1, 1), 0x04030201u);
    CHECK_EQ(m.at(2, 1), 0x04030201u);
    CHECK_EQ(m.gs.stats.pixels, 2u);

    // IMAGE: the quadwords are pixels of the transfer set up before.
    std::vector<u8> image = quads({
        gif_tag(4, false, 0, 1),
        0xE,
        (u64{64} << 32) | (u64{1} << 48),
        gsreg::BITBLTBUF,
        0,
        gsreg::TRXPOS,
        4 | (u64{2} << 32),
        gsreg::TRXREG,
        0,
        gsreg::TRXDIR,
        gif_tag(2, true, 2, 0),
        0,
        0x0000000200000001,
        0x0000000400000003,
        0x0000000600000005,
        0x0000000800000007,
    });
    m.gif.write(3, image.data(), image.size() / 16);
    CHECK(m.gif.idle(3));
    CHECK_EQ(m.gs.memory.read(PSMCT32, 64, 1, 0, 0), 1u);
    CHECK_EQ(m.gs.memory.read(PSMCT32, 64, 1, 3, 0), 4u);
    CHECK_EQ(m.gs.memory.read(PSMCT32, 64, 1, 2, 1), 7u);
}

// --- VIF1 --------------------------------------------------------------------

u32 vif(u32 cmd, u32 num, u32 imm) {
    return (cmd << 24) | (num << 16) | imm;
}

void test_vif_unpack() {
    Machine m;
    Vif1& v = m.vif;
    auto word = [&](u32 address, u32 field) {
        return load<u32>(&v.data[address * 16 + field * 4]);
    };
    auto send = [&](const std::vector<u8>& bytes) {
        v.write(bytes.data(), bytes.size());
    };

    // V4-32: four words a vector.
    send(words({vif(0x01, 0, 0x0101), vif(0x6C, 2, 10), 1, 2, 3, 4, 5, 6, 7, 8}));
    CHECK_EQ(word(10, 0), 1u);
    CHECK_EQ(word(10, 3), 4u);
    CHECK_EQ(word(11, 2), 7u);

    // V3-16 signed: sign extended; W takes what follows in the list.
    std::vector<u8> v316 = words({vif(0x69, 2, 20), 0, 0, 0});
    const s16 values[6] = {-1, 2, -3, 4, 5, -6};
    std::memcpy(&v316[4], values, sizeof(values));
    send(v316);
    CHECK_EQ(word(20, 0), 0xFFFFFFFFu);
    CHECK_EQ(word(20, 1), 2u);
    CHECK_EQ(word(20, 2), 0xFFFFFFFDu);
    CHECK_EQ(word(20, 3), 4u);
    CHECK_EQ(word(21, 2), 0xFFFFFFFAu);

    // S-8 unsigned: one byte to all four fields.
    send(words({vif(0x62, 3, 30 | 0x4000), 0x00FF8001}));
    CHECK_EQ(word(30, 0), 1u);
    CHECK_EQ(word(30, 3), 1u);
    CHECK_EQ(word(31, 1), 0x80u);
    CHECK_EQ(word(32, 2), 0xFFu);

    // V2-16: the two missing fields repeat the two present.
    send(words({vif(0x65, 1, 40 | 0x4000), 0x00070005}));
    CHECK_EQ(word(40, 0), 5u);
    CHECK_EQ(word(40, 1), 7u);
    CHECK_EQ(word(40, 2), 5u);
    CHECK_EQ(word(40, 3), 7u);

    // V4-5: a 16-bit colour to four fields.
    send(words({vif(0x6F, 1, 50), 0x8000u | 3 | (5 << 5) | (7 << 10)}));
    CHECK_EQ(word(50, 0), 3u << 3);
    CHECK_EQ(word(50, 1), 5u << 3);
    CHECK_EQ(word(50, 2), 7u << 3);
    CHECK_EQ(word(50, 3), 0x80u);

    // The address flag adds TOPS, which BASE and OFFSET set up.
    send(words({vif(0x03, 0, 100), vif(0x02, 0, 200), vif(0x6C, 1, 5 | 0x8000), 9, 9, 9, 9}));
    CHECK_EQ(v.tops, 100u);
    CHECK_EQ(word(105, 0), 9u);

    // A skipping write: CL 3, WL 1 writes every third address.
    send(words({vif(0x01, 0, 0x0103), vif(0x60, 3, 300), 0xA, 0xB, 0xC, vif(0x01, 0, 0x0101)}));
    CHECK_EQ(word(300, 0), 0xAu);
    CHECK_EQ(word(303, 0), 0xBu);
    CHECK_EQ(word(306, 0), 0xCu);
    CHECK_EQ(word(301, 0), 0u);

    // A filling write: CL 1, WL 2 takes every other vector from the mask's
    // sources. The mask here: first line all data, second line row, row, column, protected.
    v.data.fill(0);
    store<u32>(&v.data[401 * 16 + 12], 0x77);
    send(words({
        vif(0x30, 0, 0),
        0x1000,
        0x2000,
        0x3000,
        0x4000,
        vif(0x31, 0, 0),
        0xC0,
        0xC1,
        0xC2,
        0xC3,
        vif(0x20, 0, 0),
        (1u << 8) | (1u << 10) | (2u << 12) | (3u << 14),
        vif(0x01, 0, 0x0201),
        vif(0x7C, 4, 400),
        1,
        2,
        3,
        4,
        5,
        6,
        7,
        8,
        vif(0x01, 0, 0x0101),
    }));
    CHECK_EQ(word(400, 0), 1u);
    CHECK_EQ(word(400, 3), 4u);
    CHECK_EQ(word(401, 0), 0x1000u);
    CHECK_EQ(word(401, 1), 0x2000u);
    CHECK_EQ(word(401, 2), 0xC1u);
    CHECK_EQ(word(401, 3), 0x77u);
    CHECK_EQ(word(402, 1), 6u);
    CHECK_EQ(word(403, 0), 0x1000u);

    // Offset mode adds the row; difference mode also keeps the sum.
    send(words({vif(0x05, 0, 1), vif(0x6C, 1, 500), 1, 1, 1, 1}));
    CHECK_EQ(word(500, 0), 0x1001u);
    CHECK_EQ(word(500, 3), 0x4001u);
    send(words({vif(0x05, 0, 2), vif(0x6C, 2, 501), 1, 1, 1, 1, 1, 1, 1, 1, vif(0x05, 0, 0)}));
    CHECK_EQ(word(501, 0), 0x1001u);
    CHECK_EQ(word(502, 0), 0x1002u);
    CHECK_EQ(v.row[0], 0x1002u);
}

void test_vif_programs_and_direct() {
    Machine m;
    m.target();
    Vif1& v = m.vif;
    std::vector<u32> starts;
    v.on_start = [&](u32 address, bool resume) {
        starts.push_back(resume ? 0xFFFFFFFFu : address);
    };

    // MPG: two instructions (four words) loaded at instruction 4.
    std::vector<u8> program = words({vif(0x4A, 2, 4), 0x11, 0x22, 0x33, 0x44});
    // Sent a byte short of a word boundary at a time: nothing happens until
    // the whole command is there.
    v.write(program.data(), 8);
    CHECK_EQ(load<u32>(&v.micro[32]), 0u);
    v.write(program.data() + 8, program.size() - 8);
    CHECK_EQ(load<u32>(&v.micro[32]), 0x11u);
    CHECK_EQ(load<u32>(&v.micro[44]), 0x44u);

    // MSCAL starts the program and swaps the buffer pair; MSCNT continues.
    std::vector<u8> run = words(
        {vif(0x03, 0, 8),
         vif(0x02, 0, 100),
         vif(0x04, 0, 7),
         vif(0x14, 0, 6),
         vif(0x17, 0, 0),
         vif(0x14, 0, 9)}
    );
    v.write(run.data(), run.size());
    CHECK_EQ(starts.size(), std::size_t{3});
    CHECK_EQ(starts[0], 6u);
    CHECK_EQ(starts[1], 0xFFFFFFFFu);
    CHECK_EQ(starts[2], 9u);
    CHECK_EQ(v.itop, 7u);
    // Three starts from TOPS = 8: 8 -> 108 -> 8 -> 108.
    CHECK_EQ(v.tops, 108u);
    CHECK_EQ(v.top, 8u);

    // DIRECT: the quadwords go to the GIF as path 2.
    const u64 x0 = (kOffset + 2) * 16, y0 = (kOffset + 2) * 16, x1 = (kOffset + 4) * 16,
              y1 = (kOffset + 4) * 16;
    std::vector<u8> packet = words({0, 0, 0, vif(0x50, 0, 4)});
    std::vector<u8> gif = quads({
        gif_tag(1, true, 0, 3, true, kPrimSprite),
        0x551,
        0x10,
        0,
        x0 | (y0 << 32),
        0,
        x1 | (y1 << 32),
        0,
    });
    packet.insert(packet.end(), gif.begin(), gif.end());
    v.write(packet.data(), packet.size());
    CHECK_EQ(m.at(2, 2), 0x10u);
    CHECK_EQ(m.gs.stats.pixels, 4u);
    CHECK_EQ(v.unknown_codes, u64{0});
}

// --- DMA ---------------------------------------------------------------------

void test_dma_chain() {
    GuestMemory memory;

    struct Piece {
        u32 first_word;
        std::size_t bytes;
    };

    std::vector<Piece> pieces;
    DmaSink sink = [&](const u8* data, std::size_t bytes) {
        pieces.push_back({load<u32>(data), bytes});
    };
    auto tag = [&](u32 at, u32 id, u32 qwc, u32 address, bool irq = false, u32 code = 0) {
        memory.write<u32>(at, qwc | (code << 16) | (id << 28) | (u32{irq} << 31));
        memory.write<u32>(at + 4, address);
        memory.write<u32>(at + 8, 0x7A600000 | at);  // what tag transfer sends
        memory.write<u32>(at + 12, 0);
    };
    auto data = [&](u32 at, u32 value) {
        memory.write<u32>(at, value);
    };

    // cnt(1) -> ref(2 at 0x8000) -> call 0x2000 { cnt(1), ret } -> next 0x3000 -> cnt with IRQ -> end(1)
    tag(0x1000, dmatag::CNT, 1, 0);
    data(0x1010, 0xC0);
    tag(0x1020, dmatag::REF, 2, 0x8000);
    data(0x8000, 0xC1);
    tag(0x1030, dmatag::CALL, 0, 0x2000);
    tag(0x2000, dmatag::CNT, 1, 0);
    data(0x2010, 0xC2);
    tag(0x2020, dmatag::RET, 0, 0);
    tag(0x1040, dmatag::NEXT, 0, 0x3000);
    tag(0x3000, dmatag::CNT, 1, 0, true, 0x201);
    data(0x3010, 0xC3);
    tag(0x3020, dmatag::END, 1, 0);
    data(0x3030, 0xC4);

    DmaChannel ch;
    ch.tadr = 0x1000;
    ch.chcr = 0x185;  // tag interrupts on, tag transfer off
    CHECK(run_source_chain(memory, ch, sink) == DmaStop::Interrupt);
    CHECK_EQ(pieces.size(), std::size_t{4});
    CHECK_EQ(pieces[0].first_word, 0xC0u);
    CHECK_EQ(pieces[1].first_word, 0xC1u);
    CHECK_EQ(pieces[1].bytes, std::size_t{32});
    CHECK_EQ(pieces[2].first_word, 0xC2u);
    CHECK_EQ(pieces[3].first_word, 0xC3u);
    // Stopped after the IRQ tag's data: STR off, the tag's top half in CHCR,
    // TADR at the tag that would come next.
    CHECK_EQ(ch.chcr & 0x100, 0u);
    CHECK_EQ(ch.chcr >> 16, 0x9201u);
    CHECK_EQ(ch.tadr, 0x3020u);
    // Resumed, it runs to the end.
    ch.chcr |= 0x100;
    CHECK(run_source_chain(memory, ch, sink) == DmaStop::End);
    CHECK_EQ(pieces.size(), std::size_t{5});
    CHECK_EQ(pieces[4].first_word, 0xC4u);
    CHECK_EQ(ch.chcr >> 28, 7u);

    // With tag transfer on, the top half of every tag goes out before its data.
    pieces.clear();
    ch = DmaChannel{};
    ch.tadr = 0x1000;
    ch.chcr = 0x145;  // tag transfer on, tag interrupts off
    CHECK(run_source_chain(memory, ch, sink) == DmaStop::End);
    CHECK_EQ(pieces.size(), std::size_t{5 + 8});
    CHECK_EQ(pieces[0].first_word, 0x7A601000u);
    CHECK_EQ(pieces[0].bytes, std::size_t{8});
    CHECK_EQ(pieces[1].first_word, 0xC0u);

    // The scratchpad is reached through bit 31 of an address.
    store<u32>(memory.scratchpad(0x100), 0x5C);
    tag(0x4000, dmatag::REFE, 1, 0x80000100);
    pieces.clear();
    ch = DmaChannel{};
    ch.tadr = 0x4000;
    ch.chcr = 0x105;
    CHECK(run_source_chain(memory, ch, sink) == DmaStop::End);
    CHECK_EQ(pieces.size(), std::size_t{1});
    CHECK_EQ(pieces[0].first_word, 0x5Cu);

    // A list that never ends is reported, not followed for ever.
    tag(0x5000, dmatag::NEXT, 0, 0x5000);
    ch = DmaChannel{};
    ch.tadr = 0x5000;
    ch.chcr = 0x105;
    CHECK(run_source_chain(memory, ch, sink) == DmaStop::Runaway);
}

void test_whole_path() {
    // A list in guest memory, the way a game's frame arrives: one CNT tag whose
    // spare words are NOP and DIRECT, a GIF packet behind it, then END.
    GuestMemory memory;
    Machine m;
    m.target();
    const u64 x0 = (kOffset + 1) * 16, y0 = (kOffset + 1) * 16, x1 = (kOffset + 5) * 16,
              y1 = (kOffset + 3) * 16;
    const u64 list[] = {
        4 | (u64{dmatag::CNT} << 28),
        u64{vif(0x50, 0, 4)} << 32,
        gif_tag(1, true, 0, 3, true, kPrimSprite),
        0x551,
        0x7F,
        0,
        x0 | (y0 << 32),
        0,
        x1 | (y1 << 32),
        0,
        u64{dmatag::END} << 28,
        0,
    };
    std::memcpy(memory.ram(0x100000), list, sizeof(list));
    DmaChannel ch;
    ch.tadr = 0x100000;
    ch.chcr = 0x1C5;
    DmaStop stop =
        run_source_chain(memory, ch, [&](const u8* d, std::size_t n) { m.vif.write(d, n); });
    CHECK(stop == DmaStop::End);
    CHECK_EQ(m.gs.stats.pixels, 8u);
    CHECK_EQ(m.at(4, 2), 0x7Fu);
    CHECK_EQ(m.vif.unknown_codes, u64{0});
}

}  // namespace

int main() {
    const TestCase tests[] = {
        {"memory layout", test_memory_layout},
        {"memory is one to one", test_memory_is_one_to_one},
        {"memory tables", test_memory_tables},
        {"memory formats", test_memory_formats},
        {"sprite coverage", test_sprite_coverage},
        {"triangles share edges", test_triangles_share_edges},
        {"strip and fan", test_strip_and_fan},
        {"gouraud", test_gouraud},
        {"depth", test_depth},
        {"blend and masks", test_blend_and_masks},
        {"alpha test", test_alpha_test},
        {"16-bit target", test_16_bit_target},
        {"transfer round trip", test_transfer_round_trip},
        {"local copy", test_local_copy},
        {"texture, direct colour", test_texture_direct_colour},
        {"texture, indexed", test_texture_indexed},
        {"fog", test_fog},
        {"display", test_display},
        {"gif packed", test_gif_packed},
        {"gif reglist and image", test_gif_reglist_and_image},
        {"vif unpack", test_vif_unpack},
        {"vif programs and direct", test_vif_programs_and_direct},
        {"dma chain", test_dma_chain},
        {"whole path", test_whole_path},
    };
    return run_tests(tests);
}
