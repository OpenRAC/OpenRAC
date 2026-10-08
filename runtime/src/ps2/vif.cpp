// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "vif.h"

#include <algorithm>

namespace ps2 {
namespace {

enum : u32 {
  kNop = 0x00,
  kStcycl = 0x01,
  kOffset = 0x02,
  kBase = 0x03,
  kItop = 0x04,
  kStmod = 0x05,
  kMskpath3 = 0x06,
  kMark = 0x07,
  kFlushe = 0x10,
  kFlush = 0x11,
  kFlusha = 0x13,
  kMscal = 0x14,
  kMscalf = 0x15,
  kMscnt = 0x17,
  kStmask = 0x20,
  kStrow = 0x30,
  kStcol = 0x31,
  kMpg = 0x4A,
  kDirect = 0x50,
  kDirecthl = 0x51,
};

inline u32 command(u32 code) {
  return (code >> 24) & 0x7F;
}

inline u32 count(u32 code) {
  u32 n = (code >> 16) & 0xFF;
  return n ? n : 256;
}

inline bool is_unpack(u32 code) {
  return (command(code) & 0x60) == 0x60;
}

// Bytes one vector takes in the list: `vn + 1` elements of 32, 16 or 8 bits,
// or 16 bits in all for the 5:5:5:1 colour format.
inline u32 vector_bytes(u32 code) {
  u32 vn = (command(code) >> 2) & 3, vl = command(code) & 3;
  return (vn == 3 && vl == 3) ? 2 : (vn + 1) * (4u >> vl);
}

}  // namespace

void Vif1::reset() {
  data.fill(0);
  micro.fill(0);
  cl = wl = 1;
  mode = mask = 0;
  row.fill(0);
  col.fill(0);
  base = ofst = tops = top = itops = itop = mark = 0;
  dbf = false;
  path3_masked = false;
  pending_.clear();
}

std::size_t Vif1::operand_words(u32 code) const {
  u32 cmd = command(code);
  if (is_unpack(code)) {
    // With WL <= CL every vector written comes from the list. With WL > CL
    // only the first CL of every WL do; the rest are filled in.
    u32 n = count(code);
    u32 from_list = (wl <= cl || wl == 0) ? n : cl * (n / wl) + std::min(n % wl, cl);
    return (from_list * vector_bytes(code) + 3) / 4;
  }
  switch (cmd) {
    case kStmask:
      return 1;
    case kStrow:
    case kStcol:
      return 4;
    case kMpg:
      return count(code) * 2;
    case kDirect:
    case kDirecthl: {
      u32 quadwords = code & 0xFFFF;
      return (quadwords ? quadwords : 65536) * 4;
    }
    default:
      return 0;
  }
}

void Vif1::write(const u8* bytes, std::size_t size) {
  std::size_t words = size / 4;
  std::size_t old = pending_.size();
  pending_.resize(old + words);
  std::memcpy(pending_.data() + old, bytes, words * 4);

  std::size_t at = 0;
  while (at < pending_.size()) {
    u32 code = pending_[at];
    std::size_t operands = operand_words(code);
    if (at + 1 + operands > pending_.size()) {
      break;
    }
    execute(code, pending_.data() + at + 1, operands);
    at += 1 + operands;
  }
  pending_.erase(pending_.begin(), pending_.begin() + static_cast<std::ptrdiff_t>(at));
}

void Vif1::start(u32 address, bool resume) {
  // Starting a program hands it the buffer just filled and moves the VIF on
  // to the other one of the pair.
  itop = itops;
  top = tops;
  if (dbf) {
    tops = base;
    dbf = false;
  } else {
    tops = (base + ofst) & 0x3FF;
    dbf = true;
  }
  if (on_start) {
    on_start(address, resume);
  }
}

void Vif1::execute(u32 code, const u32* operands, std::size_t words) {
  u32 imm = code & 0xFFFF;
  if (is_unpack(code)) {
    unpack(code, operands, words);
    return;
  }
  switch (command(code)) {
    case kNop:
    case kFlushe:
    case kFlush:
    case kFlusha:
      // Everything here completes before the next command is read, so there
      // is nothing to wait for.
      break;
    case kStcycl:
      cl = imm & 0xFF;
      wl = imm >> 8;
      break;
    case kOffset:
      ofst = imm & 0x3FF;
      dbf = false;
      tops = base;
      break;
    case kBase:
      base = imm & 0x3FF;
      break;
    case kItop:
      itops = imm & 0x3FF;
      break;
    case kStmod:
      mode = imm & 3;
      break;
    case kMskpath3:
      path3_masked = (imm & 0x8000) != 0;
      break;
    case kMark:
      mark = imm;
      break;
    case kMscal:
    case kMscalf:
      start(imm, false);
      break;
    case kMscnt:
      start(0, true);
      break;
    case kStmask:
      mask = operands[0];
      break;
    case kStrow:
      std::copy(operands, operands + 4, row.begin());
      break;
    case kStcol:
      std::copy(operands, operands + 4, col.begin());
      break;
    case kMpg: {
      // `imm` is the load address in instructions of 8 bytes; the program
      // memory wraps.
      std::size_t at = (static_cast<std::size_t>(imm) * 8) & (kMemoryBytes - 1);
      const u8* src = reinterpret_cast<const u8*>(operands);
      for (std::size_t i = 0; i < words * 4; i++) {
        micro[(at + i) & (kMemoryBytes - 1)] = src[i];
      }
      if (on_program) {
        on_program();
      }
      break;
    }
    case kDirect:
    case kDirecthl:
      gif_.write(2, reinterpret_cast<const u8*>(operands), words / 4);
      break;
    default:
      unknown_codes++;
      break;
  }
}

void Vif1::unpack(u32 code, const u32* operands, std::size_t words) {
  u32 cmd = command(code), imm = code & 0xFFFF;
  u32 vn = (cmd >> 2) & 3, vl = cmd & 3;
  bool masked = (cmd & 0x10) != 0;
  bool is_unsigned = (imm & 0x4000) != 0;
  u32 address = imm & 0x3FF;
  if (imm & 0x8000) {
    address += tops;
  }

  const u8* src = reinterpret_cast<const u8*>(operands);
  const u8* end = src + words * 4;
  u32 bytes = vector_bytes(code);

  auto element = [&](const u8* p) -> u32 {
    if (p >= end) {
      return 0;
    }
    switch (vl) {
      case 0:
        return load<u32>(p);
      case 1:
        return is_unsigned ? u32{load<u16>(p)} : static_cast<u32>(static_cast<s32>(load<s16>(p)));
      default:
        return is_unsigned ? u32{*p} : static_cast<u32>(static_cast<s32>(static_cast<s8>(*p)));
    }
  };
  u32 step = 4u >> vl;

  u32 total = count(code);
  u32 cycle = 0;
  for (u32 n = 0; n < total; n++) {
    bool from_list = wl <= cl || cycle < cl;
    std::array<u32, 4> in{};
    if (from_list) {
      if (vn == 3 && vl == 3) {
        u32 c = load<u16>(src);
        in = {(c & 0x1F) << 3, ((c >> 5) & 0x1F) << 3, ((c >> 10) & 0x1F) << 3, ((c >> 15) & 1) << 7};
      } else if (vn == 0) {
        in.fill(element(src));
      } else if (vn == 1) {
        // The two fields that are not in the list repeat the two that are.
        u32 x = element(src), y = element(src + step);
        in = {x, y, x, y};
      } else {
        // A three-element vector's W is whatever follows it in the list.
        for (u32 f = 0; f < 4; f++) {
          in[f] = element(src + f * step);
        }
      }
      src += bytes;
    }

    u8* dst = &data[(address & 0x3FF) * 16];
    u32 line = std::min<u32>(cycle, 3);
    for (u32 f = 0; f < 4; f++) {
      u32 m = masked ? (mask >> ((line * 4 + f) * 2)) & 3 : 0;
      u32 value;
      switch (m) {
        case 0:
          value = in[f];
          if (mode == 1) {
            value += row[f];
          } else if (mode == 2) {
            value += row[f];
            row[f] = value;
          }
          break;
        case 1:
          value = row[f];
          break;
        case 2:
          value = col[line];
          break;
        default:
          continue;  // write protected
      }
      store<u32>(dst + f * 4, value);
    }

    address++;
    cycle++;
    if (wl <= cl) {
      if (cycle == wl) {
        address += cl - wl;
        cycle = 0;
      }
    } else if (cycle == wl) {
      cycle = 0;
    }
  }
}

}  // namespace ps2
