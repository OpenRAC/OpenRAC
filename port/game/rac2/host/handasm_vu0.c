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

/* The render routines themselves: whole hand-written VU0 kernels that build the frame's draw packets.
 * Their only callers are routines the port replaces with its own renderer (host_functions:
 * func_002EF3B0 for func_002EF480, func_002F0EF8 for the other three), so the port never runs them.
 * They are not written in C. Like the fragments, they report if anything ever reaches them. */
#define REPLACED(name, caller)                                                                       \
    void name(void) {                                                                                \
        static openrac_reach_site site = {#name, "unreachable",                                      \
                                          "a renderer only " caller " calls, which the port replaces", \
                                          0, 0};                                                     \
        openrac_guest_reach(&site);                                                                  \
    }

REPLACED(func_002EF480, "func_002EF3B0")
REPLACED(func_002F1010, "func_002F0EF8")
REPLACED(func_002F1E88, "func_002F0EF8")
REPLACED(func_002F2A38, "func_002F0EF8")
