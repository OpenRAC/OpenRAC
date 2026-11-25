/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/* func_002B7B18 as it is: the destination's value, where to read from, and how many bytes. */
extern int openrac_read_part(int, int, int) __asm__("func_002B7B18");

/* Level 0's copy of a part load (as func_00293CA8): func_002B7B18 reads `n` bytes, the size the
 * matching C leaves in $a2 after computing it (next - cur) and does not pass. */
int func_L00_00364EE8(int index)
{
    int j = index + 1;
    int *d = D_L00_001BCAC0;
    int *b = D_0014B540;
    int *t = (int *)((char *)b + 16040);
    int n = d[12] * 332;
    int hold = d[28];
    int j4 = j * 4;
    int cur = *(int *)((char *)t + (n + index * 4));
    int next = *(int *)((char *)t + (j4 + n));

    n = next - cur;
    if (n > 0) {
        openrac_read_part(hold, cur + b[3977], n);
        func_002B7CE0(0);
    }
    return 1;
}
