/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

extern int FUN_L02_00335630(char *, char *);
extern int func_L01_00323068(char *, char *);
extern int FUN_002B5BA8(char *, float *, float);

/* Runs the kind's entry hook (0x003CE8B0). The decompilation's C returns void, but the assembly
 * hands back the hook's $v0 and its callers (func_V1_L02_003CD970, func_V1_L14_004275E0) test bit 0x80
 * of it; with void hostgen passes them 0. */
int func_L02_003D2898(char *a0, char *a1, int a2)
{
    char *p = *(char **)(a0 + 0x68);

    if (D_L02_001A9070)
        return FUN_L02_00335630(a0, a1);
    else if (a2 == 0)
        return func_L01_00323068(a0, a1);
    else
        return FUN_002B5BA8(a0, (float *)a1, *(float *)(p + 0x31C));
}
