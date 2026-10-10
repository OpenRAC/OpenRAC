// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The console graphics chip's drawing registers and the GIF tag, as fields.
// The layouts are the chip's public documentation (the GS register map, the
// GIF tag), as every PS2 emulator and homebrew SDK also describes them; no
// Sony SDK headers were used.
//
// This is a vocabulary for reading the packets the game builds, not a model
// of the chip: the direct renderer (direct.h) turns these register writes
// into GPU state and draw calls, and the texture pool (texture_pool.h) treats
// a texture's base pointer as an identifier, never as an address in a 4 MB
// memory.

#pragma once

#include <cstdint>

namespace openrac::renderer::gs {

using u64 = std::uint64_t;

constexpr std::uint32_t bits(u64 value, int first, int count) {
    return static_cast<std::uint32_t>((value >> first) & ((u64{1} << count) - 1));
}

// ---- Register addresses (A+D and REGLIST) ----
enum Reg : std::uint8_t {
    kPrim = 0x00,
    kRgbaq = 0x01,
    kSt = 0x02,
    kUv = 0x03,
    kXyzf2 = 0x04,
    kXyz2 = 0x05,
    kTex0_1 = 0x06,
    kTex0_2 = 0x07,
    kClamp1 = 0x08,
    kClamp2 = 0x09,
    kFog = 0x0A,
    kXyzf3 = 0x0C,
    kXyz3 = 0x0D,
    kTex1_1 = 0x14,
    kTex1_2 = 0x15,
    kTex2_1 = 0x16,
    kTex2_2 = 0x17,
    kXyoffset1 = 0x18,
    kXyoffset2 = 0x19,
    kPrmodecont = 0x1A,
    kPrmode = 0x1B,
    kTexclut = 0x1C,
    kScanmsk = 0x22,
    kMiptbp1_1 = 0x34,
    kMiptbp1_2 = 0x35,
    kMiptbp2_1 = 0x36,
    kMiptbp2_2 = 0x37,
    kTexa = 0x3B,
    kFogcol = 0x3D,
    kTexflush = 0x3F,
    kScissor1 = 0x40,
    kScissor2 = 0x41,
    kAlpha1 = 0x42,
    kAlpha2 = 0x43,
    kDimx = 0x44,
    kDthe = 0x45,
    kColclamp = 0x46,
    kTest1 = 0x47,
    kTest2 = 0x48,
    kPabe = 0x49,
    kFba1 = 0x4A,
    kFba2 = 0x4B,
    kFrame1 = 0x4C,
    kFrame2 = 0x4D,
    kZbuf1 = 0x4E,
    kZbuf2 = 0x4F,
    kBitbltbuf = 0x50,
    kTrxpos = 0x51,
    kTrxreg = 0x52,
    kTrxdir = 0x53,
    kHwreg = 0x54,
    kSignal = 0x60,
    kFinish = 0x61,
    kLabel = 0x62,
};

// ---- Pixel storage formats (TEX0.PSM, BITBLTBUF.DPSM, FRAME.PSM) ----
enum Psm : std::uint8_t {
    kPsmct32 = 0x00,
    kPsmct24 = 0x01,
    kPsmct16 = 0x02,
    kPsmct16s = 0x0A,
    kPsmt8 = 0x13,
    kPsmt4 = 0x14,
    kPsmt8h = 0x1B,   // 8-bit index in bits 24-31 of a 32-bit pixel
    kPsmt4hl = 0x24,  // 4-bit index in bits 24-27
    kPsmt4hh = 0x2C,  // 4-bit index in bits 28-31
    kPsmz32 = 0x30,
    kPsmz24 = 0x31,
    kPsmz16 = 0x32,
    kPsmz16s = 0x3A,
};

// Bits per pixel of a format, 0 if unknown.
constexpr int bits_per_pixel(std::uint8_t psm) {
    switch (psm) {
        case kPsmct32:
        case kPsmct24:
        case kPsmt8h:
        case kPsmt4hl:
        case kPsmt4hh:
        case kPsmz32:
        case kPsmz24:
            return 32;
        case kPsmct16:
        case kPsmct16s:
        case kPsmz16:
        case kPsmz16s:
            return 16;
        case kPsmt8:
            return 8;
        case kPsmt4:
            return 4;
        default:
            return 0;
    }
}

// ---- The GIF tag: 128 bits before each block of data ----
struct GifTag {
    enum Format : std::uint8_t {
        kPacked = 0,
        kReglist = 1,
        kImage = 2,
        kDisable = 3
    };

    std::uint32_t nloop;  // how many times the register list repeats
    bool eop;             // the last tag of the packet
    bool pre;             // write `prim` to PRIM first (PACKED only)
    std::uint32_t prim;
    Format flg;
    std::uint32_t nreg;  // 1..16
    u64 regs;            // nreg 4-bit register descriptors, first in the low bits

