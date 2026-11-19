/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

extern int func_L02_00333480(char *object, void *target, void *input);

/* Steers the object toward a point (0x00333438). The decompilation's C returns void, but the
 * assembly hands back func_L02_00333480's $v0 untouched, and func_L02_003CE8B0 passes it on to
 * callers that test it. */
int func_L02_00333438(char *arg0, char *arg1)
{
    int tmp[4];

    func_00282CF0((float *)tmp, (const float *)arg1, (const float *)(arg0 + 0x10));
    return func_L02_00333480(arg0, arg1, tmp);
}
