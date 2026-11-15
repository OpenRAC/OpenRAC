/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

extern int func_L02_00335630(char *, char *);
extern int func_L01_00323068(char *, char *);
extern int func_002B5BA8(char *, float *, float);
extern int D_L02_001A9070 __attribute__((sda));

/* Runs the kind's entry hook (0x003D2898, v1.01's func_L02_003CE8B0). The decompilation's C returns
 * void, but the assembly hands back the hook's $v0 and its callers (func_L02_003D18D8,
 * func_L14_0042CCA8) test bit 0x80 of it; with void hostgen passes them 0. */
int func_L02_003D2898(char *a0, char *a1, int a2)
{
    char *p = *(char **)(a0 + 0x68);

    if (D_L02_001A9070)
        return func_L02_00335630(a0, a1);
    else if (a2 == 0)
        return func_L01_00323068(a0, a1);
    else
        return func_002B5BA8(a0, (float *)a1, *(float *)(p + 0x31C));
}
