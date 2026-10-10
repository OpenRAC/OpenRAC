/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Ratchet & Clank (PAL): the moby animation routines that only feed the game's own joint evaluator
 * (hostgen.json, "host_functions"). That evaluator (func_00211808) is not run by the port, whose
 * renderer poses each moby itself from the animation fields (frame, key pair, blend timer), so what
 * these routines would leave behind has no reader yet. Each is where the port's own pose evaluator,
 * built from ReRAC's moby animation code, takes over.
 */
#include "game_protos.h"
#include "openrac/game_host.h"

/* func_0020FC38 (hand-written, 3240 bytes): snapshots a moby's current pose into the blend slot the
 * caller took (func_00213DE0 / func_00213F28 on a sequence change: a1 = slot | flags), unpacking the
 * key frames on the scratchpad, so that the evaluator can blend from it. Not done: the slot keeps what
 * it had; the sequence change itself (the caller's C) still happens. */
void func_0020FC38(gaddr a0, int a1) {
    (void)a0;
    (void)a1;
}

/* func_L00_00252D20 (hand-written, 2216 bytes): the level programs' variant of func_0020FC38 for
 * Ratchet (from func_L00_00232C10): the same pose snapshot into a blend slot. Not done, as above. */
void func_L00_00252D20(gaddr a0, int a1) {
    (void)a0;
    (void)a1;
}
