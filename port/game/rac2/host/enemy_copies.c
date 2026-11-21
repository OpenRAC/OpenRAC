/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Going Commando: copies of a level's placed enemies for a mod (openrac_api moby_copy_check and
 * moby_copy, interface 1.25; play.c hands them to the mods). Everything here is read from the
 * game's own data, as the level's load leaves it (rac2-decomp: LoadLevelData, func_L00_00300618 in
 * src/levels/shared/L00_002FC7A0-00300618-g8.c; CreateMoby, func_0029EFD0 in src/game/0029DE38.c;
 * SetDeathBits, func_002A6C58 in src/game/002A0430.c).
 *
 * Which mobys. A copy is of a moby the level placed (its record in the gameplay file) whose class is
 * of the kind the bestiary counts (the class header's s16 at +0x46 is 5: SetDeathBits counts a kill
 * and the kill streak for that kind only), and that nothing of the level's own needs alone:
 *
 *   - its record names no slot of the level's table of mobys the scripts look up (the record's last
 *     word, +0x84, not -1: LoadLevelData hands it to level_store_value_in_table_slot);
 *   - it is in no set piece: a moby group (section 0x50) of anything but enemies of one class
 *     (the waves, the arenas, the doors that open when a group is defeated count their group's
 *     members; a squad of one class, which nearly every enemy is in, wakes and fights together,
 *     and a copy fights with it);
 *   - no other moby's, camera's or sound's data names it, and its own names no other (section 0x58,
 *     the fields LoadLevelData turns into moby indices: a copy would name the same one);
 *   - its class is not in rac2_copy_never (bosses, turrets, spawners and the crystals' creatures of
 *     Tabora and Grelbin: the table says how each was found);
 *   - it gives a reward (a save id, +0xB2, and +0xB1's 0x80 clear: SetDeathBits gives nothing
 *     otherwise, so a copy would drop nothing either).
 *
 * The level's file is read as LoadLevelData returns (it decompressed it at g_level_load_context
 * +0x0C; the wrap below): what follows in the load reuses that memory on some levels (Tabora's).
 * Its pvar table (section 0x5C) has each block's address in the level's heap by then: a placed
 * moby's pvars are the block its record names. The scan holds only when that holds for each placed
 * enemy; a file found otherwise lets no moby be copied.
 *
 * A copy. CreateMoby gives a slot of the pool past the placed mobys, the class's update and 0x80
 * bytes of pvars of its own. The copy takes the original's record but what is its slot's own (its
 * pvars' address, its sound, its spawn frame and its cells, its slot index), its place, and the
 * original's pvars: in its own 0x80 bytes, or in room the port keeps for larger ones; the pointers
 * into the block (section 0x64) point into the copy's. It keeps the original's save id and bolts
 * (+0xB2, +0xB4), so its death drops bolts and counts as the original's would, through the same
 * SetDeathBits and moby_spawn_bolt_pickups; SetDeathBits also marks the save id killed and counts
 * the kill toward the id's falloff (the nibble table and the two collection tables), which the wrap
 * below puts back for a copy, so the level's own record of the original never changes. A copy dies
 * once: a class that runs its death again pays nothing more.
 *
 * Awake. A copy is put in the moby cells at its place (MobyUpdateCell, as the game's spawners do)
 * and on its squad's group list (moved to room of the port's, the copy after the members): a
 * squad's members sleep until it is woken and alerted through that list, so a copy left off it
 * never woke, was never drawn and could not be hit (0.4.1's copies: in hidden runs of every planet
 * none of the copies past the first of a squad woke; now they wake with their originals).
 *
 * Where. On the original's floor (the ground under the spot within a unit of the ground under the
 * original), in the open (no wall on the line from the original), 2.5 to 4 units away, at the
 * original's height over its floor, and no nearer than 2 units to a moby of its class. The turn
 * around the original follows the copy's index, so each copy of one moby finds its own spot.
 * Room: the pool keeps RESERVE free slots for what the level spawns. */
#include "enemy_copies.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "game_protos.h"
#include "moby_pool.h"
#include "rac2.h"

/* ---- The game's data ---- */

/* g_level_load_context (D_L00_001BF140): +0x0C the gameplay file LoadLevelData read. */
#define LOAD_CONTEXT OPENRAC_LDATA(0, RAC2_LADDR(00, 0x001BF140u))
#define CONTEXT_FILE 0x0Cu

/* The gameplay file's sections: each a word of its header, the section's offset in the file. */
#define SECTION_MOBYS 0x4Cu
#define SECTION_GROUPS 0x50u
#define SECTION_REFS 0x58u
#define SECTION_BLOBS 0x5Cu
#define SECTION_INNER 0x64u

/* A moby record of section 0x4C (LoadLevelData's reads): its size (the first word, in bytes), its
 * save id, its pvar block, its slot in the scripts' table (-1 none). The records start 0x10 in. */
#define RECORDS_FIRST 0x10u
#define RECORD_SIZE 0x00u
#define RECORD_UID 0x10u
#define RECORD_PVARS 0x68u
#define RECORD_SCRIPT_SLOT 0x84u
#define RECORD_MIN 0x88u

/* The pvar table: per block, its address (the level's heap once loaded) and its size. */
#define BLOB_BYTES 8u

/* A moby (InitMobyFromClass's layout, func_0029F0C8; SetDeathBits's RewardMoby). */
#define MOBY_SPHERE 0x00u /* its bounding sphere: centre and radius */
#define MOBY_POS 0x10u
#define MOBY_STATE 0x20u /* 0x80 and over: dead or free; 0xFE a free slot */
#define MOBY_CLASS 0x24u
#define MOBY_PVARS 0x68u
#define MOBY_SOUND 0x6Cu   /* its sound and voice, 0xFF each for none */
#define MOBY_SPAWNED 0xA0u /* the frame its slot may be taken again, then its cells (empty) */
#define MOBY_OCLASS 0xAAu
#define MOBY_SLOT 0xACu   /* its slot's index << 16 */
#define MOBY_REWARD 0xB1u /* 0x80: no reward (a record with no flags) */
#define MOBY_UID 0xB2u
#define MOBY_SIZE 0x100u
#define CLASS_KIND 0x46u
#define KIND_ENEMY 5
#define STATE_FREE 0xFEu
#define STATE_UNUSED 0xFFu
/* What CreateMoby gives a moby of the pool of pvars. */
#define SLOT_PVARS 0x80u

/* SetDeathBits's marks for a save id: its kill count, 3 bits a nibble by id in the map's 1024 bytes
 * (g_moby_nibble_table, D_0019B4A8, by g_current_map_index, the save block's first word), and its
 * byte in the collection table and its checkpoint copy (g_bolt_collection_table, D_00210D40, and
 * g_bolt_collection_checkpoint, D_002119B0, each +0x464 by id). */
#define NIBBLES RAC2_ADDR(0x0019B4A8u)
#define NIBBLE_MAP_BYTES 0x400u
#define COLLECTED OPENRAC_DATA(RAC2_ADDR(0x00210D40u))
#define COLLECTED_CHECKPOINT OPENRAC_DATA(RAC2_ADDR(0x002119B0u))
#define COLLECTED_FIRST 0x464u
#define MAP_INDEX RAC2_g_playerProgress
#define UID_MAX 2048 /* the nibble table's ids: 1024 bytes of two */

/* ---- What a copy may be ---- */

/* The largest pvars a copy takes, the room for those over SLOT_PVARS, the pool's slots kept free,
 * the copies of one load at most. */
#define PVARS_MAX 0x800u
#define ROOM_BYTES 0x30000u
#define RESERVE 128
#define COPIES_MAX 1024
/* The records and blocks a file is believed to have at most, the entries of a list. */
#define RECORDS_MAX 4096
#define BLOBS_MAX 8192
#define LIST_MAX 65536
/* Placed mobys this file tracks (slots). */
#define PLACED_MAX 4096
/* The pointers into blocks the scan keeps. */
#define INNER_MAX 8192
/* g_frame_l00_counter (D_L00_001B19C8): the level loop's ticks, which LoadLevelArchiveResources
 * counts from; a scan that found the file gone is tried again a tick later at most. */
#define TICKS OPENRAC_LDATA(0, RAC2_LADDR(00, 0x001B19C8u))

/* The spot: the tries, the turn between them, the jitter, the distances, the floor's tolerance, the
 * line's height over the floor, the gap to the class's others. */
#define SPOT_TRIES 8
#define SPOT_COPY_STRIDE 3
#define SPOT_STEP 0.78539816f
#define SPOT_JITTER 0.4f
#define SPOT_NEAR 2.5f
#define SPOT_RANGE 1.5f
#define SPOT_FLOOR_TOLERANCE 1.0f
#define SPOT_LINE_HEIGHT 1.0f
#define SPOT_PROBE_HEIGHT 0.5f
#define SPOT_BELOW_FLOOR (-0.5f)
#define SPOT_GAP 2.0f

/* The classes never copied, whatever their records say. Found in rac2-decomp: the classes
 * docs/game/moby-dispatch.tsv labels a turret or a spawner, the bosses (NG++'s kBossClasses, in
 * openrac-mods ng-plus-plus/src/tiers.h, with the labels where the table has one), and the creatures
 * of Tabora's and Grelbin's crystal fields, whose deaths make the crystals and moonstones the
 * collectors buy. */
const rac2_copy_class rac2_copy_never[] = {
    {3098, "Spawner (moby-dispatch.tsv): makes the level's waves"},
    {3104, "Turret (moby-dispatch.tsv): fixed in its place"},
    /* Tabora's crystal field: the creatures whose death makes the crystals the collector buys
     * (class 2244, whose pickup marks its maker's save id killed: func_L08_003CC750). A copy's
     * crystals would mark its original and pay the collector twice. */
    {2410,
     "Desert Crystal Shell, Tabora: crystal_shell_update_damage (func_L08_003D2F50) makes "
     "crystals"},
    {4098, "Desert Dune Monster, Tabora: dune_monster_update (func_L08_00426730) makes crystals"},
    /* Grelbin's moonstones: the creatures and the breakable crystals that drop them (class 4710,
     * marking the dropper's save id: func_L19_004160A0, func_L19_004203F0). */
    {3927, "Arctic Leviathan, Grelbin (moby-dispatch.tsv): its death drops moonstones"},
    {4551,
     "Grelbin's breakable moonstone crystal: UpdateBreakablePickupSpawner (func_L19_004203F0)"},
    /* Squads a controller holds back and lets out (a spawner of the level's, not one of the
     * group): their copies would come out at once. */
    {4304, "Y.E.T.I., Grelbin: yeti_group_spawner_update (func_L19_0041EE60) lets them out"},
    /* The bosses. */
    {2644, "Swamp Monster II, Oozla (moby-dispatch.tsv)"},
    {2701, "Swamp Monster I Arm, Oozla (moby-dispatch.tsv)"},
    {2702, "Swamp Monster I Eye, Oozla (moby-dispatch.tsv)"},
    {2552, "Chainblade, Maktar's arena (moby-dispatch.tsv)"},
    {2628, "B2 Brawler Legs, Maktar's arena (moby-dispatch.tsv)"},
    {2635, "B2 Brawler Head, Maktar's arena (moby-dispatch.tsv)"},
    {3210, "Endako's boss"},
    {3679, "Siberius's boss"},
    {3973, "Hrugis's boss"},
    {4785, "Todano's boss"},
    {3961, "Snivelak's boss"},
    {2874, "Arachnoid, Joba's arena (moby-dispatch.tsv)"},
    {3402, "Joba's arena boss"},
    {3145, "the Thugs-4-Less leader"},
    {3806, "the Thugs-4-Less leader's mech, Damosel"},
    {3967, "Mothership Boss Part, Damosel's orbit (moby-dispatch.tsv)"},
    {4623, "the mothership, Damosel's orbit"},
    {4576, "the mutated Protopet, the last boss"},
};
const unsigned rac2_copy_never_count = sizeof rac2_copy_never / sizeof rac2_copy_never[0];

/* ---- The scan of a load ---- */

/* What the scan keeps of the file for the copies (the file's memory is the level's to reuse once
 * it has loaded): each placed moby's answer, why a scripted one is, its pvars' size and block, and
 * the pointers into the blocks. It holds for the program's load and the loads of the same level
 * after a death, which lay the level out alike. */
static struct {
    int valid; /* 1 the scan holds, -1 it found the file gone (tried again a tick later) */
    uint32_t loads;
    uint32_t tick;
    gaddr base;
    gaddr dynamic;
    gaddr file;
    int8_t verdict[PLACED_MAX];
    uint8_t detail[PLACED_MAX];
    int16_t blob[PLACED_MAX];
    uint16_t size[PLACED_MAX];
    int inner_count;
    int16_t inner_blob[INNER_MAX];
    uint16_t inner_offset[INNER_MAX];
} scan;

/* The copies of this load, and the room their larger pvars take. */
static struct {
    uint32_t loads;
    int count;
    gaddr moby[COPIES_MAX];
    gaddr orig[COPIES_MAX];
    int16_t uid[COPIES_MAX];
    uint8_t died[COPIES_MAX];
    gaddr room;
    uint32_t room_used;
} made;

/* The marks of the file's records, by save id and by record. */
static int16_t record_of_uid[UID_MAX];
static gaddr record_at[UID_MAX];
static int16_t uid_of_record[RECORDS_MAX];
static uint8_t uid_marks[UID_MAX];
static uint8_t names_moby[BLOBS_MAX / 8];
static uint8_t named[PLACED_MAX / 8];
#define MARK_SCRIPT RAC2_COPY_WHY_SCRIPT
#define MARK_GROUP RAC2_COPY_WHY_GROUP

static int in_ram(gaddr a, uint32_t bytes) {
    return a >= 0x00100000u && a < openrac_guest_ram_size && bytes <= openrac_guest_ram_size - a;
}

static uint32_t this_load(void) {
    return rac2_program_loads();
}

static uint32_t this_tick(void) {
    return GREF(uint32_t, TICKS);
}

static int listed(int16_t oclass) {
    for (unsigned i = 0; i < rac2_copy_never_count; ++i) {
        if (rac2_copy_never[i].oclass == oclass) {
            return 1;
        }
    }
    return 0;
}

static gaddr section(gaddr file, uint32_t which) {
    const int32_t off = GREF(int32_t, file + which);
    return off > 0 && in_ram(file + (uint32_t)off, 16) ? file + (uint32_t)off : 0;
}

static int bit(const uint8_t* bits, uint32_t i) {
    return (bits[i >> 3] >> (i & 7)) & 1;
}

static void set_bit(uint8_t* bits, uint32_t i) {
    bits[i >> 3] |= (uint8_t)(1u << (i & 7));
}

/* The records: each one's save id, its script slot. 0 when the section is not as expected. */
static int read_records(gaddr file, int32_t* count) {
    const gaddr s = section(file, SECTION_MOBYS);
    if (s == 0) {
        return 0;
    }
    const int32_t n = GREF(int32_t, s);
    if (n <= 0 || n > RECORDS_MAX) {
        return 0;
    }
    gaddr p = s + RECORDS_FIRST;
    for (int32_t i = 0; i < n; ++i) {
        const uint32_t size = GREF(uint32_t, p + RECORD_SIZE) & ~3u;
        if (size < RECORD_MIN || !in_ram(p, size)) {
            return 0;
        }
        const int32_t uid = GREF(int32_t, p + RECORD_UID);
        uid_of_record[i] = uid >= 0 && uid < UID_MAX ? (int16_t)uid : -1;
        if (uid >= 0 && uid < UID_MAX) {
            record_of_uid[uid] = (int16_t)i;
            record_at[uid] = p;
            if (GREF(int32_t, p + RECORD_SCRIPT_SLOT) != -1) {
                uid_marks[uid] |= MARK_SCRIPT;
            }
        }
        p += size;
    }
    *count = n;
    return 1;
}

/* The placed moby of a save id, 0 for none (one the load left out). */
static gaddr moby_of_uid(const rac2_moby_table* t, int16_t uid) {
    for (gaddr m = t->base; m < t->dynamic; m += MOBY_SIZE) {
        if (GREF(uint8_t, m + MOBY_STATE) < 0x80 && GREF(int16_t, m + MOBY_UID) == uid) {
            return m;
        }
    }
    return 0;
}

/* The groups (section 0x50): each a list of record indices (low 15 bits), the last with 0x8000.
 * Going Commando puts nearly every enemy in one: a squad of one class, which wakes and fights
 * together (moby +0x21 is its group). A squad's copy fights beside it and is copied. A group of
 * anything else (enemies of several classes, or enemies with a door, a crate, a prop or a moby of
 * no class loaded) is a set piece a controller of the level runs: its enemies are not copied. */
static void read_groups(const rac2_moby_table* t, gaddr file, int32_t records) {
    const gaddr s = section(file, SECTION_GROUPS);
    if (s == 0) {
        return;
    }
    const int32_t n = GREF(int32_t, s);
    if (n <= 0 || n > RECORDS_MAX) {
        return;
    }
    const gaddr offs = s + 0x10u;
    const gaddr data = offs + 4u * (uint32_t)n;
    for (int32_t g = 0; g < n; ++g) {
        const int32_t off = GREF(int32_t, offs + 4u * (uint32_t)g);
        if (off < 0) {
            continue;
        }
        /* A squad: every member the load placed an enemy of one class. */
        int squad = 1;
        int16_t oclass = -1;
        gaddr e = data + (uint32_t)off;
        for (int k = 0; k < LIST_MAX && in_ram(e, 2); ++k, e += 2) {
            const uint16_t v = GREF(uint16_t, e);
            const int32_t id = v & 0x7FFF;
            const gaddr m =
                id < records && uid_of_record[id] >= 0 ? moby_of_uid(t, uid_of_record[id]) : 0;
            if (m != 0) {
                const gaddr cls = GREF(gaddr, m + MOBY_CLASS);
                const int16_t c = GREF(int16_t, m + MOBY_OCLASS);
                if (cls == 0 || !in_ram(cls, CLASS_KIND + 2)
                    || GREF(int16_t, cls + CLASS_KIND) != KIND_ENEMY
                    || (oclass >= 0 && c != oclass)) {
                    squad = 0;
                }
                oclass = c;
            }
            if ((v & 0x8000u) != 0) {
                break;
            }
        }
        if (squad) {
            continue;
        }
        e = data + (uint32_t)off;
        for (int k = 0; k < LIST_MAX && in_ram(e, 2); ++k, e += 2) {
            const uint16_t v = GREF(uint16_t, e);
            const int32_t id = v & 0x7FFF;
            if (id < records && uid_of_record[id] >= 0) {
                uid_marks[uid_of_record[id]] |= MARK_GROUP;
            }
            if ((v & 0x8000u) != 0) {
                break;
            }
        }
    }
}

/* The fields that name a moby (section 0x58): (block, offset) pairs to a negative block. Each names
 * the moby of its value's index, -1 none. */
static void read_refs(gaddr file, gaddr blobs, gaddr base, gaddr dynamic) {
    const gaddr s = section(file, SECTION_REFS);
    if (s == 0) {
        return;
    }
    const uint32_t placed = (dynamic - base) / MOBY_SIZE;
    for (gaddr q = s; in_ram(q, 8) && GREF(int32_t, q) >= 0 && q < s + 8u * LIST_MAX; q += 8) {
        const int32_t k = GREF(int32_t, q);
        const int32_t off = GREF(int32_t, q + 4);
        if (k >= BLOBS_MAX || off < 0) {
            continue;
        }
        const gaddr x = GREF(gaddr, blobs + BLOB_BYTES * (uint32_t)k);
        if (!in_ram(x + (uint32_t)off, 4)) {
            continue;
        }
        const int32_t v = GREF(int32_t, x + (uint32_t)off);
        if (v >= 0) {
            set_bit(names_moby, (uint32_t)k);
            if ((uint32_t)v < placed && (uint32_t)v < PLACED_MAX) {
                set_bit(named, (uint32_t)v);
            }
        }
    }
}

static int8_t judge(
    gaddr m,
    uint32_t slot,
    gaddr blobs,
    int16_t* blob,
    uint16_t* size_out,
    uint8_t* detail,
    int* stale
) {
    *blob = -1;
    *size_out = 0;
    *detail = 0;
    if (GREF(uint8_t, m + MOBY_STATE) >= 0x80) {
        return RAC2_COPY_NOT_PLACED;
    }
    const gaddr cls = GREF(gaddr, m + MOBY_CLASS);
    if (cls == 0 || !in_ram(cls, CLASS_KIND + 2) || GREF(int16_t, cls + CLASS_KIND) != KIND_ENEMY) {
        return RAC2_COPY_NOT_ENEMY;
    }
    const int16_t uid = GREF(int16_t, m + MOBY_UID);
    if (uid < 0 || uid >= UID_MAX || (GREF(uint8_t, m + MOBY_REWARD) & 0x80u) != 0
        || record_of_uid[uid] < 0) {
        return RAC2_COPY_NOT_PLACED;
    }
    /* The record's block is the moby's pvars while the file is the one loaded. */
    const int32_t k = GREF(int32_t, record_at[uid] + RECORD_PVARS);
    const gaddr pvars = GREF(gaddr, m + MOBY_PVARS);
    uint32_t size = 0;
    if (k >= 0 && k < BLOBS_MAX) {
        if (GREF(gaddr, blobs + BLOB_BYTES * (uint32_t)k) != pvars) {
            *stale = 1;
            return RAC2_COPY_UNKNOWN;
        }
        size = GREF(uint32_t, blobs + BLOB_BYTES * (uint32_t)k + 4u);
        *blob = (int16_t)k;
    } else if (pvars != 0) {
        *stale = 1;
        return RAC2_COPY_UNKNOWN;
    }
    if (listed(GREF(int16_t, m + MOBY_OCLASS))) {
        return RAC2_COPY_LISTED;
    }
    *detail =
        (uint8_t)(uid_marks[uid] | (bit(named, slot) ? RAC2_COPY_WHY_NAMED : 0u)
                  | (*blob >= 0 && bit(names_moby, (uint32_t)*blob) ? RAC2_COPY_WHY_NAMES : 0u));
    if (*detail != 0) {
        return RAC2_COPY_SCRIPTED;
    }
    *size_out = (uint16_t)(size <= PVARS_MAX ? size : 0);
    return size > PVARS_MAX ? RAC2_COPY_NO_ROOM : RAC2_COPY_OK;
}

/* The scan for this load, made at the first ask. 0 when the file is not the level's now. */
static int scanned(const rac2_moby_table* t) {
    const gaddr ctx = LOAD_CONTEXT;
    const gaddr file = in_ram(ctx, 16) ? GREF(gaddr, ctx + CONTEXT_FILE) : 0;
    if (scan.valid != 0 && scan.loads == this_load() && scan.base == t->base
        && scan.dynamic == t->dynamic && scan.file == file
        && (scan.valid > 0 || scan.tick == this_tick())) {
        return scan.valid > 0;
    }
    memset(&scan, 0, sizeof scan);
    scan.loads = this_load();
    scan.tick = this_tick();
    scan.base = t->base;
    scan.dynamic = t->dynamic;
    scan.file = file;
    scan.valid = -1;
    const uint32_t placed = (t->dynamic - t->base) / MOBY_SIZE;
    if (file == 0 || !in_ram(file, 0xC0) || placed > PLACED_MAX) {
        return 0;
    }
    const gaddr blobs = section(file, SECTION_BLOBS);
    const gaddr inner = section(file, SECTION_INNER);
    memset(record_of_uid, 0xFF, sizeof record_of_uid);
    memset(record_at, 0, sizeof record_at);
    memset(uid_of_record, 0xFF, sizeof uid_of_record);
    memset(uid_marks, 0, sizeof uid_marks);
    memset(names_moby, 0, sizeof names_moby);
    memset(named, 0, sizeof named);
    int32_t records = 0;
    if (blobs == 0 || !read_records(file, &records)) {
        return 0;
    }
    read_groups(t, file, records);
    read_refs(file, blobs, t->base, t->dynamic);
    int stale = 0;
    for (uint32_t s = 0; s < placed && !stale; ++s) {
        scan.verdict[s] = judge(
            t->base + s * MOBY_SIZE, s, blobs, &scan.blob[s], &scan.size[s], &scan.detail[s], &stale
        );
    }
    /* The pointers into blocks (section 0x64: block, offset pairs to a negative block). */
    for (gaddr q = inner; q != 0 && in_ram(q, 8) && GREF(int32_t, q) >= 0
                          && q < inner + 8u * LIST_MAX && scan.inner_count < INNER_MAX;
         q += 8) {
        const int32_t k = GREF(int32_t, q), off = GREF(int32_t, q + 4);
        if (k < BLOBS_MAX && off >= 0 && off < (int32_t)PVARS_MAX) {
            scan.inner_blob[scan.inner_count] = (int16_t)k;
            scan.inner_offset[scan.inner_count] = (uint16_t)off;
            ++scan.inner_count;
        }
    }
    scan.valid = stale ? -1 : 1;
    return !stale;
}

int rac2_copy_check(const rac2_moby_table* t, gaddr moby) {
    if (t == NULL || moby < t->base || moby >= t->end || (moby - t->base) % MOBY_SIZE != 0) {
        return RAC2_COPY_UNKNOWN;
    }
    if (moby >= t->dynamic) {
        return RAC2_COPY_NOT_PLACED;
    }
    if (!scanned(t)) {
        return RAC2_COPY_UNKNOWN;
    }
    return scan.verdict[(moby - t->base) / MOBY_SIZE];
}

int rac2_copy_detail(const rac2_moby_table* t, gaddr moby) {
    return rac2_copy_check(t, moby) == RAC2_COPY_SCRIPTED
               ? scan.detail[(moby - t->base) / MOBY_SIZE]
               : 0;
}

/* The level's mobys as LoadLevelData leaves them (the boot program's words, the level's copy):
 * the first, the first past the placed ones, the end. */
#define TABLE_BASE OPENRAC_DATA(RAC2_g_mobyTableBase)
#define TABLE_DYNAMIC OPENRAC_DATA(RAC2_g_mobyTableBase + 4u)
#define TABLE_END OPENRAC_DATA(RAC2_g_mobyTableEnd)

/* LoadLevelData (func_L00_00300618; a level's own load, and its load again after a death): the
 * level's file read for the copies as the game has just laid the level out from it. */
gaddr func_L00_00300618(int reload) {
    /* The level's gameplay file may get spare mobys beyond the disc's as it is decompressed. */
    rac2_pool_loading();
    const gaddr heap = func_L00_00300618__game(reload);
    rac2_pool_loaded();
    const rac2_moby_table t =
        {GREF(gaddr, TABLE_BASE), GREF(gaddr, TABLE_DYNAMIC), GREF(gaddr, TABLE_END)};
    if (t.base != 0 && t.dynamic >= t.base && t.end >= t.dynamic
        && (t.end - t.base) % MOBY_SIZE == 0) {
        scan.valid = 0;
        (void)scanned(&t);
    }
    return heap;
}

/* ---- The copies ---- */

/* The level's moby groups at run time (LoadLevelData lays them out from section 0x50): per group a
 * list of pool indices (from g_moby_pool_start, the table's base), the last with 0x8000, and the
 * highest group (GetFirstMobyInGroup, GetNextMobyInGroup, moby_count_group_not_in_state in
 * src/game/002A7750.c). A squad's controller wakes, alerts and counts its members through the list,
 * so a copy that is not on it sleeps forever (0.4.1's copies: never drawn, never run). */
#define GROUP_LISTS OPENRAC_DATA(RAC2_ADDR(0x001EE5A0u))
#define GROUP_COUNT OPENRAC_DATA(RAC2_ADDR(0x001B1B78u))
#define MOBY_GROUP 0x21u
#define GROUP_NONE 0xFFu
#define GROUP_LAST 0x8000u
#define GROUP_MEMBERS_MAX 1024
#define GROUP_ROOM_BYTES 0x18000u

/* The lists the copies' groups were moved to, in room of the port's (one load's). */
static struct {
    uint32_t loads;
    gaddr room;
    uint32_t used;
} lists;

/* Puts copy `m` on its group's list, after the members: the list moved to the port's room with one
 * more entry. 0 when it has no group, or no room. */
static int join_group(const rac2_moby_table* t, gaddr m) {
    const uint32_t group = GREF(uint8_t, m + MOBY_GROUP);
    if (group == GROUP_NONE || (int32_t)group > GREF(int32_t, GROUP_COUNT)) {
        return 0;
    }
    if (lists.loads != this_load()) {
        lists.loads = this_load();
        lists.used = 0;
    }
    const gaddr at = GROUP_LISTS + 4u * group;
    const gaddr list = GREF(gaddr, at);
    /* The level's own list, or one of ours (the port's room is past the game's RAM). */
    const int ours = lists.room != 0 && list >= lists.room && list < lists.room + GROUP_ROOM_BYTES;
    if (list == 0 || (!ours && !in_ram(list, 2))) {
        return 0;
    }
    int n = 0;
    while (n < GROUP_MEMBERS_MAX && (GREF(uint16_t, list + 2u * (uint32_t)n) & GROUP_LAST) == 0) {
        ++n;
    }
    if (n >= GROUP_MEMBERS_MAX) {
        return 0;
    }
    ++n; /* the last, with its mark */
    const uint32_t bytes = ((uint32_t)(n + 1) * 2u + 15u) & ~15u;
    if (lists.room == 0) {
        /* Taken once for the program: the statics are never given back, and each level load asks again. */
        static gaddr block;
        if (block == 0) {
            block = openrac_guest_static(GROUP_ROOM_BYTES);
        }
        lists.room = block;
    }
    if (lists.room == 0 || lists.used + bytes > GROUP_ROOM_BYTES) {
        return 0;
    }
    const gaddr moved = lists.room + lists.used;
    lists.used += bytes;
    memmove(G(moved), G(list), 2u * (uint32_t)n);
    GREF(uint16_t, moved + 2u * (uint32_t)(n - 1)) &= (uint16_t)~GROUP_LAST;
    GREF(uint16_t, moved + 2u * (uint32_t)n) = (uint16_t)(((m - t->base) / MOBY_SIZE) | GROUP_LAST);
    GREF(gaddr, at) = moved;
    return 1;
}

static int copy_alive(int i) {
    const gaddr m = made.moby[i];
    return GREF(uint8_t, m + MOBY_STATE) < 0x80 && GREF(int16_t, m + MOBY_UID) == made.uid[i];
}

/* Another program forgets the copies; so does a load of the same level after a death, which
 * leaves none of them (the pool laid out again): their room is free again. */
static void forget_old(void) {
    int alive = made.loads == this_load() ? 0 : -1;
    for (int i = 0; alive == 0 && i < made.count; ++i) {
        alive = copy_alive(i);
    }
    if (alive > 0 || (alive == 0 && made.count == 0)) {
        return;
    }
    made.loads = this_load();
    made.count = 0;
    made.room_used = 0;
}

int rac2_copy_count(void) {
    forget_old();
    return made.count;
}

gaddr rac2_copy_at(int i, gaddr* orig) {
    forget_old();
    if (i < 0 || i >= made.count) {
        return 0;
    }
    if (orig != NULL) {
        *orig = made.orig[i];
    }
    return made.moby[i];
}

/* The index of the live copy at `moby` in this load's, -1 for none. */
static int copy_index(gaddr moby) {
    forget_old();
    for (int i = 0; i < made.count; ++i) {
        if (made.moby[i] == moby) {
            return copy_alive(i) ? i : -1;
        }
    }
    return -1;
}

int rac2_copy_is(gaddr moby) {
    return copy_index(moby) >= 0;
}

/* The pool's free slots: each one given back (0xFE), and every one from the first never used on
 * (0xFF: CreateMoby moves that mark up as it takes one). */
static int free_slots(const rac2_moby_table* t) {
    int n = 0;
    for (gaddr m = t->dynamic; m < t->end; m += MOBY_SIZE) {
        const uint8_t state = GREF(uint8_t, m + MOBY_STATE);
        if (state == STATE_UNUSED) {
            return n + (int)((t->end - m) / MOBY_SIZE);
        }
        n += state == STATE_FREE;
    }
    return n;
}

/* The ground under the point (x, y, z): its height, or 0 for none (coll_probe_ground_height_below,
 * func_002A89A8, from `over` above the point). */
static float ground(float x, float y, float z, float over) {
    GFRAME(point, 16);
    GREF(float, point) = x;
    GREF(float, point + 4u) = y;
    GREF(float, point + 8u) = z;
    GREF(float, point + 12u) = 0.0f;
    return func_002A89A8(point, 0, over);
}

/* Whether a wall of the level stands on the line between two points (CollLine, func_002761F8; the
 * moby `ignore` passed over). */
static int blocked(const float a[3], const float b[3], gaddr ignore) {
    GFRAME(line, 32);
    for (unsigned c = 0; c < 3; ++c) {
        GREF(float, line + 4u * c) = a[c];
        GREF(float, line + 16u + 4u * c) = b[c];
    }
    GREF(float, line + 12u) = 0.0f;
    GREF(float, line + 28u) = 0.0f;
    return func_002761F8(line, line + 16u, 0, ignore, 0) != 0;
}

/* A number in [0, 1) from three, the same on every machine. */
static float unit_hash(uint32_t a, uint32_t b, uint32_t c) {
    uint32_t h =
        a * 0x9E3779B1u ^ (b + 0x7F4A7C15u) * 0x85EBCA77u ^ (c + 0x165667B1u) * 0xC2B2AE3Du;
    h ^= h >> 15;
    h *= 0x2C1B3C6Du;
    h ^= h >> 12;
    return (float)(h >> 8) / 16777216.0f;
}

static int crowded(const rac2_moby_table* t, int16_t oclass, const float at[3]) {
    for (gaddr m = t->base; m < t->end; m += MOBY_SIZE) {
        if (GREF(uint8_t, m + MOBY_STATE) >= 0x80 || GREF(int16_t, m + MOBY_OCLASS) != oclass) {
            continue;
        }
        const float dx = at[0] - GREF(float, m + MOBY_POS);
        const float dy = at[1] - GREF(float, m + MOBY_POS + 4u);
        const float dz = at[2] - GREF(float, m + MOBY_POS + 8u);
        if (dx * dx + dy * dy < SPOT_GAP * SPOT_GAP && fabsf(dz) < SPOT_GAP) {
            return 1;
        }
    }
    return 0;
}

/* A clear spot for copy `index` of `orig` (the header's rules), into `out`: 1, or 0 for none. */
static int find_spot(const rac2_moby_table* t, gaddr orig, int index, float out[3]) {
    const float x = GREF(float, orig + MOBY_POS), y = GREF(float, orig + MOBY_POS + 4u),
                z = GREF(float, orig + MOBY_POS + 8u);
    const float floor0 = ground(x, y, z, 1.0f);
    if (!(floor0 > 0.0f) || z - floor0 < SPOT_BELOW_FLOOR) {
        return 0;
    }
    const float over = z - floor0;
    const uint32_t uid = (uint16_t)GREF(int16_t, orig + MOBY_UID);
    for (int attempt = 0; attempt < SPOT_TRIES; ++attempt) {
        const float angle = (float)(index * SPOT_COPY_STRIDE + attempt) * SPOT_STEP
                            + unit_hash(uid, (uint32_t)index, (uint32_t)attempt) * SPOT_JITTER;
        const float r =
            SPOT_NEAR + unit_hash(uid, (uint32_t)index, 100u + (uint32_t)attempt) * SPOT_RANGE;
        const float cx = x + r * cosf(angle), cy = y + r * sinf(angle);
        const float from[3] = {x, y, floor0 + SPOT_LINE_HEIGHT};
        const float to[3] = {cx, cy, floor0 + SPOT_LINE_HEIGHT};
        if (blocked(from, to, orig)) {
            continue;
        }
        const float floor = ground(cx, cy, floor0 + SPOT_PROBE_HEIGHT, 1.0f);
        if (!(floor > 0.0f) || fabsf(floor - floor0) > SPOT_FLOOR_TOLERANCE) {
            continue;
        }
        const float at[3] = {cx, cy, floor + over};
        if (crowded(t, GREF(int16_t, orig + MOBY_OCLASS), at)) {
            continue;
        }
        memcpy(out, at, sizeof at);
        return 1;
    }
    return 0;
}

/* The fields a copy keeps of its own slot: offset and bytes. */
static const struct {
    uint32_t at, bytes;
} own_fields[] = {
    {MOBY_PVARS, 4},
    {MOBY_SOUND, 2},
    {MOBY_SPAWNED, 8},
    {MOBY_SLOT, 4},
};

/* Whether the `size` bytes of pvars at `pvars` hold the address of a moby of the pool other than
 * `self` and the hero's: a part the moby made of its own (a head, a gun, a shield) or one it
 * follows. A copy would drive the same one, and once the original died and freed it, a moby that
 * is no longer its (Todano's and Dobbo's guards: a hang or a fault in their update). */
static int names_a_moby(const rac2_moby_table* t, gaddr pvars, uint32_t size, gaddr self) {
    const gaddr hero = GREF(gaddr, OPENRAC_DATA(RAC2_g_pHeroMoby));
    for (uint32_t off = 0; pvars != 0 && off + 4u <= size; off += 4u) {
        const gaddr v = GREF(gaddr, pvars + off);
        if (v >= t->base && v < t->end && (v - t->base) % MOBY_SIZE == 0 && v != self
            && v != hero) {
            return 1;
        }
    }
    return 0;
}

gaddr rac2_copy_make(const rac2_moby_table* t, gaddr orig, int index) {
    if (index < 0 || rac2_copy_check(t, orig) != RAC2_COPY_OK) {
        return 0;
    }
    forget_old();
    if (made.count >= COPIES_MAX || free_slots(t) <= RESERVE) {
        return 0;
    }
    const int16_t k = scan.blob[(orig - t->base) / MOBY_SIZE];
    const uint32_t size = scan.size[(orig - t->base) / MOBY_SIZE];
    const uint32_t room_need = size > SLOT_PVARS ? (size + 15u) & ~15u : 0;
    if (room_need > 0) {
        if (made.room == 0) {
            /* Taken once for the program, as the groups' room is. */
            static gaddr block;
            if (block == 0) {
                block = openrac_guest_static(ROOM_BYTES);
            }
            made.room = block;
        }
        if (made.room == 0 || made.room_used + room_need > ROOM_BYTES) {
            return 0;
        }
    }
    if (names_a_moby(t, GREF(gaddr, orig + MOBY_PVARS), size, orig)) {
        return 0;
    }
    float spot[3];
    if (!find_spot(t, orig, index, spot)) {
        return 0;
    }
    const int16_t oclass = GREF(int16_t, orig + MOBY_OCLASS);
    const gaddr m = func_0029EFD0(oclass);
    if (m == 0) {
        return 0;
    }
    /* A class the level has not loaded: InitMobyFromClass leaves it with no class. */
    if (GREF(gaddr, m + MOBY_CLASS) == 0) {
        func_0029F338(m);
        return 0;
    }
    uint8_t own[MOBY_SIZE];
    memcpy(own, G(m), MOBY_SIZE);
    memcpy(G(m), G(orig), MOBY_SIZE);
    for (unsigned i = 0; i < sizeof own_fields / sizeof own_fields[0]; ++i) {
        memcpy(G(m + own_fields[i].at), own + own_fields[i].at, own_fields[i].bytes);
    }
    for (unsigned c = 0; c < 3; ++c) {
        const float delta = spot[c] - GREF(float, orig + MOBY_POS + 4u * c);
        GREF(float, m + MOBY_POS + 4u * c) = spot[c];
        if (GREF(float, m + MOBY_SPHERE + 12u) > 0.0f) {
            GREF(float, m + MOBY_SPHERE + 4u * c) += delta;
        }
    }
    const gaddr from = GREF(gaddr, orig + MOBY_PVARS);
    gaddr to = GREF(gaddr, m + MOBY_PVARS);
    if (room_need > 0) {
        to = made.room + made.room_used;
        made.room_used += room_need;
        GREF(gaddr, m + MOBY_PVARS) = to;
    }
    if (from != 0 && to != 0 && size > 0) {
        memmove(G(to), G(from), size);
        /* The pointers into the block (section 0x64) point into the copy's. */
        for (int i = 0; i < scan.inner_count; ++i) {
            const uint32_t off = scan.inner_offset[i];
            if (scan.inner_blob[i] != k || off + 4u > size) {
                continue;
            }
            const gaddr v = GREF(gaddr, to + off);
            if (v >= from && v < from + size) {
                GREF(gaddr, to + off) = v - from + to;
            }
        }
    }
    /* In the moby cells at its place, as a spawned moby is once placed (func_002A0F48 after the
     * place, as the game's spawners do): the cells near the hero are where the moby loop wakes a
     * sleeping enemy, so one in no cell sleeps forever. */
    func_002A0F48(m);
    (void)join_group(t, m);
    made.moby[made.count] = m;
    made.orig[made.count] = orig;
    made.died[made.count] = 0;
    made.uid[made.count] = GREF(int16_t, m + MOBY_UID);
    ++made.count;
    return m;
}

/* ---- A copy's death ---- */

/* SetDeathBits (func_002A6C58; a mod's rac2.moby.record_death runs within, as
 * func_002A6C58__hooked): for a copy, the marks it leaves on the save id are put back after, so the
 * original's record is the level's own; its bolts, its experience and the kill streak are the
 * game's. */
int rac2_copy_trace;

void func_002A6C58(gaddr moby, int flags, int arg) {
    const int index = copy_index(moby);
    if (index < 0) {
        if (rac2_copy_trace) {
            fprintf(
                stderr,
                "PTRACE orig death 0x%X uid %d class %d\n",
                (unsigned)moby,
                (int)GREF(int16_t, moby + MOBY_UID),
                (int)GREF(int16_t, moby + MOBY_OCLASS)
            );
        }
        func_002A6C58__hooked(moby, flags, arg);
        return;
    }
    /* A copy dies once: a class that runs its death again (the save id's marks put back read to
     * it as alive) drops nothing more and counts no second kill. */
    if (made.died[index]) {
        return;
    }
    made.died[index] = 1;
    const int16_t uid = GREF(int16_t, moby + MOBY_UID);
    const int32_t map = GREF(int32_t, MAP_INDEX);
    const int known = uid >= 0 && uid < UID_MAX && map >= 0 && map < 32;
    const gaddr nibble =
        known ? NIBBLES + (uint32_t)(uid >> 1) + NIBBLE_MAP_BYTES * (uint32_t)map : 0;
    const gaddr collected = known ? COLLECTED + COLLECTED_FIRST + (uint32_t)uid : 0;
    const gaddr checkpoint = known ? COLLECTED_CHECKPOINT + COLLECTED_FIRST + (uint32_t)uid : 0;
    const uint8_t kept[3] = {
        known ? GREF(uint8_t, nibble) : 0,
        known ? GREF(uint8_t, collected) : 0,
        known ? GREF(uint8_t, checkpoint) : 0,
    };
    const int32_t word_before = GREF(int32_t, RAC2_g_playerProgress + 0x1Cu);
    func_002A6C58__hooked(moby, flags, arg);
    if (rac2_copy_trace) {
        fprintf(
            stderr,
            "PTRACE copy death 0x%X uid %d bolts %d word %d->%d\n",
            (unsigned)moby,
            (int)uid,
            (int)GREF(int16_t, moby + 0xB4u),
            (int)word_before,
            (int)GREF(int32_t, RAC2_g_playerProgress + 0x1Cu)
        );
    }
    if (known) {
        GREF(uint8_t, nibble) = kept[0];
        GREF(uint8_t, collected) = kept[1];
        GREF(uint8_t, checkpoint) = kept[2];
    }
}
