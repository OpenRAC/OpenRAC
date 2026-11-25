/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * RAC1 recorder ownership at VU1_initChain and VU1_swapChain. Their pointer/buffer/counter work stays
 * in the pinned game C; the native recording learns only which buffer is beginning a fresh build
 * (seen in game code: src/game/render/vuchain.c). A capture tick is not a construction boundary.
 */
#include "frame_list_owner.h"

#include "rac1.h"

#if !RAC1_OMIT(frame_list_ownership)
/* Retains initialization of both buffer pointers, the active index and allocation limits. */
void func_002348E8(void) {
    rac1_frame_list_rebuilt(func_002348E8__game, OPENRAC_DATA(RAC1_ADDR(0x00161000u)));
}

/* Retains the previous cursor, buffer alternation and allocation limits before starting its owner. */
void func_00234948(void) {
    rac1_frame_list_rebuilt(func_00234948__game, OPENRAC_DATA(RAC1_ADDR(0x00161000u)));
}
#endif
