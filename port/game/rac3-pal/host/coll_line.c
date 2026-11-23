/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * coll_line (the hand-written line query, IDENTIFIED.tsv; the boot program's 0x003D4068,
 * frontbin's 0x003D3CB0, Veldin's 0x003FA5F0 and every level's copy) walks a line through the
 * 4-unit cells of the world and gathers the cells it crosses in the scratchpad: each axis's
 * crossings at 0x70003000/3200/3400, merged into 8-byte entries from 0x70003800 up to the byte
 * flags at 0x70003FFC. It sets no bound on their count, as on the console: a line crossing more
 * than about 254 cells (|dx| + |dy| + |dz| over about 1,000 units) writes past the scratchpad's
 * 16 KB, where the console's TLB maps nothing and the game would stop. Retail's lines are shorter;
 * the port's teleports and jumps make longer ones (an enemy's sight line to the hero far away).
 *
 * The port bounds the gather here: a line that would cross more cells than the scratchpad holds is
 * cut at the length it can (from its start, along its own direction), and the query runs on that.
 * Shorter lines, all of retail's, run as they come.
 *
 *   int coll_line(const vec4 *from, const vec4 *to, int flags, int moby, int x)
 */
#include <math.h>
#include <stdint.h>

#include "openrac/guest.h"
#include "rac3_coll_line.h"

/* The entries from 0x70003800 to the flags at 0x70003FFC: 255, one the line's start; a margin of
 * three for the cells each axis's ends add. */
#define CELLS_HELD 248.0f
#define CELL 4.0f

int rac3_coll_line(rac3_coll_line_fn query, int from, int to, int flags, int moby, int x) {
    const float* a = (const float*)G((gaddr)from);
    const float* b = (const float*)G((gaddr)to);
    const float d[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
    const float cells = (fabsf(d[0]) + fabsf(d[1]) + fabsf(d[2])) / CELL;

    /* NaN fails the test as well: such a line leaves the world's bounds at its first check. */
    if (!(cells > CELLS_HELD)) {
        return query(from, to, flags, moby, x);
    }
    GFRAME(cut, 16);
    float* end = (float*)G(cut);
    const float s = CELLS_HELD / cells;
    end[0] = a[0] + d[0] * s;
    end[1] = a[1] + d[1] * s;
    end[2] = a[2] + d[2] * s;
    end[3] = b[3];
    return query(from, (int)cut, flags, moby, x);
}

/* The boot program's copy, frontbin's, and the single-player levels' (Veldin's place, the
 * catalogue's row func_003D3CB0). Level 39's and the maps' are in mp.c and mp_maps.c. */
int func_003D4068__game(int from, int to, int flags, int moby, int x);
int func_003D3CB0__frontbin__game(int from, int to, int flags, int moby, int x);
int func_L01_003FA5F0__game(int from, int to, int flags, int moby, int x);

int func_003D4068(int from, int to, int flags, int moby, int x) {
    return rac3_coll_line(func_003D4068__game, from, to, flags, moby, x);
}

int func_003D3CB0__frontbin(int from, int to, int flags, int moby, int x) {
    return rac3_coll_line(func_003D3CB0__frontbin__game, from, to, flags, moby, x);
}

int func_L01_003FA5F0(int from, int to, int flags, int moby, int x) {
    return rac3_coll_line(func_L01_003FA5F0__game, from, to, flags, moby, x);
}
