/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

extern float func_00283CE0(int);
extern int func_002A8240(char *object, float *movement, int flags, float height, float radius,
                        float offset);

/* func_002A8240 with a scaled angle (0x0033CB08). The decompilation's C returns a float, but
 * func_002A8240 returns an int in $v0, which the assembly hands back untouched and its callers
 * (func_L04_0033CC00, func_L06_0045A0D0, func_L06_0045A5B0) test; with float hostgen passes them 0. */
int func_L04_0033CB08(void *a0, void *a1, int a2, int a3, float f12, float f13)
{
    return func_002A8240(a0, a1, a3, f12, func_00283CE0(a2) * 0.0009765625f, f13);
}
