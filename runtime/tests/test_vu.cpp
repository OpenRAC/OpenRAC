// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// Tests of the vector unit interpreter, on programs written here with the
// encoders in vu_asm.h. What they expect is the documented behaviour of the
// instruction set and its timing.

#include <array>
#include <cstring>
#include <vector>

#include <cfenv>

#include "fp_quad.h"
#include "graphics.h"
#include "vu.h"
#include "vu_asm.h"

#include "check.h"

using namespace ps2;
using namespace ps2::vuasm;

namespace {

// A vector unit with VU1's memory sizes and nothing attached.
struct Unit {
  std::array<u8, 16384> micro{};
  std::array<u8, 16384> data{};
  Vu vu{Vu::Memory{micro.data(), 16384, data.data(), 16384}};

  u64 run(const Program& program, u32 at = 0) {
    std::memcpy(micro.data(), program.words().data(), program.words().size() * 4);
    return vu.run(at);
  }
  void set(unsigned reg, float x, float y, float z, float w) {
    vu.vf[reg] = {as_u32(x), as_u32(y), as_u32(z), as_u32(w)};
  }
  u32 word(u32 quad, u32 field) const { return load<u32>(&data[quad * 16 + field * 4]); }
  void set_word(u32 quad, u32 field, u32 v) { store<u32>(&data[quad * 16 + field * 4], v); }
};

// The usual ending: the E bit on a pair, then one more.
void finish(Program& p) {
  p.add(nop() | E, lnop());
  p.hi(nop());
}

void test_arithmetic_and_masks() {
  Unit u;
  u.set(1, 1.5f, 2.0f, -3.0f, 4.0f);
  u.set(2, 0.5f, 10.0f, 1.0f, 2.0f);
  u.set(3, 9.0f, 9.0f, 9.0f, 9.0f);
  Program p;
  p.hi(add(XY, 3, 1, 2));          // only x and y are written
  p.hi(mulbc(XYZW, 4, 1, 2, 3));   // every field times vf2.w
  p.hi(sub(XYZW, 0, 1, 2));        // VF0 cannot be written
  p.hi(mula(XYZW, 1, 2));          // ACC = vf1 * vf2
  p.hi(madd(XYZW, 5, 1, 1));       // vf5 = ACC + vf1 * vf1
  p.hi(msubbc(X, 6, 1, 2, 1));     // vf6.x = ACC.x - vf1.x * vf2.y
  p.hi(max(XYZW, 7, 1, 2));
  p.hi(mini(XYZW, 8, 1, 2));
  p.hi(abs(XYZW, 9, 1));
  finish(p);
  u64 ran = u.run(p);
  CHECK_EQ(ran, u64{11});
  CHECK(u.vu.stopped());
  CHECK_EQ(u.vu.pc, 11u);
  CHECK(u.vu.f(3, 0) == 2.0f && u.vu.f(3, 1) == 12.0f && u.vu.f(3, 2) == 9.0f && u.vu.f(3, 3) == 9.0f);
  CHECK(u.vu.f(4, 0) == 3.0f && u.vu.f(4, 2) == -6.0f && u.vu.f(4, 3) == 8.0f);
  CHECK(u.vu.f(0, 0) == 0.0f && u.vu.f(0, 3) == 1.0f);
  CHECK(u.vu.f(5, 0) == 0.75f + 2.25f && u.vu.f(5, 2) == -3.0f + 9.0f);
  CHECK(u.vu.f(6, 0) == 0.75f - 15.0f);
  CHECK(u.vu.f(7, 0) == 1.5f && u.vu.f(7, 1) == 10.0f && u.vu.f(7, 2) == 1.0f);
  CHECK(u.vu.f(8, 0) == 0.5f && u.vu.f(8, 2) == -3.0f);
  CHECK(u.vu.f(9, 2) == 3.0f);
  CHECK_EQ(u.vu.unknown_ops, u64{0});
}

void test_cross_product() {
  Unit u;
  u.set(1, 1.0f, 0.0f, 0.0f, 0.0f);
  u.set(2, 0.0f, 1.0f, 0.0f, 0.0f);
  Program p;
  p.hi(opmula(1, 2));
  p.hi(opmsub(3, 2, 1));
  finish(p);
  u.run(p);
  CHECK(u.vu.f(3, 0) == 0.0f && u.vu.f(3, 1) == 0.0f && u.vu.f(3, 2) == 1.0f);
}

void test_numbers() {
  Unit u;
  // There are no infinities: the largest number plus itself is the largest
  // number, with the overflow flag; a product too small to hold is zero,
  // with underflow; the largest exponent is an ordinary one.
  u.vu.vf[1] = {0x7FFFFFFFu, as_u32(1e-30f), as_u32(1.0f), 0x7F800000u};
  u.vu.vf[2] = {0x7FFFFFFFu, as_u32(1e-30f), 0x33C00000u /* 3 * 2^-25 */, as_u32(1.0f)};
  u.vu.vi[5] = 0xFFFF;
  Program p;
  p.hi(add(X, 3, 1, 2));
  p.hi(mul(Y, 3, 1, 2));
  p.hi(add(Z, 3, 1, 2));   // cut, not rounded: 1 + 0.75 of a last place is 1
  p.hi(mul(W, 3, 1, 2));   // 2^128 times one is 2^128
  finish(p);
  u.run(p);
  CHECK_EQ(u.vu.vf[3][0], 0x7FFFFFFFu);
  CHECK_EQ(u.vu.vf[3][1], 0u);
  CHECK_EQ(u.vu.vf[3][2], 0x3F800000u);
  CHECK_EQ(u.vu.vf[3][3], 0x7F800000u);
  // The status register remembers overflow (bit 9) and underflow (bit 8).
  CHECK_EQ(u.vu.status & 0x300, 0x300u);
}

void test_conversions() {
  Unit u;
  u.vu.vf[1] = {static_cast<u32>(40), static_cast<u32>(-24), 0x7FFFFFFFu, 0};
  u.set(2, 2.75f, -2.75f, 3e9f, -3e9f);
  Program p;
  p.hi(itof4(XY, 3, 1));   // 40 / 16, -24 / 16
  p.hi(itof0(X, 4, 1));
  p.hi(ftoi0(XYZW, 5, 2)); // towards zero, saturating
  p.hi(ftoi4(XY, 6, 2));
  finish(p);
  u.run(p);
  CHECK(u.vu.f(3, 0) == 2.5f && u.vu.f(3, 1) == -1.5f);
  CHECK(u.vu.f(4, 0) == 40.0f);
  CHECK_EQ(u.vu.vf[5][0], 2u);
  CHECK_EQ(u.vu.vf[5][1], static_cast<u32>(-2));
  CHECK_EQ(u.vu.vf[5][2], 0x7FFFFFFFu);
  CHECK_EQ(u.vu.vf[5][3], 0x80000000u);
  CHECK_EQ(u.vu.vf[6][0], 44u);
  CHECK_EQ(u.vu.vf[6][1], static_cast<u32>(-44));
}

void test_pair_reads_old_values() {
  Unit u;
  u.set(1, 1.0f, 1.0f, 1.0f, 1.0f);
  u.set(2, 2.0f, 2.0f, 2.0f, 2.0f);
  u.set_word(9, 0, as_u32(50.0f));
  Program p;
  // The lower instruction stores vf1 while the upper one changes it: memory
  // gets the old value.
  p.add(add(XYZW, 1, 1, 2), sq(XYZW, 1, 8, 0));
  // Both write vf3: the upper instruction's result stays.
  p.add(add(X, 3, 2, 2), lq(X, 3, 9, 0));
  // The upper instruction reads vf4 while the lower one loads it: it adds
  // the old value.
  p.add(add(X, 5, 4, 2), lq(X, 4, 9, 0));
  finish(p);
  u.run(p);
  CHECK(u.vu.f(1, 0) == 3.0f);
  CHECK_EQ(u.word(8, 0), as_u32(1.0f));
  CHECK(u.vu.f(3, 0) == 4.0f);
  CHECK(u.vu.f(4, 0) == 50.0f);
  CHECK(u.vu.f(5, 0) == 2.0f);
}

void test_immediate() {
  Unit u;
  u.set(1, 4.0f, 0, 0, 0);
  Program p;
  p.loi(nop(), 2.5f);
  p.hi(muli(X, 2, 1));
  p.hi(addi(X, 3, 1));
  p.hi(subi(X, 4, 1));
  finish(p);
  u.run(p);
  CHECK(u.vu.f(2, 0) == 10.0f && u.vu.f(3, 0) == 6.5f && u.vu.f(4, 0) == 1.5f);
}

void test_flags_arrive_four_later() {
  Unit u;
  u.set(1, 5.0f, -1.0f, 0, 0);
  u.vu.vi[5] = 0xFFFF;
  Program p;
  p.hi(sub(XY, 3, 1, 1));     // 0: x and y zero: MAC zero flags for x (bit 3) and y (bit 2)
  p.lo(fmand(1, 5));          // 1
  p.lo(fmand(2, 5));          // 2
  p.lo(fmand(3, 5));          // 3
  p.lo(fmand(4, 5));          // 4: the first instruction that sees them
  p.hi(add(Y, 3, 1, 1));      // 5: -2: sign flag for y (bit 6), no zero flags
  p.hi(nop());
  p.hi(nop());
  p.hi(nop());
  p.lo(fmand(6, 5));          // 9
  p.lo(fsand(7, 0xFFF));      // 10: status: sign now, and "zero" and "sign" remembered
  finish(p);
  u.run(p);
  CHECK_EQ(u.vu.vi[1], 0u);
  CHECK_EQ(u.vu.vi[2], 0u);
  CHECK_EQ(u.vu.vi[3], 0u);
  CHECK_EQ(u.vu.vi[4], 0x000Cu);
  CHECK_EQ(u.vu.vi[6], 0x0040u);
  CHECK_EQ(u.vu.vi[7], 0x0002u | 0x0040u | 0x0080u);
}

void test_clip() {
  Unit u;
  u.set(1, 2.0f, -3.0f, 0.5f, 0.0f);
  u.set(2, 0, 0, 0, -1.0f);   // the bound is |w|
  Program p;
  p.hi(clip(1, 2));           // 0: +x and -y are outside: bits 0 and 3
  p.hi(nop());
  p.hi(nop());
  p.lo(fcand(0x000009));      // 3: not visible yet
  p.lo(iadd(3, 1, 0));        // 4: keep that answer in vi3
  p.lo(fcand(0x000009));      // 5: visible since instruction 4
  p.lo(iadd(4, 1, 0));
  p.hi(clip(2, 2));           // 7: inside on every side: shifts the old result up six bits
  p.hi(nop());
  p.hi(nop());
  p.hi(nop());
  p.lo(fcget(5));             // 11
  finish(p);
  u.run(p);
  CHECK_EQ(u.vu.vi[3], 0u);
  CHECK_EQ(u.vu.vi[4], 1u);
  CHECK_EQ(u.vu.vi[5], 0x09u << 6);
  CHECK_EQ(u.vu.clip, 0x09u << 6);
}

void test_divider() {
  Unit u;
  u.set(1, 4.0f, 9.0f, 0.0f, 0.0f);
  Program p;
  p.lo(div(0, 3, 1, 0));      // 0: Q = vf0.w / vf1.x = 0.25, ready for instruction 7
  for (int n = 1; n <= 5; n++) {
    p.hi(nop());
  }
  p.hi(addq(X, 2, 0));        // 6: still the old Q (0)
  p.hi(addq(X, 3, 0));        // 7: the new one
  p.lo(sqrt(1, 1));           // 8: Q = 3
  p.add(nop(), waitq());      // 9: wait for it
  p.hi(addq(X, 4, 0));        // 10
  p.lo(rsqrt(0, 3, 1, 0));    // 11: Q = 1 / sqrt(4)
  p.lo(waitq());
  p.hi(addq(X, 5, 0));
  p.lo(div(0, 3, 1, 2));      // 14: 1 / 0: the largest number, and the divide flag
  p.lo(waitq());
  p.hi(addq(X, 6, 0));
  p.lo(fsand(7, 0x030));
  finish(p);
  u.run(p);
  CHECK(u.vu.f(2, 0) == 0.0f);
  CHECK(u.vu.f(3, 0) == 0.25f);
  CHECK(u.vu.f(4, 0) == 3.0f);
  CHECK(u.vu.f(5, 0) == 0.5f);
  CHECK_EQ(u.vu.vf[6][0], 0x7FFFFFFFu);
  CHECK_EQ(u.vu.vi[7], 0x020u);
  CHECK_EQ(u.vu.status & 0x800, 0x800u);  // and it is remembered
}

void test_function_unit() {
  Unit u;
  u.set(1, 3.0f, 4.0f, 12.0f, 0.0f);
  Program p;
  p.lo(eleng(1));             // P = sqrt(9 + 16 + 144) = 13
  p.lo(mfp(X, 2));            // too early: the old P
  p.lo(waitp());
  p.lo(mfp(X, 3));
  p.lo(esadd(1));
  p.lo(waitp());
  p.lo(mfp(X, 4));
  finish(p);
  u.run(p);
  CHECK(u.vu.f(2, 0) == 0.0f);
  CHECK(u.vu.f(3, 0) == 13.0f);
  CHECK(u.vu.f(4, 0) == 169.0f);
}

void test_branches() {
  Unit u;
  Program p;
  p.lo(iaddiu(1, 0, 3));      // 0: three times round
  p.hi(nop());                // 1
  p.lo(iaddi(2, 2, 1));       // 2: loop
  p.lo(iaddi(1, 1, -1));      // 3
  p.hi(nop());                // 4
  p.lo(ibne(1, 0, 2 - 6));    // 5
  p.lo(iaddi(3, 3, 1));       // 6: the delay slot runs every time
  p.lo(bal(15, 2));           // 7: call 10
  p.hi(nop());                // 8: delay slot
  p.lo(b(3));                 // 9: (returned here) skip to 13
  p.lo(iaddiu(4, 0, 77));     // 10: delay slot of the b, and the subroutine's first instruction
  p.lo(jr(15));               // 11
  p.lo(iaddi(5, 5, 1));       // 12: delay slot of the jr
  finish(p);                  // 13, 14
  u64 ran = u.run(p);
  CHECK_EQ(u.vu.vi[2], 3u);
  CHECK_EQ(u.vu.vi[3], 3u);
  CHECK_EQ(u.vu.vi[15], 9u);
  CHECK_EQ(u.vu.vi[4], 77u);
  CHECK_EQ(u.vu.vi[5], 1u);
  CHECK_EQ(u.vu.pc, 15u);
  CHECK(ran < 40);
}

void test_branch_sees_the_older_integer() {
  // A conditional branch tests a register as it was before the instruction
  // right ahead of it.
  Unit u;
  Program p;
  p.lo(iaddiu(1, 0, 1));      // 0
  p.hi(nop());                // 1
  p.lo(iaddi(1, 1, -1));      // 2: vi1 becomes 0
  p.lo(ibne(1, 0, 2));        // 3: still tests 1: taken, to 6
  p.hi(nop());                // 4
  p.lo(iaddiu(2, 0, 7));      // 5: skipped
  p.lo(iaddi(3, 3, -1));      // 6
  p.hi(nop());                // 7: with an instruction between, the branch sees the new value
  p.lo(ibltz(3, 2));          // 8: taken, to 11
  p.hi(nop());                // 9
  p.lo(iaddiu(4, 0, 7));      // 10: skipped
  finish(p);
  u.run(p);
  CHECK_EQ(u.vu.vi[1], 0u);
  CHECK_EQ(u.vu.vi[2], 0u);
  CHECK_EQ(u.vu.vi[3], 0xFFFFu);
  CHECK_EQ(u.vu.vi[4], 0u);
}

void test_branch_after_a_flag_read() {
  // The instructions that read flags into an integer register are done in
  // time for a branch right after them (the games' clipping code tests two
  // flag reads this way).
  Unit u;
  u.vu.set_f(1, 3, -2.0f);
  Program p;
  p.hi(add(W, 2, 1, 1));      // 0: negative: the sign flag
  p.hi(nop());                // 1
  p.hi(nop());                // 2
  p.hi(nop());                // 3
  p.lo(fsand(5, 2));          // 4: vi5 = 2
  p.lo(ibne(5, 0, 2));        // 5: tests the 2 just read: taken, to 8
  p.hi(nop());                // 6
  p.lo(iaddiu(6, 0, 7));      // 7: skipped
  finish(p);
  u.run(p);
  CHECK_EQ(u.vu.vi[5], 2u);
  CHECK_EQ(u.vu.vi[6], 0u);
}

void test_branch_that_waits_sees_the_new_integer() {
  // A branch whose pair has to wait for a float register no longer tests the
  // old value: the write ahead of it has got through by then.
  Unit u;
  u.vu.set_f(1, 0, 1.0f);
  Program p;
  p.lo(iaddiu(1, 0, 1));                // 0
  p.hi(nop());                          // 1
  p.add(add(X, 2, 1, 1), iaddi(1, 1, -1));  // 2: vi1 becomes 0, vf2 on its way
  p.add(add(X, 3, 2, 2), ibne(1, 0, 2));    // 3: waits for vf2: tests 0, not taken
  p.hi(nop());                          // 4
  p.lo(iaddiu(4, 0, 7));                // 5: runs
  finish(p);
  u.run(p);
  CHECK_EQ(u.vu.vi[1], 0u);
  CHECK_EQ(u.vu.vi[4], 7u);
}

void test_memory_and_integers() {
  Unit u;
  for (u32 f = 0; f < 4; f++) {
    u.set_word(20, f, 0x100 + f);
    u.set_word(21, f, 0x200 + f);
  }
  u.vu.vf[7] = {0x11, 0x22, 0x33, 0x44};
  Program p;
  p.lo(iaddiu(1, 0, 20));
  p.lo(lqi(XYZW, 1, 1));          // vf1 = [20], vi1 = 21
  p.lo(lq(X | Z, 2, 0, 1));        // vf2.xz = [21]
  p.lo(mr32(XYZW, 3, 1));
  p.lo(iaddiu(2, 0, 32));
  p.lo(sqd(XYZW, 7, 2));          // vi2 = 31, [31] = vf7
  p.lo(sq(Y, 7, 3, 2));           // [34].y = vf7.y
  p.lo(ilw(Z, 3, -1, 1));         // vi3 = low half of [20].z
  p.lo(isw(X | W, 3, 5, 2));       // [36].x and .w = vi3
  p.lo(mtir(4, 7, 3));            // vi4 = vf7.w
  p.lo(isubiu(5, 0, 2));          // vi5 = -2
  p.lo(mfir(X, 4, 5));            // vf4.x = -2 as an integer
  p.lo(iand(6, 3, 4));
  p.lo(ior(7, 3, 4));
  p.lo(isub(8, 3, 4));
  p.lo(move(Y, 5, 7));
  finish(p);
  u.run(p);
  CHECK_EQ(u.vu.vf[1][2], 0x102u);
  CHECK_EQ(u.vu.vi[1], 21u);
  CHECK_EQ(u.vu.vf[2][0], 0x200u);
  CHECK_EQ(u.vu.vf[2][1], 0u);
  CHECK_EQ(u.vu.vf[2][2], 0x202u);
  CHECK(u.vu.vf[3] == (std::array<u32, 4>{0x101, 0x102, 0x103, 0x100}));
  CHECK_EQ(u.vu.vi[2], 31u);
  CHECK_EQ(u.word(31, 3), 0x44u);
  CHECK_EQ(u.word(34, 1), 0x22u);
  CHECK_EQ(u.word(34, 0), 0u);
  CHECK_EQ(u.vu.vi[3], 0x102u);
  CHECK_EQ(u.word(36, 0), 0x102u);
  CHECK_EQ(u.word(36, 3), 0x102u);
  CHECK_EQ(u.word(36, 1), 0u);
  CHECK_EQ(u.vu.vi[4], 0x44u);
  CHECK_EQ(u.vu.vf[4][0], 0xFFFFFFFEu);
  CHECK_EQ(u.vu.vi[6], 0x102u & 0x44u);
  CHECK_EQ(u.vu.vi[7], 0x102u | 0x44u);
  CHECK_EQ(u.vu.vi[8], 0x102u - 0x44u);
  CHECK_EQ(u.vu.vf[5][1], 0x22u);
  CHECK_EQ(u.vu.unknown_ops, u64{0});
}

void test_quad_arithmetic() {
  // The four-at-once arithmetic either declines or gives exactly what the
  // integer model gives, on ordinary numbers, zeros, numbers close together
  // and far apart, and the ends of the range.
  std::fesetround(FE_TOWARDZERO);
  u32 seed = 12345;
  auto next = [&seed] {
    seed = seed * 1664525u + 1013904223u;
    return seed;
  };
  auto value = [&]() -> u32 {
    u32 r = next();
    switch ((r >> 28) & 15) {
      case 0: return next() & 0x80000000u;                                // a zero
      case 1: return (next() & 0x807FFFFFu) | (((next() >> 8) % 3) << 23);   // tiny exponents
      case 2: return (next() & 0x807FFFFFu) | ((253 + (next() >> 8) % 3) << 23);  // huge exponents
      case 3: return next();                                              // anything
      default: return (next() & 0x807FFFFFu) | ((100 + (next() >> 8) % 60) << 23);  // ordinary, near each other
    }
  };
  u64 accepted = 0, wrong = 0;
  for (int n = 0; n < 200000; n++) {
    u32 a[4], b[4], out[4];
    for (int f = 0; f < 4; f++) {
      a[f] = value();
      b[f] = value();
    }
    u32 dest = (next() >> 20) & 15;
    for (int op = 0; op < 3; op++) {
      bool ok = op == 0 ? fp::quad_add(a, b, dest, out) : op == 1 ? fp::quad_add(a, b, dest, out, true) : fp::quad_mul(a, b, dest, out);
      if (!ok) {
        continue;
      }
      accepted++;
      for (unsigned f = 0; f < 4; f++) {
        if (!(dest & (8u >> f))) {
          continue;
        }
        u32 problems = 0;
        u32 want = op == 0 ? fp::add(a[f], b[f], problems) : op == 1 ? fp::sub(a[f], b[f], problems) : fp::mul(a[f], b[f], problems);
        if (want != out[f] || problems) {
          wrong++;
        }
      }
    }
  }
  std::fesetround(FE_TONEAREST);
  CHECK_EQ(wrong, u64{0});
#if OPENRAC_FP_QUAD
  CHECK(accepted > 100000);  // and it does take most of them
#endif
}

// --- with the rest of the machine ----------------------------------------------

std::vector<u8> bytes_of(const std::vector<u32>& words) {
  std::vector<u8> out(words.size() * 4);
  std::memcpy(out.data(), words.data(), out.size());
  return out;
}

u32 vif(u32 cmd, u32 num, u32 imm) {
  return (cmd << 24) | (num << 16) | imm;
}

void test_kick_through_vif() {
  // A display list loads a program and a GIF packet, sets the buffer base and
  // starts the program. The program finds the packet through XTOP, kicks it,
  // and writes the packet's colour in the instruction after the kick, which
  // the kick still picks up.
  Graphics g;
  const u32 offset = 2048;
  g.gs.write(gsreg::PRMODECONT, 1);
  g.gs.write(gsreg::FRAME_1, u64{1} << 16);
  g.gs.write(gsreg::XYOFFSET_1, (offset * 16) | (u64{offset * 16} << 32));
  g.gs.write(gsreg::SCISSOR_1, (u64{63} << 16) | (u64{63} << 48));

  Program p;
  p.lo(xtop(1));                 // vi1 = where the list put the packet
  p.lo(lq(XYZW, 1, 4, 1));       // the colour, stored after the packet
  p.lo(xgkick(1));
  p.lo(sq(XYZW, 1, 1, 1));       // the packet's second quadword
  finish(p);

  const u64 x0 = (offset + 2) * 16, y0 = (offset + 2) * 16, x1 = (offset + 5) * 16, y1 = (offset + 4) * 16;
  std::vector<u32> list;
  list.push_back(vif(0x4A, static_cast<u32>(p.words().size() / 2), 0));  // MPG at 0
  list.insert(list.end(), p.words().begin(), p.words().end());
  list.push_back(vif(0x03, 0, 100));     // BASE
  list.push_back(vif(0x02, 0, 0));       // OFFSET: TOPS = BASE
  list.push_back(vif(0x01, 0, 0x0101));  // STCYCL
  list.push_back(vif(0x6C, 5, 0x8000));  // UNPACK V4-32, five quadwords, at TOPS
  const u64 packet[] = {
      1 | (u64{1} << 15) | (u64{1} << 46) | (u64{6} << 47) | (u64{3} << 60), 0x551,  // sprite: RGBAQ, XYZ2, XYZ2
      0, 0,                                                                          // (the program fills this in)
      x0 | (y0 << 32), 0,
      x1 | (y1 << 32), 0,
      0x21 | (u64{0x43} << 32), 0x65 | (u64{0x87} << 32),                            // the colour, one component a word
  };
  for (u64 quad : packet) {
    list.push_back(static_cast<u32>(quad));
    list.push_back(static_cast<u32>(quad >> 32));
  }
  list.push_back(vif(0x14, 0, 0));       // MSCAL 0
  std::vector<u8> bytes = bytes_of(list);
  g.vif.write(bytes.data(), bytes.size());

  CHECK_EQ(g.vu1.vi[1], 100u);
  CHECK(g.vu1.stopped());
  CHECK_EQ(g.vu1_instructions, u64{6});
  CHECK_EQ(g.gs.stats.pixels, u64{6});
  CHECK_EQ(g.gs.memory.read(PSMCT32, 0, 1, 2, 2), 0x87654321u);
  CHECK_EQ(g.gs.memory.read(PSMCT32, 0, 1, 4, 3), 0x87654321u);
  CHECK_EQ(g.gs.memory.read(PSMCT32, 0, 1, 5, 3), 0u);
  CHECK_EQ(g.vif.unknown_codes, u64{0});
  CHECK_EQ(g.vu1.unknown_ops, u64{0});
}

}  // namespace

int main() {
  const TestCase tests[] = {
      {"arithmetic and masks", test_arithmetic_and_masks},
      {"cross product", test_cross_product},
      {"numbers", test_numbers},
      {"conversions", test_conversions},
      {"a pair reads old values", test_pair_reads_old_values},
      {"immediate", test_immediate},
      {"flags arrive four later", test_flags_arrive_four_later},
      {"clip", test_clip},
      {"divider", test_divider},
      {"function unit", test_function_unit},
      {"branches", test_branches},
      {"a branch sees the older integer", test_branch_sees_the_older_integer},
      {"a branch after a flag read", test_branch_after_a_flag_read},
      {"a branch that waits sees the new integer", test_branch_that_waits_sees_the_new_integer},
      {"memory and integers", test_memory_and_integers},
      {"four-field arithmetic", test_quad_arithmetic},
      {"kick through vif", test_kick_through_vif},
  };
  return run_tests(tests);
}
