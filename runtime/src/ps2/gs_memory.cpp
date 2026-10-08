// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#include "gs_memory.h"

#include <array>

namespace ps2 {
namespace {

// Blocks within a page. In the 32-bit and 8-bit formats a page is 8 blocks
// wide and 4 high and the block number interleaves the bits of the block's
// column and row, column first. In the 16-bit and 4-bit formats a page is 4
// wide and 8 high and the row comes first. PSMCT16S moves the row's top bit
// down. The Z formats use the same orders with the two top bits inverted.
constexpr u32 block_wide(u32 bx, u32 by) {
  return (bx & 1) | ((by & 1) << 1) | ((bx & 2) << 1) | ((by & 2) << 2) | ((bx & 4) << 2);
}

constexpr u32 block_tall(u32 bx, u32 by) {
  return (by & 1) | ((bx & 1) << 1) | ((by & 2) << 1) | ((bx & 2) << 2) | ((by & 4) << 2);
}

constexpr u32 block_tall_s(u32 bx, u32 by) {
  return (by & 1) | ((bx & 1) << 1) | (by & 4) | ((by & 2) << 2) | ((bx & 2) << 3);
}

// Pixels within a block. A block is four columns of sixteen 32-bit words. A
// column holds eight words per pixel row pair, ordered by (x bit 0, y bit 0,
// then the rest of x). Narrower formats pack several pixels into each word:
// the part of the word a pixel takes depends on which eighth of the column
// row it is in and on which half of the column's rows, and in the second
// half (the first half in odd columns) the two groups of four words swap.
constexpr u32 column_word(u32 x8, u32 y2) {
  return (x8 & 1) | ((y2 & 1) << 1) | ((x8 >> 1) << 2);
}

struct Tables {
  std::array<std::array<u8, 8>, 8> column32{};
  std::array<std::array<u8, 16>, 8> column16{};
  std::array<std::array<u8, 16>, 16> column8{};
  std::array<std::array<u16, 32>, 16> column4{};

