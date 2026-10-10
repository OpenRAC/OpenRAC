// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/tfrag_light.rs
// and crates/rc-engine/src/tie_lod.rs:
// ISC License, Copyright (c) 2026 ReRAC contributors.

#include "renderer/world/vu_float.h"

#include <bit>
#include <cstdlib>

namespace openrac::renderer::world::vu {

std::uint32_t mul_positive(std::uint32_t a, std::uint32_t b) {
    const int ea = static_cast<int>((a >> 23) & 0xff);
    const int eb = static_cast<int>((b >> 23) & 0xff);
    if (ea == 0 || eb == 0) {
        return 0;
    }
    const std::uint32_t ma = (a & 0x7fffffu) | 0x800000u;
    const std::uint32_t mb = (b & 0x7fffffu) | 0x800000u;
    // floor(ma * mb / 2^24) in 32-bit pieces, as the shader must compute it.
    const std::uint32_t ah = ma >> 12;
    const std::uint32_t al = ma & 0xfffu;
    const std::uint32_t bh = mb >> 12;
    const std::uint32_t bl = mb & 0xfffu;
    const std::uint32_t mid = ah * bl + al * bh;
    const std::uint32_t low = ((mid & 0xfffu) << 12) + al * bl;
    const std::uint32_t top = ah * bh + (mid >> 12) + (low >> 24);
    std::uint32_t m = top;
    int e = ea + eb - 126;
    if (top < 0x800000u) {
        m = (top << 1) | ((low >> 23) & 1u);
        e = ea + eb - 127;
    }
    if (e < 1) {
        return 0;
    }
    if (e > 255) {
        return kMax;
    }
    return (static_cast<std::uint32_t>(e) << 23) | (m & 0x7fffffu);
}

std::uint32_t add_positive(std::uint32_t a, std::uint32_t b) {
    const int ea = static_cast<int>(a >> 23);
    const int eb = static_cast<int>(b >> 23);
    if (ea == 0) {
        return b;
    }
    if (eb == 0) {
        return a;
    }
    const std::uint32_t hi = ea >= eb ? a : b;
    const std::uint32_t lo = ea >= eb ? b : a;
    const int eh = ea >= eb ? ea : eb;
    const int el = ea >= eb ? eb : ea;
    const auto d = static_cast<std::uint32_t>(eh - el);
    if (d >= 25) {
        return hi;
    }
    const std::uint32_t s = ((hi & 0x7fffffu) | 0x800000u) + (((lo & 0x7fffffu) | 0x800000u) >> d);
    if (s >= 0x1000000u) {
        return (static_cast<std::uint32_t>(eh + 1) << 23) | ((s >> 1) & 0x7fffffu);
    }
    return (static_cast<std::uint32_t>(eh) << 23) | (s & 0x7fffffu);
}

std::uint8_t fat_colour_lane(
    std::uint8_t c0, std::uint8_t c1, std::uint8_t c2, std::uint32_t w, std::uint32_t z
) {
    auto lane = [](std::uint8_t c) {
        return 0x4b000000u | c;
    };
    const std::uint32_t half = 0x3f000000u;
    const std::uint32_t avg = add(mul(lane(c1), half), mul(lane(c2), half));
    return static_cast<std::uint8_t>(add(mul(avg, w), mul(lane(c0), z)) & 0xffu);
}

}  // namespace openrac::renderer::world::vu
