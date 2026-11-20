/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal's hand-written renderers the port does not run. As Going Commando's
 * (rac2/host/handasm_render_big.c), the game's own packet builders give way to the port's native
 * renderers, which draw from the game's state; until rac3's are written these draw nothing, so the
 * game goes on past them.
 *
 * - 0x003CB6B8 (boot_elf, hand-written): draws the moby list (D_001D9D30, D_001D9D38) through
 *   0x003C81D8 with VU0 and the scratchpad, called by 0x003C39A8 in the level loading screen's
 *   pass (0x003C3BB8). Translated from its instructions it read past the scratchpad
 *   (0x700045FC). */
#include <stdio.h>

#include "openrac/game_host.h"
#include "openrac/guest.h"

void func_003CB6B8(int list, int count) {
    static int told;
    (void)list;
    (void)count;
    if (!told) {
        told = 1;
        fprintf(
            stderr,
            "[info] rac3: the moby renderer (0x003CB6B8) draws nothing until the native one\n"
        );
    }
}

/* A level's frame draw: frontbin's 0x003D9CF0, which rac3-decomp's catalogue places in every level
 * (hostgen names those copies func_L01_004B3238). It uploads the VU1 programs and builds the
 * world, moby and particle packets through hand-written code (0x003D9E28 and on) that waits on the
 * DMAC in the scratchpad (Veldin's start-up stopped in one such wait, func_L01_004B44CC). The
 * native level renderers take its place; until then the level's update runs on without a picture. */
void func_L01_004B3238(void) {
    static int told;
    if (!told) {
        told = 1;
        fprintf(
            stderr,
            "[info] rac3: the level's frame draw (0x003D9CF0's copies) draws nothing until the "
            "native "
            "renderers\n"
        );
    }
}

/* frontbin's and the levels' copies of the moby renderer (frontbin's 0x003CB300, which the
 * catalogue places in every level: hostgen names those copies func_L01_00441A98), called by the
 * moby pass 0x003C35F0 outside the frame draw. Translated from its instructions it went on past
 * the scratchpad's 16 KB (0x700045FC, through 0x003C7E20) in Veldin's first seconds. */
void func_003CB300__frontbin(int list, int count) {
    func_003CB6B8(list, count);
}

void func_L01_00441A98(int list, int count) {
    func_003CB6B8(list, count);
}
