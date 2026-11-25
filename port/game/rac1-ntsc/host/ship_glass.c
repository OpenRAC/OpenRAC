/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 * The ship's glass: rac1-pal/host/ship_glass.c at the NTSC decomp's function (PAL func_00233AB8).
 * Its quads get ids so that a frame between ticks moves them with the hull. */
#include <stdint.h>

#include "game_protos.h"
#include "openrac/game_host.h"

extern uint32_t rac1_effect_quad_id;

void game_FUN_002327a0(unsigned int a0) {
    rac1_effect_quad_id = a0 + 1; /* a moby is 0x100-aligned: its quads count up from here */
    game_FUN_002327a0__game(a0);
    rac1_effect_quad_id = 0;
}
