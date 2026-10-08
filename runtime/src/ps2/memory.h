// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

#include <vector>

#include "types.h"

namespace ps2 {

// The memory a game program sees: 32 MB of main memory and the 16 KB
// scratchpad. Every address the game stores is an offset into one of them.
class GuestMemory {
 public:
  static constexpr u32 kRamBytes = 32 * 1024 * 1024;
  static constexpr u32 kScratchpadBytes = 16 * 1024;

  GuestMemory() : ram_(kRamBytes), scratchpad_(kScratchpadBytes) {}

  // A DMA address: bit 31 selects the scratchpad (the SPR flag of a tag or of
  // MADR), otherwise it is a main memory address in any segment.
  u8* dma(u32 address) {
    if (address & 0x80000000u) {
      return scratchpad_.data() + (address & (kScratchpadBytes - 1) & ~0xFu);
    }
    return ram_.data() + (address & (kRamBytes - 1));
  }

  u8* ram(u32 address) { return ram_.data() + (address & (kRamBytes - 1)); }
  u8* scratchpad(u32 offset) { return scratchpad_.data() + (offset & (kScratchpadBytes - 1)); }

  template <typename T>
  T read(u32 address) {
    return load<T>(ram(address));
  }

  template <typename T>
  void write(u32 address, T value) {
    store<T>(ram(address), value);
  }

 private:
  std::vector<u8> ram_;
  std::vector<u8> scratchpad_;
};

}  // namespace ps2
