/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Imported moby classes: a class of another level's program placed in this level by OpenRAC
 * Editor (editor/foreign_classes.py packs it). Its data is in this level's core like the level's
 * own classes; its update is its own level's code, which reads that level's addresses, so it runs
 * in its own program for the call: that level's data is swapped in over this one's, and its code
 * is read from a copy (openrac_guest_foreign_enter, openrac/foreign.h). Every level's data ends
 * below every level's program end, so a class of any level runs in any other.
 *
 * The pack lists the classes in <mods>/level_NN/classes.tsv, one "class<TAB>level" a line.
 * assign_moby_class_data (moby_class.c) binds such a class to a stand-in address, which the moby
 * update loop calls as any update: the stand-in enters the class's program, runs its update and
 * leaves. A level with no such file runs exactly as before.
 *
 * Its own level's tables come with it (<mods>/level_NN/worlds.bin, from the pack): the zones,
 * paths, rails and cameras its pvars name by number, moved to where each moby of it stands. For
 * its update they take the place of this level's (openrac_guest_foreign_set), so a guard that
 * waits for the hero in its own zone waits in that zone, moved here.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "openrac/foreign.h"
#include "openrac/guest.h"
#include "rac1.h"

#define FOREIGN_MAX 48
#define LEVELS 19
/* Stand-in code addresses: past the game's 32 MiB, where no program has code. */
#define FOREIGN_TAG_BASE 0x0FF00000u
/* The imported classes' data: the game adds a class's address to its offsets as a signed word, and
 * its last material run's flag only ends the list when that address is below 0x80000000. */
#define FOREIGN_BLOBS GUEST_PORT_LOW
#define FOREIGN_BLOBS_SIZE GUEST_PORT_LOW_SIZE
/* The engine's 224 class slots less two: once the level's classes and the imports are in, the
 * texture bank registers the hero's two armor classes (load_level_texture_bank), unchecked. */
#define MOBY_CLASS_SLOTS (224u - 2u)

typedef struct {
    int oclass;
    int level;    /* the program its code is in */
    gaddr update; /* in that program; 0 until bound */
    int ran;
} Foreign;

static Foreign s_foreign[FOREIGN_MAX];
static int s_foreign_count;
static int s_registered;
/* The imported class being registered (-1: none): only it binds to a stand-in, never a class of
 * the level's own that has no update. */
static int s_registering = -1;

/* worlds.bin (editor/foreign_classes.py, write_worlds): "ORWD", 1, worlds, bindings; each world
 * {at, size, relocs, sets}; each binding {class, world, x, y}; at a world's `at` its relocs
 * (offsets in its data of words its address is added to), its sets {global, index, value, value
 * is an offset}, its data. The globals, as games/rac1 names them in level 0's program. */
enum {
    WORLD_GLOBALS = 15,
    WORLD_MAX = 64,
    WORLD_SETS_MAX = 16384,
    BINDING_MAX = 1024,
    HOST_SAVED_MAX = 1024,
    WORLD_LEAD = 0x10000, /* before the first world: 512 records of 0x80 */
};

static const gaddr k_world_globals[WORLD_GLOBALS] = {
    RAC1_LADDR(00, 0x0015F050u), /* camera_descriptors */
    RAC1_LADDR(00, 0x0015F054u), /* gCameraDefCountLvl */
    RAC1_LADDR(00, 0x0015F7ECu), /* level_rails */
    RAC1_LADDR(00, 0x0015F7F0u), /* level_rail_count */
    RAC1_LADDR(00, 0x001601A4u), /* level_clip_volumes */
    RAC1_LADDR(00, 0x001601A8u), /* level_clip_volume_count */
    RAC1_LADDR(00, 0x001601ACu), /* level_cuboids */
    RAC1_LADDR(00, 0x001601B0u), /* level_cuboid_count */
    RAC1_LADDR(00, 0x001601B4u), /* level_spheres */
    RAC1_LADDR(00, 0x001601B8u), /* level_sphere_count */
    RAC1_LADDR(00, 0x001601BCu), /* level_cylinders */
    RAC1_LADDR(00, 0x001601C0u), /* level_cylinder_count */
    RAC1_LADDR(00, 0x001601C4u), /* level_path_count */
    RAC1_LADDR(00, 0x001601C8u), /* level_path_data */
    RAC1_LADDR(00, 0x001B0830u), /* toad_paths[] */
};

