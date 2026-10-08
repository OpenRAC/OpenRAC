// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "ee.h"

#include "fp.h"

namespace ps2 {
namespace {

constexpr u64 kCyclesPerInstruction = 2;

inline unsigned rs_of(u32 op) { return (op >> 21) & 31; }
inline unsigned rt_of(u32 op) { return (op >> 16) & 31; }
inline unsigned rd_of(u32 op) { return (op >> 11) & 31; }
inline unsigned sa_of(u32 op) { return (op >> 6) & 31; }
inline s32 imm_of(u32 op) { return static_cast<s16>(op & 0xFFFF); }

inline u64 sext32(u32 v) { return static_cast<u64>(static_cast<s64>(static_cast<s32>(v))); }

// FCR31 bits.
constexpr u32 kC = 1u << 23, kI = 1u << 17, kD = 1u << 16, kO = 1u << 15, kU = 1u << 14;
constexpr u32 kSI = 1u << 6, kSD = 1u << 5, kSO = 1u << 4, kSU = 1u << 3;

}  // namespace

Ee::Ee(GuestMemory& memory, Vu& vu0) : memory_(memory), vu0_(vu0) {
  reset();
}

void Ee::reset() {
  gpr = {};
  hi = lo = hi1 = lo1 = 0;
  pc = 0;
  next_pc = 4;
  sa = 0;
  fpr = {};
  facc = 0;
  fcr31 = 0x01000001;
  cop0 = {};
  cop0[12] = 0x70030C13;  // Status as a program finds it: interrupts on
  cop0[15] = 0x2E20;      // PRId
  cycles = 0;
  stop_ = returned_ = false;
}

// --- memory ------------------------------------------------------------------

u8* Ee::pointer(u32 address) {
  if ((address >> 28) == 7) {
    return memory_.scratchpad(address);
  }
  u32 physical = address & 0x1FFFFFFFu;
  if (physical < GuestMemory::kRamBytes) {
    return memory_.ram(physical);
  }
  return nullptr;
}

u8 Ee::read8(u32 address) {
  if (u8* p = pointer(address)) return *p;
  return on_read ? static_cast<u8>(on_read(address & 0x1FFFFFFFu, 1)) : 0;
}
u16 Ee::read16(u32 address) {
  if (u8* p = pointer(address)) return load<u16>(p);
  return on_read ? static_cast<u16>(on_read(address & 0x1FFFFFFFu, 2)) : 0;
}
u32 Ee::read32(u32 address) {
  if (u8* p = pointer(address)) return load<u32>(p);
  return on_read ? static_cast<u32>(on_read(address & 0x1FFFFFFFu, 4)) : 0;
}
u64 Ee::read64(u32 address) {
  if (u8* p = pointer(address)) return load<u64>(p);
  return on_read ? on_read(address & 0x1FFFFFFFu, 8) : 0;
}
void Ee::write8(u32 address, u8 value) {
  if (u8* p = pointer(address)) {
    *p = value;
  } else if (on_write) {
    on_write(address & 0x1FFFFFFFu, value, 1);
  }
}
void Ee::write16(u32 address, u16 value) {
  if (u8* p = pointer(address)) {
    store<u16>(p, value);
  } else if (on_write) {
    on_write(address & 0x1FFFFFFFu, value, 2);
  }
}
void Ee::write32(u32 address, u32 value) {
  if (u8* p = pointer(address)) {
    store<u32>(p, value);
  } else if (on_write) {
    on_write(address & 0x1FFFFFFFu, value, 4);
  }
}
void Ee::write64(u32 address, u64 value) {
  if (u8* p = pointer(address)) {
    store<u64>(p, value);
  } else if (on_write) {
    on_write(address & 0x1FFFFFFFu, value, 8);
  }
}

// --- running -----------------------------------------------------------------

void Ee::run(u64 until) {
  stop_ = false;
  while (!stop_ && !returned_) {
    if (cycles >= event_at) {
      if (on_event) {
        on_event();
      } else {
        event_at = ~u64{0};
      }
      continue;
    }
    if (cycles >= until) {
      break;
    }
    u64 limit = until < event_at ? until : event_at;
    while (cycles < limit && !stop_ && !returned_) {
      step();
    }
  }
}

u64 Ee::call(u32 function, u64 a0, u64 a1, u64 a2, u64 a3) {
  struct Saved {
    std::array<Reg, 32> gpr;
    u64 hi, lo, hi1, lo1;
    u32 pc, next_pc, sa, facc, fcr31;
    std::array<u32, 32> fpr;
    bool stop, returned;
  } saved{gpr, hi, lo, hi1, lo1, pc, next_pc, sa, facc, fcr31, fpr, stop_, returned_};

  gpr[4].lo = a0;
  gpr[5].lo = a1;
  gpr[6].lo = a2;
  gpr[7].lo = a3;
  gpr[31].lo = kReturnAddress;
  gpr[29].lo = sext32((static_cast<u32>(gpr[29].lo) - 0x400) & ~0xFu);
  pc = function;
  next_pc = function + 4;
  returned_ = false;
  // A handler that never returns is a bug in the model; give it a generous
  // number of instructions and then say so.
  u64 guard = u64{1} << 28;
  while (!returned_ && guard--) {
    step();
  }
  if (!returned_) {
    unknown++;
    last_unknown_pc = function;
    last_unknown = 0xCA11CA11;
  }
  u64 result = gpr[2].lo;

  gpr = saved.gpr;
  hi = saved.hi;
  lo = saved.lo;
  hi1 = saved.hi1;
  lo1 = saved.lo1;
  pc = saved.pc;
  next_pc = saved.next_pc;
  sa = saved.sa;
  facc = saved.facc;
  fcr31 = saved.fcr31;
  fpr = saved.fpr;
  stop_ = saved.stop;
  returned_ = saved.returned;
  return result;
}

void Ee::not_known(u32 op, u32 at) {
  unknown++;
  last_unknown = op;
  last_unknown_pc = at;
}

void Ee::branch(bool taken, u32 at, s32 offset, bool likely) {
  if (taken) {
    next_pc = at + 4 + static_cast<u32>(offset << 2);
  } else if (likely) {
    // The instruction in the delay slot is not run.
    pc = next_pc;
    next_pc += 4;
  }
}

void Ee::step() {
  u32 at = pc;
  if (at == kReturnAddress) {
    returned_ = true;
    return;
  }
  u32 op;
  if (const u8* p = pointer(at)) {
    op = load<u32>(p);
  } else {
    not_known(0, at);
    stop_ = true;
    return;
  }
  pc = next_pc;
  next_pc += 4;
  cycles += kCyclesPerInstruction;

  unsigned rs = rs_of(op), rt = rt_of(op);
  s32 imm = imm_of(op);
  u32 address = static_cast<u32>(gpr[rs].lo) + static_cast<u32>(imm);

  switch (op >> 26) {
    case 0x00:
      special(op);
      break;
    case 0x01:
      regimm(op, at);
      break;
    case 0x02:  // J
      next_pc = ((at + 4) & 0xF0000000u) | ((op & 0x03FFFFFFu) << 2);
      break;
    case 0x03:  // JAL
      gpr[31].lo = at + 8;
      next_pc = ((at + 4) & 0xF0000000u) | ((op & 0x03FFFFFFu) << 2);
      break;
    case 0x04: branch(gpr[rs].lo == gpr[rt].lo, at, imm, false); break;                  // BEQ
    case 0x05: branch(gpr[rs].lo != gpr[rt].lo, at, imm, false); break;                  // BNE
    case 0x06: branch(static_cast<s64>(gpr[rs].lo) <= 0, at, imm, false); break;         // BLEZ
    case 0x07: branch(static_cast<s64>(gpr[rs].lo) > 0, at, imm, false); break;          // BGTZ
    case 0x08:                                                                           // ADDI
    case 0x09: set32(rt, static_cast<u32>(gpr[rs].lo) + static_cast<u32>(imm)); break;   // ADDIU
    case 0x0A: set64(rt, static_cast<s64>(gpr[rs].lo) < imm); break;                     // SLTI
    case 0x0B: set64(rt, gpr[rs].lo < static_cast<u64>(static_cast<s64>(imm))); break;   // SLTIU
    case 0x0C: set64(rt, gpr[rs].lo & (op & 0xFFFF)); break;                             // ANDI
    case 0x0D: set64(rt, gpr[rs].lo | (op & 0xFFFF)); break;                             // ORI
    case 0x0E: set64(rt, gpr[rs].lo ^ (op & 0xFFFF)); break;                             // XORI
    case 0x0F: set32(rt, (op & 0xFFFF) << 16); break;                                    // LUI
    case 0x10:
      cop0_op(op);
      break;
    case 0x11:
      cop1_op(op, at);
      break;
    case 0x12:
      cop2_op(op, at);
      break;
    case 0x14: branch(gpr[rs].lo == gpr[rt].lo, at, imm, true); break;                   // BEQL
    case 0x15: branch(gpr[rs].lo != gpr[rt].lo, at, imm, true); break;                   // BNEL
    case 0x16: branch(static_cast<s64>(gpr[rs].lo) <= 0, at, imm, true); break;          // BLEZL
    case 0x17: branch(static_cast<s64>(gpr[rs].lo) > 0, at, imm, true); break;           // BGTZL
    case 0x18:                                                                           // DADDI
    case 0x19: set64(rt, gpr[rs].lo + static_cast<u64>(static_cast<s64>(imm))); break;   // DADDIU
    case 0x1A: {  // LDL
      unsigned n = 7 - (address & 7);
      u64 mem = read64(address & ~7u);
      set64(rt, n ? (mem << (8 * n)) | (gpr[rt].lo & ((u64{1} << (8 * n)) - 1)) : mem);
      break;
    }
    case 0x1B: {  // LDR
      unsigned s = address & 7;
      u64 mem = read64(address & ~7u);
      set64(rt, s ? (mem >> (8 * s)) | (gpr[rt].lo & ~(~u64{0} >> (8 * s))) : mem);
      break;
    }
    case 0x1C:
      mmi(op);
      break;
    case 0x1E: {  // LQ
      u32 a = address & ~15u;
      if (rt) {
        gpr[rt].lo = read64(a);
        gpr[rt].hi = read64(a + 8);
      }
      break;
    }
    case 0x1F: {  // SQ
      u32 a = address & ~15u;
      if (pointer(a) || !on_write128) {
        write64(a, gpr[rt].lo);
        write64(a + 8, gpr[rt].hi);
      } else {
        on_write128(a & 0x1FFFFFFFu, gpr[rt].lo, gpr[rt].hi);
      }
      break;
    }
    case 0x20: set64(rt, static_cast<u64>(static_cast<s64>(static_cast<s8>(read8(address))))); break;    // LB
    case 0x21: set64(rt, static_cast<u64>(static_cast<s64>(static_cast<s16>(read16(address))))); break;  // LH
    case 0x22: {  // LWL
      unsigned n = 3 - (address & 3);
      u32 mem = read32(address & ~3u), old = static_cast<u32>(gpr[rt].lo);
      set32(rt, n ? (mem << (8 * n)) | (old & ((1u << (8 * n)) - 1)) : mem);
      break;
    }
    case 0x23: set32(rt, read32(address)); break;  // LW
    case 0x24: set64(rt, read8(address)); break;   // LBU
    case 0x25: set64(rt, read16(address)); break;  // LHU
    case 0x26: {  // LWR
      unsigned s = address & 3;
      u32 mem = read32(address & ~3u), old = static_cast<u32>(gpr[rt].lo);
      if (s == 0) {
        set32(rt, mem);
      } else if (rt) {
        // Only the low word changes when the load is partial.
        u32 merged = (mem >> (8 * s)) | (old & ~(0xFFFFFFFFu >> (8 * s)));
        gpr[rt].lo = (gpr[rt].lo & 0xFFFFFFFF00000000ull) | merged;
      }
      break;
    }
    case 0x27: set64(rt, read32(address)); break;                                // LWU
    case 0x28: write8(address, static_cast<u8>(gpr[rt].lo)); break;              // SB
    case 0x29: write16(address, static_cast<u16>(gpr[rt].lo)); break;            // SH
    case 0x2A: {  // SWL
      unsigned n = 3 - (address & 3);
      u32 mem = read32(address & ~3u), value = static_cast<u32>(gpr[rt].lo);
      write32(address & ~3u, n ? (value >> (8 * n)) | (mem & ~(0xFFFFFFFFu >> (8 * n))) : value);
      break;
    }
    case 0x2B: write32(address, static_cast<u32>(gpr[rt].lo)); break;            // SW
    case 0x2C: {  // SDL
      unsigned n = 7 - (address & 7);
      u64 mem = read64(address & ~7u);
      write64(address & ~7u, n ? (gpr[rt].lo >> (8 * n)) | (mem & ~(~u64{0} >> (8 * n))) : gpr[rt].lo);
      break;
    }
    case 0x2D: {  // SDR
      unsigned s = address & 7;
      u64 mem = read64(address & ~7u);
      write64(address & ~7u, s ? (gpr[rt].lo << (8 * s)) | (mem & ((u64{1} << (8 * s)) - 1)) : gpr[rt].lo);
      break;
    }
    case 0x2E: {  // SWR
      unsigned s = address & 3;
      u32 mem = read32(address & ~3u), value = static_cast<u32>(gpr[rt].lo);
      write32(address & ~3u, s ? (value << (8 * s)) | (mem & ((1u << (8 * s)) - 1)) : value);
      break;
    }
    case 0x2F:  // CACHE
    case 0x33:  // PREF
      break;
    case 0x31: fpr[rt] = read32(address); break;   // LWC1
    case 0x39: write32(address, fpr[rt]); break;   // SWC1
    case 0x36: {  // LQC2
      u32 a = address & ~15u;
      vu0_sync();
      if (rt) {
        vu0_.vf[rt] = {read32(a), read32(a + 4), read32(a + 8), read32(a + 12)};
      }
      break;
    }
    case 0x3E: {  // SQC2
      u32 a = address & ~15u;
      vu0_sync();
      for (unsigned f = 0; f < 4; f++) {
        write32(a + f * 4, vu0_.vf[rt][f]);
      }
      break;
    }
    case 0x37: set64(rt, read64(address)); break;      // LD
    case 0x3F: write64(address, gpr[rt].lo); break;    // SD
    default:
      not_known(op, at);
      break;
  }
}

// --- SPECIAL and REGIMM --------------------------------------------------------

void Ee::special(u32 op) {
  unsigned rs = rs_of(op), rt = rt_of(op), rd = rd_of(op), shift = sa_of(op);
  u64 s = gpr[rs].lo, t = gpr[rt].lo;
  u32 s32v = static_cast<u32>(s), t32v = static_cast<u32>(t);
  u32 at = pc - 4;

  switch (op & 0x3F) {
    case 0x00: set32(rd, t32v << shift); break;                                               // SLL
    case 0x02: set32(rd, t32v >> shift); break;                                               // SRL
    case 0x03: set32(rd, static_cast<u32>(static_cast<s32>(t32v) >> shift)); break;           // SRA
    case 0x04: set32(rd, t32v << (s32v & 31)); break;                                         // SLLV
    case 0x06: set32(rd, t32v >> (s32v & 31)); break;                                         // SRLV
    case 0x07: set32(rd, static_cast<u32>(static_cast<s32>(t32v) >> (s32v & 31))); break;     // SRAV
    case 0x08: next_pc = s32v; break;                                                         // JR
    case 0x09:                                                                                // JALR
      next_pc = s32v;
      set64(rd, at + 8);
      break;
    case 0x0A: if (t == 0) set64(rd, s); break;  // MOVZ
    case 0x0B: if (t != 0) set64(rd, s); break;  // MOVN
    case 0x0C:                                   // SYSCALL
      if (on_syscall) {
        on_syscall((op >> 6) & 0xFFFFF);
      }
      break;
    case 0x0D:  // BREAK: the compiler's divide-by-zero trap; the game never survives one
      not_known(op, at);
      break;
    case 0x0F:  // SYNC
      break;
    case 0x10: set64(rd, hi); break;  // MFHI
    case 0x11: hi = s; break;         // MTHI
    case 0x12: set64(rd, lo); break;  // MFLO
    case 0x13: lo = s; break;         // MTLO
    case 0x14: set64(rd, t << (s & 63)); break;                                               // DSLLV
    case 0x16: set64(rd, t >> (s & 63)); break;                                               // DSRLV
    case 0x17: set64(rd, static_cast<u64>(static_cast<s64>(t) >> (s & 63))); break;           // DSRAV
    case 0x18: {  // MULT: the EE also writes the low half to rd
      s64 product = static_cast<s64>(static_cast<s32>(s32v)) * static_cast<s32>(t32v);
      lo = sext32(static_cast<u32>(product));
      hi = sext32(static_cast<u32>(product >> 32));
      set64(rd, lo);
      break;
    }
    case 0x19: {  // MULTU
      u64 product = static_cast<u64>(s32v) * t32v;
      lo = sext32(static_cast<u32>(product));
      hi = sext32(static_cast<u32>(product >> 32));
      set64(rd, lo);
      break;
    }
    case 0x1A: {  // DIV
      s32 n = static_cast<s32>(s32v), d = static_cast<s32>(t32v);
      if (d == 0) {
        lo = sext32(n < 0 ? 1u : 0xFFFFFFFFu);
        hi = sext32(static_cast<u32>(n));
      } else if (n == INT32_MIN && d == -1) {
        lo = sext32(0x80000000u);
        hi = 0;
      } else {
        lo = sext32(static_cast<u32>(n / d));
        hi = sext32(static_cast<u32>(n % d));
      }
      break;
    }
    case 0x1B:  // DIVU
      if (t32v == 0) {
        lo = sext32(0xFFFFFFFFu);
        hi = sext32(s32v);
      } else {
        lo = sext32(s32v / t32v);
        hi = sext32(s32v % t32v);
      }
      break;
    case 0x20:                                         // ADD
    case 0x21: set32(rd, s32v + t32v); break;          // ADDU
    case 0x22:                                         // SUB
    case 0x23: set32(rd, s32v - t32v); break;          // SUBU
    case 0x24: set64(rd, s & t); break;                // AND
    case 0x25: set64(rd, s | t); break;                // OR
    case 0x26: set64(rd, s ^ t); break;                // XOR
    case 0x27: set64(rd, ~(s | t)); break;             // NOR
    case 0x28: set64(rd, sa); break;                   // MFSA
    case 0x29: sa = s32v; break;                       // MTSA
    case 0x2A: set64(rd, static_cast<s64>(s) < static_cast<s64>(t)); break;  // SLT
    case 0x2B: set64(rd, s < t); break;                // SLTU
    case 0x2C:                                         // DADD
    case 0x2D: set64(rd, s + t); break;                // DADDU
    case 0x2E:                                         // DSUB
    case 0x2F: set64(rd, s - t); break;                // DSUBU
    case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x36:  // traps: never taken by working code
      break;
    case 0x38: set64(rd, t << shift); break;                                                  // DSLL
    case 0x3A: set64(rd, t >> shift); break;                                                  // DSRL
    case 0x3B: set64(rd, static_cast<u64>(static_cast<s64>(t) >> shift)); break;              // DSRA
    case 0x3C: set64(rd, t << (shift + 32)); break;                                           // DSLL32
    case 0x3E: set64(rd, t >> (shift + 32)); break;                                           // DSRL32
    case 0x3F: set64(rd, static_cast<u64>(static_cast<s64>(t) >> (shift + 32))); break;       // DSRA32
    default:
      not_known(op, at);
      break;
  }
}

void Ee::regimm(u32 op, u32 at) {
  unsigned rs = rs_of(op);
  s64 s = static_cast<s64>(gpr[rs].lo);
  s32 imm = imm_of(op);
  switch (rt_of(op)) {
    case 0x00: branch(s < 0, at, imm, false); break;   // BLTZ
    case 0x01: branch(s >= 0, at, imm, false); break;  // BGEZ
    case 0x02: branch(s < 0, at, imm, true); break;    // BLTZL
    case 0x03: branch(s >= 0, at, imm, true); break;   // BGEZL
    case 0x10:                                         // BLTZAL
      gpr[31].lo = at + 8;
      branch(s < 0, at, imm, false);
      break;
    case 0x11:  // BGEZAL
      gpr[31].lo = at + 8;
      branch(s >= 0, at, imm, false);
      break;
    case 0x12:  // BLTZALL
      gpr[31].lo = at + 8;
      branch(s < 0, at, imm, true);
      break;
    case 0x13:  // BGEZALL
      gpr[31].lo = at + 8;
      branch(s >= 0, at, imm, true);
      break;
    case 0x18:  // MTSAB: the shift amount in bytes
      sa = (static_cast<u32>(gpr[rs].lo) & 0xF) ^ (static_cast<u32>(imm) & 0xF);
      break;
    case 0x19:  // MTSAH: in halfwords
      sa = ((static_cast<u32>(gpr[rs].lo) & 7) ^ (static_cast<u32>(imm) & 7)) << 1;
      break;
    default:
      if (rt_of(op) >= 0x08 && rt_of(op) <= 0x0E) {
        break;  // traps
      }
      not_known(op, at);
      break;
  }
}

// --- COP0 ----------------------------------------------------------------------

void Ee::cop0_op(u32 op) {
  unsigned rt = rt_of(op), rd = rd_of(op);
  switch (rs_of(op)) {
    case 0x00:  // MFC0
      set32(rt, rd == 9 ? static_cast<u32>(cycles) : cop0[rd]);
      break;
    case 0x04:  // MTC0
      cop0[rd] = static_cast<u32>(gpr[rt].lo);
      break;
    case 0x08:  // BC0: the condition is "no DMA transfer is running"
      branch((rt & 1) != 0, pc - 4, imm_of(op), (rt & 2) != 0);
      break;
    case 0x10:
      switch (op & 0x3F) {
        case 0x18:  // ERET
          pc = cop0[14];
          next_pc = pc + 4;
          break;
        case 0x38:  // EI
          cop0[12] |= 0x10000;
          break;
        case 0x39:  // DI
          cop0[12] &= ~0x10000u;
          break;
        default:  // the TLB instructions: there is no TLB here
          break;
      }
      break;
    default:
      not_known(op, pc - 4);
      break;
  }
}

// --- COP1: the FPU -------------------------------------------------------------

void Ee::fpu_flags(u32 problems) {
  fcr31 &= ~(kO | kU);
  if (problems & fp::kOverflow) fcr31 |= kO | kSO;
  if (problems & fp::kUnderflow) fcr31 |= kU | kSU;
}

void Ee::cop1_op(u32 op, u32 at) {
  unsigned rt = rt_of(op), fs = rd_of(op), fd = sa_of(op), ft = rt;
  u32 problems = 0;
  switch (rs_of(op)) {
    case 0x00: set32(rt, fpr[fs]); break;                         // MFC1
    case 0x02: set32(rt, fs == 31 ? fcr31 : fs == 0 ? 0x2E00u : 0u); break;  // CFC1
    case 0x04: fpr[fs] = static_cast<u32>(gpr[rt].lo); break;     // MTC1
    case 0x06:                                                    // CTC1
      if (fs == 31) {
        fcr31 = (static_cast<u32>(gpr[rt].lo) & 0x0083C078u) | 0x01000001u;
      }
      break;
    case 0x08:  // BC1F, BC1T and the likely forms
      branch(((fcr31 & kC) != 0) == ((rt & 1) != 0), at, imm_of(op), (rt & 2) != 0);
      break;
    case 0x10:
      switch (op & 0x3F) {
        case 0x00:
          fpr[fd] = fp::add(fpr[fs], fpr[ft], problems);
          fpu_flags(problems);
          break;
        case 0x01:
          fpr[fd] = fp::sub(fpr[fs], fpr[ft], problems);
          fpu_flags(problems);
          break;
        case 0x02:
          fpr[fd] = fp::mul(fpr[fs], fpr[ft], problems);
          fpu_flags(problems);
          break;
        case 0x03:  // DIV
          fpr[fd] = fp::div(fpr[fs], fpr[ft], problems);
          fcr31 &= ~(kI | kD);
          if (problems & fp::kInvalid) fcr31 |= kI | kSI;
          if (problems & fp::kDivideByZero) fcr31 |= kD | kSD;
          break;
        case 0x04:  // SQRT of ft
          fpr[fd] = fp::sqrt(fpr[ft], problems);
          fcr31 &= ~(kI | kD);
          if (problems & fp::kInvalid) fcr31 |= kI | kSI;
          break;
        case 0x05:  // ABS
          fpr[fd] = fpr[fs] & ~fp::kSign;
          fcr31 &= ~(kO | kU);
          break;
        case 0x06:  // MOV
          fpr[fd] = fpr[fs];
          break;
        case 0x07:  // NEG
          fpr[fd] = fpr[fs] ^ fp::kSign;
          fcr31 &= ~(kO | kU);
          break;
        case 0x16:  // RSQRT: fs / sqrt(ft)
          fpr[fd] = fp::rsqrt(fpr[fs], fpr[ft], problems);
          fcr31 &= ~(kI | kD);
          if (problems & fp::kInvalid) fcr31 |= kI | kSI;
          if (problems & fp::kDivideByZero) fcr31 |= kD | kSD;
          break;
        case 0x18:
          facc = fp::add(fpr[fs], fpr[ft], problems);
          fpu_flags(problems);
          break;
        case 0x19:
          facc = fp::sub(fpr[fs], fpr[ft], problems);
          fpu_flags(problems);
          break;
        case 0x1A:
          facc = fp::mul(fpr[fs], fpr[ft], problems);
          fpu_flags(problems);
          break;
        case 0x1C:
          fpr[fd] = fp::add(facc, fp::mul(fpr[fs], fpr[ft], problems), problems);
          fpu_flags(problems);
          break;
        case 0x1D:
          fpr[fd] = fp::sub(facc, fp::mul(fpr[fs], fpr[ft], problems), problems);
          fpu_flags(problems);
          break;
        case 0x1E:
          facc = fp::add(facc, fp::mul(fpr[fs], fpr[ft], problems), problems);
          fpu_flags(problems);
          break;
        case 0x1F:
          facc = fp::sub(facc, fp::mul(fpr[fs], fpr[ft], problems), problems);
          fpu_flags(problems);
          break;
        case 0x24:  // CVT.W: towards zero, saturating
          fpr[fd] = static_cast<u32>(fp::to_int(fpr[fs]));
          break;
        case 0x28:
          fpr[fd] = fp::max(fpr[fs], fpr[ft]);
          fcr31 &= ~(kO | kU);
          break;
        case 0x29:
          fpr[fd] = fp::min(fpr[fs], fpr[ft]);
          fcr31 &= ~(kO | kU);
          break;
        case 0x30: fcr31 &= ~kC; break;                                                                    // C.F
        case 0x32: fcr31 = (fcr31 & ~kC) | (fp::key(fpr[fs]) == fp::key(fpr[ft]) ? kC : 0); break;         // C.EQ
        case 0x34: fcr31 = (fcr31 & ~kC) | (fp::key(fpr[fs]) < fp::key(fpr[ft]) ? kC : 0); break;          // C.LT
        case 0x36: fcr31 = (fcr31 & ~kC) | (fp::key(fpr[fs]) <= fp::key(fpr[ft]) ? kC : 0); break;         // C.LE
        default:
          not_known(op, at);
          break;
      }
      break;
    case 0x14:  // W: CVT.S
      if ((op & 0x3F) == 0x20) {
        fpr[fd] = fp::from_int(static_cast<s32>(fpr[fs]));
      } else {
        not_known(op, at);
      }
      break;
    default:
      not_known(op, at);
      break;
  }
}

// --- COP2: VU0 from the EE -----------------------------------------------------

// VU0 runs beside the EE once a microprogram is started. It is advanced
// when the EE next touches it: by as many instructions as the EE has run
// since (one VU instruction to an EE instruction), or further when the EE's
// instruction is one that waits.
void Ee::vu0_sync() {
  if (!vu0_.stopped()) {
    vu0_.advance((cycles - vu0_cycles_) / kCyclesPerInstruction);
  }
  vu0_cycles_ = cycles;
}

void Ee::vu0_finish(u32 at) {
  if (vu0_.stopped()) {
    return;
  }
  // A microprogram is a few thousand instructions at most; one that runs on
  // is waiting for something the EE will never send, or has gone wrong here.
  vu0_.advance(2'000'000);
  vu0_cycles_ = cycles;
  if (!vu0_.stopped()) {
    if (vu0_runaways++ == 0) {
      vu0_runaway_start = vu0_started_at_;
      vu0_runaway_from = at;
    }
  }
}

void Ee::cop2_op(u32 op, u32 at) {
  unsigned rt = rt_of(op), rd = rd_of(op);
  if (op & (1u << 25)) {
    // An operation on VU0 itself: the EE waits for a running microprogram.
    vu0_finish(at);
    switch (op & 0x3F) {
      case 0x38:  // VCALLMS: start the microprogram at this instruction
      case 0x39:  // VCALLMSR: at the address in CMSAR0
        vu0_started_at_ = (op & 0x3F) == 0x38 ? (op >> 6) & 0x7FFF : vu0_.control(27);
        vu0_.start(vu0_started_at_);
        vu0_cycles_ = cycles;
        break;
      default:
        vu0_.macro(op);
        break;
    }
    return;
  }
  // Moves between the EE and VU0. With the interlock bit, a move from VU0
  // waits for the microprogram to end and a move to VU0 waits for the next
  // point the microprogram marks (its M bit); without it, the move happens
  // while the microprogram runs.
  if (!vu0_.stopped()) {
    unsigned kind = rs_of(op);
    if ((op & 1) && (kind == 0x01 || kind == 0x02)) {
      vu0_finish(at);
    } else if ((op & 1) && (kind == 0x05 || kind == 0x06)) {
      vu0_sync();
      vu0_.advance_to_sync(2'000'000);
      vu0_cycles_ = cycles;
    } else {
      vu0_sync();
    }
  }
  switch (rs_of(op)) {
    case 0x01:  // QMFC2
      if (rt) {
        gpr[rt].lo = vu0_.vf[rd][0] | (static_cast<u64>(vu0_.vf[rd][1]) << 32);
        gpr[rt].hi = vu0_.vf[rd][2] | (static_cast<u64>(vu0_.vf[rd][3]) << 32);
      }
      break;
    case 0x02:  // CFC2
      set32(rt, vu0_.control(rd));
      break;
    case 0x05:  // QMTC2
      if (rd) {
        vu0_.vf[rd] = {static_cast<u32>(gpr[rt].lo), static_cast<u32>(gpr[rt].lo >> 32), static_cast<u32>(gpr[rt].hi),
                       static_cast<u32>(gpr[rt].hi >> 32)};
      }
      break;
    case 0x06:  // CTC2
      vu0_.set_control(rd, static_cast<u32>(gpr[rt].lo));
      break;
    case 0x08:  // BC2: the condition is "VU0 is running", and it never is when the EE looks
      branch((rt & 1) == 0, at, imm_of(op), (rt & 2) != 0);
      break;
    default:
      not_known(op, at);
      break;
  }
}

}  // namespace ps2
