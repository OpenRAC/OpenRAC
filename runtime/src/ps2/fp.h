// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

#include <bit>
#include <cmath>

#include "types.h"

// Single-precision arithmetic as the console's FPU and vector units do it,
// on raw 32-bit patterns, so that results do not depend on the host:
//
// - an exponent of 0 is zero, whatever the fraction (no denormals);
// - an exponent of 255 is an ordinary large number (no infinities or NaNs);
// - results are cut to 24 significant bits, never rounded up;
// - a result too large is the largest number, 0x7FFFFFFF with its sign, and
//   one too small is zero;
// - the adder keeps one bit of the smaller operand below the larger one's
//   last place and drops the rest before it adds.
//
// The rules are the ones ReRAC's `tfrag_light::ps2` module checked against a
// running game's lighting output (ISC, https://github.com/re-rac/rerac); this
// is an independent implementation of them. Rare last-bit deviations of the
// hardware multiplier are not modelled.
namespace ps2::fp {

constexpr u32 kSign = 0x80000000u, kMax = 0x7FFFFFFFu, kOne = 0x3F800000u;

// What went wrong, for the status flags.
enum : u32 { kOverflow = 1, kUnderflow = 2, kDivideByZero = 4, kInvalid = 8 };

constexpr s32 exponent(u32 x) {
  return static_cast<s32>((x >> 23) & 0xFF);
}
constexpr u64 mantissa(u32 x) {
  return (x & 0x7FFFFFu) | 0x800000u;
}
constexpr bool is_zero(u32 x) {
  return exponent(x) == 0;
}

// sign * magnitude * 2^(unit - 150): `unit` is the biased exponent a
// magnitude of exactly 2^23 would have.
constexpr u32 pack(u32 sign, u64 magnitude, s32 unit, u32& flags) {
  if (magnitude == 0) {
    return sign;
  }
  s32 top = 63 - std::countl_zero(magnitude);
  s32 e = unit + (top - 23);
  u64 m = top >= 23 ? magnitude >> (top - 23) : magnitude << (23 - top);
  if (e > 255) {
    flags |= kOverflow;
    return sign | kMax;
  }
  if (e < 1) {
    flags |= kUnderflow;
    return sign;
  }
  return sign | (static_cast<u32>(e) << 23) | (static_cast<u32>(m) & 0x7FFFFFu);
}

constexpr u32 mul(u32 a, u32 b, u32& flags) {
  u32 sign = (a ^ b) & kSign;
  if (is_zero(a) || is_zero(b)) {
    return sign;
  }
  return pack(sign, (mantissa(a) * mantissa(b)) >> 23, exponent(a) + exponent(b) - 127, flags);
}

constexpr u32 add(u32 a, u32 b, u32& flags) {
  if (is_zero(a) || is_zero(b)) {
    if (!is_zero(a)) return a;
    if (!is_zero(b)) return b;
    return a & b & kSign;
  }
  u32 hi = exponent(a) >= exponent(b) ? a : b;
  u32 lo = exponent(a) >= exponent(b) ? b : a;
  s32 d = exponent(hi) - exponent(lo);
  if (d >= 25) {
    return hi;
  }
  if (d >= 1) {
    lo &= 0xFFFFFFFFu << (d - 1);
  }
  s64 big = static_cast<s64>(mantissa(hi) << d), small = static_cast<s64>(mantissa(lo));
  s64 sum = ((hi & kSign) ? -big : big) + ((lo & kSign) ? -small : small);
  if (sum == 0) {
    return 0;
  }
  return pack(sum < 0 ? kSign : 0, static_cast<u64>(sum < 0 ? -sum : sum), exponent(lo), flags);
}

constexpr u32 sub(u32 a, u32 b, u32& flags) {
  return add(a, b ^ kSign, flags);
}

constexpr u32 div(u32 a, u32 b, u32& flags) {
  u32 sign = (a ^ b) & kSign;
  if (is_zero(b)) {
    flags |= is_zero(a) ? kInvalid : kDivideByZero;
    return sign | kMax;
  }
  if (is_zero(a)) {
    return sign;
  }
  return pack(sign, (mantissa(a) << 24) / mantissa(b), exponent(a) - exponent(b) + 126, flags);
}

constexpr u64 isqrt(u64 n) {
  u64 r = 0;
  for (u64 bit = u64{1} << 31; bit; bit >>= 1) {
    u64 t = r | bit;
    if (t * t <= n) {
      r = t;
    }
  }
  return r;
}

// The square root of the magnitude; a negative operand is flagged.
constexpr u32 sqrt(u32 x, u32& flags) {
  if (is_zero(x)) {
    return 0;
  }
  if (x & kSign) {
    flags |= kInvalid;
  }
  s32 e = exponent(x) - 127;
  s32 odd = ((e % 2) + 2) % 2;
  u32 unused = 0;
  return pack(0, isqrt(mantissa(x) << (23 + odd)), (e - odd) / 2 + 127, unused);
}

// a / sqrt(|b|): the root is cut first, then the division.
constexpr u32 rsqrt(u32 a, u32 b, u32& flags) {
  if (is_zero(b)) {
    flags |= is_zero(a) ? kInvalid : kDivideByZero;
    return (a & kSign) | kMax;
  }
  return div(a, sqrt(b, flags), flags);
}

// Order as sign and magnitude, with every zero equal.
constexpr s64 key(u32 x) {
  if (is_zero(x)) {
    return 0;
  }
  return (x & kSign) ? -static_cast<s64>(x & kMax) : static_cast<s64>(x);
}
constexpr u32 max(u32 a, u32 b) {
  return key(b) > key(a) ? b : a;
}
constexpr u32 min(u32 a, u32 b) {
  return key(b) < key(a) ? b : a;
}

// An integer divided by 2^shift.
constexpr u32 from_int(s32 value, s32 shift = 0) {
  u32 unused = 0;
  u64 magnitude = value < 0 ? static_cast<u64>(-static_cast<s64>(value)) : static_cast<u64>(value);
  return pack(value < 0 ? kSign : 0, magnitude, 150 - shift, unused);
}

// The number times 2^shift as an integer, cut towards zero, saturating.
constexpr s32 to_int(u32 x, s32 shift = 0) {
  if (is_zero(x)) {
    return 0;
  }
  s32 e = exponent(x) - 150 + shift;  // the mantissa's last place is worth 2^e
  if (e >= 8) {
    return (x & kSign) ? static_cast<s32>(0x80000000u) : 0x7FFFFFFF;
  }
  if (e <= -24) {
    return 0;
  }
  s64 magnitude = static_cast<s64>(e >= 0 ? mantissa(x) << e : mantissa(x) >> -e);
  return static_cast<s32>((x & kSign) ? -magnitude : magnitude);
}

// To and from the host's numbers, for the few functions (sines, arctangents)
// that are computed there.
inline double to_double(u32 x) {
  if (is_zero(x)) {
    return (x & kSign) ? -0.0 : 0.0;
  }
  double v = std::ldexp(static_cast<double>(mantissa(x)), exponent(x) - 150);
  return (x & kSign) ? -v : v;
}
inline u32 from_double(double v, u32& flags) {
  if (v == 0.0 || std::isnan(v)) {
    return std::signbit(v) ? kSign : 0;
  }
  u32 sign = std::signbit(v) ? kSign : 0;
  if (std::isinf(v)) {
    flags |= kOverflow;
    return sign | kMax;
  }
  int e = 0;
  double m = std::frexp(std::fabs(v), &e);  // m in [0.5, 1)
  return pack(sign, static_cast<u64>(std::ldexp(m, 40)), e + 110, flags);
}

}  // namespace ps2::fp