typedef struct {
    uint32_t global, index, value;
} WorldSet;

typedef struct {
    int first, count; /* its sets in s_world_sets */
} World;

typedef struct {
    int oclass, world;
    float x, y;
    gaddr moby; /* the moby packed with it (0: none found) */
} Binding;

static World s_worlds[WORLD_MAX];
static int s_world_count;
static WorldSet s_world_sets[WORLD_SETS_MAX];
static int s_world_set_count;
static Binding s_bindings[BINDING_MAX];
static int s_binding_count;
/* This level's words under a world while a moby of its own class runs in it. */
static uint32_t s_host_saved[HOST_SAVED_MAX];

#define MOBY_POS 0x10u
#define MOBY_OCLASS 0xA6u
#define MOBY_SIZE 0x100u

/* The world of a moby of class `oclass`: the one it was packed with, else the nearest one of
 * the mobys of its class packed with one (a moby one of them made), else -1 (this level's). */
static int world_of(int oclass, gaddr moby) {
    int best = -1;
    float best_d = 0.0f;
    const float x = GREF(float, moby + MOBY_POS), y = GREF(float, moby + MOBY_POS + 4u);
    for (int i = 0; i < s_binding_count; ++i) {
        const Binding* b = &s_bindings[i];
        if (b->oclass != oclass) {
            continue;
        }
        if (b->moby == moby) {
            return b->world;
        }
        const float dx = b->x - x, dy = b->y - y, d = dx * dx + dy * dy;
        if (best < 0 || d < best_d) {
            best = b->world;
            best_d = d;
        }
    }
    return best;
}

static void world_enter(int w) {
    const World* world = &s_worlds[w];

    for (int i = world->first; i < world->first + world->count; ++i) {
        const WorldSet* set = &s_world_sets[i];
        const gaddr at = OPENRAC_LDATA(0, k_world_globals[set->global]) + set->index * 4u;
        openrac_guest_foreign_set(at, set->value);
    }
}

/* A moby of a class this level has, which an imported moby brought along (its lift, its
 * hatch): its world in this level's globals for its update (crash_state.c's loop), put back
 * after. Returns what to hand rac1_foreign_host_world_leave (-1: none). */
int rac1_foreign_host_world_enter(gaddr moby) {
    if (s_binding_count == 0) {
        return -1;
    }
    const int oclass = GREF(int16_t, moby + MOBY_OCLASS);
    for (int k = 0; k < s_foreign_count; ++k) {
        if (s_foreign[k].oclass == oclass && s_foreign[k].update != 0) {
            return -1; /* an imported class: its stand-in enters its world (foreign_run) */
        }
    }
    int w = -1;
    for (int i = 0; i < s_binding_count; ++i) {
        if (s_bindings[i].moby == moby && s_bindings[i].oclass == oclass) {
            w = s_bindings[i].world;
            break;
        }
    }
    if (w < 0 || s_worlds[w].count > HOST_SAVED_MAX) {
        return -1;
    }
    const World* world = &s_worlds[w];
    for (int i = 0; i < world->count; ++i) {
        const WorldSet* set = &s_world_sets[world->first + i];
        const gaddr at = OPENRAC_LDATA(0, k_world_globals[set->global]) + set->index * 4u;
        s_host_saved[i] = GREF(uint32_t, at);
        GREF(uint32_t, at) = set->value;
    }
    return w;
}

void rac1_foreign_host_world_leave(int w) {
    if (w < 0) {
        return;
    }
    const World* world = &s_worlds[w];
    for (int i = world->count - 1; i >= 0; --i) {
        const WorldSet* set = &s_world_sets[world->first + i];
        const gaddr at = OPENRAC_LDATA(0, k_world_globals[set->global]) + set->index * 4u;
        GREF(uint32_t, at) = s_host_saved[i];
    }
}

/* Inside an imported class's update (foreign_run): InitMobyInstance's class check below. */
static int s_foreign_depth;

void func_0020D440__game(gaddr moby, int oclass);

