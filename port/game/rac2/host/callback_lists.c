/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Going Commando: the draw callback list register routine that only some levels' programs carry.
 * The engine keeps four lists of (function, argument) pairs, each with a count of its own
 * (src/game/0027B948.c): counts 0x001B1608 to 0x001B1614, pairs from 0x001B9E40, 0x001BA040,
 * 0x001BA240 and 0x001BA440 (functions, then the arguments 0x100 bytes on). The executable has a
 * register routine for three of them and a runner for all four; levels 8, 9, 19 and 30 also carry
 * a register for the fourth, just before the second list's. The catalogue folds every register
 * into one function, so split_places.tsv gives each place to the routine it is. Run as the
 * second list's (func_0027CFD0), this one paired that list's tables with the fourth's: its runner
 * (func_0027D020) then called the fourth list's empty entries, address 0 once a frame, and the
 * second list's callbacks never ran. As Ratchet & Clank's (rac1/host/callback_lists.c).
 */
#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "rac2.h"

static void register_callback(gaddr count_at, gaddr fns, gaddr args, gaddr fn, gaddr arg) {
    const int count = GREF(int, count_at);
    if (count < 64) {
        GREF(gaddr, fns + (gaddr)count * 4u) = fn;
        GREF(gaddr, args + (gaddr)count * 4u) = arg;
        GREF(int, count_at) = count + 1;
    }
}

/* The fourth list: count D_001B1614, pairs in D_001BA440 / D_001BA540 (its runner func_0027CF58). */
void func_L08_002F2C48(gaddr fn, gaddr arg) {
    register_callback(OPENRAC_DATA(RAC2_ADDR(0x001B1614u)), OPENRAC_DATA(RAC2_ADDR(0x001BA440u)), OPENRAC_DATA(RAC2_ADDR(0x001BA540u)), fn, arg);
}
