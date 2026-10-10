// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-video/src/bits.rs (its
// tests): ISC License, Copyright (c) 2026 ReRAC contributors.
//
// The bit reader and the start-code search.

#include "media/bit_reader.h"

#include <array>
#include <cstdint>

#include "tests/check.h"

using namespace openrac::media;

int main() {
    // Most significant bit first, zeros past the end.
    const std::array<std::uint8_t, 4> d = {0b1011'0011, 0xff, 0x00, 0x80};
    BitReader r(d, 0);
    CHECK(r.read(1) == 1);
    CHECK(r.read(3) == 0b011);
    CHECK(r.peek(8) == 0b0011'1111);
    CHECK(r.read(12) == 0b0011'1111'1111);
    CHECK(r.read(9) == 1);
    CHECK(!r.overrun());
    CHECK(r.read(32) == 0);
    CHECK(r.overrun());
    BitReader s(d, 1);
    s.skip(3);
    s.align();
    CHECK(s.byte_pos() == 2);
    // A long buffer takes the fast path; the same bits either way.
    std::array<std::uint8_t, 16> long_data{};
    long_data[7] = 0xa5;
    BitReader l(long_data, 0);
    l.skip(56);
    CHECK(l.peek(8) == 0xa5 && l.peek(32) == 0xa500'0000u);
    CHECK(!l.at_start_code());
    l.skip(8);
    CHECK(l.at_start_code());

    const std::array<std::uint8_t, 15> codes = {0xff, 0, 0, 1, 0xb3, 0, 0, 0, 1, 0, 0, 2, 0, 0, 1};
    CHECK(next_start_code(codes, 0) == std::size_t{1});
    CHECK(next_start_code(codes, 2) == std::size_t{6});
    CHECK(next_start_code(codes, 7) == std::size_t{12});
    CHECK(!next_start_code(codes, 13));
    return openrac::test::result();
}
