// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

#include <array>
#include <functional>

#include "types.h"

namespace ps2 {

// A vector unit running microprograms: VU1 behind VIF1, or VU0 when the EE
// starts a microprogram on it. It is an interpreter that keeps the timing
// microprograms depend on: the upper and lower instruction of a pair see the
// same state, flags appear four instructions after the one that set them,
// Q and P arrive when their dividers and function units finish, a branch
// tests the value an integer register had before the instruction just ahead
// of it, and XGKICK sends its packet one instruction late.
class Vu {
 public:
  struct Memory {
    u8* micro = nullptr;  // program memory, 8 bytes an instruction
    u32 micro_bytes = 0;  // a power of two: 16 KB for VU1, 4 KB for VU0
    u8* data = nullptr;   // data memory, 16 bytes a quadword
    u32 data_bytes = 0;
  };

  explicit Vu(Memory memory);

  void reset();

  // Run from instruction `address` until an instruction with the E bit has
  // finished (with the one after it), or `limit` instructions have run.
  // Returns how many ran. `resume` continues after the last stop.
  u64 run(u32 address, u64 limit = u64{1} << 26);
  u64 resume(u64 limit = u64{1} << 26);

  bool stopped() const { return !running_; }

  // XGKICK: a GIF packet starts at this quadword of data memory.
  std::function<void(u32 quadword)> on_kick;
  // XTOP and XITOP read VIF registers.
  std::function<u32()> on_top, on_itop;

  // Registers. A float register is four raw 32-bit values, x first, so that
  // integers moved through them survive. VF0 is (0, 0, 0, 1) and VI0 is 0.
  std::array<std::array<u32, 4>, 32> vf{};
  std::array<u16, 16> vi{};
  std::array<u32, 4> acc{};
  u32 q = 0, p = 0, i = 0, r = 0;
  u32 mac = 0, status = 0, clip = 0;  // as instructions read them now
  u32 pc = 0;                         // in instructions

  float f(unsigned reg, unsigned field) const { return as_float(vf[reg][field]); }
  void set_f(unsigned reg, unsigned field, float value) { vf[reg][field] = as_u32(value); }

  u64 unknown_ops = 0;  // instruction words that decode to nothing

 private:
  struct Flags {
    u32 mac = 0, status = 0, clip = 0;
    bool valid = false;
  };
  enum class Op { Add, Sub, Mul, Madd, Msub };
  enum class From { Ft, Bc, Q, I };

  void step();
  void upper(u32 code);
  void lower(u32 code, u32 at);
  void upper_special(u32 code);
  void lower_special(u32 code);

  void arith(u32 code, Op op, From from, bool to_acc);
  void min_max(u32 code, From from, bool max);
  float operand(u32 code, From from, unsigned field) const;
  u32 result(double value, unsigned field, u32& flags) const;
  void post_flags(u32 mac_bits);
  void post();
  void finish_q();
  void fire_kick();

  void write_vf(unsigned reg, u32 mask, const std::array<u32, 4>& value);
  void write_vi(unsigned reg, u16 value);
  u16 branch_vi(unsigned reg) const;
  void branch(u32 target);
  void start_q(u32 value, unsigned latency, u32 divide_flags);
  void start_p(double value, unsigned latency);

  u8* quad(u32 address) { return memory_.data + ((address * 16) & (memory_.data_bytes - 1)); }

  Memory memory_;
  u32 pc_mask_ = 0;
  bool running_ = false;

  // The upper instruction's register write, held back while the lower one runs.
  bool in_upper_ = false;
  unsigned upper_reg_ = 0;
  std::array<u32, 4> upper_old_{};

  std::array<Flags, 4> flag_pipe_{};
  u32 mac_latest_ = 0, status_latest_ = 0, clip_latest_ = 0;  // the newest values, visible or not

  u32 q_next_ = 0, q_flags_ = 0, p_next_ = 0;
  unsigned q_wait_ = 0, p_wait_ = 0;

  unsigned branch_in_ = 0, stop_in_ = 0, kick_in_ = 0;
  u32 branch_target_ = 0, kick_address_ = 0;

  unsigned backup_reg_ = 0, backup_ttl_ = 0;
  u16 backup_value_ = 0;
};

}  // namespace ps2
