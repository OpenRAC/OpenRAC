// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
#pragma once

#include "gif.h"
#include "gs.h"
#include "vif.h"
#include "vu.h"

namespace ps2 {

// The drawing side of the machine, connected as it is on the board: VIF1
// feeds VU1's memories and starts its programs, VU1 kicks GIF packets out of
// its data memory on path 1, VIF1 passes packets on path 2, and both end at
// the GS.
struct Graphics {
  Gs gs;
  Gif gif{gs};
  Vif1 vif{gif};
  Vu vu1{Vu::Memory{vif.micro.data(), Vif1::kMemoryBytes, vif.data.data(), Vif1::kMemoryBytes}};

  u64 vu1_instructions = 0, vu1_starts = 0, vu1_runaways = 0;
  u32 vu1_runaway_start = 0, vu1_runaway_pc = 0;

  Graphics() {
    vif.on_start = [this](u32 address, bool resume) {
      // No real program runs this long between two stops; one that does has
      // gone wrong here.
      const u64 limit = 4'000'000;
      vu1_instructions += resume ? vu1.resume(limit) : vu1.run(address, limit);
      vu1_starts++;
      if (!vu1.stopped()) {
        vu1_runaways++;
        vu1_runaway_start = address;
        vu1_runaway_pc = vu1.pc;
      }
    };
    vu1.on_top = [this] { return vif.top; };
    vu1.on_itop = [this] { return vif.itop; };
    vu1.on_kick = [this](u32 quadword) {
      // A packet runs to the end of the data its last tag (the one with EOP)
      // announces, and wraps at the end of data memory.
      const u32 mask = Vif1::kMemoryBytes / 16 - 1;
      u32 at = quadword & mask;
      for (u32 n = 0; n <= mask; n++) {
        gif.write(1, &vif.data[at * 16], 1);
        at = (at + 1) & mask;
        if (gif.idle(1)) {
          break;
        }
      }
    };
  }

  Graphics(const Graphics&) = delete;
  Graphics& operator=(const Graphics&) = delete;
};

}  // namespace ps2
