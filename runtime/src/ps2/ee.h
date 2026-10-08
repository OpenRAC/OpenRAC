// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

#include <array>
#include <functional>

#include "memory.h"
#include "types.h"
#include "vu.h"

namespace ps2 {

// The Emotion Engine's CPU core as the games' code needs it: the MIPS III
// instruction set with the EE's additions (128-bit registers and the
// multimedia instructions, a second multiply unit, three-operand multiply),
// the single-precision FPU, and the vector instructions that run on VU0.
//
// It is an interpreter over guest memory. It has no kernel, no TLB and no
// exception vectors: a SYSCALL goes to `on_syscall`, an access outside main
// memory and the scratchpad goes to the hardware callbacks, and interrupts
// are delivered by whoever owns the core calling `call()` on a handler.
class Ee {
 public:
  struct Reg {
    u64 lo = 0, hi = 0;
  };

  Ee(GuestMemory& memory, Vu& vu0);

  void reset();

  // Run until `cycles` reaches `until`, a stop is asked for, or the program
  // returns to the address `call()` set up.
  void run(u64 until);
  // Call a guest function with up to four integer arguments and run it to
  // its return, leaving every register as it was. For interrupt handlers and
  // callbacks.
  u64 call(u32 function, u64 a0 = 0, u64 a1 = 0, u64 a2 = 0, u64 a3 = 0);
  void stop() { stop_ = true; }

  // --- state ---
  std::array<Reg, 32> gpr{};
  u64 hi = 0, lo = 0, hi1 = 0, lo1 = 0;
  u32 pc = 0, next_pc = 4;
  u32 sa = 0;
  std::array<u32, 32> fpr{};
  u32 facc = 0, fcr31 = 0;
  std::array<u32, 32> cop0{};
  u64 cycles = 0;

  // --- what the core hands out ---
  // SYSCALL. `code` is the instruction's 20-bit field.
  std::function<void(u32 code)> on_syscall;
  // Reads and writes outside memory, by physical address, of 1, 2, 4 or 8 bytes.
  std::function<u64(u32 address, unsigned bytes)> on_read;
  std::function<void(u32 address, u64 value, unsigned bytes)> on_write;
  // A quadword written outside memory (the FIFOs).
  std::function<void(u32 address, u64 lo, u64 hi)> on_write128;
  // Called when `cycles` reaches `event_at`: timers, the display's timing,
  // interrupts. The callee sets the next `event_at`.
  std::function<void()> on_event;
  u64 event_at = ~u64{0};

  // --- memory, as the program sees it ---
  u8 read8(u32 address);
  u16 read16(u32 address);
  u32 read32(u32 address);
  u64 read64(u32 address);
  void write8(u32 address, u8 value);
  void write16(u32 address, u16 value);
  void write32(u32 address, u32 value);
  void write64(u32 address, u64 value);
  // A pointer into main memory or the scratchpad, or null for anything else.
  u8* pointer(u32 address);

  // Integer and float argument and result registers, by their ABI names.
  u64& a(unsigned n) { return gpr[4 + n].lo; }
  u64& v0() { return gpr[2].lo; }
  u32& sp32() { return *reinterpret_cast<u32*>(&gpr[29].lo); }
  void set_result(s64 value) { gpr[2].lo = static_cast<u64>(value); }
  // Return from the function the program is at the entry of.
  void leave() {
    pc = static_cast<u32>(gpr[31].lo);
    next_pc = pc + 4;
  }

  u64 unknown = 0;         // instructions the core does not know
  u32 last_unknown_pc = 0, last_unknown = 0;

  u64 vu0_runaways = 0;    // VU0 microprograms that did not stop
  u32 vu0_runaway_start = 0, vu0_runaway_from = 0;

  // The address a `call()` returns to. Nothing is mapped there.
  static constexpr u32 kReturnAddress = 0x1FC00FF0;

 private:
  void step();
  void special(u32 op);
  void regimm(u32 op, u32 at);
  void mmi(u32 op);
  void mmi0(u32 op);
  void mmi1(u32 op);
  void mmi2(u32 op);
  void mmi3(u32 op);
  void cop0_op(u32 op);
  void cop1_op(u32 op, u32 at);
  void cop2_op(u32 op, u32 at);
  void vu0_sync();
  void vu0_finish(u32 at);
  void fpu_flags(u32 problems);
  void not_known(u32 op, u32 at);

  void branch(bool taken, u32 at, s32 offset, bool likely);
  void set32(unsigned reg, u32 value) {
    if (reg) {
      gpr[reg].lo = static_cast<u64>(static_cast<s64>(static_cast<s32>(value)));
    }
  }
  void set64(unsigned reg, u64 value) {
    if (reg) {
      gpr[reg].lo = value;
    }
  }

  GuestMemory& memory_;
  Vu& vu0_;
  bool stop_ = false, returned_ = false;
  u64 vu0_cycles_ = 0;
  u32 vu0_started_at_ = 0;
};

}  // namespace ps2
