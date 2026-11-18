/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

extern int func_L01_00321178(char *, char *);
extern int func_002B4FF0(char *, float *, float);

/* Runs the kind's entry hook (0x00404F90), the level 9 twin of func_L02_003CE8B0. The
 * decompilation's C returns void, but the assembly hands back the hook's $v0, which
 * func_L09_00405610 tests (bit 0x80); with void hostgen passes it 0. */
int func_L09_00404F90(char *a0, char *a1, s32 a2)
{
    char *p = *(char **)(a0 + 0x68);

    if (a2 == 0)
        return func_L01_00321178(a0, a1);
    else
        return func_002B4FF0(a0, (float *)a1, *(f32 *)(p + 0x33C));
}