    static GifTag decode(u64 low, u64 high) {
        GifTag t{};
        t.nloop = bits(low, 0, 15);
        t.eop = bits(low, 15, 1) != 0;
        t.pre = bits(low, 46, 1) != 0;
        t.prim = bits(low, 47, 11);
        t.flg = static_cast<Format>(bits(low, 58, 2));
        const std::uint32_t n = bits(low, 60, 4);
        t.nreg = n == 0 ? 16 : n;
        t.regs = high;
        return t;
    }

    std::uint32_t reg(std::uint32_t i) const { return bits(regs, static_cast<int>(i * 4), 4); }
};

// PACKED register descriptors (the 4-bit codes of GifTag::regs).
enum PackedReg : std::uint8_t {
    kPackedPrim = 0x0,
    kPackedRgbaq = 0x1,
    kPackedSt = 0x2,
    kPackedUv = 0x3,
    kPackedXyzf2 = 0x4,
    kPackedXyz2 = 0x5,
    kPackedTex0_1 = 0x6,
    kPackedTex0_2 = 0x7,
    kPackedClamp1 = 0x8,
    kPackedClamp2 = 0x9,
    kPackedFog = 0xA,
    kPackedXyzf3 = 0xC,
    kPackedXyz3 = 0xD,
    kPackedAd = 0xE,
    kPackedNop = 0xF,
};

// ---- Drawing registers ----

enum class PrimKind : std::uint8_t {
    Point = 0,
    Line = 1,
    LineStrip = 2,
    Triangle = 3,
    TriangleStrip = 4,
    TriangleFan = 5,
    Sprite = 6,
    Invalid = 7,
};

struct Prim {
    PrimKind kind;
    bool iip;   // Gouraud shading
    bool tme;   // textured
    bool fge;   // fogged
    bool abe;   // alpha blended
    bool aa1;   // antialiased (not drawn differently here)
    bool fst;   // UV (texel) coordinates instead of STQ
    bool ctxt;  // use the second register context
    bool fix;   // fixed fragment value control

    static Prim decode(u64 v) {
        return {
            static_cast<PrimKind>(bits(v, 0, 3)),
            bits(v, 3, 1) != 0,
            bits(v, 4, 1) != 0,
            bits(v, 5, 1) != 0,
            bits(v, 6, 1) != 0,
            bits(v, 7, 1) != 0,
            bits(v, 8, 1) != 0,
            bits(v, 9, 1) != 0,
            bits(v, 10, 1) != 0
        };
    }

    bool operator==(const Prim&) const = default;
};

// Texture function (TEX0.TFX).
enum class Tfx : std::uint8_t {
    Modulate = 0,
    Decal = 1,
    Highlight = 2,
    Highlight2 = 3
};

struct Tex0 {
    std::uint32_t tbp0;  // base, in 256-byte blocks
    std::uint32_t tbw;   // buffer width, in 64-pixel units
    std::uint8_t psm;
    std::uint32_t tw;  // log2 width
    std::uint32_t th;  // log2 height
    bool tcc;          // take alpha from the texture
    Tfx tfx;
    std::uint32_t cbp;  // CLUT base, in blocks
    std::uint8_t cpsm;  // CLUT format: PSMCT32 or PSMCT16(S)
    bool csm;           // CLUT storage: false CSM1, true CSM2
    std::uint32_t csa;  // CLUT entry offset, in 16-entry units
    std::uint32_t cld;  // CLUT buffer load control

    static Tex0 decode(u64 v) {
        return {
            bits(v, 0, 14),
            bits(v, 14, 6),
            static_cast<std::uint8_t>(bits(v, 20, 6)),
            bits(v, 26, 4),
            bits(v, 30, 4),
            bits(v, 34, 1) != 0,
            static_cast<Tfx>(bits(v, 35, 2)),
            bits(v, 37, 14),
            static_cast<std::uint8_t>(bits(v, 51, 4)),
            bits(v, 55, 1) != 0,
            bits(v, 56, 5),
            bits(v, 61, 3)
        };
    }

    int width() const { return 1 << tw; }

    int height() const { return 1 << th; }
};

struct Tex1 {
    bool lcm;
    std::uint32_t mxl;
    bool mmag;  // linear magnification
    std::uint32_t mmin;

    static Tex1 decode(u64 v) {
        return {bits(v, 0, 1) != 0, bits(v, 2, 3), bits(v, 5, 1) != 0, bits(v, 6, 3)};
    }
};

// Wrap modes (CLAMP.WMS/WMT).
enum class Wrap : std::uint8_t {
    Repeat = 0,
    Clamp = 1,
    RegionClamp = 2,
    RegionRepeat = 3
};

struct Clamp {
    Wrap wms;
    Wrap wmt;

    static Clamp decode(u64 v) {
        return {static_cast<Wrap>(bits(v, 0, 2)), static_cast<Wrap>(bits(v, 2, 2))};
    }
};

// Blend: Cv = (A - B) * C / 128 + D, where A, B and D pick Cs (the source),
// Cd (the frame buffer) or 0, and C picks As, Ad or FIX.
struct Alpha {
    std::uint8_t a, b, c, d;
    std::uint8_t fix;

