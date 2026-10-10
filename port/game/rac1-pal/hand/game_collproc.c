/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Ratchet & Clank (PAL): hand-written assembly of src/game/collproc.c, in C, written from what
 * each retail routine does.
 */
#include "common.h"

extern unsigned char D_00194200[];

/* The low five bits of the word at 0x1C of D_00194200, or -1 when the word is negative or those
 * bits are all set. */
int func_001F0F00(void) {
    int v = *(int *)(D_00194200 + 0x1C);
    if (v < 0) {
        return -1;
    }
    v &= 0x1F;
    if (v == 0x1F) {
        return -1;
    }
    return v;
}
