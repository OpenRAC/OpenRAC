// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-video/src/vlc.rs (its
// tests): ISC License, Copyright (c) 2026 ReRAC contributors.
//
// The MPEG-2 code tables: prefix codes (building them throws otherwise) with
// the expected sizes and code spaces, and a few decodes by hand.

#include "media/vlc.h"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <vector>

#include "tests/check.h"

using namespace openrac::media;

namespace {

std::vector<std::int16_t> run_levels(Mpeg2Codes table) {
    std::vector<std::int16_t> v;
    for (const Mpeg2Codes t : {table, Mpeg2Codes::DctLong}) {
        for (const VlcCode& c : mpeg2_codes(t)) {
            if (c.value >= 0) {
                v.push_back(c.value);
            }
        }
    }
    std::sort(v.begin(), v.end());
    return v;
}

}  // namespace

int main() {
    const Mpeg2VlcTables& t = mpeg2_vlc_tables();
    // The macroblock-type, DC-size and motion codes are complete but for the
    // codes the standard leaves unused.
    CHECK(t.macroblock_type[0].kraft() == 0.75);
    CHECK(t.macroblock_type[1].kraft() == 1.0 - 1.0 / 64.0);
    CHECK(t.macroblock_type[2].kraft() == 1.0 - 1.0 / 64.0);
    CHECK(t.dc_size_luminance.kraft() == 1.0);
    CHECK(t.dc_size_chrominance.kraft() == 1.0);
    // Motion magnitudes: the unused 0000 0000 xx and 0000 0001 xx are 3/256.
    CHECK(t.motion_code.kraft() == 0.98828125);
    CHECK(mpeg2_codes(Mpeg2Codes::MacroblockAddress).size() == 35);
    CHECK(mpeg2_codes(Mpeg2Codes::CodedBlockPattern).size() == 64);
    CHECK(t.dct_zero.max_length() == 16 && t.dct_one.max_length() == 16);
    // Both DCT tables code the same 111 (run, level) pairs, each once.
    const auto a = run_levels(Mpeg2Codes::DctZero);
    const auto b = run_levels(Mpeg2Codes::DctOne);
    CHECK(a.size() == 111);
    CHECK(a == b);
    CHECK(std::adjacent_find(a.begin(), a.end()) == a.end());
    // Every 6-bit coded_block_pattern appears once (0 only as the 4:2:2 code).
    std::vector<std::int16_t> cbps;
    for (const VlcCode& c : mpeg2_codes(Mpeg2Codes::CodedBlockPattern)) {
        cbps.push_back(c.value);
    }
    std::sort(cbps.begin(), cbps.end());
    bool all = cbps.size() == 64;
    for (std::size_t i = 0; all && i < 64; ++i) {
        all = cbps[i] == static_cast<std::int16_t>(i);
    }
    CHECK(all);

    // "0000 0101 10" is increment 17, then "011" is 2.
    const std::array<std::uint8_t, 2> d1 = {0b0000'0101, 0b1001'1000};
    BitReader r1(d1, 0);
    CHECK(t.macroblock_address.decode(r1) == std::int16_t{17});
    CHECK(t.macroblock_address.decode(r1) == std::int16_t{2});
    CHECK(r1.bit_pos() == 13);
    // B-15 "0110" is the end of block, B-14 "0000 01" the escape.
    const std::array<std::uint8_t, 2> d2 = {0b0110'0000, 0b0100'0000};
    BitReader r2(d2, 0);
    CHECK(t.dct_one.decode(r2) == kDctEndOfBlock);
    CHECK(t.dct_zero.decode(r2) == kDctEscape);
    // No code starts with eight zeros in B-1: nothing is consumed.
    const std::array<std::uint8_t, 2> d3 = {0, 0};
    BitReader r3(d3, 0);
    CHECK(!t.macroblock_address.decode(r3));
    CHECK(r3.bit_pos() == 0);

    // Overlapping codes are refused.
    const VlcCode overlap[] = {{"1", 1}, {"10", 2}};
    bool threw = false;
    try {
        VlcTable bad(overlap);
    } catch (const std::logic_error&) {
        threw = true;
    }
    CHECK(threw);
    return openrac::test::result();
}
