// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

#include <cfenv>

#include "fp.h"
#include "types.h"

#if defined(__aarch64__)
#include <arm_neon.h>
#define OPENRAC_FP_QUAD 1
#else
#define OPENRAC_FP_QUAD 0
#endif

// The console's arithmetic (fp.h) on four fields at once, in the host's
// floating point where that gives the same bits.
//
// With the host rounding towards zero, a product of two ordinary numbers is
// the console's product, and a sum is the console's sum once the smaller
// operand has lost the bits the console's adder drops. That leaves the
// special values: operands with the smallest or largest exponent, and
// results that leave the ordinary range. Each function works out which
// fields are ordinary; when a field the caller needs is not, it returns
// false and the caller computes the instruction with fp.h instead.
//
// The caller must have set the host's rounding to "towards zero"
// (want_toward_zero).
namespace ps2::fp {

// The host's rounding mode, read and set directly where the C library's way
// is slow (it is called around every packet a vector unit sends).
#if defined(__aarch64__)
inline u64 host_rounding() {
    return __builtin_arm_rsr64("FPCR");
}

inline void set_host_rounding(u64 saved) {
    __builtin_arm_wsr64("FPCR", saved);
}

inline void round_toward_zero() {
    __builtin_arm_wsr64("FPCR", __builtin_arm_rsr64("FPCR") | (u64{3} << 22));
}

inline void round_to_nearest() {
    __builtin_arm_wsr64("FPCR", __builtin_arm_rsr64("FPCR") & ~(u64{3} << 22));
}

// Changing the mode is slow and reading it is not, so code that needs one
// mode asks for it on entry and nobody switches back: the vector units ask
// for "towards zero", whatever computes as the host does (the GS, the
// program around the machine) asks for "to nearest".
inline void want_toward_zero() {
    u64 mode = __builtin_arm_rsr64("FPCR");
    if ((mode & (u64{3} << 22)) != (u64{3} << 22)) {
        __builtin_arm_wsr64("FPCR", mode | (u64{3} << 22));
    }
}

inline void want_nearest() {
    u64 mode = __builtin_arm_rsr64("FPCR");
    if (mode & (u64{3} << 22)) {
        __builtin_arm_wsr64("FPCR", mode & ~(u64{3} << 22));
    }
}
#else
inline u64 host_rounding() {
    return static_cast<u64>(std::fegetround());
}

inline void set_host_rounding(u64 saved) {
    std::fesetround(static_cast<int>(saved));
}

inline void round_toward_zero() {
    std::fesetround(FE_TOWARDZERO);
}

inline void round_to_nearest() {
    std::fesetround(FE_TONEAREST);
}

inline void want_toward_zero() {
    if (std::fegetround() != FE_TOWARDZERO) {
        std::fesetround(FE_TOWARDZERO);
    }
}

inline void want_nearest() {
    if (std::fegetround() != FE_TONEAREST) {
        std::fesetround(FE_TONEAREST);
    }
}
#endif

#if OPENRAC_FP_QUAD

namespace quad_detail {

inline uint32x4_t exponent(uint32x4_t v) {
    return vandq_u32(vshrq_n_u32(v, 23), vdupq_n_u32(0xFF));
}

// All ones in the fields that are ordinary numbers or plain zeros.
inline uint32x4_t usable(uint32x4_t v, uint32x4_t e) {
    uint32x4_t ordinary = vandq_u32(vcgtq_u32(e, vdupq_n_u32(0)), vcltq_u32(e, vdupq_n_u32(255)));
    uint32x4_t zero = vceqq_u32(vandq_u32(v, vdupq_n_u32(0x7FFFFFFFu)), vdupq_n_u32(0));
    return vorrq_u32(ordinary, zero);
}

// The dest mask (x is bit 3) as all ones in the fields it names.
inline uint32x4_t fields(u32 dest) {
    const uint32x4_t bit = {8, 4, 2, 1};
    return vtstq_u32(vdupq_n_u32(dest), bit);
}

inline bool none(uint32x4_t v) {
    return vmaxvq_u32(v) == 0;
}

}  // namespace quad_detail

// out = a * b in the fields of `dest`. False: use fp::mul for this instruction.
inline bool quad_mul(const u32* a, const u32* b, u32 dest, u32* out) {
    using namespace quad_detail;
    uint32x4_t va = vld1q_u32(a), vb = vld1q_u32(b);
    uint32x4_t ea = exponent(va), eb = exponent(vb);
    // The result's exponent is the sum of the two, or one more: keep well
    // inside the range where it is neither too small nor too large.
    uint32x4_t sum = vaddq_u32(ea, eb);
    uint32x4_t in_range =
        vandq_u32(vcgtq_u32(sum, vdupq_n_u32(127 + 1)), vcltq_u32(sum, vdupq_n_u32(127 + 253)));
    uint32x4_t either_zero = vorrq_u32(
        vceqq_u32(vandq_u32(va, vdupq_n_u32(0x7FFFFFFFu)), vdupq_n_u32(0)),
        vceqq_u32(vandq_u32(vb, vdupq_n_u32(0x7FFFFFFFu)), vdupq_n_u32(0))
    );
    uint32x4_t ok =
        vandq_u32(vandq_u32(usable(va, ea), usable(vb, eb)), vorrq_u32(in_range, either_zero));
    if (!none(vbicq_u32(fields(dest), ok))) {
        return false;
    }
    vst1q_u32(
        out, vreinterpretq_u32_f32(vmulq_f32(vreinterpretq_f32_u32(va), vreinterpretq_f32_u32(vb)))
    );
    return true;
}

// out = a + b (or a - b) in the fields of `dest`. False: use fp::add.
inline bool quad_add(const u32* a, const u32* b, u32 dest, u32* out, bool subtract = false) {
    using namespace quad_detail;
    uint32x4_t va = vld1q_u32(a), vb = vld1q_u32(b);
    if (subtract) {
        vb = veorq_u32(vb, vdupq_n_u32(kSign));
    }
    uint32x4_t ea = exponent(va), eb = exponent(vb);
    // The operand with the smaller exponent keeps one bit below the other's
    // last place; with exponents 25 or more apart it does not count at all.
    uint32x4_t a_small = vcltq_u32(ea, eb);
    uint32x4_t hi = vbslq_u32(a_small, vb, va), lo = vbslq_u32(a_small, va, vb);
    uint32x4_t ehi = vbslq_u32(a_small, eb, ea);
    uint32x4_t d = vsubq_u32(ehi, vbslq_u32(a_small, ea, eb));
    // (A shift count of 32 or more gives zero, which clears the operand.)
    int32x4_t drop = vreinterpretq_s32_u32(vqsubq_u32(d, vdupq_n_u32(1)));
    uint32x4_t mask = vshlq_u32(vdupq_n_u32(0xFFFFFFFFu), drop);
    mask = vbslq_u32(vcgtq_u32(d, vdupq_n_u32(24)), vdupq_n_u32(0), mask);
    lo = vandq_u32(lo, mask);
    uint32x4_t r =
        vreinterpretq_u32_f32(vaddq_f32(vreinterpretq_f32_u32(hi), vreinterpretq_f32_u32(lo)));

    // Ordinary operands, no risk of leaving the top of the range, and a result
    // that is an ordinary number or an exact zero.
    uint32x4_t er = exponent(r);
    uint32x4_t result_ok = vorrq_u32(vcgtq_u32(er, vdupq_n_u32(0)), vceqq_u32(r, vdupq_n_u32(0)));
    uint32x4_t ok = vandq_u32(
        vandq_u32(usable(va, ea), usable(vb, eb)),
        vandq_u32(vcltq_u32(ehi, vdupq_n_u32(254)), result_ok)
    );
    if (!none(vbicq_u32(fields(dest), ok))) {
        return false;
    }
    vst1q_u32(out, r);
    return true;
}

// to = from in the fields of `dest`.
inline void quad_merge(u32* to, const u32* from, u32 dest) {
    vst1q_u32(to, vbslq_u32(quad_detail::fields(dest), vld1q_u32(from), vld1q_u32(to)));
}

#else

inline void quad_merge(u32* to, const u32* from, u32 dest) {
    for (unsigned field = 0; field < 4; field++) {
        if (dest & (8u >> field)) {
            to[field] = from[field];
        }
    }
}

inline bool quad_mul(const u32*, const u32*, u32, u32*) {
    return false;
}

inline bool quad_add(const u32*, const u32*, u32, u32*, bool = false) {
    return false;
}

#endif

}  // namespace ps2::fp
