/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Going Commando: more room for spawned mobys than on a PS2, for a mod that asks.
 *
 * The level loader LoadLevelData (func_L00_00300618) sizes the level's moby table from its gameplay
 * file: the moby section (header +0x4C) gives the placed mobys' count, then the spare count n2. The
 * table is (placed + n2) * 0x100 bytes, the spare mobys' pvars n2 * 0x80 after it, and CreateMoby
 * hands slots out of the n2 past the placed ones (0xFE and up is free; 0xFF marks the first never
 * used); with none free it answers 0 and the spawn is lost. Barlow's pool is 398 slots, and the
 * mod NG++'s copies of its enemies take 79 of them before the level spawns anything. Everything the
 * loader allocates after the table moves with n2; nothing sits at a fixed address, as on Ratchet &
 * Clank (rac1/host/moby_pool.c, docs/port/KNOWN_ISSUES.md).
 *
 * The loader decompresses the gameplay file itself (WadDecompress, func_0029D4D0, into the load
 * context's data pointer, +0x0C) and reads it at once, so the count is raised as that decompression
 * ends (wad.c calls rac2_wad_decompressed), while a run of the loader is under way and the
 * destination is the file it named at its start (enemy_copies.c's wrap of the loader calls
 * rac2_pool_loading). A request lasts until the program ends and applies to the next level load;
 * OPENRAC_MOBY_EXTRA=N asks for N from the environment (a test).
 *
 * The 15-bit moby indices of the groups' lists and the grid bound the table: placed plus spare
 * stay at most 0x7FFF.
 */
#include "moby_pool.h"

#include <stdio.h>
#include <stdlib.h>

#include "openrac/host_env.h"
#include "rac2.h"

#define LOAD_CONTEXT OPENRAC_LDATA(0, RAC2_LADDR(00, 0x001BF140u))
#define CONTEXT_FILE 0x0Cu
#define HEADER_MOBYS 0x4Cu /* the moby section's offset in the file's header */
#define MOBYS_SPARE 0x4u   /* in the section: the placed count, then the spare count n2 */
#define TABLE_BASE OPENRAC_DATA(RAC2_g_mobyTableBase)
#define TABLE_DYNAMIC OPENRAC_DATA(RAC2_g_mobyTableBase + 4u)
#define TABLE_END OPENRAC_DATA(RAC2_g_mobyTableEnd)
#define SLOT_SIZE 0x100u
#define SLOT_STATE 0x20u

static gaddr g_header;  /* the file the running loader will decompress; 0 while none runs */
static int g_requested; /* the slots a mod asked for */
static int g_applied;   /* the slots added to the last load */

static int pool_in_ram(gaddr a, uint32_t bytes) {
    return a >= 0x00100000u && a < openrac_guest_ram_size && bytes <= openrac_guest_ram_size - a;
}

int rac2_pool_raise(int extra) {
    if (extra > RAC2_POOL_EXTRA_MAX) {
        extra = RAC2_POOL_EXTRA_MAX;
    }
    if (extra > g_requested) {
        g_requested = extra;
    }
    return extra > 0 ? extra : 0;
}

/* The slots to add: a mod's request, else the environment's. */
static int wanted(void) {
    if (g_requested > 0) {
        return g_requested;
    }
    const char* env = openrac_env_string("OPENRAC_MOBY_EXTRA");
    const int n = env != NULL ? atoi(env) : 0;
    return n > RAC2_POOL_EXTRA_MAX ? RAC2_POOL_EXTRA_MAX : (n > 0 ? n : 0);
}

void rac2_pool_loading(void) {
    g_header = 0;
    g_applied = 0;
    const gaddr ctx = LOAD_CONTEXT;
    if (pool_in_ram(ctx, 16)) {
        g_header = GREF(gaddr, ctx + CONTEXT_FILE);
    }
}

void rac2_pool_loaded(void) {
    g_header = 0;
}

void rac2_wad_decompressed(gaddr to) {
    /* Only the file the running loader named: every other WAD decompresses as it is. */
    if (g_header == 0 || to != g_header) {
        return;
    }
    g_header = 0;
    const int extra = wanted();
    if (extra <= 0 || !pool_in_ram(to, 0x100)) {
        return;
    }
    const int32_t off = GREF(int32_t, to + HEADER_MOBYS);
    if (off <= 0 || !pool_in_ram(to + (uint32_t)off, 16)) {
        return;
    }
    const gaddr section = to + (uint32_t)off;
    const int32_t placed = GREF(int32_t, section);
    const int32_t spare = GREF(int32_t, section + MOBYS_SPARE);
    if (placed < 0 || spare < 0 || placed + spare >= 0x7FFF) {
        return;
    }
    int add = extra;
    if (placed + spare + add > 0x7FFF) {
        add = 0x7FFF - placed - spare;
    }
    GREF(int32_t, section + MOBYS_SPARE) = spare + add;
    g_applied = add;
    fprintf(
        stderr,
        "[info] moby pool: %d spare slots, %d more than the level ships\n",
        (int)(spare + add),
        add
    );
}

int rac2_pool_status(int32_t out[5]) {
    const gaddr base = GREF(gaddr, TABLE_BASE);
    const gaddr dynamic = GREF(gaddr, TABLE_DYNAMIC);
    const gaddr end = GREF(gaddr, TABLE_END);
    for (int i = 0; i < 5; ++i) {
        out[i] = 0;
    }
    if (base == 0) {
        return 0;
    }
    if (end <= base || dynamic < base || dynamic > end || (end - base) % SLOT_SIZE != 0
        || !pool_in_ram(base, end - base)) {
        return -1;
    }
    int live = 0;
    int freed = 0;
    gaddr m = dynamic;
    for (; m < end; m += SLOT_SIZE) {
        const uint8_t state = GREF(uint8_t, m + SLOT_STATE);
        if (state == 0xFF) {
            /* From the first never used on, every slot is free. */
            freed += (int)((end - m) / SLOT_SIZE);
            break;
        }
        if (state == 0xFE) {
            ++freed;
        } else if (state < 0x80) {
            ++live;
        }
    }
    out[0] = (int32_t)((end - dynamic) / SLOT_SIZE);
    out[1] = live;
    out[2] = freed;
    out[3] = g_applied;
    out[4] = g_requested;
    return 1;
}
