// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

#include <array>
#include <vector>

#include "types.h"

namespace ps2 {

// Pixel storage formats (the PSM field of FRAME, ZBUF, TEX0 and BITBLTBUF).
enum : u32 {
    PSMCT32 = 0x00,
    PSMCT24 = 0x01,
    PSMCT16 = 0x02,
    PSMCT16S = 0x0A,
    PSMT8 = 0x13,
    PSMT4 = 0x14,
    PSMT8H = 0x1B,
    PSMT4HL = 0x24,
    PSMT4HH = 0x2C,
    PSMZ32 = 0x30,
    PSMZ24 = 0x31,
    PSMZ16 = 0x32,
    PSMZ16S = 0x3A,
};

// How many bits one pixel of a format takes in a transfer.
unsigned transfer_bits(u32 psm);

// GS local memory: 4 MB, addressed by the games in blocks of 256 bytes. A
// buffer is a block pointer `bp` and a width `bw` in units of 64 pixels.
// Where the pixel (x, y) of a buffer lies depends on the format: a page of
// 8,192 bytes holds 32 blocks in a format-specific order, and a block holds
// four columns whose pixels are interleaved.
class GsMemory {
public:
    static constexpr u32 kBytes = 4 * 1024 * 1024;
    static constexpr u32 kBlocks = kBytes / 256;

    GsMemory();

    // The stored value of a pixel, as wide as the format (32, 24, 16, 8 or 4
    // bits). PSMT8H, PSMT4HL and PSMT4HH read their bits out of a 32-bit word.
    u32 read(u32 psm, u32 bp, u32 bw, u32 x, u32 y) const;
    void write(u32 psm, u32 bp, u32 bw, u32 x, u32 y, u32 value);

    // Index of the pixel's storage unit: a 32-bit word, a 16-bit half, a byte
    // or a nibble of local memory. Exposed for tests.
    static u32 address32(u32 bp, u32 bw, u32 x, u32 y, bool z = false);
    static u32 address16(u32 bp, u32 bw, u32 x, u32 y, bool z = false);
    static u32 address16s(u32 bp, u32 bw, u32 x, u32 y, bool z = false);
    static u32 address8(u32 bp, u32 bw, u32 x, u32 y);
    static u32 address4(u32 bp, u32 bw, u32 x, u32 y);

    // The same addresses as tables, for inner loops. A pixel's block is
    // `bp + rowbase[y] * (bw >> bw_shift) + sy[y] + px[x]` and its place in the
    // block is `cx[x] ^ cy[y]`; a Row holds the parts that depend on y.
    struct Layout {
        std::array<u16, 2048> px{}, cx{}, sy{}, cy{}, rowbase{};
        unsigned bw_shift = 0;
        u32 unit = 64;  // storage units in a block: 64 words, 128 halves, 256 bytes, 512 nibbles
    };

    struct Row {
        u32 block = 0, in_block = 0;
        const Layout* layout = nullptr;
    };

    static const Layout& layout(u32 psm);

    static Row row(const Layout& l, u32 bp, u32 bw, u32 y) {
        y &= 2047;
        return {bp + l.rowbase[y] * (bw >> l.bw_shift) + l.sy[y], l.cy[y], &l};
    }

    static u32 index(const Row& r, u32 x) {
        x &= 2047;
        return ((r.block + r.layout->px[x]) & (kBlocks - 1)) * r.layout->unit
               + (r.layout->cx[x] ^ r.in_block);
    }

    static u32 index(const Layout& l, u32 bp, u32 bw, u32 x, u32 y) {
        return index(row(l, bp, bw, y), x);
    }

    // Storage units by index, as `index` gives them.
    u32 word(u32 i) const { return load<u32>(&bytes_[(i & (kBytes / 4 - 1)) * 4]); }

    void set_word(u32 i, u32 v) { store<u32>(&bytes_[(i & (kBytes / 4 - 1)) * 4], v); }

    u16 half(u32 i) const { return load<u16>(&bytes_[(i & (kBytes / 2 - 1)) * 2]); }

    void set_half(u32 i, u16 v) { store<u16>(&bytes_[(i & (kBytes / 2 - 1)) * 2], v); }

    u8 byte(u32 i) const { return bytes_[i & (kBytes - 1)]; }

    u8 nibble(u32 i) const { return (bytes_[(i >> 1) & (kBytes - 1)] >> ((i & 1) * 4)) & 0xF; }

    u8* data() { return bytes_.data(); }

    const u8* data() const { return bytes_.data(); }

private:
    std::vector<u8> bytes_;
};

}  // namespace ps2
