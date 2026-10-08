// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

#include <functional>

#include "memory.h"
#include "types.h"

namespace ps2 {

// The registers of one DMA channel, as the EE's DMA controller keeps them.
struct DmaChannel {
  u32 chcr = 0;  // bit 6 TTE, bit 7 TIE, bit 8 STR, bits 4-5 ASP, bits 16-31 TAG
  u32 madr = 0;
  u32 qwc = 0;
  u32 tadr = 0;
  u32 asr[2] = {0, 0};
};

namespace dmatag {
enum : u32 { REFE, CNT, NEXT, REF, REFS, CALL, RET, END };
}

enum class DmaStop {
  End,        // the chain finished
  Interrupt,  // a tag with its IRQ bit set, with TIE on: the game's handler runs, then the chain may be resumed
  Runaway,    // more tags than any real list has: the list is corrupt
};

// Where a channel's data goes (VIF1 for channel 1).
using DmaSink = std::function<void(const u8* data, std::size_t bytes)>;

// Run a source chain from the channel's current state until it stops. The
// channel's registers are left as the hardware leaves them, so a handler can
// read TADR and the TAG field and the chain can be resumed.
DmaStop run_source_chain(GuestMemory& memory, DmaChannel& channel, const DmaSink& sink);

}  // namespace ps2
