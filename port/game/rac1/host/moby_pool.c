/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Ratchet & Clank: more room for spawned mobys than on a PS2. A fix the player can switch back
 * (differences.json, "bigger-moby-pool", fix_bigger_moby_pool): off, every level keeps the disc's
 * spare count. Vanilla RAM uses the retail-sized target list; expanded RAM redirects it to a
 * reserved 32,768-pointer table so large modded levels can target every moby.
 *
 * The level loader LoadLevelGameplay (func_L00_002422D8; the US disc's FUN_L00_00241940) sizes the
 * level's moby array from its gameplay file: the header's moby section (header +0x44) gives the
 * placed mobys' count, then the spare count n2. The array is (placed + n2) * 0x100 bytes, the
 * spawned mobys' pvars n2 * 0x80 after it, and the pool CreateMoby hands slots out of ends at
 * base + (placed + n2 - 1) * 0x100, its last slot holding the free mark. Everything the loader
 * allocates after them moves with n2; nothing sits at a fixed address. Every level ships 256 spare
 * slots (255 usable) but Drek's fleet (level 17), which ships 400.
 *
 * Busy effects fill 255: on Gaspar (level 9) the falling rocks' lava sparks come in bursts of 100
 * to 150, and fcheck (docs/port/BACKFILL.md) saw retail's pool reach 254 of 255 there. A full
 * pool makes CreateMoby answer 0, and a caller that does not check reads through the null moby: on
 * a console into low memory, in the port a crash (docs/port/KNOWN_ISSUES.md; scene_actors.c guards
 * the one call site seen).
 *
 * In vanilla mode the loader sees 768 spare slots where the 32 MiB level heap has room: Rilgar,
 * Blarg Station and Quartu keep the disc's count. With an expanded-level mount the heap extends
 * into the 128 MiB arena, so those levels can use the larger pool too. Each spare slot takes
 * 0x180 bytes (the moby and its pvars); the expanded pool uses the remaining 15-bit moby indices,
 * capped so placed plus spare mobys never exceed 32768.
 *
 * The loader decompresses the gameplay file's
 * header itself (WadDecompress, func_0020C468, into the load state's header pointer, D_L00_00173F00
 * + 4) and reads it at once, so the count is raised as that decompression ends (hand.c calls
 * rac1_wad_decompressed), while a run of the loader is under way and the destination is the header
 * it named at its start. The array, the pvars, the pool's end and what follows all follow; expanded
 * mode caps the complete array to the 15-bit index range. The moby grid's blocks and renderer have
 * their own limits (docs/port/KNOWN_ISSUES.md).
 *
 * LoadLevelGameplay is a hook point for mods too (rac1.level.load_gameplay; the difficulty mod
 * takes it): the hook sits under this, as func_L00_002422D8__hooked (hostgen's hooks.c: the mod's
 * replacement, or the game's C), so a mod's run of the original sees the bigger pool, and so does
 * what it reads of the pool afterwards.
 */
#include <string.h>

#include "rac1.h"

/* The level load's state (Load244110 in loaders_00240398.c): +4 the gameplay file's header. */
#define LOAD RAC1_LADDR(00, 0x00173F00u)

enum {
    LOAD_HEADER = 0x4,
    HEADER_MOBYS = 0x44, /* the moby section's offset in the header */
    MOBYS_SPARE = 0x4,   /* in the section: the placed count, then the spare count n2 */
    VANILLA_SPARE = 768  /* spare slots in the original 32 MiB arena (767 usable) */
};

#define CURRENT_LEVEL RAC1_ADDR(0x0015EE84u)

/* The vanilla heap has no room for more on Rilgar, Blarg Station and Quartu. */
static int heap_full(int level) {
    return level == 5 || level == 6 || level == 15;
}

/* Where the running loader decompresses its header; 0 while no loader runs. */
static gaddr g_header;

/* The class-0 placement is the hero's start (initialize_hero in help_00203E98.c). Change its
 * unpacked record before the game's loader creates it, so collision, the camera and the hero's
 * motion state are initialized together. This buffer belongs to this load; the disc is untouched. */
static void map_spawn(gaddr header) {
    if (openrac_game_map == NULL
        || GREF(int, OPENRAC_DATA(CURRENT_LEVEL)) != openrac_game_map->level) {
        return;
    }
    const int offset = GREF(int, header + HEADER_MOBYS);
    if (offset <= 0) {
        return;
    }
    const gaddr table = header + (gaddr)offset;
    const int count = GREF(int, table);
    for (int i = 0; i < count; ++i) {
        const gaddr record = table + 0x10u + (gaddr)i * 0x78u;
        if (GREF(int, record + 0x18u) == 0) {
            for (unsigned axis = 0; axis < 3; ++axis) {
                GREF(float, record + 0x30u + axis * 4u) = openrac_game_map->spawn[axis];
            }
            GREF(float, record + 0x44u) = openrac_game_map->heading;
            return;
        }
    }
}

/* fix_bigger_moby_pool: the bigger spawn pool; target-list capacity follows the active RAM mode. */
static int bigger_pool(void) {
    static const unsigned char* fix;
    if (!fix) {
        fix = openrac_game_fix("fix_bigger_moby_pool");
    }
    return *fix != 0;
}

/* Hostgen's translated hand routines cannot read runtime-only globals directly. */
int game_rac1_expanded_ram(void) {
    return openrac_guest_ram_size > GUEST_RAM_SIZE;
}

void game_rac1_warn_moby_grid_full(void) {
    static int warned;
    if (!warned) {
        warned = 1;
        openrac_game_warn("moby collision grid cell full: a moby was left out of collision");
    }
}

/* The gameplay header decompressed to `to`, while a loader that named it runs: its spare count raised
 * to the selected arena's spare capacity (once a run; a level that ships more keeps its own). */
void rac1_wad_decompressed(gaddr to) {
    /* Only the header the running loader named: every other WAD decompresses as it is. */
    if (g_header == 0 || to != g_header) {
        return;
    }
    g_header = 0;
    map_spawn(to);
    /* Switched off: the disc's count, as on a console. */
    if (!bigger_pool()) {
        return;
    }
    const gaddr spare = to + (gaddr)GREF(int, to + HEADER_MOBYS) + MOBYS_SPARE;
    /* Vanilla keeps its measured heap-safe pool. The expanded arena has room to use every spare
     * index that fits the game's signed-short moby index representation. */
    const int level = GREF(int, OPENRAC_DATA(CURRENT_LEVEL));
    const int placed = GREF(int, to + GREF(int, to + HEADER_MOBYS));
    const int expanded = openrac_guest_ram_size > GUEST_RAM_SIZE;
    if (!expanded && heap_full(level)) {
        return;
    }
    const int wanted = expanded ? (placed < 0x8000 ? 0x8000 - placed : 0) : VANILLA_SPARE;
    /* A level that ships more spare slots keeps its own. */
    if (GREF(int, spare) < wanted) {
        GREF(int, spare) = wanted;
    }
}

/* LoadLevelGameplay: the game's (or a mod's) loader, with the header it names marked for the raise.
 * Returns the level heap's cursor after what it loaded. */
gaddr func_L00_002422D8(int skip_read) {
    const gaddr outer = g_header;
    g_header = GREF(gaddr, OPENRAC_LDATA(0, LOAD) + LOAD_HEADER);
    const gaddr cursor = func_L00_002422D8__hooked(skip_read);
    g_header = outer;
    return cursor;
}

/*
 * The run list of a tick, with room for the bigger pool: build_resident_visibility_list
 * (func_0020E6B8; the US disc's FUN_0020d868), which UpdateMobys runs first (crash_state.c). It
 * links every active moby into its class slot's run list and writes each with mode bit 0x1000 to
 * the scratchpad target list (0x70000800, 0x400 bytes) before copying it to the pointer table.
 * Vanilla RAM preserves that retail-sized 512-pointer table. Expanded RAM writes pointers directly
 * to the reserved table at the top of guest RAM, which holds all 32,768 moby indices and NULL.
 *
 * Under 512 targets, this is the
 * decompilation's C as it is (rac1-decomp nonmatching/functional/func_0020E6B8.c, fcheck SAME;
 * the group pass advances the list for mode & 0x1000, as retail does); its own copy stays as
 * func_0020E6B8__game, unused (libraries.tsv, wrap).
 */
#define MOBY_ARRAY RAC1_ADDR(0x00160018u)
#define CAMERA_BLOCK RAC1_ADDR(0x00187040u)
#define GROUP_MEMBERS                                                                              \
    RAC1_ADDR(0x001C7AC0u) /* u16 member lists, one per group, the last with bit 15 */
#define TARGETS RAC1_ADDR(0x001C7C80u)

enum {
    SLOTS = 0x70000000u, /* 224 class slots: {head, tail} */
    SLOTS_END = 0x70000700u,
    TARGET_LIST = 0x70000800u,
    TARGET_LIST_END = 0x70000C00u,
    GROUP_FLAGS = 0x70000C80u,
    GROUP_FLAGS_END = 0x70000D00u,
    GROUPS = 0x70,
    TARGETS_MAX = 511,
    EXPANDED_TARGETS_MAX = 0x8000,
    MOBY_POS = 0x10,
    MOBY_STATE = 0x20,
    MOBY_GROUP = 0x21,
    MOBY_SLOT = 0x22,
    MOBY_NEXT = 0x28,
    MOBY_DIST = 0x30,
    MOBY_DRAWN = 0x31,
    MOBY_MODE = 0x34
};

static int g_targets_capped;

/* The moby `m` appended to its class slot's run list. */
static void run_list_append(gaddr m) {
    const gaddr sl = SLOTS + (gaddr)GREF(unsigned char, m + MOBY_SLOT) * 8u;
    const gaddr tail = GREF(gaddr, sl + 4);
    GREF(gaddr, sl + 4) = m;
    /* The slot's first moby is its head; a later one hangs off the last. */
    if (tail == 0) {
        GREF(gaddr, sl) = m;
    } else {
        GREF(gaddr, tail + MOBY_NEXT) = m;
    }
}

/* Index `idx` written at the target list's end, kept when `mode` has 0x1000 and there is room. */
static gaddr target_add(gaddr tgt, int idx, unsigned mode, gaddr base, gaddr expanded_table,
                        unsigned* expanded_count) {
    if (openrac_guest_ram_size > GUEST_RAM_SIZE) {
        if (!(mode & 0x1000)) {
            return tgt;
        }
        if (*expanded_count < EXPANDED_TARGETS_MAX) {
            GREF(gaddr, expanded_table + 4u * (*expanded_count)) =
                base + ((gaddr)(unsigned short)idx << 8);
            ++*expanded_count;
        } else if (!g_targets_capped) {
            g_targets_capped = 1;
            openrac_game_warn("moby target list full: more than 32768 active mobys to target");
        }
        return tgt;
    }
    GREF(short, tgt) = (short)idx;
    /* Without 0x1000 the next moby writes over it, as retail's list does. */
    if (!(mode & 0x1000)) {
        return tgt;
    }
    /* Two bytes an index: room while fewer than TARGETS_MAX are kept. Switched off, nothing bounds
     * it, as on a console (past 511 it runs into the group flags and the next table). */
    if (!bigger_pool() || tgt < TARGET_LIST + TARGETS_MAX * 2u) {
        return tgt + 2;
    }
    /* One warning a session is enough for a report. */
    if (!g_targets_capped) {
        g_targets_capped = 1;
        openrac_game_warn(
            "moby target list full: more than 511 active mobys to target, the rest left out"
        );
    }
    return tgt;
}

/* Whether moby `m` is within `reach2` (its update distance, squared) of one of the mods' eyes
 * (world.c). */
static int near_an_eye(gaddr m, float reach2) {
    const float* eyes;
    const int count = rac1_moby_eyes(&eyes);
    for (int i = 0; i < count; ++i) {
        const float dx = GREF(float, m + MOBY_POS) - eyes[3 * i];
        const float dy = GREF(float, m + MOBY_POS + 4) - eyes[3 * i + 1];
        const float dz = GREF(float, m + MOBY_POS + 8) - eyes[3 * i + 2];
        if (dx * dx + dy * dy + dz * dz <= reach2) {
            return 1;
        }
    }
    return 0;
}

/* What the last run list of the level's mobys did with each slot (interface 1.9, moby_run: the
 * OPENRAC_MOBY_RUN_* values, -1 not looked at), the camera it measured from, and how many were
 * built in the level load. The pause menu's own mobys' lists are not kept. */
#define RUN_SLOTS 4096

static struct {
    uint32_t load;
    uint32_t runs;
    float camera[3];
    int8_t ran[RUN_SLOTS];
} last_run = {0xFFFFFFFFu, 0, {0, 0, 0}, {0}};

static int8_t* run_slots;

static void run_note(int idx, int8_t how) {
    if (run_slots != NULL && idx >= 0 && idx < RUN_SLOTS) {
        run_slots[idx] = how;
    }
}

int rac1_moby_run(int32_t slot, uint32_t* runs, float camera[3]) {
    if (last_run.load != rac1_program_loads()) {
        return -1;
    }
    *runs = last_run.runs;
    memcpy(camera, last_run.camera, sizeof last_run.camera);
    return slot >= 0 && slot < RUN_SLOTS ? last_run.ran[slot] : -1;
}

/* build_resident_visibility_list: the tick's run lists and target list built; the first run list's
 * head, every slot's list chained after it. */
gaddr func_0020E6B8(void) {
    run_slots = NULL;
    if (rac1_level_mobys_current()) {
        if (last_run.load != rac1_program_loads()) {
            last_run.load = rac1_program_loads();
            last_run.runs = 0;
        }
        ++last_run.runs;
        memcpy(last_run.camera, G(OPENRAC_DATA(CAMERA_BLOCK) + 0x140), sizeof last_run.camera);
        memset(last_run.ran, -1, sizeof last_run.ran);
        run_slots = last_run.ran;
    }
    for (gaddr q = SLOTS; q != SLOTS_END; q += 4) {
        GREF(int, q) = 0;
    }
    for (gaddr q = TARGET_LIST; q != TARGET_LIST_END; q += 4) {
        GREF(int, q) = 0;
    }
    for (gaddr q = GROUP_FLAGS; q != GROUP_FLAGS_END; q += 4) {
        GREF(int, q) = 0;
    }

    const gaddr base = (gaddr)GREF(int, OPENRAC_DATA(MOBY_ARRAY));
    const gaddr cam = OPENRAC_DATA(CAMERA_BLOCK) + 0x140;
    gaddr tgt = TARGET_LIST;
    const int expanded = openrac_guest_ram_size > GUEST_RAM_SIZE;
    const gaddr expanded_targets = expanded ? OPENRAC_TARGET_TABLE(OPENRAC_DATA(TARGETS)) : 0;
    unsigned expanded_target_count = 0;
    if (expanded) {
        GREF(gaddr, expanded_targets) = 0;
    }
    gaddr cur = base - 0x100;
    int idx = -1;
    for (;;) {
        const int st = GREF(signed char, cur + 0x100 + MOBY_STATE);
        cur += 0x100;
        idx++;
        /* A negative state is a free or dead slot; 0xFF marks the array's end. */
        if (st < 0) {
            if (st == -1) {
                break;
            }
            continue;
        }
        const gaddr m = cur;
        const unsigned mode = GREF(unsigned short, m + MOBY_MODE);
        /* Mode 2: the moby is not run. */
        if (mode & 2) {
            continue;
        }
        const unsigned drawn = GREF(unsigned char, m + MOBY_DRAWN);
        const unsigned dist = GREF(unsigned char, m + MOBY_DIST);
        int8_t how = drawn != 0 ? 1 : 2; /* OPENRAC_MOBY_RUN_DRAWN, _ALWAYS */
        /* Not drawn last tick and with an update distance (0xFF: always): run only within it. */
        if (drawn == 0 && dist != 0xFF) {
            const float dx = GREF(float, m + MOBY_POS) - GREF(float, cam);
            const float dy = GREF(float, m + MOBY_POS + 4) - GREF(float, cam + 4);
            const float dz = GREF(float, m + MOBY_POS + 8) - GREF(float, cam + 8);
            const float d = (float)dist;
            float t = d * d;
            t = t - dx * dx;
            t = t - dy * dy;
            t = t - dz * dz;
            /* Farther than its update distance from the camera, and from every eye a mod set
             * (world.c: other players, in the game that runs the world). */
            if (t < 0.0f && !near_an_eye(m, d * d)) {
                run_note(idx, 0); /* OPENRAC_MOBY_RUN_FAR */
                continue;
            }
            how = t < 0.0f ? 4 : 3; /* OPENRAC_MOBY_RUN_EYE, _NEAR */
        }
        run_note(idx, how);
        /* Every active moby flags its group (group -1 writes 0x70000C7F, as retail's delay slot
         * does); a group's members are added by the group pass. */
        const int group = GREF(signed char, m + MOBY_GROUP);
        GREF(char, GROUP_FLAGS + (gaddr)group) = 1;
        /* A group's member waits for the group pass. */
        if (group >= 0) {
            continue;
        }
        tgt = target_add(tgt, idx, mode, base, expanded_targets, &expanded_target_count);
        run_list_append(m);
    }

    /* The group pass: each flagged group's live members. */
    for (int gi = 0; gi != GROUPS; gi++) {
        /* No member of this group was active. */
        if (GREF(unsigned char, GROUP_FLAGS + (gaddr)gi) == 0) {
            continue;
        }
        gaddr lp = GREF(gaddr, OPENRAC_DATA(GROUP_MEMBERS) + 4u * (gaddr)gi);
        for (int r5 = 0; r5 >= 0;) {
            r5 = GREF(short, lp);
            lp += 2;
            const int member = r5 & 0x7FFF;
            const gaddr m = base + ((gaddr)member << 8);
            const unsigned mode = GREF(unsigned short, m + MOBY_MODE);
            /* A dead member is written but not kept, as retail does. */
            if (GREF(signed char, m + MOBY_STATE) < 0) {
                if (!expanded) {
                    GREF(short, tgt) = (short)member;
                }
                continue;
            }
            tgt = target_add(tgt, member, mode, base, expanded_targets, &expanded_target_count);
            run_list_append(m);
            run_note(member, 5); /* OPENRAC_MOBY_RUN_GROUP */
        }
    }

    /* The target list as the NULL-terminated pointer table. */
    if (expanded) {
        GREF(gaddr, expanded_targets + 4u * expanded_target_count) = 0;
    } else {
        gaddr out = OPENRAC_DATA(TARGETS);
        for (gaddr rp = TARGET_LIST; rp != tgt; rp += 2) {
            GREF(gaddr, out) = base + ((gaddr)GREF(unsigned short, rp) << 8);
            out += 4;
        }
        GREF(gaddr, out) = 0;
    }

    /* The class slots' lists chained in slot order; the first head is the result. */
    gaddr first = 0, last = 0;
    for (gaddr sl = SLOTS; sl != SLOTS_END; sl += 8) {
        const gaddr head = GREF(gaddr, sl);
        /* An empty slot adds nothing. */
        if (head == 0) {
            continue;
        }
        const gaddr tail = GREF(gaddr, sl + 4);
        /* The first slot with mobys gives the result; later ones hang off the last tail. */
        if (first == 0) {
            first = head;
        } else {
            GREF(gaddr, last + MOBY_NEXT) = head;
        }
        last = tail;
        GREF(gaddr, tail + MOBY_NEXT) = 0;
    }
    return first;
}
