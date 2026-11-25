/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal's kernel entry points the generated binding cannot take (libraries.tsv):
 * sceOpen is declared variadic, as rac2's (rac2/host/io.c), and two hand-written routines read
 * or write COP0 and kernel memory, which the port does not have. */
#include "openrac/game_lib.h"
#include "openrac/guest.h"

/* sceOpen (0x00123430, libkernl's fileio client): the mode is its only optional argument; the
 * shared replacement answers it. */
int func_00123430(gaddr path, int flags, ...) {
    return openrac_lib_sceOpen(path, flags);
}

/* 0x0011F860: 1 when Status.IE is clear, so that a thread call made with interrupts off returns
 * -1 (0x0011FD40). The port runs the game's code with interrupts on, never from an interrupt. */
int func_0011F860(void) {
    return 0;
}

/* 0x00125090: a store made in kernel mode (interrupts off, EXL set, `sw a1, 0(a0)`, eret). The
 * timer setters (0x00125100, 0x00125110, 0x00125120) write T0's COUNT, MODE and COMP through it at
 * 0xB0001000, the registers' kernel segment address; the port keeps them at their physical one. */
void func_00125090(unsigned int address, int value) {
    GREF(uint32_t, address & 0x1FFFFFFFu) = (uint32_t)value;
}
