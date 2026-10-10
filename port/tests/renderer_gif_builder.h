// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Builds GIF packets for the renderer tests, the way the game's 2D code lays
// them out: a tag, then A+D pairs or packed vertices. Synthetic data only.

#pragma once

#include <bit>
#include <cstdint>
#include <cstring>
#include <vector>

#include "renderer/gs.h"

namespace openrac::test {

namespace gs = openrac::renderer::gs;

class GifBuilder {
public:
    std::vector<std::uint8_t> bytes;

    void qword(std::uint64_t lo, std::uint64_t hi) {
        const std::size_t at = bytes.size();
        bytes.resize(at + 16);
        std::memcpy(bytes.data() + at, &lo, 8);
        std::memcpy(bytes.data() + at + 8, &hi, 8);
    }

    // A GIF tag. `regs` holds the descriptors, first in the low nibble.
    void tag(
        std::uint32_t nloop,
        bool eop,
        std::uint32_t flg,
        std::uint32_t nreg,
        std::uint64_t regs,
        bool pre = false,
        std::uint32_t prim = 0
    ) {
        std::uint64_t lo = nloop & 0x7FFFu;
        lo |= std::uint64_t{eop} << 15;
        lo |= std::uint64_t{pre} << 46;
        lo |= std::uint64_t{prim & 0x7FFu} << 47;
        lo |= std::uint64_t{flg & 3u} << 58;
        lo |= std::uint64_t{nreg & 0xFu} << 60;
        qword(lo, regs);
    }

    // A+D writes: one PACKED tag with the A+D descriptor, then the pairs.
    struct Write {
        std::uint8_t reg;
        std::uint64_t value;
    };

    void ad(std::initializer_list<Write> writes, bool eop = false) {
        tag(static_cast<std::uint32_t>(writes.size()), eop, gs::GifTag::kPacked, 1, gs::kPackedAd);
        for (const Write& w : writes) {
            qword(w.value, w.reg);
        }
    }

    // A whole IMAGE transfer: BITBLTBUF, TRXPOS, TRXREG, TRXDIR, then the data.
    void image(
        std::uint32_t dbp,
        std::uint32_t dbw,
        std::uint8_t dpsm,
        std::uint32_t w,
        std::uint32_t h,
        const std::vector<std::uint8_t>& data
    ) {
        ad({{gs::kBitbltbuf,
             (std::uint64_t{dbp} << 32) | (std::uint64_t{dbw} << 48) | (std::uint64_t{dpsm} << 56)},
            {gs::kTrxpos, 0},
            {gs::kTrxreg, std::uint64_t{w} | (std::uint64_t{h} << 32)},
            {gs::kTrxdir, 0}});
        std::vector<std::uint8_t> padded = data;
        padded.resize((data.size() + 15) / 16 * 16);
        tag(static_cast<std::uint32_t>(padded.size() / 16), false, gs::GifTag::kImage, 0, 0);
        bytes.insert(bytes.end(), padded.begin(), padded.end());
    }
};

// Register values, as the game composes them.
inline std::uint64_t prim(
    gs::PrimKind kind, bool iip, bool tme, bool abe, bool fst = false, bool ctxt = false
) {
    return static_cast<std::uint64_t>(kind) | (std::uint64_t{iip} << 3) | (std::uint64_t{tme} << 4)
           | (std::uint64_t{abe} << 6) | (std::uint64_t{fst} << 8) | (std::uint64_t{ctxt} << 9);
}

inline std::uint64_t rgbaq(
    std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a, float q = 1.0f
) {
    return std::uint64_t{r} | (std::uint64_t{g} << 8) | (std::uint64_t{b} << 16)
           | (std::uint64_t{a} << 24) | (std::uint64_t{std::bit_cast<std::uint32_t>(q)} << 32);
}

// A screen position (pixels from the top left of the 512 x 448 picture)
// in the chip's 12.4 coordinates around (2048, 2048).
inline std::uint64_t xyz2(double px, double py, std::uint32_t z) {
    const auto x = static_cast<std::uint64_t>((2048.0 - 256.0 + px) * 16.0);
    const auto y = static_cast<std::uint64_t>((2048.0 - 224.0 + py) * 16.0);
    return x | (y << 16) | (std::uint64_t{z} << 32);
}

inline std::uint64_t uv(double u, double v) {
    return static_cast<std::uint64_t>(u * 16.0) | (static_cast<std::uint64_t>(v * 16.0) << 16);
}

inline std::uint64_t tex0(
    std::uint32_t tbp,
    std::uint32_t tbw,
    std::uint8_t psm,
    std::uint32_t tw,
    std::uint32_t th,
    bool tcc,
    std::uint32_t cbp = 0,
    std::uint8_t cpsm = 0
) {
    return std::uint64_t{tbp} | (std::uint64_t{tbw} << 14) | (std::uint64_t{psm} << 20)
           | (std::uint64_t{tw} << 26) | (std::uint64_t{th} << 30) | (std::uint64_t{tcc} << 34)
           | (std::uint64_t{cbp} << 37) | (std::uint64_t{cpsm} << 51);
}

inline std::uint64_t test_reg(
    bool ate, gs::AlphaTest atst, std::uint8_t aref, gs::AlphaFail afail, gs::DepthTest ztst
) {
    return std::uint64_t{ate} | (static_cast<std::uint64_t>(atst) << 1) | (std::uint64_t{aref} << 4)
           | (static_cast<std::uint64_t>(afail) << 12) | (std::uint64_t{1} << 16)
           | (static_cast<std::uint64_t>(ztst) << 17);
}

inline std::uint64_t alpha_reg(
    std::uint8_t a, std::uint8_t b, std::uint8_t c, std::uint8_t d, std::uint8_t fix = 0
) {
    return std::uint64_t{a} | (std::uint64_t{b} << 2) | (std::uint64_t{c} << 4)
           | (std::uint64_t{d} << 6) | (std::uint64_t{fix} << 32);
}

inline std::uint64_t scissor(
    std::uint32_t x0, std::uint32_t x1, std::uint32_t y0, std::uint32_t y1
) {
    return std::uint64_t{x0} | (std::uint64_t{x1} << 16) | (std::uint64_t{y0} << 32)
           | (std::uint64_t{y1} << 48);
}

}  // namespace openrac::test
