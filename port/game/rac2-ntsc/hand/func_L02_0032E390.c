/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

extern int func_L02_0032E3B0(void *a0, float *a1, int a2);

/* The two-argument wrapper of the target lookup (0x0032E390). The decompilation's C returns void,
 * but the assembly hands back func_L02_0032E3B0's $v0 untouched and its callers (the level 2, 6,
 * 9, 12 and 14 movers) read it; with void hostgen passes them 0. The lookup gets the record index
 * the wrapper never sets ($a2 as the caller left it), as the decompilation's C does. */
int func_L02_0032E390(void *a0, void *a1)
{
    return ((int (*)(void *, void *))func_L02_0032E3B0)(a0, (char *)a1 + 0x10);
}
