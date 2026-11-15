/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Fragments of Going Commando's hand-written render assembly: entries that live inside or next to a
 * larger routine and work on registers that routine set up, so no C call can reach them meaningfully.
 * The DMA waits spin on a scratchpad channel's busy bit and leave $ra pointing at the scratchpad
 * (0x70000000), which their parent then uses as its base register; the others continue the parent's
 * work on registers it keeps live ($s5..$s7, $fp). Their parents are written as whole routines, the
 * fragments as the parents' own helpers, so these entries only report when something calls them. */
#include "openrac/game_host.h"
#include "openrac/guest.h"

#define FRAGMENT(name, parent)                                                                         \
    void name(void) {                                                                                  \
        static openrac_reach_site site = {#name, "unreachable",                                        \
                                          "a fragment of " parent ", entered only from its assembly", \
                                          0, 0};                                                       \
        openrac_guest_reach(&site);                                                                    \
    }

FRAGMENT(func_002F1E60, "func_002F1010")
FRAGMENT(func_002F29E4, "func_002F1E88")
FRAGMENT(func_002F2A0C, "func_002F1E88 and func_002F2A38")
FRAGMENT(func_002F32AC, "func_002F2A38")
FRAGMENT(func_002F32D4, "func_002F2A38")
FRAGMENT(func_002F3C40, "func_002F35B0")
FRAGMENT(func_002F3D20, "func_002F35B0")
FRAGMENT(func_002F3E40, "func_002F35B0")
FRAGMENT(func_002F4F78, "func_002F3E78")
