/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * The games' printf conversions, for the prints the port answers itself (sprintf, printf,
 * kprintf). Their own C walks the console's argument save area, which a host va_list is not, so
 * each game's host C takes the arguments as a host va_list and formats them here
 * (rac1/host/hand.c, rac1-pal/host/kprintf.c, rac2/host/print.c). */
#ifndef OPENRAC_GUEST_FORMAT_H
#define OPENRAC_GUEST_FORMAT_H

#include <stdarg.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Formats `format` with `args` into `out`: the conversions d i u x X o c s f e g % with flags,
 * width and precision (l and h are skipped: the game's int and long arrive as host ints). A %s
 * argument is a game address. At most `size` - 1 characters are written and `out` is terminated;
 * returns the count written. */
int openrac_guest_vformat(char* out, size_t size, const char* format, va_list args);

#ifdef __cplusplus
}
#endif

#endif