    static Alpha decode(u64 v) {
        return {
            static_cast<std::uint8_t>(bits(v, 0, 2)),
            static_cast<std::uint8_t>(bits(v, 2, 2)),
            static_cast<std::uint8_t>(bits(v, 4, 2)),
            static_cast<std::uint8_t>(bits(v, 6, 2)),
            static_cast<std::uint8_t>(bits(v, 32, 8))
        };
    }

    bool operator==(const Alpha&) const = default;
};

enum class AlphaTest : std::uint8_t {
    Never = 0,
    Always = 1,
    Less = 2,
    Lequal = 3,
    Equal = 4,
    Gequal = 5,
    Greater = 6,
    Notequal = 7,
};

// What a pixel that fails the alpha test still writes.
enum class AlphaFail : std::uint8_t {
    Keep = 0,
    FbOnly = 1,
    ZbOnly = 2,
    RgbOnly = 3
};

enum class DepthTest : std::uint8_t {
    Never = 0,
    Always = 1,
    Gequal = 2,
    Greater = 3
};

struct Test {
    bool ate;
    AlphaTest atst;
    std::uint8_t aref;
    AlphaFail afail;
    bool date;
    bool datm;
    bool zte;
    DepthTest ztst;

    static Test decode(u64 v) {
        return {
            bits(v, 0, 1) != 0,
            static_cast<AlphaTest>(bits(v, 1, 3)),
            static_cast<std::uint8_t>(bits(v, 4, 8)),
            static_cast<AlphaFail>(bits(v, 12, 2)),
            bits(v, 14, 1) != 0,
            bits(v, 15, 1) != 0,
            bits(v, 16, 1) != 0,
            static_cast<DepthTest>(bits(v, 17, 2))
        };
    }

    bool operator==(const Test&) const = default;
};

struct Zbuf {
    std::uint32_t zbp;
    std::uint8_t psm;  // 0 Z32, 1 Z24, 2 Z16, 0xA Z16S (low 4 bits of PSMZ*)
    bool zmsk;         // no depth writes

    static Zbuf decode(u64 v) {
        return {bits(v, 0, 9), static_cast<std::uint8_t>(bits(v, 24, 4)), bits(v, 32, 1) != 0};
    }

    // The largest depth value the format holds.
    double max_z() const {
        switch (psm & 0xF) {
            case 0:
                return 4294967295.0;
            case 1:
                return 16777215.0;
            default:
                return 65535.0;
        }
    }
};

struct Frame {
    std::uint32_t fbp;
    std::uint32_t fbw;
    std::uint8_t psm;
    std::uint32_t fbmsk;  // bits set are not written

    static Frame decode(u64 v) {
        return {
            bits(v, 0, 9),
            bits(v, 16, 6),
            static_cast<std::uint8_t>(bits(v, 24, 6)),
            bits(v, 32, 32)
        };
    }
};

struct Scissor {
    std::uint32_t x0, x1, y0, y1;  // inclusive, in frame buffer pixels

    static Scissor decode(u64 v) {
        return {bits(v, 0, 11), bits(v, 16, 11), bits(v, 32, 11), bits(v, 48, 11)};
    }

    bool operator==(const Scissor&) const = default;
};

struct Xyoffset {
    std::uint32_t ofx;  // 12.4 fixed point
    std::uint32_t ofy;

    static Xyoffset decode(u64 v) { return {bits(v, 0, 16), bits(v, 32, 16)}; }
};

struct Texa {
    std::uint8_t ta0;  // alpha of a 24-bit texel, and of a 16-bit one with A = 0
    bool aem;          // a black texel (RGB 0) is transparent
    std::uint8_t ta1;  // alpha of a 16-bit texel with A = 1

    static Texa decode(u64 v) {
        return {
            static_cast<std::uint8_t>(bits(v, 0, 8)),
            bits(v, 15, 1) != 0,
            static_cast<std::uint8_t>(bits(v, 32, 8))
        };
    }

    bool operator==(const Texa&) const = default;
};

struct Bitbltbuf {
    std::uint32_t sbp, sbw;
    std::uint8_t spsm;
    std::uint32_t dbp, dbw;
    std::uint8_t dpsm;

    static Bitbltbuf decode(u64 v) {
        return {
            bits(v, 0, 14),
            bits(v, 16, 6),
            static_cast<std::uint8_t>(bits(v, 24, 6)),
            bits(v, 32, 14),
            bits(v, 48, 6),
            static_cast<std::uint8_t>(bits(v, 56, 6))
        };
    }
};

struct Trxpos {
    std::uint32_t ssax, ssay, dsax, dsay, dir;

    static Trxpos decode(u64 v) {
        return {bits(v, 0, 11), bits(v, 16, 11), bits(v, 32, 11), bits(v, 48, 11), bits(v, 59, 2)};
    }
};

struct Trxreg {
    std::uint32_t rrw, rrh;

    static Trxreg decode(u64 v) { return {bits(v, 0, 12), bits(v, 32, 12)}; }
};

}  // namespace openrac::renderer::gs