/* InitMobyInstance (libraries.tsv, wrap). An imported class's update can make a class this level
 * does not have (one its code takes from a table, as Qwark's starfighter takes its parts): its
 * slot is 0xFF, past the class tables' 224 entries. So is an imported class the level had no slot
 * left for, wherever its mobys are made. The game's own path for a class with no data (no update,
 * no record: a dead moby) is taken instead, by those two entries reading 0 for the call. On a
 * level with no imported classes every call is the game's, unchanged. */
void func_0020D440(gaddr moby, int oclass) {
    const gaddr slots = OPENRAC_DATA(RAC1_ADDR(0x001B3E40u));
    if ((s_foreign_depth == 0 && s_foreign_count == 0) || oclass < 0 || oclass >= 0x800
        || GREF(uint8_t, slots + (gaddr)oclass) != 0xFF) {
        func_0020D440__game(moby, oclass);
        return;
    }
    const gaddr record = OPENRAC_DATA(RAC1_ADDR(0x001B3580u)) + 0xFFu * 4u;
    const gaddr update = OPENRAC_DATA(RAC1_ADDR(0x001B3900u)) + 0xFFu * 4u;
    const uint32_t was_record = GREF(uint32_t, record), was_update = GREF(uint32_t, update);
    GREF(uint32_t, record) = 0;
    GREF(uint32_t, update) = 0;
    func_0020D440__game(moby, oclass);
    GREF(uint32_t, record) = was_record;
    GREF(uint32_t, update) = was_update;
}

static void foreign_run(int k, gaddr moby) {
    Foreign* f = &s_foreign[k];
    const int loaded = openrac_game_loaded_overlay();
    if (f->update == 0 || loaded < 0) {
        return;
    }
    if (openrac_guest_foreign_enter(f->level, loaded) != 0) {
        if (!f->ran) {
            f->ran = 1;
            char message[200];
            snprintf(
                message,
                sizeof(message),
                "imported moby class %d does not run: level %d's program is missing or bigger "
                "than this level's",
                f->oclass,
                f->level
            );
            openrac_game_warn(message);
        }
        return;
    }
    if (!f->ran) {
        f->ran = 1;
        char message[160];
        snprintf(
            message,
            sizeof(message),
            "imported moby class %d runs (level %d's update %08x)",
            f->oclass,
            f->level,
            f->update
        );
        openrac_game_warn(message);
    }
    openrac_game_restore_overlay(f->level);
    const int world = world_of(f->oclass, moby);
    if (world >= 0) {
        world_enter(world);
    }
    ++s_foreign_depth;
    openrac_guest_call_a0(f->update, moby);
    --s_foreign_depth;
    openrac_guest_foreign_leave();
    openrac_game_restore_overlay(loaded);
}

#define FOREIGN_STAND_IN(k)                                                                        \
    static void foreign_update_##k(gaddr moby) {                                                   \
        foreign_run(k, moby);                                                                      \
    }
FOREIGN_STAND_IN(0)
FOREIGN_STAND_IN(1)
FOREIGN_STAND_IN(2)
FOREIGN_STAND_IN(3)
FOREIGN_STAND_IN(4)
FOREIGN_STAND_IN(5)
FOREIGN_STAND_IN(6)
FOREIGN_STAND_IN(7)
FOREIGN_STAND_IN(8)
FOREIGN_STAND_IN(9)
FOREIGN_STAND_IN(10)
FOREIGN_STAND_IN(11)
FOREIGN_STAND_IN(12)
FOREIGN_STAND_IN(13)
FOREIGN_STAND_IN(14)
FOREIGN_STAND_IN(15)
FOREIGN_STAND_IN(16)
FOREIGN_STAND_IN(17)
FOREIGN_STAND_IN(18)
FOREIGN_STAND_IN(19)
FOREIGN_STAND_IN(20)
FOREIGN_STAND_IN(21)
FOREIGN_STAND_IN(22)
FOREIGN_STAND_IN(23)
FOREIGN_STAND_IN(24)
FOREIGN_STAND_IN(25)
FOREIGN_STAND_IN(26)
FOREIGN_STAND_IN(27)
FOREIGN_STAND_IN(28)
FOREIGN_STAND_IN(29)
FOREIGN_STAND_IN(30)
FOREIGN_STAND_IN(31)
FOREIGN_STAND_IN(32)
FOREIGN_STAND_IN(33)
FOREIGN_STAND_IN(34)
FOREIGN_STAND_IN(35)
FOREIGN_STAND_IN(36)
FOREIGN_STAND_IN(37)
FOREIGN_STAND_IN(38)
FOREIGN_STAND_IN(39)
FOREIGN_STAND_IN(40)
FOREIGN_STAND_IN(41)
FOREIGN_STAND_IN(42)
FOREIGN_STAND_IN(43)
FOREIGN_STAND_IN(44)
FOREIGN_STAND_IN(45)
FOREIGN_STAND_IN(46)
FOREIGN_STAND_IN(47)

