// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/tfrag_light.rs
// (the `ps2` module) and crates/rc-game/src/ps2v.rs: ISC License, Copyright (c) 2026 ReRAC
// contributors.
//
// The console's single-precision rounding, as arithmetic on float bit patterns:
// the EE FPU and VU0 truncate every result toward zero, have no denormals, NaN
// or infinity (exponent 255 is an ordinary number), and their adder drops the
// smaller operand's bits below half an ulp of the larger before adding. This
// is a numeric model, not an emulation: nothing runs console code. The world
// queries (collision_query.h) can compute with it to give the results the
// game's own kernels give, bit for bit, when a tool must compare with them;
// the port's own code computes in IEEE floats.

#pragma once

#include <cstdint>

namespace openrac::assets::console_float {

using Bits = std::uint32_t;

inline constexpr Bits kSign = 0x8000'0000;
inline constexpr Bits kMax = 0x7fff'ffff;  // the largest magnitude
inline constexpr Bits kOne = 0x3f80'0000;

Bits add(Bits a, Bits b);
Bits sub(Bits a, Bits b);
Bits mul(Bits a, Bits b);
// A truncated quotient; division by zero gives the largest magnitude.
Bits div(Bits a, Bits b);
// The square root of |x|, truncated.
Bits sqrt(Bits x);
// `a / sqrt(b)`: the root is truncated first, then divided.
Bits rsqrt(Bits a, Bits b);
// An integer divided by 4096 (ITOF12), truncating wider than 24 bits.
Bits itof12(std::int32_t i);
// ITOF0 / cvt.s.w: exact below 2^24, truncated above.
Bits itof0(std::int32_t i);
// FTOI0 / cvt.w.s: truncation toward zero, saturating at the int32 range.
std::int32_t ftoi0(Bits x);
// max.s / min.s on the console's order (-0 below +0).
Bits max(Bits a, Bits b);
Bits min(Bits a, Bits b);

// The sign bit, as the game's `bltz` on the raw word tests it (-0 counts).
constexpr bool negative(Bits x) {
    return (x & kSign) != 0;
}

// A console float as an IEEE float: identical bits, except exponent 255,
// which the console treats as a finite number, clamps to the largest finite.
float to_float(Bits x);

// An IEEE float as the console reads it: denormals flush to zero.
Bits from_float(float x);

}  // namespace openrac::assets::console_float
