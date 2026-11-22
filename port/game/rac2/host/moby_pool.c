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
 * loader allocates after the table moves with n2, as on Ratchet & Clank (rac1/host/moby_pool.c,
 * docs/port/KNOWN_ISSUES.md).
 *
 * But the level's heap has a ceiling: the data the boot program keeps at the top of RAM for every
 * level, the classes of the ship (3412) and the rest from 0x01F0C000 on NTSC-U v1.01. Each slot
 * takes 0x180 bytes (the record, its pvars), and a level leaves 170 KB (Damosel) to 1 MB below the
 * ceiling, Boldan 277 KB: 1,024 more slots there (384 KB) laid the particle pool and the lists
 * after the table over the ship's class, whose joint table then held a particle's float where the
 * ship's cockpit glass (func_002ECB70) reads a pointer (a crash at 0x3D4CCCD1), and a load again
 * after a death read the level's own broken lists; on Smolg the loader itself broke on the ship's
 * class it had just laid its lists over. So the first load of a level is measured: it runs without
 * the raise, the loader returns the heap's end, and the ceiling is the lowest class of a moby above
 * that end (the ship's; a level's own classes are below it). Memory from the heap's start up is
 * kept before the loader runs and put back after the measure, and the loader runs again with the
 * raise that fits (enemy_copies.c's wrap). The measure is kept, so the level's next loads (after a
 * death, or a return to the planet) fit at once; a raise that still crossed the ceiling is taken
 * back the same way. The slots granted are the request, cut to the room the level has.
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
#include <string.h>

#include "openrac/host_env.h"
#include "rac2.h"

#define LOAD_CONTEXT OPENRAC_LDATA(0, RAC2_LADDR(00, 0x001BF140u))
#define CONTEXT_FILE 0x0Cu
#define CONTEXT_HEAP 0x48u /* where the loader starts the level's heap */
#define HEADER_MOBYS 0x4Cu /* the moby section's offset in the file's header */
#define MOBYS_SPARE 0x4u   /* in the section: the placed count, then the spare count n2 */
#define TABLE_BASE OPENRAC_DATA(RAC2_g_mobyTableBase)
#define TABLE_DYNAMIC OPENRAC_DATA(RAC2_g_mobyTableBase + 4u)
#define TABLE_END OPENRAC_DATA(RAC2_g_mobyTableEnd)
#define SLOT_SIZE 0x100u
#define SLOT_STATE 0x20u
#define SLOT_CLASS 0x24u    /* the moby's class */
#define SLOT_HEAP 0x180u    /* what a slot takes of the heap: its record and its pvars (0x80) */
#define CEILING_GAP 0x1000u /* kept free below the ceiling */
#define MEASURES 64

/* A level as its loader lays it out without a raise: where its heap starts and its moby counts (the
 * same file each time), where the heap ends, and the ceiling above it. */
typedef struct {
    gaddr start;
    int32_t placed;
    int32_t spare;
    gaddr end;
    gaddr ceiling;
} Measure;

static gaddr g_header;  /* the file the running loader will decompress; 0 while none runs */
static int g_requested; /* the slots a mod asked for */
static int g_applied;   /* the slots added to the last load */
static gaddr g_start;   /* the heap's start of the running load */
static int32_t g_placed, g_spare; /* its file's counts as the disc has them, -1 before */
static Measure g_measures[MEASURES];
static int g_measured;     /* the measures kept (a ring past MEASURES) */
static int g_measuring;    /* the running load is a level's first, without the raise */
static uint8_t* g_kept;    /* the memory from the heap's start up, before the loader ran */
static uint32_t g_kept_at; /* where it starts; 0 for none kept */

static int pool_in_ram(gaddr a, uint32_t bytes) {
    return a >= 0x00100000u && a < openrac_guest_ram_size && bytes <= openrac_guest_ram_size - a;
}

static Measure* measure_of(gaddr start, int32_t placed, int32_t spare) {
    const int n = g_measured < MEASURES ? g_measured : MEASURES;
    for (int i = 0; i < n; ++i) {
        Measure* m = &g_measures[i];
        if (m->start == start && m->placed == placed && m->spare == spare) {
            return m;
        }
    }
    return NULL;
}

/* The slots a level measured `m` has room for below its ceiling. */
static int room_of(const Measure* m) {
    if (m->ceiling == 0 || m->ceiling < m->end + CEILING_GAP) {
        return 0;
    }
    const uint32_t room = (m->ceiling - CEILING_GAP - m->end) / SLOT_HEAP;
    return room > RAC2_POOL_EXTRA_MAX ? RAC2_POOL_EXTRA_MAX : (int)room;
}

/* The ceiling over a heap that would end at `end` without the raise: the lowest class of a moby in
 * the table at or above it (the ship's, which the loader made), 0 for none found. */
static gaddr ceiling_over(gaddr end) {
    const gaddr base = GREF(gaddr, TABLE_BASE);
    const gaddr top = GREF(gaddr, TABLE_END);
    gaddr lowest = 0;
    if (base == 0 || top <= base || (top - base) % SLOT_SIZE != 0
        || !pool_in_ram(base, top - base)) {
        return 0;
    }
    for (gaddr m = base; m < top; m += SLOT_SIZE) {
        const uint8_t state = GREF(uint8_t, m + SLOT_STATE);
        if (state == 0xFF) {
            break;
        }
        const gaddr c = GREF(gaddr, m + SLOT_CLASS);
        if (state < 0x80 && c >= end && pool_in_ram(c, 0x10) && (lowest == 0 || c < lowest)) {
            lowest = c;
        }
    }
    return lowest;
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
    g_start = 0;
    g_placed = g_spare = -1;
    g_kept_at = 0;
    g_measuring = 0;
    const gaddr ctx = LOAD_CONTEXT;
    if (!pool_in_ram(ctx, CONTEXT_HEAP + 4u)) {
        return;
    }
    g_header = GREF(gaddr, ctx + CONTEXT_FILE);
    g_start = GREF(gaddr, ctx + CONTEXT_HEAP);
    /* A raise to come: the memory it may lay the level over is kept, to put back if it crosses the
     * ceiling. */
    if (wanted() > 0 && pool_in_ram(g_start, 0x100)) {
        if (g_kept == NULL) {
            g_kept = malloc(openrac_guest_ram_size);
        }
        if (g_kept != NULL) {
            memcpy(g_kept, G(g_start), openrac_guest_ram_size - g_start);
            g_kept_at = g_start;
        }
    }
}

int rac2_pool_loaded(gaddr heap_end) {
    g_header = 0;
    if (g_placed < 0 || !pool_in_ram(heap_end, 0)) {
        return 0;
    }
    /* The level as it is without the raise, kept for its loads to come. */
    const gaddr end = heap_end - (gaddr)g_applied * SLOT_HEAP;
    Measure* m = measure_of(g_start, g_placed, g_spare);
    if (m == NULL) {
        m = &g_measures[g_measured++ % MEASURES];
    }
    *m = (Measure){g_start, g_placed, g_spare, end, ceiling_over(end)};
    /* The first load measured the level: again with the raise, where there is room for one. */
    if (g_measuring) {
        g_measuring = 0;
        return room_of(m) > 0 && g_kept_at == g_start;
    }
    if (g_applied <= room_of(m)) {
        return 0;
    }
    if (m->ceiling == 0 || g_kept_at != g_start) {
        fprintf(
            stderr,
            "[warning] moby pool: %d extra slots may reach the data above the level\n",
            g_applied
        );
        return 0;
    }
    fprintf(
        stderr,
        "[info] moby pool: %d extra slots would reach the data above the level at 0x%08x; %d fit, "
        "the level is laid out again\n",
        g_applied,
        (unsigned)m->ceiling,
        room_of(m)
    );
    return 1;
}

void rac2_pool_restore(void) {
    if (g_kept != NULL && g_kept_at != 0) {
        memcpy(G(g_kept_at), g_kept, openrac_guest_ram_size - g_kept_at);
    }
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
    g_placed = placed;
    g_spare = spare;
    int add = extra;
    /* A level measured before: what fits below its ceiling. Its first load, with the memory kept
     * to lay it out again: none, to measure it. */
    const Measure* m = measure_of(g_start, placed, spare);
    if (m != NULL && add > room_of(m)) {
        add = room_of(m);
    } else if (m == NULL && g_kept_at == g_start && g_start != 0) {
        g_measuring = 1;
        return;
    }
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