  constexpr Tables() {
    for (u32 y = 0; y < 8; y++) {
      for (u32 x = 0; x < 8; x++) {
        column32[y][x] = static_cast<u8>((y >> 1) * 16 + column_word(x, y & 1));
      }
      for (u32 x = 0; x < 16; x++) {
        u32 word = (y >> 1) * 16 + column_word(x & 7, y & 1);
        column16[y][x] = static_cast<u8>(word * 2 + (x >> 3));
      }
    }
    for (u32 y = 0; y < 16; y++) {
      u32 column = y >> 2;
      u32 row = y & 3;
      bool swap = ((row >> 1) ^ (column & 1)) != 0;
      for (u32 x = 0; x < 16; x++) {
        u32 x8 = (x & 7) ^ (swap ? 4 : 0);
        u32 word = column * 16 + column_word(x8, row & 1);
        column8[y][x] = static_cast<u8>(word * 4 + (x >> 3) * 2 + (row >> 1));
      }
      for (u32 x = 0; x < 32; x++) {
        u32 x8 = (x & 7) ^ (swap ? 4 : 0);
        u32 word = column * 16 + column_word(x8, row & 1);
        column4[y][x] = static_cast<u16>(word * 8 + (x >> 3) * 2 + (row >> 1));
      }
    }
  }
};

constexpr Tables kTables{};

constexpr u32 kBlockMask = GsMemory::kBlocks - 1;

}  // namespace

unsigned transfer_bits(u32 psm) {
  switch (psm) {
    case PSMCT32:
    case PSMZ32:
      return 32;
    case PSMCT24:
    case PSMZ24:
      return 24;
    case PSMCT16:
    case PSMCT16S:
    case PSMZ16:
    case PSMZ16S:
      return 16;
    case PSMT8:
    case PSMT8H:
      return 8;
    case PSMT4:
    case PSMT4HL:
    case PSMT4HH:
      return 4;
    default:
      return 32;
  }
}

GsMemory::GsMemory() : bytes_(kBytes) {}

u32 GsMemory::address32(u32 bp, u32 bw, u32 x, u32 y, bool z) {
  u32 block = bp + (y & ~0x1Fu) * bw + ((x >> 1) & ~0x1Fu) +
              (block_wide((x >> 3) & 7, (y >> 3) & 3) ^ (z ? 0x18u : 0u));
  return (block & kBlockMask) * 64 + kTables.column32[y & 7][x & 7];
}

u32 GsMemory::address16(u32 bp, u32 bw, u32 x, u32 y, bool z) {
  u32 block = bp + ((y >> 1) & ~0x1Fu) * bw + ((x >> 1) & ~0x1Fu) +
              (block_tall((x >> 4) & 3, (y >> 3) & 7) ^ (z ? 0x18u : 0u));
  return (block & kBlockMask) * 128 + kTables.column16[y & 7][x & 15];
}

u32 GsMemory::address16s(u32 bp, u32 bw, u32 x, u32 y, bool z) {
  u32 block = bp + ((y >> 1) & ~0x1Fu) * bw + ((x >> 1) & ~0x1Fu) +
              (block_tall_s((x >> 4) & 3, (y >> 3) & 7) ^ (z ? 0x18u : 0u));
  return (block & kBlockMask) * 128 + kTables.column16[y & 7][x & 15];
}

u32 GsMemory::address8(u32 bp, u32 bw, u32 x, u32 y) {
  u32 block = bp + ((y >> 1) & ~0x1Fu) * (bw >> 1) + ((x >> 2) & ~0x1Fu) +
              block_wide((x >> 4) & 7, (y >> 4) & 3);
  return (block & kBlockMask) * 256 + kTables.column8[y & 15][x & 15];
}

u32 GsMemory::address4(u32 bp, u32 bw, u32 x, u32 y) {
  u32 block = bp + ((y >> 2) & ~0x1Fu) * (bw >> 1) + ((x >> 2) & ~0x1Fu) +
              block_tall((x >> 5) & 3, (y >> 4) & 7);
  return (block & kBlockMask) * 512 + kTables.column4[y & 15][x & 31];
}

u32 GsMemory::read(u32 psm, u32 bp, u32 bw, u32 x, u32 y) const {
  switch (psm) {
    case PSMCT32:
      return word(address32(bp, bw, x, y));
    case PSMCT24:
      return word(address32(bp, bw, x, y)) & 0x00FFFFFFu;
    case PSMCT16:
      return half(address16(bp, bw, x, y));
    case PSMCT16S:
      return half(address16s(bp, bw, x, y));
    case PSMT8:
      return bytes_[address8(bp, bw, x, y)];
    case PSMT4: {
      u32 a = address4(bp, bw, x, y);
      return (bytes_[a >> 1] >> ((a & 1) * 4)) & 0xF;
    }
    case PSMT8H:
      return word(address32(bp, bw, x, y)) >> 24;
    case PSMT4HL:
      return (word(address32(bp, bw, x, y)) >> 24) & 0xF;
    case PSMT4HH:
      return word(address32(bp, bw, x, y)) >> 28;
    case PSMZ32:
      return word(address32(bp, bw, x, y, true));
    case PSMZ24:
      return word(address32(bp, bw, x, y, true)) & 0x00FFFFFFu;
    case PSMZ16:
      return half(address16(bp, bw, x, y, true));
    case PSMZ16S:
      return half(address16s(bp, bw, x, y, true));
    default:
      return 0;
  }
}

void GsMemory::write(u32 psm, u32 bp, u32 bw, u32 x, u32 y, u32 value) {
  auto merge = [this](u32 index, u32 v, u32 mask) {
    set_word(index, (word(index) & ~mask) | (v & mask));
  };
  switch (psm) {
    case PSMCT32:
      set_word(address32(bp, bw, x, y), value);
      break;
    case PSMCT24:
      merge(address32(bp, bw, x, y), value, 0x00FFFFFFu);
      break;
    case PSMCT16:
      set_half(address16(bp, bw, x, y), static_cast<u16>(value));
      break;
    case PSMCT16S:
      set_half(address16s(bp, bw, x, y), static_cast<u16>(value));
      break;
    case PSMT8:
      bytes_[address8(bp, bw, x, y)] = static_cast<u8>(value);
      break;
    case PSMT4: {
      u32 a = address4(bp, bw, x, y);
      unsigned shift = (a & 1) * 4;
      u8& b = bytes_[a >> 1];
      b = static_cast<u8>((b & ~(0xF << shift)) | ((value & 0xF) << shift));
      break;
    }
    case PSMT8H:
      merge(address32(bp, bw, x, y), value << 24, 0xFF000000u);
      break;
    case PSMT4HL:
      merge(address32(bp, bw, x, y), value << 24, 0x0F000000u);
      break;
    case PSMT4HH:
      merge(address32(bp, bw, x, y), value << 28, 0xF0000000u);
      break;
    case PSMZ32:
      set_word(address32(bp, bw, x, y, true), value);
      break;
    case PSMZ24:
      merge(address32(bp, bw, x, y, true), value, 0x00FFFFFFu);
      break;
    case PSMZ16:
      set_half(address16(bp, bw, x, y, true), static_cast<u16>(value));
      break;
    case PSMZ16S:
      set_half(address16s(bp, bw, x, y, true), static_cast<u16>(value));
      break;
    default:
      break;
  }
}

}  // namespace ps2
