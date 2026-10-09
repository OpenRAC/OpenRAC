// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "vu.h"

#include <cfenv>
#include <cmath>

#include "fp.h"
#include "fp_quad.h"

namespace ps2 {
namespace {

constexpr u32 kXyz = 14, kAll = 15;

// The divider's two status bits: invalid (bit 4) and divide by zero (bit 5).
inline u32 divide_flags(u32 problems) {
  return ((problems & fp::kInvalid) ? 0x10u : 0u) | ((problems & fp::kDivideByZero) ? 0x20u : 0u);
}

// The four-field arithmetic needs the host to round towards zero while a
// unit computes, and only then: other code on the thread expects the usual
// rounding.
struct TowardZero {
  u64 saved = fp::host_rounding();
  TowardZero() { fp::round_toward_zero(); }
  ~TowardZero() { fp::set_host_rounding(saved); }
};

inline s32 sign_extend(u32 v, unsigned width) {
  u32 m = 1u << (width - 1);
  return static_cast<s32>((v ^ m) - m);
}

inline bool has(u32 dest, unsigned field) {
  return (dest & (8u >> field)) != 0;
}

// How long each function unit takes: the instruction this many later sees
// the result.
constexpr unsigned kDiv = 7, kSqrt = 7, kRsqrt = 13;

}  // namespace

Vu::Vu(Memory memory) : memory_(memory), pc_mask_(memory.micro_bytes / 8 - 1), needs_(memory.micro_bytes / 8) {
  reset();
}

void Vu::reset() {
  for (auto& reg : vf) {
    reg.fill(0);
  }
  vf[0][3] = as_u32(1.0f);
  vi.fill(0);
  acc.fill(0);
  q = p = i = 0;
  r = 0x3F800000;
  mac = status = clip = 0;
  mac_latest_ = status_latest_ = clip_latest_ = 0;
  flag_first_ = flag_count_ = 0;
  q_at_ = p_at_ = 0;
  cycle_ = 0;
  readable_ = {};
  branch_in_ = stop_in_ = kick_in_ = 0;
  backup_ttl_ = 0;
  pc = 0;
  running_ = false;
}

// --- running -----------------------------------------------------------------

u64 Vu::run(u32 address, u64 limit) {
  pc = address & pc_mask_;
  branch_in_ = stop_in_ = 0;
  return resume(limit);
}

void Vu::start(u32 address) {
  pc = address & pc_mask_;
  branch_in_ = stop_in_ = 0;
  running_ = true;
}

u64 Vu::advance(u64 instructions) {
  TowardZero rounding;
  u64 count = 0;
  while (running_ && count < instructions) {
    step();
    count++;
  }
  return count;
}

u64 Vu::advance_to_sync(u64 limit) {
  TowardZero rounding;
  u64 count = 0;
  sync_point_ = false;
  while (running_ && !sync_point_ && count < limit) {
    step();
    count++;
  }
  return count;
}

u64 Vu::resume(u64 limit) {
  TowardZero rounding;
  running_ = true;
  u64 count = 0;
  while (running_ && count < limit) {
    step();
    count++;
  }
  return count;
}

void Vu::finish_q() {
  q = q_next_;
  q_at_ = 0;
  // The divider's two flags (invalid, divide by zero) arrive with its result.
  auto apply = [this](u32& s) { s = (s & ~0x30u) | q_flags_ | (q_flags_ << 6); };
  apply(status);
  apply(status_latest_);
  for (Flags& f : flag_pipe_) {
    apply(f.status);
  }
}

void Vu::fire_kick() {
  kick_in_ = 0;
  if (on_kick) {
    // What takes the packet (the GS) computes with the usual rounding.
    u64 mode = fp::host_rounding();
    fp::round_to_nearest();
    on_kick(kick_address_);
    fp::set_host_rounding(mode);
  }
}

// What a pair reads that can make it wait: float registers (a register
// written less than four cycles ago holds the pair up), the divider, the
// function unit.
void Vu::work_out(Needs& needs, u32 up, u32 low) const {
  needs = Needs{};
  needs.up = up;
  needs.low = low;
  needs.known = true;
  auto read = [&](unsigned reg, u32 mask) {
    if (reg && mask && needs.count < 4) {
      needs.reg[needs.count] = static_cast<u8>(reg);
      needs.mask[needs.count] = static_cast<u8>(mask);
      needs.count++;
    }
  };

  u32 dest = (up >> 21) & 0xF;
  unsigned ft = (up >> 16) & 31, fs = (up >> 11) & 31;
  u32 fn = up & 0x3F;
  if (fn < 0x1C) {
    read(fs, dest);
    read(ft, 8u >> (fn & 3));
  } else if (fn < 0x28) {
    read(fs, dest);
  } else if (fn < 0x30) {
    read(fs, dest);
    read(ft, dest);
  } else if (fn >= 0x3C) {
    u32 index = (((up >> 6) & 0x1F) << 2) | (up & 3);
    if (index < 0x10 || (index >= 0x18 && index < 0x1C)) {
      read(fs, dest);
      read(ft, 8u >> (index & 3));
    } else if (index == 0x1F) {  // CLIP
      read(fs, 14);
      read(ft, 1);
    } else if (index >= 0x28 && index < 0x2F) {
      read(fs, dest);
      read(ft, dest);
    } else if (index != 0x2F) {
      read(fs, dest);
    }
  }

  if (!(up & 0x80000000u)) {
    u32 op = low >> 25;
    u32 ldest = (low >> 21) & 0xF;
    unsigned it = (low >> 16) & 31, is = (low >> 11) & 31;
    u32 fsf = 8u >> ((low >> 21) & 3), ftf = 8u >> ((low >> 23) & 3);
    if (op == 0x01) {  // SQ
      read(is, ldest);
    } else if (op == 0x40 && (low & 0x3F) >= 0x3C) {
      switch ((((low >> 6) & 0x1F) << 2) | (low & 3)) {
        case 0x30: read(is, ldest); break;  // MOVE
        case 0x31: read(is, 15); break;     // MR32
        case 0x35: case 0x37: read(is, ldest); break;  // SQI, SQD
        case 0x38: case 0x3A:  // DIV, RSQRT
          read(is, fsf);
          read(it, ftf);
          needs.wait = 1;
          break;
        case 0x39:  // SQRT
          read(it, ftf);
          needs.wait = 1;
          break;
        case 0x3B: needs.wait = 1; break;  // WAITQ
        case 0x3C: case 0x42: case 0x43: read(is, fsf); break;  // MTIR, RINIT, RXOR
        case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: read(is, 14); break;
        case 0x76: read(is, 15); break;
        case 0x78: case 0x79: case 0x7A: case 0x7C: case 0x7D: case 0x7E: read(is, fsf); break;
        case 0x7B: needs.wait = 2; break;  // WAITP
        default: break;
      }
    }
  }
}

void Vu::program_changed() {
  for (Needs& n : needs_) {
    n.known = false;
  }
  programs_looked_at_ = false;
}

namespace {

// Does this upper instruction set MAC and status flags?
inline bool sets_flags(u32 up) {
  u32 fn = up & 0x3F;
  if (fn < 0x10 || (fn >= 0x18 && fn <= 0x1C) || fn == 0x1E || (fn >= 0x20 && fn <= 0x2A) || (fn >= 0x2C && fn <= 0x2E)) {
    return true;
  }
  if (fn < 0x3C) {
    return false;
  }
  u32 index = (((up >> 6) & 0x1F) << 2) | (up & 3);
  return index < 0x10 || (index >= 0x18 && index <= 0x1C) || index == 0x1E || (index >= 0x20 && index <= 0x2A) ||
         (index >= 0x2C && index <= 0x2E);
}

// Lower instructions that read the MAC or status flags.
inline bool reads_flags(u32 low) {
  u32 op = low >> 25;
  return op == 0x14 || op == 0x16 || op == 0x17 || op == 0x18 || op == 0x1A || op == 0x1B;
}

}  // namespace

// What is in program memory as a whole: does anything read flags in a way
// that depends on every instruction (the bits that remember, the MAC flags)?
void Vu::look_at_programs() {
  programs_looked_at_ = true;
  sticky_readers_ = false;
  for (u32 n = 0; n <= pc_mask_; n++) {
    u32 low = load<u32>(memory_.micro + n * 8), up = load<u32>(memory_.micro + n * 8 + 4);
    if (up & 0x80000000u) {
      continue;
    }
    u32 op = low >> 25;
    u32 imm12 = ((low >> 10) & 0x800) | (low & 0x7FF);
    if (op == 0x15 || op == 0x18 || op == 0x1A || op == 0x1B || ((op == 0x14 || op == 0x16 || op == 0x17) && (imm12 & 0xFC0))) {
      sticky_readers_ = true;
      return;
    }
  }
}

// Can any instruction read the flags the upper instruction at `at` sets?
// They are seen from four cycles on and until the next instruction that
// sets flags has had its four cycles. A reader in that stretch, or anything
// that leaves the straight line (a branch, the end of the program), counts.
bool Vu::flags_can_be_read(u32 at) const {
  if (!skip_unread_flags || sticky_readers_) {
    return true;
  }
  int next_setter = -1;
  // (From the pair before: if that one branches, this is its delay slot and
  // what follows is somewhere else. The pair itself is looked at only for a
  // branch.)
  for (int n = -1; n <= 40; n++) {
    u32 where = (at + static_cast<u32>(n)) & pc_mask_;
    u32 low = load<u32>(memory_.micro + where * 8), up = load<u32>(memory_.micro + where * 8 + 4);
    if (up & 0x40000000u) {
      return true;
    }
    if (!(up & 0x80000000u)) {
      u32 op = low >> 25;
      if ((n > 0 && reads_flags(low)) || (op >= 0x20 && op <= 0x2F)) {
        return true;
      }
    }
    if (n <= 0) {
      continue;
    }
    if (next_setter < 0 && sets_flags(up)) {
      next_setter = n;
    }
    // Five more pairs after the next setter: its four cycles, and one over.
    if (next_setter >= 0 && n >= next_setter + 5) {
      return false;
    }
  }
  return true;
}

void Vu::step() {
  u32 at = pc;
  u32 low = load<u32>(memory_.micro + at * 8), up = load<u32>(memory_.micro + at * 8 + 4);
  pc = (pc + 1) & pc_mask_;
  if (on_step) [[unlikely]] {
    on_step(at, up, low);
  }

  // When this pair runs: the next cycle, or later if it reads a float
  // register not yet readable, or needs a unit that is busy.
  Needs& needs = needs_[at];
  if (!needs.known || needs.up != up || needs.low != low) {
    if (!programs_looked_at_) {
      look_at_programs();
    }
    work_out(needs, up, low);
    needs.flags_wanted = !sets_flags(up) || flags_can_be_read(at);
  }
  flags_wanted_ = needs.flags_wanted;
  u64 ready = cycle_ + 1;
  for (unsigned n = 0; n < needs.count; n++) {
    const std::array<u64, 4>& fields = readable_[needs.reg[n]];
    u32 mask = needs.mask[n];
    if ((mask & 8) && fields[0] > ready) ready = fields[0];
    if ((mask & 4) && fields[1] > ready) ready = fields[1];
    if ((mask & 2) && fields[2] > ready) ready = fields[2];
    if ((mask & 1) && fields[3] > ready) ready = fields[3];
  }
  if (needs.wait == 1 && q_at_ > ready) ready = q_at_;
  if (needs.wait == 2 && p_at_ > ready) ready = p_at_;
  // A branch tests the value an integer register had before the instruction
  // just ahead of it, unless it had to wait: then the write got through.
  if (backup_ttl_) {
    u64 passed = ready - cycle_;
    backup_ttl_ = passed >= backup_ttl_ ? 0 : backup_ttl_ - static_cast<unsigned>(passed);
  }
  cycle_ = ready;
  // What has arrived by then.
  while (flag_count_ && flag_pipe_[flag_first_].at <= cycle_) {
    const Flags& f = flag_pipe_[flag_first_];
    mac = f.mac;
    status = f.status;
    clip = f.clip;
    flag_first_ = (flag_first_ + 1) & 7;
    flag_count_--;
  }
  if (q_at_ && q_at_ <= cycle_) {
    finish_q();
  }
  if (p_at_ && p_at_ <= cycle_) {
    p = p_next_;
    p_at_ = 0;
  }
  timed_ = true;


  in_upper_ = true;
  upper_reg_ = 0;
  if (up & 0x20000000u) {
    sync_point_ = true;  // the M bit
  }
  if (up & 0x80000000u) {
    // The I bit: the lower word is a number for the I register, not an
    // instruction. The upper instruction of the same pair still reads the
    // old I (the games' own sine routine depends on it).
    upper(up);
    in_upper_ = false;
    i = low;
  } else {
    upper(up);
    in_upper_ = false;
    // Both halves read the registers as they were. So the lower instruction
    // runs with the upper one's register put back, and the upper one's
    // result goes in afterwards, over anything the lower one wrote there.
    std::array<u32, 4> result{};
    if (upper_reg_) {
      result = vf[upper_reg_];
      vf[upper_reg_] = upper_old_;
    }
    lower(low, at);
    if (upper_reg_) {
      vf[upper_reg_] = result;
    }
  }

  if ((up & 0x40000000u) && !stop_in_) {
    stop_in_ = 2;  // the E bit: this instruction and the next, then stop
  }
  if (kick_in_ && --kick_in_ == 0) {
    fire_kick();
  }
  if (branch_in_ && --branch_in_ == 0) {
    pc = branch_target_;
  }
  timed_ = false;
  if (stop_in_ && --stop_in_ == 0) {
    running_ = false;
    // Nothing is left waiting when a program has stopped.
    settle();
  }
}

// Bring everything in flight to its end: flags, the divider, the function
// unit, a pending kick.
void Vu::settle() {
  while (flag_count_) {
    const Flags& f = flag_pipe_[flag_first_];
    mac = f.mac;
    status = f.status;
    clip = f.clip;
    flag_first_ = (flag_first_ + 1) & 7;
    flag_count_--;
  }
  if (q_at_) {
    finish_q();
  }
  if (p_at_) {
    p = p_next_;
    p_at_ = 0;
  }
  if (kick_in_) {
    fire_kick();
  }
  backup_ttl_ = 0;
}

void Vu::macro(u32 code) {
  TowardZero rounding;
  in_upper_ = false;
  flags_wanted_ = true;
  u32 fn = code & 0x3F;
  if (fn < 0x30) {
    upper(code);
  } else if (fn < 0x38) {
    lower_special(code);  // the integer operations
  } else if (fn >= 0x3C) {
    if (((((code >> 6) & 0x1F) << 2) | (code & 3)) < 0x30) {
      upper_special(code);
    } else {
      lower_special(code);
    }
  } else {
    unknown_ops++;
  }
  settle();
}

u32 Vu::control(unsigned reg) const {
  if (reg < 16) {
    return vi[reg];
  }
  switch (reg) {
    case 16: return status;
    case 17: return mac;
    case 18: return clip;
    case 20: return r & 0x7FFFFF;
    case 21: return i;
    case 22: return q;
    case 26: return pc * 8;
    case 27: return cmsar0_;
    case 28: return fbrst_;
    case 29: return running_ ? 1 : 0;  // VPU-STAT: bit 0, VU0 is running
    default: return 0;
  }
}

void Vu::set_control(unsigned reg, u32 value) {
  if (reg < 16) {
    if (reg) {
      vi[reg] = static_cast<u16>(value);
    }
    return;
  }
  switch (reg) {
    case 16:  // only the remembered bits can be written
      status = (status & 0x3F) | (value & 0xFC0);
      status_latest_ = status;
      break;
    case 18:
      clip = clip_latest_ = value & 0xFFFFFF;
      break;
    case 20:
      r = (value & 0x7FFFFF) | 0x3F800000;
      break;
    case 21:
      i = value;
      break;
    case 22:
      q = value;
      break;
    case 27:
      cmsar0_ = value & 0xFFFF;
      break;
    case 28:
      fbrst_ = value & 0x0C0C;
      break;
    default:
      break;
  }
}

// --- helpers -----------------------------------------------------------------

void Vu::write_vf(unsigned reg, u32 mask, const std::array<u32, 4>& value) {
  if (reg == 0) {
    return;
  }
  if (in_upper_) {
    upper_reg_ = reg;
    upper_old_ = vf[reg];
  }
  if (timed_) {
    for (unsigned field = 0; field < 4; field++) {
      if (has(mask, field)) {
        readable_[reg][field] = cycle_ + 4;
      }
    }
  }
  for (unsigned field = 0; field < 4; field++) {
    if (has(mask, field)) {
      vf[reg][field] = value[field];
    }
  }
}

void Vu::write_vi(unsigned reg, u16 value) {
  reg &= 15;
  if (reg == 0) {
    return;
  }
  // A branch right after this instruction still tests the old value. When
  // two writes to one register follow each other, the value kept is the one
  // from before the first.
  if (!(backup_ttl_ && backup_reg_ == reg)) {
    backup_reg_ = reg;
    backup_value_ = vi[reg];
  }
  backup_ttl_ = 2;
  vi[reg] = value;
}

// The instructions that read flags into an integer register finish early: a
// branch right after one tests the new value.
void Vu::write_vi_from_flags(unsigned reg, u16 value) {
  reg &= 15;
  if (reg == 0) {
    return;
  }
  if (backup_reg_ == reg) {
    backup_ttl_ = 0;
  }
  vi[reg] = value;
}

u16 Vu::branch_vi(unsigned reg) const {
  reg &= 15;
  return (backup_ttl_ && backup_reg_ == reg) ? backup_value_ : vi[reg];
}

void Vu::branch(u32 target) {
  branch_target_ = target & pc_mask_;
  branch_in_ = 2;  // after the instruction in the delay slot
}

void Vu::start_q(u32 value, unsigned latency, u32 divide_flags) {
  if (q_at_) {
    finish_q();  // (the instruction has already waited for the divider)
  }
  q_next_ = value;
  q_flags_ = divide_flags;
  q_at_ = cycle_ + latency;
}

void Vu::start_p(double value, unsigned latency) {
  u32 unused = 0;
  if (p_at_) {
    p = p_next_;
  }
  p_next_ = fp::from_double(value, unused);
  p_at_ = cycle_ + latency;
}

u32 Vu::operand(u32 code, From from, unsigned field) const {
  unsigned ft = (code >> 16) & 31;
  switch (from) {
    case From::Ft:
      return vf[ft][field];
    case From::Bc:
      return vf[ft][code & 3];
    case From::Q:
      return q;
    default:
      return i;
  }
}

// Note a computed field's four MAC flags: zero, sign, underflow, overflow.
u32 Vu::result(u32 value, u32 problems, unsigned field, u32& flags) const {
  unsigned shift = 3 - field;
  if (value & fp::kSign) {
    flags |= 0x0010u << shift;
  }
  if (fp::is_zero(value)) {
    flags |= 0x0001u << shift;
  }
  if (problems & fp::kUnderflow) {
    flags |= 0x0100u << shift;
  }
  if (problems & fp::kOverflow) {
    flags |= 0x1000u << shift;
  }
  return value;
}

void Vu::post() {
  if (flag_count_ == 8) {  // cannot happen: at most two a cycle, four cycles deep
    flag_first_ = (flag_first_ + 1) & 7;
    flag_count_--;
  }
  flag_pipe_[(flag_first_ + flag_count_) & 7] = {mac_latest_, status_latest_, clip_latest_, cycle_ + 4};
  flag_count_++;
}

void Vu::post_flags(u32 mac_bits) {
  u32 now = 0;
  if (mac_bits & 0x000F) now |= 1;
  if (mac_bits & 0x00F0) now |= 2;
  if (mac_bits & 0x0F00) now |= 4;
  if (mac_bits & 0xF000) now |= 8;
  mac_latest_ = mac_bits;
  // Bits 0-3 are this result's; bits 6-9 remember every one seen.
  status_latest_ = (status_latest_ & 0xFF0u) | now | (now << 6);
  post();
}

void Vu::arith(u32 code, Op op, From from, bool to_acc) {
  u32 dest = (code >> 21) & 0xF;
  unsigned ft = (code >> 16) & 31, fs = (code >> 11) & 31, fd = (code >> 6) & 31;
  std::array<u32, 4> out = to_acc ? acc : vf[fd];
  u32 flags = 0;

  // The second operand, a value a field.
  std::array<u32, 4> b;
  switch (from) {
    case From::Ft:
      b = vf[ft];
      break;
    case From::Bc:
      b.fill(vf[ft][code & 3]);
      break;
    case From::Q:
      b.fill(q);
      break;
    default:
      b.fill(i);
      break;
  }
  const std::array<u32, 4>& a = vf[fs];

  // All four fields at once where every field wanted is an ordinary number.
  std::array<u32, 4> quick, product;
  bool fast = false;
  switch (op) {
    case Op::Add:
      fast = fp::quad_add(a.data(), b.data(), dest, quick.data());
      break;
    case Op::Sub:
      fast = fp::quad_add(a.data(), b.data(), dest, quick.data(), true);
      break;
    case Op::Mul:
      fast = fp::quad_mul(a.data(), b.data(), dest, quick.data());
      break;
    case Op::Madd:
      fast = fp::quad_mul(a.data(), b.data(), dest, product.data()) && fp::quad_add(acc.data(), product.data(), dest, quick.data());
      break;
    default:
      fast = fp::quad_mul(a.data(), b.data(), dest, product.data()) &&
             fp::quad_add(acc.data(), product.data(), dest, quick.data(), true);
      break;
  }
  if (fast && !flags_wanted_) {
    for (unsigned field = 0; field < 4; field++) {
      if (has(dest, field)) {
        out[field] = quick[field];
      }
    }
    if (to_acc) {
      acc = out;
    } else {
      write_vf(fd, kAll, out);
    }
    return;
  }
  if (fast) {
    for (unsigned field = 0; field < 4; field++) {
      if (has(dest, field)) {
        out[field] = result(quick[field], 0, field, flags);
      }
    }
  } else {
    for (unsigned field = 0; field < 4; field++) {
      if (!has(dest, field)) {
        continue;
      }
      u32 problems = 0, value;
      switch (op) {
        case Op::Add:
          value = fp::add(a[field], b[field], problems);
          break;
        case Op::Sub:
          value = fp::sub(a[field], b[field], problems);
          break;
        case Op::Mul:
          value = fp::mul(a[field], b[field], problems);
          break;
        case Op::Madd:
          value = fp::add(acc[field], fp::mul(a[field], b[field], problems), problems);
          break;
        default:
          value = fp::sub(acc[field], fp::mul(a[field], b[field], problems), problems);
          break;
      }
      out[field] = result(value, problems, field, flags);
    }
  }
  if (to_acc) {
    acc = out;
  } else {
    write_vf(fd, kAll, out);
  }
  if (flags_wanted_) {
    post_flags(flags);
  }
}

void Vu::min_max(u32 code, From from, bool max) {
  u32 dest = (code >> 21) & 0xF;
  unsigned fs = (code >> 11) & 31, fd = (code >> 6) & 31;
  std::array<u32, 4> out = vf[fd];
  for (unsigned field = 0; field < 4; field++) {
    if (has(dest, field)) {
      u32 a = vf[fs][field], b = operand(code, from, field);
      out[field] = max ? fp::max(a, b) : fp::min(a, b);
    }
  }
  write_vf(fd, kAll, out);
}

// --- upper instructions --------------------------------------------------------

void Vu::upper(u32 code) {
  u32 fn = code & 0x3F;
  switch (fn) {
    case 0x00: case 0x01: case 0x02: case 0x03: arith(code, Op::Add, From::Bc, false); break;
    case 0x04: case 0x05: case 0x06: case 0x07: arith(code, Op::Sub, From::Bc, false); break;
    case 0x08: case 0x09: case 0x0A: case 0x0B: arith(code, Op::Madd, From::Bc, false); break;
    case 0x0C: case 0x0D: case 0x0E: case 0x0F: arith(code, Op::Msub, From::Bc, false); break;
    case 0x10: case 0x11: case 0x12: case 0x13: min_max(code, From::Bc, true); break;
    case 0x14: case 0x15: case 0x16: case 0x17: min_max(code, From::Bc, false); break;
    case 0x18: case 0x19: case 0x1A: case 0x1B: arith(code, Op::Mul, From::Bc, false); break;
    case 0x1C: arith(code, Op::Mul, From::Q, false); break;
    case 0x1D: min_max(code, From::I, true); break;
    case 0x1E: arith(code, Op::Mul, From::I, false); break;
    case 0x1F: min_max(code, From::I, false); break;
    case 0x20: arith(code, Op::Add, From::Q, false); break;
    case 0x21: arith(code, Op::Madd, From::Q, false); break;
    case 0x22: arith(code, Op::Add, From::I, false); break;
    case 0x23: arith(code, Op::Madd, From::I, false); break;
    case 0x24: arith(code, Op::Sub, From::Q, false); break;
    case 0x25: arith(code, Op::Msub, From::Q, false); break;
    case 0x26: arith(code, Op::Sub, From::I, false); break;
    case 0x27: arith(code, Op::Msub, From::I, false); break;
    case 0x28: arith(code, Op::Add, From::Ft, false); break;
    case 0x29: arith(code, Op::Madd, From::Ft, false); break;
    case 0x2A: arith(code, Op::Mul, From::Ft, false); break;
    case 0x2B: min_max(code, From::Ft, true); break;
    case 0x2C: arith(code, Op::Sub, From::Ft, false); break;
    case 0x2D: arith(code, Op::Msub, From::Ft, false); break;
    case 0x2E: {  // OPMSUB: the second half of a cross product
      unsigned ft = (code >> 16) & 31, fs = (code >> 11) & 31, fd = (code >> 6) & 31;
      std::array<u32, 4> out = vf[fd];
      u32 flags = 0;
      for (unsigned field = 0; field < 3; field++) {
        unsigned a = (field + 1) % 3, b = (field + 2) % 3;
        u32 problems = 0;
        u32 value = fp::sub(acc[field], fp::mul(vf[fs][a], vf[ft][b], problems), problems);
        out[field] = result(value, problems, field, flags);
      }
      write_vf(fd, kXyz, out);
      post_flags(flags);
      break;
    }
    case 0x2F: min_max(code, From::Ft, false); break;
    case 0x3C: case 0x3D: case 0x3E: case 0x3F: upper_special(code); break;
    default:
      unknown_ops++;
      break;
  }
}

void Vu::upper_special(u32 code) {
  u32 fn = (((code >> 6) & 0x1F) << 2) | (code & 3);
  u32 dest = (code >> 21) & 0xF;
  unsigned ft = (code >> 16) & 31, fs = (code >> 11) & 31;
  switch (fn) {
    case 0x00: case 0x01: case 0x02: case 0x03: arith(code, Op::Add, From::Bc, true); break;
    case 0x04: case 0x05: case 0x06: case 0x07: arith(code, Op::Sub, From::Bc, true); break;
    case 0x08: case 0x09: case 0x0A: case 0x0B: arith(code, Op::Madd, From::Bc, true); break;
    case 0x0C: case 0x0D: case 0x0E: case 0x0F: arith(code, Op::Msub, From::Bc, true); break;
    case 0x10: case 0x11: case 0x12: case 0x13: {  // ITOF0, 4, 12, 15
      static constexpr s32 shift[4] = {0, 4, 12, 15};
      std::array<u32, 4> out{};
      for (unsigned field = 0; field < 4; field++) {
        out[field] = fp::from_int(static_cast<s32>(vf[fs][field]), shift[fn & 3]);
      }
      write_vf(ft, dest, out);
      break;
    }
    case 0x14: case 0x15: case 0x16: case 0x17: {  // FTOI0, 4, 12, 15
      static constexpr s32 shift[4] = {0, 4, 12, 15};
      std::array<u32, 4> out{};
      for (unsigned field = 0; field < 4; field++) {
        out[field] = static_cast<u32>(fp::to_int(vf[fs][field], shift[fn & 3]));
      }
      write_vf(ft, dest, out);
      break;
    }
    case 0x18: case 0x19: case 0x1A: case 0x1B: arith(code, Op::Mul, From::Bc, true); break;
    case 0x1C: arith(code, Op::Mul, From::Q, true); break;
    case 0x1D: {  // ABS
      std::array<u32, 4> out{};
      for (unsigned field = 0; field < 4; field++) {
        out[field] = vf[fs][field] & ~fp::kSign;
      }
      write_vf(ft, dest, out);
      break;
    }
    case 0x1E: arith(code, Op::Mul, From::I, true); break;
    case 0x1F: {  // CLIP: x, y and z of fs against plus and minus |w| of ft
      s64 w = fp::key(vf[ft][3] & ~fp::kSign);
      u32 now = 0;
      for (unsigned field = 0; field < 3; field++) {
        s64 v = fp::key(vf[fs][field]);
        if (v > w) now |= 1u << (field * 2);
        if (v < -w) now |= 2u << (field * 2);
      }
      clip_latest_ = ((clip_latest_ << 6) | now) & 0xFFFFFFu;
      post();
      break;
    }
    case 0x20: arith(code, Op::Add, From::Q, true); break;
    case 0x21: arith(code, Op::Madd, From::Q, true); break;
    case 0x22: arith(code, Op::Add, From::I, true); break;
    case 0x23: arith(code, Op::Madd, From::I, true); break;
    case 0x24: arith(code, Op::Sub, From::Q, true); break;
    case 0x25: arith(code, Op::Msub, From::Q, true); break;
    case 0x26: arith(code, Op::Sub, From::I, true); break;
    case 0x27: arith(code, Op::Msub, From::I, true); break;
    case 0x28: arith(code, Op::Add, From::Ft, true); break;
    case 0x29: arith(code, Op::Madd, From::Ft, true); break;
    case 0x2A: arith(code, Op::Mul, From::Ft, true); break;
    case 0x2C: arith(code, Op::Sub, From::Ft, true); break;
    case 0x2D: arith(code, Op::Msub, From::Ft, true); break;
    case 0x2E: {  // OPMULA: the first half of a cross product
      u32 flags = 0;
      std::array<u32, 4> out = acc;
      for (unsigned field = 0; field < 3; field++) {
        unsigned a = (field + 1) % 3, b = (field + 2) % 3;
        u32 problems = 0;
        u32 value = fp::mul(vf[fs][a], vf[ft][b], problems);
        out[field] = result(value, problems, field, flags);
      }
      acc = out;
      post_flags(flags);
      break;
    }
    case 0x2F:  // NOP
      break;
    default:
      unknown_ops++;
      break;
  }
}

// --- lower instructions --------------------------------------------------------

void Vu::lower(u32 code, u32 at) {
  u32 op = code >> 25;
  u32 dest = (code >> 21) & 0xF;
  unsigned it = (code >> 16) & 31, is = (code >> 11) & 31;
  s32 imm11 = sign_extend(code & 0x7FF, 11);
  u32 imm12 = ((code >> 10) & 0x800) | (code & 0x7FF);
  u32 imm15 = ((code >> 10) & 0x7800) | (code & 0x7FF);
  u32 imm24 = code & 0xFFFFFF;
  u32 next = at + 1 + static_cast<u32>(imm11);

  auto first_field = [dest]() -> unsigned {
    for (unsigned field = 0; field < 4; field++) {
      if (has(dest, field)) {
        return field;
      }
    }
    return 0;
  };

  switch (op) {
    case 0x00: {  // LQ
      const u8* m = quad(static_cast<u32>(vi[is & 15] + imm11));
      write_vf(it, dest, {load<u32>(m), load<u32>(m + 4), load<u32>(m + 8), load<u32>(m + 12)});
      break;
    }
    case 0x01: {  // SQ
      u8* m = quad(static_cast<u32>(vi[it & 15] + imm11));
      for (unsigned field = 0; field < 4; field++) {
        if (has(dest, field)) {
          store<u32>(m + field * 4, vf[is][field]);
        }
      }
      break;
    }
    case 0x04:  // ILW
      write_vi(it, load<u16>(quad(static_cast<u32>(vi[is & 15] + imm11)) + first_field() * 4));
      break;
    case 0x05: {  // ISW
      u8* m = quad(static_cast<u32>(vi[is & 15] + imm11));
      for (unsigned field = 0; field < 4; field++) {
        if (has(dest, field)) {
          store<u32>(m + field * 4, vi[it & 15]);
        }
      }
      break;
    }
    case 0x08:  // IADDIU
      write_vi(it, static_cast<u16>(vi[is & 15] + imm15));
      break;
    case 0x09:  // ISUBIU
      write_vi(it, static_cast<u16>(vi[is & 15] - imm15));
      break;
    case 0x10:  // FCEQ
      write_vi_from_flags(1, (clip & 0xFFFFFF) == imm24);
      break;
    case 0x11:  // FCSET
      clip_latest_ = imm24;
      post();
      break;
    case 0x12:  // FCAND
      write_vi_from_flags(1, (clip & imm24) != 0);
      break;
    case 0x13:  // FCOR
      write_vi_from_flags(1, ((clip | imm24) & 0xFFFFFF) == 0xFFFFFF);
      break;
    case 0x14:  // FSEQ
      write_vi_from_flags(it, (status & 0xFFF) == imm12);
      break;
    case 0x15:  // FSSET: the remembered bits only
      status_latest_ = (status_latest_ & 0x3F) | (imm12 & 0xFC0);
      post();
      break;
    case 0x16:  // FSAND
      write_vi_from_flags(it, static_cast<u16>(status & imm12));
      break;
    case 0x17:  // FSOR
      write_vi_from_flags(it, static_cast<u16>((status | imm12) & 0xFFF));
      break;
    case 0x18:  // FMEQ
      write_vi_from_flags(it, (mac & 0xFFFF) == vi[is & 15]);
      break;
    case 0x1A:  // FMAND
      write_vi_from_flags(it, static_cast<u16>(mac & vi[is & 15]));
      break;
    case 0x1B:  // FMOR
      write_vi_from_flags(it, static_cast<u16>(mac | vi[is & 15]));
      break;
    case 0x1C:  // FCGET
      write_vi_from_flags(it, static_cast<u16>(clip & 0xFFF));
      break;
    case 0x20:  // B
      branch(next);
      break;
    case 0x21:  // BAL
      if (it & 15) {
        vi[it & 15] = static_cast<u16>(at + 2);
      }
      branch(next);
      break;
    case 0x24:  // JR
      branch(vi[is & 15]);
      break;
    case 0x25: {  // JALR
      u32 target = vi[is & 15];
      if (it & 15) {
        vi[it & 15] = static_cast<u16>(at + 2);
      }
      branch(target);
      break;
    }
    case 0x28:  // IBEQ
      if (branch_vi(it) == branch_vi(is)) branch(next);
      break;
    case 0x29:  // IBNE
      if (branch_vi(it) != branch_vi(is)) branch(next);
      break;
    case 0x2C:  // IBLTZ
      if (static_cast<s16>(branch_vi(is)) < 0) branch(next);
      break;
    case 0x2D:  // IBGTZ
      if (static_cast<s16>(branch_vi(is)) > 0) branch(next);
      break;
    case 0x2E:  // IBLEZ
      if (static_cast<s16>(branch_vi(is)) <= 0) branch(next);
      break;
    case 0x2F:  // IBGEZ
      if (static_cast<s16>(branch_vi(is)) >= 0) branch(next);
      break;
    case 0x40:
      lower_special(code);
      break;
    default:
      unknown_ops++;
      break;
  }
}

void Vu::lower_special(u32 code) {
  u32 dest = (code >> 21) & 0xF;
  unsigned it = (code >> 16) & 31, is = (code >> 11) & 31, id = (code >> 6) & 31;
  unsigned fsf = (code >> 21) & 3, ftf = (code >> 23) & 3;
  u32 fn = code & 0x3F;

  if (fn < 0x3C) {
    switch (fn) {
      case 0x30:  // IADD
        write_vi(id, static_cast<u16>(vi[is & 15] + vi[it & 15]));
        break;
      case 0x31:  // ISUB
        write_vi(id, static_cast<u16>(vi[is & 15] - vi[it & 15]));
        break;
      case 0x32:  // IADDI
        write_vi(it, static_cast<u16>(vi[is & 15] + sign_extend(id, 5)));
        break;
      case 0x34:  // IAND
        write_vi(id, vi[is & 15] & vi[it & 15]);
        break;
      case 0x35:  // IOR
        write_vi(id, vi[is & 15] | vi[it & 15]);
        break;
      default:
        unknown_ops++;
        break;
    }
    return;
  }

  auto load_quad = [this](u32 address) -> std::array<u32, 4> {
    const u8* m = quad(address);
    return {load<u32>(m), load<u32>(m + 4), load<u32>(m + 8), load<u32>(m + 12)};
  };
  auto store_quad = [this, dest](u32 address, const std::array<u32, 4>& v) {
    u8* m = quad(address);
    for (unsigned field = 0; field < 4; field++) {
      if (has(dest, field)) {
        store<u32>(m + field * 4, v[field]);
      }
    }
  };
  auto all = [](u32 v) -> std::array<u32, 4> { return {v, v, v, v}; };
  auto advance_random = [this] {
    u32 x = (r >> 4) & 1, y = (r >> 22) & 1;
    r = (((r << 1) ^ x ^ y) & 0x7FFFFF) | 0x3F800000;
  };
  u32 index = (((code >> 6) & 0x1F) << 2) | (code & 3);
  // The function unit's operands, for the instructions that use it.
  double x = 0, y = 0, z = 0, one = 0;
  if (index >= 0x70) {
    x = fp::to_double(vf[is][0]);
    y = fp::to_double(vf[is][1]);
    z = fp::to_double(vf[is][2]);
    one = fp::to_double(vf[is][fsf]);
  }

  switch (index) {
    case 0x30:  // MOVE
      write_vf(it, dest, vf[is]);
      break;
    case 0x31:  // MR32: rotate the fields one place
      write_vf(it, dest, {vf[is][1], vf[is][2], vf[is][3], vf[is][0]});
      break;
    case 0x34: {  // LQI
      u16 address = vi[is & 15];
      write_vf(it, dest, load_quad(address));
      write_vi(is, static_cast<u16>(address + 1));
      break;
    }
    case 0x35: {  // SQI
      u16 address = vi[it & 15];
      store_quad(address, vf[is]);
      write_vi(it, static_cast<u16>(address + 1));
      break;
    }
    case 0x36: {  // LQD
      u16 address = static_cast<u16>(vi[is & 15] - 1);
      write_vi(is, address);
      write_vf(it, dest, load_quad(address));
      break;
    }
    case 0x37: {  // SQD
      u16 address = static_cast<u16>(vi[it & 15] - 1);
      write_vi(it, address);
      store_quad(address, vf[is]);
      break;
    }
    case 0x38: {  // DIV
      u32 problems = 0;
      u32 value = fp::div(vf[is][fsf], vf[it][ftf], problems);
      start_q(value, kDiv, divide_flags(problems));
      break;
    }
    case 0x39: {  // SQRT
      u32 problems = 0;
      u32 value = fp::sqrt(vf[it][ftf], problems);
      start_q(value, kSqrt, divide_flags(problems));
      break;
    }
    case 0x3A: {  // RSQRT
      u32 problems = 0;
      u32 value = fp::rsqrt(vf[is][fsf], vf[it][ftf], problems);
      start_q(value, kRsqrt, divide_flags(problems));
      break;
    }
    case 0x3B:  // WAITQ: the pair has waited; the result is in
      break;
    case 0x3C:  // MTIR
      write_vi(it, static_cast<u16>(vf[is][fsf]));
      break;
    case 0x3D:  // MFIR
      write_vf(it, dest, all(static_cast<u32>(static_cast<s32>(static_cast<s16>(vi[is & 15])))));
      break;
    case 0x3E:  // ILWR
      for (unsigned field = 0; field < 4; field++) {
        if (has(dest, field)) {
          write_vi(it, load<u16>(quad(vi[is & 15]) + field * 4));
          break;
        }
      }
      break;
    case 0x3F:  // ISWR
      store_quad(vi[is & 15], all(vi[it & 15]));
      break;
    case 0x40:  // RNEXT
      advance_random();
      write_vf(it, dest, all(r));
      break;
    case 0x41:  // RGET
      write_vf(it, dest, all(r));
      break;
    case 0x42:  // RINIT
      r = (vf[is][fsf] & 0x7FFFFF) | 0x3F800000;
      break;
    case 0x43:  // RXOR
      r = ((r ^ vf[is][fsf]) & 0x7FFFFF) | 0x3F800000;
      break;
    case 0x64:  // MFP
      write_vf(it, dest, all(p));
      break;
    case 0x68:  // XTOP
      write_vi(it, static_cast<u16>(on_top ? on_top() : 0));
      break;
    case 0x69:  // XITOP
      write_vi(it, static_cast<u16>(on_itop ? on_itop() : 0));
      break;
    case 0x6C:  // XGKICK
      if (kick_in_) {
        fire_kick();
      }
      kick_address_ = vi[is & 15];
      kick_in_ = 2;  // the packet goes after the next instruction has run
      break;
    case 0x70: start_p(x * x + y * y + z * z, 11); break;                    // ESADD
    case 0x71: start_p(1.0f / (x * x + y * y + z * z), 18); break;           // ERSADD
    case 0x72: start_p(std::sqrt(x * x + y * y + z * z), 18); break;         // ELENG
    case 0x73: start_p(1.0f / std::sqrt(x * x + y * y + z * z), 24); break;  // ERLENG
    case 0x74: start_p(std::atan2(y, x), 54); break;                         // EATANxy
    case 0x75: start_p(std::atan2(z, x), 54); break;                         // EATANxz
    case 0x76: start_p(x + y + z + fp::to_double(vf[is][3]), 12); break;     // ESUM
    case 0x78: start_p(std::sqrt(std::fabs(one)), 12); break;                // ESQRT
    case 0x79: start_p(1.0f / std::sqrt(std::fabs(one)), 18); break;         // ERSQRT
    case 0x7A: start_p(1.0f / one, 12); break;                               // ERCPR
    case 0x7B:  // WAITP: likewise
      break;
    case 0x7C: start_p(std::sin(one), 29); break;    // ESIN
    case 0x7D: start_p(std::atan(one), 54); break;   // EATAN
    case 0x7E: start_p(std::exp(-one), 44); break;   // EEXP
    default:
      unknown_ops++;
      break;
  }
}

}  // namespace ps2