static void register_stand_ins(void) {
    static void (*const fns[FOREIGN_MAX])(gaddr) = {
        foreign_update_0,  foreign_update_1,  foreign_update_2,  foreign_update_3,
        foreign_update_4,  foreign_update_5,  foreign_update_6,  foreign_update_7,
        foreign_update_8,  foreign_update_9,  foreign_update_10, foreign_update_11,
        foreign_update_12, foreign_update_13, foreign_update_14, foreign_update_15,
        foreign_update_16, foreign_update_17, foreign_update_18, foreign_update_19,
        foreign_update_20, foreign_update_21, foreign_update_22, foreign_update_23,
        foreign_update_24, foreign_update_25, foreign_update_26, foreign_update_27,
        foreign_update_28, foreign_update_29, foreign_update_30, foreign_update_31,
        foreign_update_32, foreign_update_33, foreign_update_34, foreign_update_35,
        foreign_update_36, foreign_update_37, foreign_update_38, foreign_update_39,
        foreign_update_40, foreign_update_41, foreign_update_42, foreign_update_43,
        foreign_update_44, foreign_update_45, foreign_update_46, foreign_update_47,
    };
    openrac_fn_entry entries[FOREIGN_MAX];
    for (int k = 0; k < FOREIGN_MAX; ++k) {
        entries[k].address = FOREIGN_TAG_BASE + (gaddr)k * 16u;
        entries[k].fn = (openrac_host_fn)fns[k];
        entries[k].name = "foreign_class_update";
    }
    openrac_guest_register(OPENRAC_OVERLAY_EXE, entries, FOREIGN_MAX);
    s_registered = 1;
}

