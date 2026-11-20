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
