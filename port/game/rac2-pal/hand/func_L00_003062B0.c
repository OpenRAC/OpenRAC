/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/* func_002B86D0 as it is: the destination's value, where to read from, and how many bytes. */
extern int openrac_read_part(int, int, int) __asm__("func_002B86D0");

/* Level 0's copy of a part load (as func_V1_00293CA8): func_002B86D0 reads `n` bytes, the size the
 * matching C leaves in $a2 after computing it (next - cur) and does not pass. */
int func_L00_003062B0(int index)
{
    int j = index + 1;
    int *d = D_L00_001BCB00;
    int *b = D_0014B5C0;
    int *t = (int *)((char *)b + 25416);
    int n = d[12] * 332;
    int hold = d[28];
    int j4 = j * 4;
    int cur = *(int *)((char *)t + (n + index * 4));
    int next = *(int *)((char *)t + (j4 + n));

    n = next - cur;
    if (n > 0) {
        openrac_read_part(hold, cur + b[6341], n);
        FUN_002B8898(0);
    }
    return 1;
}
