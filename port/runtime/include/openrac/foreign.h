/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/* Imported moby classes: another level's program run in the level loaded (runtime/guest.cpp). */
#ifndef OPENRAC_FOREIGN_H
#define OPENRAC_FOREIGN_H

#include "openrac/guest.h"

#ifdef __cplusplus
extern "C" {
#endif

/* A moby class of level `from`'s program run in level `loaded` (an imported class): its code is
 * that level's, on that level's addresses. enter swaps `from`'s data in over the loaded one's (its
 * code is read from a copy, so a bigger program fits too),
 * carrying the engine's globals over (their pointers moved to its places), and makes it the program
 * calls go to; leave carries what the call changed back and puts the loaded program in again. The
 * foreign program keeps its own data between calls. enter returns 0, or -1 (no such program). */
int openrac_guest_foreign_enter(int from, int loaded);
void openrac_guest_foreign_leave(void);
/* In the foreign program entered last, a word of its data set to `value` for the call (a table
 * of its own level in place of the loaded level's): leave carries none of it back. */
void openrac_guest_foreign_set(gaddr address, uint32_t value);
/* An engine table that goes in and out whole for a call, as it is (no address in it to move):
 * `size` bytes at `address` in level `level`'s program, and their places in the others (rac1: the
 * moby grid, which a foreign moby's move or deletion updates). */
void openrac_guest_foreign_carry_whole(int level, gaddr address, uint32_t size);
/* A word of level `level`'s program as the disc has it (0 outside it). */
uint32_t openrac_guest_level_word(int level, gaddr address);
/* Drops the foreign programs' state (a new level load). */
void openrac_guest_forget_foreign(void);

#ifdef __cplusplus
}
#endif

#endif
