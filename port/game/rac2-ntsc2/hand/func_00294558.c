/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Going Commando: the port's own C for matching C that leans on a register the C never names.
 * Each function here is the decompilation's, with the one value the console's register held
 * written out, as its assembly has it (hostgen.json "overrides"; prep.use_overrides puts it at the
 * end of the function's own file, so that file's declarations are this C's).
 */

/* func_002B8A38 as it is: the destination's value, where to read from, and how many bytes. */
extern int openrac_read_part(int, int, int) __asm__("func_002B8A38");

/* A level part's load (0x00293CA8): func_002B8A38 reads `n` bytes, the size the matching C leaves
 * in $a2 after computing it (next - cur) and does not pass. */
int func_00294558(int index)
{
    int j = index + 1;
    int *d = F46b43b72_D_001B8840;
    int *b = F46b43b72_D_0014B540;
    int *t = (int *)((char *)b + 25416);
    int n = d[12] * 332;
    int hold = d[28];
    int j4 = j * 4;
    int cur = *(int *)((char *)t + (n + index * 4));
    int next = *(int *)((char *)t + (j4 + n));

    n = next - cur;
    if (n > 0) {
        openrac_read_part(hold, cur + b[6341], n);
        F46b43b72_FUN_002B7CE0(0);
    }
    return 1;
}
