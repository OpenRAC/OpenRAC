/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * CollLine (func_002761F8, hand-written) walks a line through the 4-unit cells of the world and
 * gathers the cells it crosses in the scratchpad, merged into 8-byte entries from 0x70003800 up to
 * the scratchpad's end. It sets no bound on their count, as on the console: a line crossing more
 * than about 254 cells (|dx| + |dy| + |dz| over about 1,000 units) writes past the scratchpad's
 * 16 KB, where the console's TLB maps nothing and the game would stop. Retail's lines are shorter;
 * the port's teleports make longer ones (an enemy's sight line to the hero far away).
 *
 * The port bounds the gather here: a line that would cross more cells than the scratchpad holds is
 * cut at the length it can (from its start, along its own direction), and the query runs on that.
 * Shorter lines, all of retail's, run as they come. Up Your Arsenal's coll_line is bounded the same
 * (rac3-pal/host/coll_line.c). */
#include <math.h>
#include <stdint.h>

#include "openrac/guest.h"

/* The entries from 0x70003800 to the scratchpad's end, with a margin for the cells each axis's ends
 * add. */
#define CELLS_HELD 248.0f
#define CELL 4.0f

int func_002761F8__game(int from, int to, int mode, int moby, int x);

int func_002761F8(int from, int to, int mode, int moby, int x) {
    const float* a = (const float*)G((gaddr)from);
    const float* b = (const float*)G((gaddr)to);
    const float d[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
    const float cells = (fabsf(d[0]) + fabsf(d[1]) + fabsf(d[2])) / CELL;

    /* NaN fails the test as well: such a line leaves the world's bounds at its first check. */
    if (!(cells > CELLS_HELD)) {
        return func_002761F8__game(from, to, mode, moby, x);
    }
    GFRAME(cut, 16);
    float* end = (float*)G(cut);
    const float s = CELLS_HELD / cells;
    end[0] = a[0] + d[0] * s;
    end[1] = a[1] + d[1] * s;
    end[2] = a[2] + d[2] * s;
    end[3] = b[3];
    return func_002761F8__game(from, (int)cut, mode, moby, x);
}