void rac1_foreign_classes_load(int level) {
    s_foreign_count = 0;
    s_world_count = 0;
    s_world_set_count = 0;
    s_binding_count = 0;
    openrac_guest_forget_foreign();
#ifndef RAC1_FOREIGN_CLASSES
    (void)level;
    return; /* a release with no wrap of the loader */
#endif
    if (level < 0 || openrac_game_mods_dir == NULL || openrac_game_mods_dir[0] == '\0') {
        return;
    }
    char path[1024];
    snprintf(path, sizeof(path), "%s/level_%02d/classes.tsv", openrac_game_mods_dir, level);
    FILE* file = fopen(path, "r");
    if (file == NULL) {
        return;
    }
    char line[128];
    int skipped = 0;
    while (fgets(line, sizeof(line), file) != NULL) {
        int oclass = -1, from = -1;
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
            continue;
        }
        int known = 0;
        if (sscanf(line, "%d\t%d", &oclass, &from) == 2 && oclass >= 0 && oclass < 0x800
            && from >= 0 && from < LEVELS && from != level) {
            for (int k = 0; k < s_foreign_count; ++k) {
                known |= s_foreign[k].oclass == oclass;
            }
        } else {
            known = 1;
        }
        if (known || s_foreign_count == FOREIGN_MAX) {
            ++skipped; /* a bad or repeated line, or one too many */
            continue;
        }
        s_foreign[s_foreign_count++] = (Foreign){oclass, from, 0, 0};
    }
    fclose(file);
    if (skipped > 0) {
        char message[200];
        snprintf(
            message,
            sizeof(message),
            "level %d: %d line(s) of classes.tsv left out (a bad or repeated class, its own level, "
            "or more than %d classes)",
            level,
            skipped,
            FOREIGN_MAX
        );
        openrac_game_warn(message);
    }
    if (s_foreign_count > 0 && !s_registered) {
        register_stand_ins();
    }
    if (s_foreign_count > 0) {
        /* The engine's tables a foreign update reads and writes past the first words of: the moby
         * grid and its blocks' bitmap (a foreign moby's move or deletion), and the class tables
         * (CreateMoby of a class: its slot, record and update are the loaded level's). */
        static const struct {
            uint32_t address, size;
        } k_whole[] = {
            {RAC1_ADDR(0x001B7A60u), 0x100C0u}, /* moby_grids, then the blocks' bitmap */
            {RAC1_ADDR(0x001B3580u), 0x380u},   /* moby_class_resources */
            {RAC1_ADDR(0x001B3900u), 0x380u},   /* moby_class_updates */
            {RAC1_ADDR(0x001B3C80u), 0x1C0u},   /* resident_class_id_by_slot */
            {RAC1_ADDR(0x001B3E40u), 0x800u},   /* resident_class_slot_by_id */
        };

        /* The level's moby groups, in level 0's program: an imported moby's groups are past the
         * first 64 (pack.py), which a run of carried globals does not reach. */
        openrac_guest_foreign_carry_whole(0, RAC1_LADDR(00, 0x001ABBC0u), 0x1C0u);

        for (size_t i = 0; i < sizeof(k_whole) / sizeof(k_whole[0]); ++i) {
            openrac_guest_foreign_carry_whole(
                OPENRAC_OVERLAY_EXE, k_whole[i].address, k_whole[i].size
            );
        }
    }
    char message[160];
    snprintf(
        message,
        sizeof(message),
        "level %d: %d moby class(es) imported from other levels",
        level,
        s_foreign_count
    );
    openrac_game_warn(message);
}

#ifdef RAC1_FOREIGN_CLASSES
/* print_moby_class_sizes, which the level's loader calls once it has registered its own classes:
 * the imported ones register after them, each from its blob <mods>/level_NN/classes/<class>.bin
 * (its data as its own level's core has it, editor/foreign_classes.py). Drawn without the level's
 * textures (the port draws its level's model); its update binds through assign_moby_class_data as any class. */
void func_L00_00242120(unsigned int record, int count) {
    func_L00_00242120__game(record, count);
    const int level = openrac_game_loaded_overlay();
    if (s_foreign_count == 0 || level < 0) {
        return;
    }
    static gaddr slots;
    if (slots == 0) {
        slots = openrac_guest_static(16);
        memset(G(slots), 0, 16);
    }
    const gaddr slot_count = OPENRAC_DATA(RAC1_ADDR(0x00160000u));
    const gaddr class_by_slot = OPENRAC_DATA(RAC1_ADDR(0x001B3C80u));
    const gaddr register_class = OPENRAC_CODE(RAC1_ADDR(0x00203E78u));
    gaddr at = FOREIGN_BLOBS;
    int registered = 0;
    for (int k = 0; k < s_foreign_count; ++k) {
        const Foreign* f = &s_foreign[k];
        const uint32_t used = GREF(uint32_t, slot_count);
        if (used >= MOBY_CLASS_SLOTS) {
            char message[400];
            int n = snprintf(message, sizeof(message), "no moby class slot left: not imported");
            for (int left = k; left < s_foreign_count && n > 0 && n < (int)sizeof(message) - 8;
                 ++left) {
                n += snprintf(
                    message + n, sizeof(message) - (size_t)n, " %d", s_foreign[left].oclass
                );
            }
            openrac_game_warn(message);
            break;
        }
        int own = 0;
        for (uint32_t slot = 0; slot < used; ++slot) {
            own |= GREF(int16_t, class_by_slot + slot * 2u) == f->oclass;
        }
        if (own) {
            char message[160];
            snprintf(
                message,
                sizeof(message),
                "moby class %d is this level's own: it runs as the level's, not imported",
                f->oclass
            );
            openrac_game_warn(message);
            continue;
        }
        char path[1024];
        snprintf(
            path,
            sizeof(path),
            "%s/level_%02d/classes/%d.bin",
            openrac_game_mods_dir,
            level,
            f->oclass
        );
        FILE* file = fopen(path, "rb");
        if (file == NULL) {
            char message[1100];
            snprintf(message, sizeof(message), "imported moby class %d: no %s", f->oclass, path);
            openrac_game_warn(message);
            continue;
        }
        fseek(file, 0, SEEK_END);
        const long size = ftell(file);
        fseek(file, 0, SEEK_SET);
        if (size < 0x50 || (gaddr)size > FOREIGN_BLOBS + FOREIGN_BLOBS_SIZE - at) {
            fclose(file);
            char message[1100];
            snprintf(
                message,
                sizeof(message),
                "imported moby class %d: %s is %s",
                f->oclass,
                path,
                size < 0x50 ? "too small to be a class" : "too big for the room left"
            );
            openrac_game_warn(message);
            continue;
        }
        const size_t read = fread(G(at), 1, (size_t)size, file);
        fclose(file);
        if (read != (size_t)size) {
            openrac_game_warn("an imported moby class could not be read");
            continue;
        }
        s_registering = k;
        GFN(void (*)(gaddr, gaddr, gaddr, int), register_class)(at, 0, slots, f->oclass);
        s_registering = -1;
        at = (at + (gaddr)size + 15u) & ~15u;
        ++registered;
    }
    char message[160];
    snprintf(
        message,
        sizeof(message),
        "level %d: %d imported moby class(es) registered",
        level,
        registered
    );
    openrac_game_warn(message);
}
#endif

