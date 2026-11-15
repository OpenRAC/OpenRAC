/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

extern int func_L01_00323068(char *, char *);
extern int func_002B5BA8(char *, float *, float);

/* Runs the kind's entry hook (0x00448968, v1.01's func_L06_00443DE0), the level 6 twin of
 * func_L02_003D2898. The decompilation's C returns void (and calls the hooks by their level 6
 * addresses without arguments), but the assembly passes $a0/$a1 through and hands back the hook's
 * $v0, which func_L06_00447DC8 tests (bit 0x80); with void hostgen passes it 0. */
int func_L06_00448968(char *a0, char *a1, int a2)
{
    char *p = *(char **)(a0 + 0x68);

    if (a2 == 0)
        return func_L01_00323068(a0, a1);
    else
        return func_002B5BA8(a0, (float *)a1, *(float *)(p + 0x31C));
}
