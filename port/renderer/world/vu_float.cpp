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

namespace {

int exponent(std::uint32_t x) {
    return static_cast<int>((x >> 23) & 0xff);
}

std::uint64_t mantissa(std::uint32_t x) {
    return (x & 0x7fffffu) | 0x800000u;
}

// sign * mag * 2^(lsb_exp - 127 - 23), truncated to 24 significant bits; mag > 0.
std::uint32_t pack(std::uint32_t sign, std::uint64_t mag, int lsb_exp) {
    const int k = 63 - std::countl_zero(mag);
    const int e = lsb_exp + (k - 23);
    const std::uint64_t m = k >= 23 ? mag >> (k - 23) : mag << (23 - k);
    if (e > 255) {
        return sign | kMax;
    }
    if (e < 1) {
        return sign;
    }
    return sign | (static_cast<std::uint32_t>(e) << 23) | (static_cast<std::uint32_t>(m) & 0x7fffffu);
}

std::int64_t key(std::uint32_t x) {
    return (x & kSign) != 0 ? -static_cast<std::int64_t>(x & kMax) : static_cast<std::int64_t>(x);
}

}  // namespace

std::uint32_t mul(std::uint32_t a, std::uint32_t b) {
    const std::uint32_t s = (a ^ b) & kSign;
    if (exponent(a) == 0 || exponent(b) == 0) {
        return s;
    }
    // The product has its binary point at bit 46; bit 23 of (p >> 23) is worth
    // 2^(ea + eb - 254).
    return pack(s, (mantissa(a) * mantissa(b)) >> 23, exponent(a) + exponent(b) - 127);
}

std::uint32_t add(std::uint32_t a, std::uint32_t b) {
    const int ea = exponent(a);
    const int eb = exponent(b);
    if (ea == 0 || eb == 0) {
        if (ea != 0) {
            return a;
        }
        if (eb != 0) {
            return b;
        }
        return (a & b & kSign) != 0 ? kSign : 0;
    }
    const std::uint32_t hi = ea >= eb ? a : b;
    std::uint32_t lo = ea >= eb ? b : a;
    const int d = exponent(hi) - exponent(lo);
    if (d >= 25) {
        return hi;
    }
    if (d >= 1) {
        lo &= ~0u << (d - 1);
    }
    auto signed_value = [](std::uint32_t x, std::uint64_t m) {
        return (x & kSign) != 0 ? -static_cast<std::int64_t>(m) : static_cast<std::int64_t>(m);
    };
    const std::int64_t sum =
        signed_value(hi, mantissa(hi) << d) + signed_value(lo, mantissa(lo));
    if (sum == 0) {
        return 0;
    }
    const std::uint64_t magnitude =
        sum < 0 ? static_cast<std::uint64_t>(-sum) : static_cast<std::uint64_t>(sum);
    return pack(sum < 0 ? kSign : 0, magnitude, exponent(lo));
}

std::uint32_t sub(std::uint32_t a, std::uint32_t b) {
    return add(a, b ^ kSign);
}

std::uint32_t div(std::uint32_t a, std::uint32_t b) {
    const std::uint32_t s = (a ^ b) & kSign;
    if (exponent(b) == 0) {
        return s | kMax;
    }
    if (exponent(a) == 0) {
        return s;
    }
    const std::uint64_t q = (mantissa(a) << 24) / mantissa(b);  // in [2^23, 2^25)
    return pack(s, q, exponent(a) - exponent(b) + 127 - 1);
}

std::uint32_t max(std::uint32_t a, std::uint32_t b) {
    return key(b) > key(a) ? b : a;
}

std::uint32_t min(std::uint32_t a, std::uint32_t b) {
    return key(b) < key(a) ? b : a;
}

std::uint32_t bits(float f) {
    return std::bit_cast<std::uint32_t>(f);
}

float value(std::uint32_t b) {
    return std::bit_cast<float>(b);
}

std::int32_t ftoi0(float x) {
    if (!(x > -2147483648.0f)) {
        return x != x ? 0 : INT32_MIN;
    }
    if (x >= 2147483520.0f) {
        return 2147483520;
    }
    return static_cast<std::int32_t>(x);
}

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

std::uint8_t fat_colour_lane(std::uint8_t c0, std::uint8_t c1, std::uint8_t c2, std::uint32_t w,
                             std::uint32_t z) {
    auto lane = [](std::uint8_t c) {
        return 0x4b000000u | c;
    };
    const std::uint32_t half = 0x3f000000u;
    const std::uint32_t avg = add(mul(lane(c1), half), mul(lane(c2), half));
    return static_cast<std::uint8_t>(add(mul(avg, w), mul(lane(c0), z)) & 0xffu);
}

}  // namespace openrac::renderer::world::vu