/* The stand-in update of an imported class (0: not one), its update and callbacks found in its own
 * level's class table (as moby_class.c reads the loaded level's). */
gaddr rac1_foreign_class_update(int oclass) {
    for (int k = 0; k < s_foreign_count; ++k) {
        Foreign* f = &s_foreign[k];
        if (f->oclass != oclass || k != s_registering) {
            continue;
        }
        const gaddr vtbl = RAC1_MOBY_VTBL(f->level);
        for (uint32_t i = 0; vtbl != 0 && i < 196u; ++i) {
            const int32_t key = (int32_t)openrac_guest_level_word(f->level, vtbl + i * 12u);
            if (key == -1) {
                break;
            }
            if (key == oclass) {
                f->update = openrac_guest_level_word(f->level, vtbl + i * 12u + 4u);
                break;
            }
        }
        if (f->update == 0) {
            char message[160];
            snprintf(
                message,
                sizeof(message),
                "moby class %d has no update in level %d's program",
                oclass,
                f->level
            );
            openrac_game_warn(message);
            return 0;
        }
        return FOREIGN_TAG_BASE + (gaddr)k * 16u;
    }
    return 0;
}

static uint32_t read_u32(const unsigned char* p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static float read_f32(const unsigned char* p) {
    const uint32_t w = read_u32(p);
    float f;
    memcpy(&f, &w, 4);
    return f;
}

/* After the level's loader (moby_pool.c), at each load and respawn: the imported mobys' tables
 * of their own levels (worlds.bin) into GUEST_PORT_WORLDS, each moby packed with one bound to it
 * (its class at its x and y). */
void rac1_foreign_worlds_load(void) {
    s_world_count = 0;
    s_world_set_count = 0;
    s_binding_count = 0;
    const int level = openrac_game_loaded_overlay();
    if (s_foreign_count == 0 || level < 0) {
        return;
    }
    char path[1024];
    snprintf(path, sizeof(path), "%s/level_%02d/worlds.bin", openrac_game_mods_dir, level);
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        return;
    }
    fseek(file, 0, SEEK_END);
    const long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    unsigned char* bytes = size >= 16 ? (unsigned char*)malloc((size_t)size) : NULL;
    const int read = bytes != NULL && fread(bytes, 1, (size_t)size, file) == (size_t)size;
    fclose(file);
    const uint32_t worlds = read ? read_u32(bytes + 8) : 0,
                   bindings = read ? read_u32(bytes + 12) : 0;
    if (!read || memcmp(bytes, "ORWD", 4) != 0 || read_u32(bytes + 4) != 1u || worlds > WORLD_MAX
        || bindings > BINDING_MAX || 16u + (uint64_t)(worlds + bindings) * 16u > (uint64_t)size) {
        free(bytes);
        openrac_game_warn("imported moby classes: worlds.bin is not one this port reads");
        return;
    }
    /* The worlds start past a cleared lead: a table read at index -1 (Fred's sentry with no
     * cuboid reads the one before the table, as on the console) reads zeros, not unmapped
     * memory before the region. */
    memset(G(GUEST_PORT_WORLDS), 0, WORLD_LEAD);
    gaddr at = GUEST_PORT_WORLDS + WORLD_LEAD;
    int bad = 0;
    for (uint32_t w = 0; w < worlds; ++w) {
        const unsigned char* head = bytes + 16u + w * 16u;
        const uint32_t from = read_u32(head), data_size = read_u32(head + 4);
        const uint32_t relocs = read_u32(head + 8), sets = read_u32(head + 12);
        const uint64_t end =
            (uint64_t)from + (uint64_t)relocs * 4u + (uint64_t)sets * 16u + data_size;
        World* world = &s_worlds[s_world_count++];
        world->first = s_world_set_count;
        world->count = 0;
        if (end > (uint64_t)size || data_size > GUEST_PORT_WORLDS + GUEST_PORT_WORLDS_SIZE - at
            || s_world_set_count + (uint64_t)sets > WORLD_SETS_MAX) {
            ++bad; /* left with no sets: its mobys run with this level's tables */
            continue;
        }
        const unsigned char* reloc = bytes + from;
        const unsigned char* set = reloc + relocs * 4u;
        memcpy(G(at), set + sets * 16u, data_size);
        int fits = 1;
        for (uint32_t r = 0; r < relocs && fits; ++r) {
            const uint32_t where = read_u32(reloc + r * 4u);
            fits = data_size >= 4u && where <= data_size - 4u;
            if (fits) {
                GREF(uint32_t, at + where) += at;
            }
        }
        if (!fits) {
            ++bad;
            continue;
        }
        for (uint32_t i = 0; i < sets; ++i) {
            const unsigned char* one = set + i * 16u;
            const uint32_t global = read_u32(one);
            if (global >= WORLD_GLOBALS) {
                continue;
            }
            WorldSet* out = &s_world_sets[s_world_set_count++];
            out->global = global;
            out->index = read_u32(one + 4);
            out->value = read_u32(one + 8) + (read_u32(one + 12) != 0 ? at : 0u);
            ++world->count;
        }
        at = (at + data_size + 15u) & ~15u;
    }
    const gaddr mobys = GREF(gaddr, OPENRAC_LDATA(0, RAC1_LADDR(00, 0x00160098u)));
    const gaddr mobys_end = GREF(gaddr, OPENRAC_LDATA(0, RAC1_LADDR(00, 0x00160098u)) + 4u);
    int unbound = 0;
    for (uint32_t i = 0; i < bindings; ++i) {
        const unsigned char* one = bytes + 16u + worlds * 16u + i * 16u;
        Binding* b = &s_bindings[s_binding_count];
        b->oclass = (int)read_u32(one);
        b->world = (int)read_u32(one + 4);
        b->x = read_f32(one + 8);
        b->y = read_f32(one + 12);
        b->moby = 0;
        if (b->world < 0 || b->world >= s_world_count) {
            ++bad;
            continue;
        }
        for (gaddr m = mobys; m != 0 && m < mobys_end; m += MOBY_SIZE) {
            if (GREF(int16_t, m + MOBY_OCLASS) == b->oclass && GREF(float, m + MOBY_POS) == b->x
                && GREF(float, m + MOBY_POS + 4u) == b->y) {
                b->moby = m;
                break;
            }
        }
        unbound += b->moby == 0;
        ++s_binding_count;
    }
    free(bytes);
    char message[220];
    snprintf(
        message,
        sizeof(message),
        "level %d: %d imported moby world(s) of their own levels, %u KB, %d moby(s) bound (%d not "
        "found, %d left out)",
        level,
        s_world_count,
        (unsigned)((at - GUEST_PORT_WORLDS - WORLD_LEAD) >> 10),
        s_binding_count - unbound,
        unbound,
        bad
    );
    openrac_game_warn(message);
}
