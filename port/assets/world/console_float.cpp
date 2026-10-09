// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/tfrag_light.rs
// (the `ps2` module) and crates/rc-game/src/ps2v.rs: ISC License, Copyright (c) 2026 ReRAC
// contributors.
//
// The console's float rounding: every operation works on the 24-bit
// significands as integers and truncates once, at the end.

#include "assets/world/console_float.h"

#include <bit>
#include <cmath>
#include <limits>

namespace openrac::assets::console_float {

namespace {

int exponent(Bits x) {
    return static_cast<int>((x >> 23) & 0xff);
}

std::uint64_t significand(Bits x) {
    return (x & 0x7f'ffff) | 0x80'0000;
}

// sign * magnitude * 2^(lsb_exponent - 127 - 23), truncated to 24 significant
// bits. `lsb_exponent` is the biased exponent a magnitude of exactly bit 23
// would have.
Bits pack(Bits sign, std::uint64_t magnitude, int lsb_exponent) {
    const int top = 63 - std::countl_zero(magnitude);
    const int e = lsb_exponent + (top - 23);
    const std::uint64_t m = top >= 23 ? magnitude >> (top - 23) : magnitude << (23 - top);
    if (e > 255) {
        return sign | kMax;
    }
    if (e < 1) {
        return sign;
    }
    return sign | static_cast<Bits>(e) << 23 | (static_cast<Bits>(m) & 0x7f'ffff);
}

std::uint64_t integer_sqrt(std::uint64_t n) {
    auto r = static_cast<std::uint64_t>(std::sqrt(static_cast<double>(n)));
    while (r * r > n) {
        --r;
    }
    while ((r + 1) * (r + 1) <= n) {
        ++r;
    }
    return r;
}

// The console's order: -0 sorts below +0.
std::int64_t order_key(Bits x) {
    const auto magnitude = static_cast<std::int64_t>(x & kMax);
    return negative(x) ? -magnitude : magnitude;
}

}  // namespace

Bits mul(Bits a, Bits b) {
    const Bits sign = (a ^ b) & kSign;
    if (exponent(a) == 0 || exponent(b) == 0) {
        return sign;
    }
    // The product's binary point is at bit 46; bit 23 of (p >> 23) is worth
    // 2^(ea + eb - 254).
    return pack(sign, (significand(a) * significand(b)) >> 23, exponent(a) + exponent(b) - 127);
}

Bits add(Bits a, Bits b) {
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
    const Bits hi = ea >= eb ? a : b;
    Bits lo = ea >= eb ? b : a;
    const int gap = exponent(hi) - exponent(lo);
    if (gap >= 25) {
        return hi;
    }
    // The adder keeps one guard bit: the smaller operand loses the bits below
    // half an ulp of the larger before the sum is formed.
    if (gap >= 1) {
        lo &= ~Bits{0} << (gap - 1);
    }
    auto signed_magnitude = [](Bits x, std::uint64_t m) {
        return negative(x) ? -static_cast<std::int64_t>(m) : static_cast<std::int64_t>(m);
    };
    const std::int64_t sum =
        signed_magnitude(hi, significand(hi) << gap) + signed_magnitude(lo, significand(lo));
    if (sum == 0) {
        return 0;
    }
    const auto magnitude = static_cast<std::uint64_t>(sum < 0 ? -sum : sum);
    return pack(sum < 0 ? kSign : 0, magnitude, exponent(lo));
}

Bits sub(Bits a, Bits b) {
    return add(a, b ^ kSign);
}

Bits div(Bits a, Bits b) {
    const Bits sign = (a ^ b) & kSign;
    if (exponent(b) == 0) {
        return sign | kMax;
    }
    if (exponent(a) == 0) {
        return sign;
    }
    const std::uint64_t q = (significand(a) << 24) / significand(b);  // in [2^23, 2^25)
    return pack(sign, q, exponent(a) - exponent(b) + 127 - 1);
}

Bits sqrt(Bits x) {
    if (exponent(x) == 0) {
        return 0;
    }
    const int e = exponent(x) - 127;
    const int odd = ((e % 2) + 2) % 2;
    const std::uint64_t r = integer_sqrt(significand(x) << (23 + odd));  // in [2^23, 2^24)
    return pack(0, r, (e - odd) / 2 + 127);
}

Bits rsqrt(Bits a, Bits b) {
    return div(a, sqrt(b));
}

Bits itof12(std::int32_t i) {
    if (i == 0) {
        return 0;
    }
    const auto magnitude = static_cast<std::uint64_t>(i < 0 ? -static_cast<std::int64_t>(i) : i);
    return pack(i < 0 ? kSign : 0, magnitude, 127 + 23 - 12);
}

Bits itof0(std::int32_t i) {
    if (i == 0) {
        return 0;
    }
    const auto magnitude = static_cast<std::uint64_t>(i < 0 ? -static_cast<std::int64_t>(i) : i);
    return pack(i < 0 ? kSign : 0, magnitude, 127 + 23);
}

std::int32_t ftoi0(Bits x) {
    const int e = exponent(x);
    if (e < 127) {
        return 0;
    }
    const bool neg = negative(x);
    if (e >= 127 + 31) {
        return neg ? std::numeric_limits<std::int32_t>::min()
                   : std::numeric_limits<std::int32_t>::max();
    }
    const auto m = static_cast<std::int64_t>(significand(x));
    const std::int64_t v = e >= 150 ? m << (e - 150) : m >> (150 - e);
    return static_cast<std::int32_t>(neg ? -v : v);
}

Bits max(Bits a, Bits b) {
    return order_key(b) > order_key(a) ? b : a;
}

Bits min(Bits a, Bits b) {
    return order_key(b) < order_key(a) ? b : a;
}

float to_float(Bits x) {
    if (exponent(x) == 0xff) {
        return negative(x) ? -std::numeric_limits<float>::max() : std::numeric_limits<float>::max();
    }
    return std::bit_cast<float>(x);
}

Bits from_float(float x) {
    const auto bits = std::bit_cast<Bits>(x);
    return exponent(bits) == 0 ? (bits & kSign) : bits;
}

}  // namespace openrac::assets::console_float
