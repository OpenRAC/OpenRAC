/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 * The ship's glass (classes 0x212 to 0x215) is not in the ship's model: a draw callback writes it
 * as world quads at the tick's place. A frame between ticks draws the hull part way from the last
 * tick, so the quads need ids to move with it; without them the glass flew ahead of the ship. */
#include "rac1.h"

extern uint32_t rac1_effect_quad_id;

void func_002347E0(gaddr a0) {
    rac1_effect_quad_id = a0 + 1; /* a moby is 0x100-aligned: its quads count up from here */
    func_002347E0__game(a0);
    rac1_effect_quad_id = 0;
}
