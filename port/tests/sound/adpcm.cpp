// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/vag.rs (its
// tests): ISC License, Copyright (c) 2026 ReRAC contributors.
//
// The SPU ADPCM decoder on synthetic frames, against values computed by hand,
// and the sample walk.

#include "assets/sound/adpcm.h"

#include <array>
#include <vector>

#include "tests/check.h"

using namespace openrac::assets;

namespace {

std::array<u8, 16> frame(u8 shift, u8 filter, u8 flags, const std::array<u8, 28>& nibbles) {
    std::array<u8, 16> f{};
    f[0] = static_cast<u8>(shift | filter << 4);
    f[1] = flags;
    for (std::size_t i = 0; i < 28; ++i) {
        f[2 + i / 2] = static_cast<u8>(f[2 + i / 2] | (nibbles[i] & 15) << ((i & 1) * 4));
    }
    return f;
}

void append(std::vector<u8>& out, const std::array<u8, 16>& f) {
    out.insert(out.end(), f.begin(), f.end());
}

}  // namespace

int main() {
    // A zero frame decodes to 28 zeros; the low nibble comes first.
    {
        std::array<u8, 16> zero{};
        const std::vector<s16> pcm = decode_adpcm(ByteView(zero.data(), zero.size()));
        CHECK(pcm.size() == 28);
        bool all_zero = true;
        for (const s16 v : pcm) {
            all_zero = all_zero && v == 0;
        }
        CHECK(all_zero);

        std::array<u8, 28> n{};
        n[0] = 1;    // low nibble of byte 2
        n[1] = 0xf;  // high nibble: -1
        auto f = frame(12, 0, 0, n);
        std::vector<s16> a = decode_adpcm(ByteView(f.data(), f.size()));
        CHECK(a[0] == 1 && a[1] == -1 && a[2] == 0);
        f = frame(0, 0, 0, n);
        a = decode_adpcm(ByteView(f.data(), f.size()));
        CHECK(a[0] == 0x1000 && a[1] == -0x1000);
    }

    // One prediction by hand, h1 = 1000, h2 = 500, filter 2, zero nibble:
    // rounded (115 * 1000 - 52 * 500 + 32) >> 6 = 1391; truncated
    // (115000 >> 6) + (-26000 >> 6) = 1796 - 407 = 1389.
    {
        const auto f = frame(0, 2, 0, {});
        AdpcmHistory h{1000, 500};
        CHECK(decode_adpcm_frame(f.data(), h, AdpcmRounding::Rounded)[0] == 1391);
        h = {1000, 500};
        CHECK(decode_adpcm_frame(f.data(), h, AdpcmRounding::Truncated)[0] == 1389);
        // The history moves on: after the first sample h1 = 1391, h2 = 1000;
        // the second is (115 * 1391 - 52 * 1000 + 32) >> 6 = 1687.
        h = {1000, 500};
        const auto pcm = decode_adpcm_frame(f.data(), h);
        CHECK(pcm[1] == 1687);
        CHECK(h.h1 == pcm[27] && h.h2 == pcm[26]);

        // Filter 0 frames never read the history: both forms agree.
        std::array<u8, 28> n{};
        for (std::size_t i = 0; i < 28; ++i) {
            n[i] = static_cast<u8>((i * 7 + 3) & 15);
        }
        const auto g = frame(2, 0, 0, n);
        CHECK(
            decode_adpcm(ByteView(g.data(), g.size()))
            == decode_adpcm(ByteView(g.data(), g.size()), AdpcmRounding::Truncated)
        );
        // Filter 1 (60/64): (60 * 1000 + 32) >> 6 = 938.
        AdpcmHistory h1{1000, 0};
        CHECK(decode_adpcm_frame(frame(0, 1, 0, {}).data(), h1)[0] == 938);
        // Clamped to 16 bits: nibble 7 at shift 0 is 0x7000, plus a large prediction.
        std::array<u8, 28> sevens{};
        sevens.fill(7);
        AdpcmHistory big{30000, 0};
        CHECK(decode_adpcm_frame(frame(0, 4, 0, sevens).data(), big)[0] == 32767);
    }

    // The walk follows the flags: a one-shot (0, 0, 1) padded with a 7 frame,
    // then a loop (6, 2, 3).
    {
        std::vector<u8> blob;
        for (const int flags : {0, 0, 1, 7, 6, 2, 3}) {
            append(blob, frame(0, 0, static_cast<u8>(flags), {}));
        }
        const ByteView v(blob);
        const SampleExtent one = sample_extent(v, 0);
        CHECK(one.frames == 3 && !one.looped && one.next_flags == u8{7} && !one.loop_points());
        const SampleExtent loop = sample_extent(v, 4 * 16);
        CHECK(loop.frames == 3 && loop.looped && loop.loop_start == std::size_t{0});
        const auto points = loop.loop_points();
        CHECK(points && points->first == 0 && points->second == 84);
        CHECK(!loop.next_flags);
        CHECK(decode_extent(v, one).size() == 84);
        bool threw = false;
        try {
            sample_extent(v, 5);
        } catch (const AssetError&) {
            threw = true;
        }
        CHECK(threw);
        threw = false;
        try {
            sample_extent(v.sub(0, 32), 0);
        } catch (const AssetError&) {
            threw = true;
        }
        CHECK(threw);
    }
    return openrac::test::result();
}
