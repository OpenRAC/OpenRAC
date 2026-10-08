// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

#include <vector>

#include "types.h"

// Encoders for vector unit instructions, for tests and for programs written
// here. An instruction is a pair: `Program::add(upper, lower)` stores the
// lower word first, as program memory does.
namespace ps2::vuasm {

// Field masks (the `dest` of an instruction).
constexpr u32 X = 8, Y = 4, Z = 2, W = 1, XY = 12, XYZ = 14, XYZW = 15;
// Bits of the upper word.
constexpr u32 E = 0x40000000u, I = 0x80000000u;

// --- upper -------------------------------------------------------------------

constexpr u32 up(u32 op, u32 dest, u32 fd, u32 fs, u32 ft) {
  return (dest << 21) | (ft << 16) | (fs << 11) | (fd << 6) | op;
}
constexpr u32 up_special(u32 index, u32 dest, u32 fs, u32 ft) {
  return (dest << 21) | (ft << 16) | (fs << 11) | ((index >> 2) << 6) | 0x3C | (index & 3);
}

constexpr u32 add(u32 dest, u32 fd, u32 fs, u32 ft) { return up(0x28, dest, fd, fs, ft); }
constexpr u32 madd(u32 dest, u32 fd, u32 fs, u32 ft) { return up(0x29, dest, fd, fs, ft); }
constexpr u32 mul(u32 dest, u32 fd, u32 fs, u32 ft) { return up(0x2A, dest, fd, fs, ft); }
constexpr u32 max(u32 dest, u32 fd, u32 fs, u32 ft) { return up(0x2B, dest, fd, fs, ft); }
constexpr u32 sub(u32 dest, u32 fd, u32 fs, u32 ft) { return up(0x2C, dest, fd, fs, ft); }
constexpr u32 msub(u32 dest, u32 fd, u32 fs, u32 ft) { return up(0x2D, dest, fd, fs, ft); }
constexpr u32 opmsub(u32 fd, u32 fs, u32 ft) { return up(0x2E, XYZ, fd, fs, ft); }
constexpr u32 mini(u32 dest, u32 fd, u32 fs, u32 ft) { return up(0x2F, dest, fd, fs, ft); }
// The second operand is one field (`bc`: 0 x, 1 y, 2 z, 3 w) of ft.
constexpr u32 addbc(u32 dest, u32 fd, u32 fs, u32 ft, u32 bc) { return up(0x00 + bc, dest, fd, fs, ft); }
constexpr u32 subbc(u32 dest, u32 fd, u32 fs, u32 ft, u32 bc) { return up(0x04 + bc, dest, fd, fs, ft); }
constexpr u32 maddbc(u32 dest, u32 fd, u32 fs, u32 ft, u32 bc) { return up(0x08 + bc, dest, fd, fs, ft); }
constexpr u32 msubbc(u32 dest, u32 fd, u32 fs, u32 ft, u32 bc) { return up(0x0C + bc, dest, fd, fs, ft); }
constexpr u32 maxbc(u32 dest, u32 fd, u32 fs, u32 ft, u32 bc) { return up(0x10 + bc, dest, fd, fs, ft); }
constexpr u32 minibc(u32 dest, u32 fd, u32 fs, u32 ft, u32 bc) { return up(0x14 + bc, dest, fd, fs, ft); }
constexpr u32 mulbc(u32 dest, u32 fd, u32 fs, u32 ft, u32 bc) { return up(0x18 + bc, dest, fd, fs, ft); }
// The second operand is Q or I.
constexpr u32 mulq(u32 dest, u32 fd, u32 fs) { return up(0x1C, dest, fd, fs, 0); }
constexpr u32 muli(u32 dest, u32 fd, u32 fs) { return up(0x1E, dest, fd, fs, 0); }
constexpr u32 addq(u32 dest, u32 fd, u32 fs) { return up(0x20, dest, fd, fs, 0); }
constexpr u32 addi(u32 dest, u32 fd, u32 fs) { return up(0x22, dest, fd, fs, 0); }
constexpr u32 subq(u32 dest, u32 fd, u32 fs) { return up(0x24, dest, fd, fs, 0); }
constexpr u32 subi(u32 dest, u32 fd, u32 fs) { return up(0x26, dest, fd, fs, 0); }
// Into the accumulator.
constexpr u32 adda(u32 dest, u32 fs, u32 ft) { return up_special(0x28, dest, fs, ft); }
constexpr u32 madda(u32 dest, u32 fs, u32 ft) { return up_special(0x29, dest, fs, ft); }
constexpr u32 mula(u32 dest, u32 fs, u32 ft) { return up_special(0x2A, dest, fs, ft); }
constexpr u32 suba(u32 dest, u32 fs, u32 ft) { return up_special(0x2C, dest, fs, ft); }
constexpr u32 msuba(u32 dest, u32 fs, u32 ft) { return up_special(0x2D, dest, fs, ft); }
constexpr u32 opmula(u32 fs, u32 ft) { return up_special(0x2E, XYZ, fs, ft); }
constexpr u32 mulabc(u32 dest, u32 fs, u32 ft, u32 bc) { return up_special(0x18 + bc, dest, fs, ft); }
constexpr u32 maddabc(u32 dest, u32 fs, u32 ft, u32 bc) { return up_special(0x08 + bc, dest, fs, ft); }
// Conversions and the rest; these write ft.
constexpr u32 itof0(u32 dest, u32 ft, u32 fs) { return up_special(0x10, dest, fs, ft); }
constexpr u32 itof4(u32 dest, u32 ft, u32 fs) { return up_special(0x11, dest, fs, ft); }
constexpr u32 itof12(u32 dest, u32 ft, u32 fs) { return up_special(0x12, dest, fs, ft); }
constexpr u32 itof15(u32 dest, u32 ft, u32 fs) { return up_special(0x13, dest, fs, ft); }
constexpr u32 ftoi0(u32 dest, u32 ft, u32 fs) { return up_special(0x14, dest, fs, ft); }
constexpr u32 ftoi4(u32 dest, u32 ft, u32 fs) { return up_special(0x15, dest, fs, ft); }
constexpr u32 ftoi12(u32 dest, u32 ft, u32 fs) { return up_special(0x16, dest, fs, ft); }
constexpr u32 ftoi15(u32 dest, u32 ft, u32 fs) { return up_special(0x17, dest, fs, ft); }
constexpr u32 abs(u32 dest, u32 ft, u32 fs) { return up_special(0x1D, dest, fs, ft); }
constexpr u32 clip(u32 fs, u32 ft) { return up_special(0x1F, XYZ, fs, ft); }
constexpr u32 nop() { return up_special(0x2F, 0, 0, 0); }

// --- lower -------------------------------------------------------------------

constexpr u32 low(u32 op, u32 dest, u32 it, u32 is, u32 imm11) {
  return (op << 25) | (dest << 21) | (it << 16) | (is << 11) | (imm11 & 0x7FF);
}
constexpr u32 low_special(u32 index, u32 dest, u32 it, u32 is) {
  return (0x40u << 25) | (dest << 21) | (it << 16) | (is << 11) | ((index >> 2) << 6) | 0x3C | (index & 3);
}

constexpr u32 lq(u32 dest, u32 ft, s32 offset, u32 is) { return low(0x00, dest, ft, is, static_cast<u32>(offset)); }
constexpr u32 sq(u32 dest, u32 fs, s32 offset, u32 it) { return low(0x01, dest, it, fs, static_cast<u32>(offset)); }
constexpr u32 ilw(u32 dest, u32 it, s32 offset, u32 is) { return low(0x04, dest, it, is, static_cast<u32>(offset)); }
constexpr u32 isw(u32 dest, u32 it, s32 offset, u32 is) { return low(0x05, dest, it, is, static_cast<u32>(offset)); }
constexpr u32 iaddiu(u32 it, u32 is, u32 imm15) {
  return (0x08u << 25) | ((imm15 & 0x7800) << 10) | (it << 16) | (is << 11) | (imm15 & 0x7FF);
}
constexpr u32 isubiu(u32 it, u32 is, u32 imm15) {
  return (0x09u << 25) | ((imm15 & 0x7800) << 10) | (it << 16) | (is << 11) | (imm15 & 0x7FF);
}
constexpr u32 fceq(u32 imm24) { return (0x10u << 25) | imm24; }
constexpr u32 fcset(u32 imm24) { return (0x11u << 25) | imm24; }
constexpr u32 fcand(u32 imm24) { return (0x12u << 25) | imm24; }
constexpr u32 fcor(u32 imm24) { return (0x13u << 25) | imm24; }
constexpr u32 fsand(u32 it, u32 imm12) { return (0x16u << 25) | ((imm12 & 0x800) << 10) | (it << 16) | (imm12 & 0x7FF); }
constexpr u32 fmand(u32 it, u32 is) { return low(0x1A, 0, it, is, 0); }
constexpr u32 fcget(u32 it) { return low(0x1C, 0, it, 0, 0); }
// Branch offsets count instructions from the one after the branch.
constexpr u32 b(s32 offset) { return low(0x20, 0, 0, 0, static_cast<u32>(offset)); }
constexpr u32 bal(u32 it, s32 offset) { return low(0x21, 0, it, 0, static_cast<u32>(offset)); }
constexpr u32 jr(u32 is) { return low(0x24, 0, 0, is, 0); }
constexpr u32 jalr(u32 it, u32 is) { return low(0x25, 0, it, is, 0); }
constexpr u32 ibeq(u32 it, u32 is, s32 offset) { return low(0x28, 0, it, is, static_cast<u32>(offset)); }
constexpr u32 ibne(u32 it, u32 is, s32 offset) { return low(0x29, 0, it, is, static_cast<u32>(offset)); }
constexpr u32 ibltz(u32 is, s32 offset) { return low(0x2C, 0, 0, is, static_cast<u32>(offset)); }
constexpr u32 ibgtz(u32 is, s32 offset) { return low(0x2D, 0, 0, is, static_cast<u32>(offset)); }
constexpr u32 iblez(u32 is, s32 offset) { return low(0x2E, 0, 0, is, static_cast<u32>(offset)); }
constexpr u32 ibgez(u32 is, s32 offset) { return low(0x2F, 0, 0, is, static_cast<u32>(offset)); }

constexpr u32 iadd(u32 id, u32 is, u32 it) { return (0x40u << 25) | (it << 16) | (is << 11) | (id << 6) | 0x30; }
constexpr u32 isub(u32 id, u32 is, u32 it) { return (0x40u << 25) | (it << 16) | (is << 11) | (id << 6) | 0x31; }
constexpr u32 iaddi(u32 it, u32 is, s32 imm5) {
  return (0x40u << 25) | (it << 16) | (is << 11) | ((static_cast<u32>(imm5) & 0x1F) << 6) | 0x32;
}
constexpr u32 iand(u32 id, u32 is, u32 it) { return (0x40u << 25) | (it << 16) | (is << 11) | (id << 6) | 0x34; }
constexpr u32 ior(u32 id, u32 is, u32 it) { return (0x40u << 25) | (it << 16) | (is << 11) | (id << 6) | 0x35; }

constexpr u32 move(u32 dest, u32 ft, u32 fs) { return low_special(0x30, dest, ft, fs); }
constexpr u32 mr32(u32 dest, u32 ft, u32 fs) { return low_special(0x31, dest, ft, fs); }
constexpr u32 lqi(u32 dest, u32 ft, u32 is) { return low_special(0x34, dest, ft, is); }
constexpr u32 sqi(u32 dest, u32 fs, u32 it) { return low_special(0x35, dest, it, fs); }
constexpr u32 lqd(u32 dest, u32 ft, u32 is) { return low_special(0x36, dest, ft, is); }
constexpr u32 sqd(u32 dest, u32 fs, u32 it) { return low_special(0x37, dest, it, fs); }
// One field of each operand (`fsf`, `ftf`: 0 x, 1 y, 2 z, 3 w).
constexpr u32 div(u32 fs, u32 fsf, u32 ft, u32 ftf) { return low_special(0x38, (ftf << 2) | fsf, ft, fs); }
constexpr u32 sqrt(u32 ft, u32 ftf) { return low_special(0x39, ftf << 2, ft, 0); }
constexpr u32 rsqrt(u32 fs, u32 fsf, u32 ft, u32 ftf) { return low_special(0x3A, (ftf << 2) | fsf, ft, fs); }
constexpr u32 waitq() { return low_special(0x3B, 0, 0, 0); }
constexpr u32 mtir(u32 it, u32 fs, u32 fsf) { return low_special(0x3C, fsf, it, fs); }
constexpr u32 mfir(u32 dest, u32 ft, u32 is) { return low_special(0x3D, dest, ft, is); }
constexpr u32 ilwr(u32 dest, u32 it, u32 is) { return low_special(0x3E, dest, it, is); }
constexpr u32 iswr(u32 dest, u32 it, u32 is) { return low_special(0x3F, dest, it, is); }
constexpr u32 mfp(u32 dest, u32 ft) { return low_special(0x64, dest, ft, 0); }
constexpr u32 xtop(u32 it) { return low_special(0x68, 0, it, 0); }
constexpr u32 xitop(u32 it) { return low_special(0x69, 0, it, 0); }
constexpr u32 xgkick(u32 is) { return low_special(0x6C, 0, 0, is); }
constexpr u32 esadd(u32 fs) { return low_special(0x70, 0, 0, fs); }
constexpr u32 eleng(u32 fs) { return low_special(0x72, 0, 0, fs); }
constexpr u32 waitp() { return low_special(0x7B, 0, 0, 0); }
// The lower half of a pair that only has an upper instruction.
constexpr u32 lnop() { return move(0, 0, 0); }

// A program being written: pairs in order, with the address of the next one.
class Program {
 public:
  u32 here() const { return static_cast<u32>(words_.size() / 2); }
  void add(u32 upper, u32 lower) {
    words_.push_back(lower);
    words_.push_back(upper);
  }
  // Only a lower instruction, only an upper one, or a number for I.
  void lo(u32 lower) { add(nop(), lower); }
  void hi(u32 upper) { add(upper, lnop()); }
  void loi(u32 upper, float value) { add(upper | I, as_u32(value)); }
  // Replace the lower word at `address` (to fill in a forward branch).
  void patch(u32 address, u32 lower) { words_[address * 2] = lower; }
  void end() { words_[words_.size() - 3] |= E; }  // E on the pair before last
  const std::vector<u32>& words() const { return words_; }

 private:
  std::vector<u32> words_;
};

}  // namespace ps2::vuasm
