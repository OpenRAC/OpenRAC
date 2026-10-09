// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Adapted from ReRAC (https://github.com/re-rac/rerac), crates/rc-formats/src/tfrag_light.rs
// (module ps2) and crates/rc-engine/src/tie_lod.rs:
// ISC License, Copyright (c) 2026 ReRAC contributors.
//
// The console's FPU and vector-unit float arithmetic on raw bit patterns, for
// the per-frame decisions the world renderers replay on the CPU where the
// game's result depends on the console's rounding (the tie morph factor and
// its colour weights, the tie fog value). What is modelled, after ReRAC's
// reading of the hardware: no NaN, infinity or denormals (an exponent of 0 is
// zero, 255 is an ordinary value, overflow clamps to +-0x7fffffff, underflow
// flushes to +-0); every result is rounded toward zero; the adder drops the
// bits of the smaller operand that lie more than one bit below the larger
// one's last bit, and returns the larger operand unchanged when the exponents
// differ by 25 or more. Not modelled: the multiplier's rare last-bit
// differences from a truncated exact product.

#pragma once

#include <cstdint>

namespace openrac::renderer::world::vu {

inline constexpr std::uint32_t kSign = 0x80000000u;
inline constexpr std::uint32_t kMax = 0x7fffffffu;
inline constexpr std::uint32_t kOne = 0x3f800000u;

std::uint32_t mul(std::uint32_t a, std::uint32_t b);
std::uint32_t add(std::uint32_t a, std::uint32_t b);
std::uint32_t sub(std::uint32_t a, std::uint32_t b);
// Division by zero gives +-kMax.
std::uint32_t div(std::uint32_t a, std::uint32_t b);
std::uint32_t max(std::uint32_t a, std::uint32_t b);
std::uint32_t min(std::uint32_t a, std::uint32_t b);

std::uint32_t bits(float f);
float value(std::uint32_t bits);

// VU ftoi0: truncation toward zero, saturating.
std::int32_t ftoi0(float x);

// The tie shader's integer-only versions of mul() and add() for non-negative
// operands (tie.vert's vu_mul / vu_add, line for line): kept here so the
// tests check the GLSL's arithmetic against the float model.
std::uint32_t mul_positive(std::uint32_t a, std::uint32_t b);
std::uint32_t add_positive(std::uint32_t a, std::uint32_t b);

// One colour lane of a tie "fat" vertex as the tie VU1 program computes it
// (ReRAC's reading of program 13507): the palette lanes are 0x4b000000 + byte
// (an unpack with a 2^23 row), the second and third slots are averaged (x 0.5),
// then weighted avg * w + c0 * z, every step truncated, and the GS keeps the
// low byte. `w` and `z` are the instance's colour weights (TieLodPick).
std::uint8_t fat_colour_lane(std::uint8_t c0, std::uint8_t c1, std::uint8_t c2, std::uint32_t w,
                             std::uint32_t z);

}  // namespace openrac::renderer::world::vu
