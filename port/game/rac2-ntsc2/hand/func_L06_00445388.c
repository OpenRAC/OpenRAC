/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

extern int func_L01_00322450(char *, char *);
extern int func_002B5F10(char *, float *, float);

/* Runs the kind's entry hook (0x00443DE0), the level 6 twin of func_L02_003CFE08. The
 * decompilation's C returns void (and calls the hooks by their level 6 addresses without
 * arguments), but the assembly passes $a0/$a1 through and hands back the hook's $v0, which
 * func_V1_L06_00443280 tests (bit 0x80); with void hostgen passes it 0. */
int func_L06_00445388(char *a0, char *a1, int a2)
{
    char *p = *(char **)(a0 + 0x68);

    if (a2 == 0)
        return func_L01_00322450(a0, a1);
    else
        return func_002B5F10(a0, (float *)a1, *(float *)(p + 0x31C));
}
