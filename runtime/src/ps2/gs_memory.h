// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

#include <vector>

#include "types.h"

namespace ps2 {

// Pixel storage formats (the PSM field of FRAME, ZBUF, TEX0 and BITBLTBUF).
enum : u32 {
  PSMCT32 = 0x00,
  PSMCT24 = 0x01,
  PSMCT16 = 0x02,
  PSMCT16S = 0x0A,
  PSMT8 = 0x13,
  PSMT4 = 0x14,
  PSMT8H = 0x1B,
  PSMT4HL = 0x24,
  PSMT4HH = 0x2C,
  PSMZ32 = 0x30,
  PSMZ24 = 0x31,
  PSMZ16 = 0x32,
  PSMZ16S = 0x3A,
};

// How many bits one pixel of a format takes in a transfer.
unsigned transfer_bits(u32 psm);

// GS local memory: 4 MB, addressed by the games in blocks of 256 bytes. A
// buffer is a block pointer `bp` and a width `bw` in units of 64 pixels.
// Where the pixel (x, y) of a buffer lies depends on the format: a page of
// 8,192 bytes holds 32 blocks in a format-specific order, and a block holds
// four columns whose pixels are interleaved.
class GsMemory {
 public:
  static constexpr u32 kBytes = 4 * 1024 * 1024;
  static constexpr u32 kBlocks = kBytes / 256;

  GsMemory();

  // The stored value of a pixel, as wide as the format (32, 24, 16, 8 or 4
  // bits). PSMT8H, PSMT4HL and PSMT4HH read their bits out of a 32-bit word.
  u32 read(u32 psm, u32 bp, u32 bw, u32 x, u32 y) const;
  void write(u32 psm, u32 bp, u32 bw, u32 x, u32 y, u32 value);

  // Index of the pixel's storage unit: a 32-bit word, a 16-bit half, a byte
  // or a nibble of local memory. Exposed for tests.
  static u32 address32(u32 bp, u32 bw, u32 x, u32 y, bool z = false);
  static u32 address16(u32 bp, u32 bw, u32 x, u32 y, bool z = false);
  static u32 address16s(u32 bp, u32 bw, u32 x, u32 y, bool z = false);
  static u32 address8(u32 bp, u32 bw, u32 x, u32 y);
  static u32 address4(u32 bp, u32 bw, u32 x, u32 y);

  u8* data() { return bytes_.data(); }
  const u8* data() const { return bytes_.data(); }

 private:
  u32 word(u32 index) const { return load<u32>(&bytes_[(index & (kBytes / 4 - 1)) * 4]); }
  void set_word(u32 index, u32 v) { store<u32>(&bytes_[(index & (kBytes / 4 - 1)) * 4], v); }
  u16 half(u32 index) const { return load<u16>(&bytes_[(index & (kBytes / 2 - 1)) * 2]); }
  void set_half(u32 index, u16 v) { store<u16>(&bytes_[(index & (kBytes / 2 - 1)) * 2], v); }

  std::vector<u8> bytes_;
};

}  // namespace ps2
