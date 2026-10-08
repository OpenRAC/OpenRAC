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

  u64 vu1_instructions = 0;

  Graphics() {
    vif.on_start = [this](u32 address, bool resume) {
      vu1_instructions += resume ? vu1.resume() : vu1.run(address);
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
