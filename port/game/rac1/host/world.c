/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Ratchet & Clank: the level's mobys for the mods' world calls (game_host.h, openrac_game_play;
 * interface 1.9: a multiplayer mod shares the world, one game running it and the others showing
 * it). play.c puts these in its table.
 *
 * Slots and keys. A moby's slot is its index in the level's moby array (D_00160018, the level's
 * copy). A placed moby's key is its record in the level's gameplay file: LoadLevelGameplay skips
 * the records of killed or collected mobys and packs the rest, keeping each record's slot in
 * moby_index_by_file_order (D_L00_001ACB00, an s16 a record, -1 for one skipped), so the same
 * record has the same key on every machine whatever was killed. The placed mobys are the slots
 * below level_mobys_end (D_0016001C); the rest are spawned (CreateMoby) and have no key. The keys
 * are read once a moby array (boot.c's generation): a respawn rebuilds it, and the slots move.
 *
 * The gate. UpdateMobys (crash_state.c) asks rac1_moby_gate before and after each moby's update;
 * the mods' loader answers (openrac_game_moby_update). Before an update, a mod may have the hero
 * stand in elsewhere for it (stand_in): his position (Hero.motion.pos at +0x80, the ground point
 * at +0xC0 and the body's centre at +0xD0 moved by the same step) and his moby (+0x2080) are
 * another player's for that update, and put back after it, with the bolts it gave (gBolts): an
 * enemy then chases, aims at and hits that player's figure, and a bolt flies to it.
 *
 * Hits. The game posts a hit to a moby in a ring of 64 records (D_L00_00178200, 0x40 bytes: the
 * position, the velocity, the attacker at +0x20, the mask at +0x24, the damage at +0x2C, the victim
 * at +0x34), the victim keeping the record's index at +0xA4 (0xFF none), through PostHitMessage
 * (func_L00_0025AC00) and record_moby_hit (func_L00_0025AAC0), which hand over here (libraries.tsv,
 * wrap). A record written is told to the mods (openrac_game_moby_hit), which may take it back:
 * the victim's index cleared, so MobyGetHitMessage finds nothing. post_hit posts one through the
 * game's own PostHitMessage, unwatched.
 *
 * Eyes. The tick's run list (moby_pool.c) runs a moby near the camera (its update distance at
 * +0x30); the eyes set here count as the camera too, so the mobys near another player run in the
 * game that runs the world.
 */
#include <math.h>
#include <string.h>

#include "rac1.h"

/* The moby array and the end of its placed part (each level's copy), the file-order table. */
#define MOBY_ARRAY RAC1_ADDR(0x00160018u)
#define MOBY_PLACED_END RAC1_ADDR(0x0016001Cu)
#define FILE_ORDER OPENRAC_LDATA(0, RAC1_LADDR(00, 0x001ACB00u))
/* The moby context (select_moby_context, func_L00_00235FF8): 0 the level's mobys, 1 the pause
 * menu's own, whose array then sits in the level's pointers while the level's wait in the alt_
 * ones. */
#define CONTEXT OPENRAC_LDATA(0, RAC1_LADDR(00, 0x0015F808u))
#define ALT_ARRAY OPENRAC_LDATA(0, RAC1_LADDR(00, 0x0015FB28u))
#define ALT_PLACED_END OPENRAC_LDATA(0, RAC1_LADDR(00, 0x0015FB2Cu))
/* The hit ring. */
#define HIT_RING OPENRAC_LDATA(0, RAC1_LADDR(00, 0x00178200u))
/* The hero (play.c): his motion's position, ground point and body centre, his moby; the bolts. */
#define HERO RAC1_ADDR(0x0013F450u)
#define HERO_MOBY (HERO + 0x2080u)
#define BOLTS RAC1_ADDR(0x0015EE98u)
/* The camera's position (the renderer's, render_native.c). */
#define CAMERA_POSITION RAC1_ADDR(0x00187180u)

enum {
    MOBY_POS = 0x10,
    MOBY_STATE = 0x20,
    MOBY_ALPHA = 0x23,
    MOBY_CLASS = 0x24,
    MOBY_SCALE = 0x2C,
    MOBY_DIST = 0x30,
    MOBY_DRAWN = 0x31,
    MOBY_DRAW_DIST = 0x32,
    MOBY_MODE = 0x34,
    MOBY_OCCLUSION = 0x36,
    MOBY_ROT = 0x40,
    MOBY_FRAME = 0x50,
    MOBY_NEXT_FRAME = 0x51,
    MOBY_SEQ = 0x52,
    MOBY_TARGET_SEQ = 0x53,
    MOBY_ANIM_TIME = 0x54,
    MOBY_ANIM_SPEED = 0x58,
    MOBY_UPDATE = 0x74,
    MOBY_HIT = 0xA4,
    MOBY_OCLASS = 0xA6,
    MOBY_SIZE = 0x100,
    CLASS_TYPE = 0x46,
    HIT_SIZE = 0x40,
    HIT_ATTACKER = 0x20,
    HIT_MASK = 0x24,
    HIT_DAMAGE = 0x2C,
    HIT_VICTIM = 0x34
};

/* A class whose type is a creature (enemies, people: moby.h). */
#define TYPE_CREATURE 5
/* openrac_moby's flags (openrac_mod.h). */
#define FLAG_UPDATES 0x1u
#define FLAG_PLACED 0x4u
#define FLAG_CREATURE 0x8u
#define FLAG_DEAD 0x10u
#define FLAG_SCENE 0x20u
#define FLAG_HIDDEN 0x40u
#define FLAG_DRAWN 0x80u
/* The mode bits that keep a moby from being drawn (MobyProc: render_native.c). */
#define MODE_HIDDEN 0x81u
/* A placed moby deleted (DeleteMoby): its slot is never used again in the level load. */
#define STATE_DELETED 0xFDu
/* The most records a level's file has that the keys follow, and eyes. */
#define KEYS_MAX 32768
#define EYES_MAX 8

static int16_t key_of[KEYS_MAX];  /* by slot: its key, -1 none */
static int16_t slot_of[KEYS_MAX]; /* by key: its slot, -1 none */
static uint32_t keys_load = 0xFFFFFFFFu;
static uint32_t array_load = 0xFFFFFFFFu;
static gaddr array_limit_base;
static int32_t array_limit_slots;

/* The last deletion seen of each slot (DeleteMoby, below): whose update did it, or a mod. */
static struct {
    int32_t by;
    int32_t by_class;
    int16_t oclass;
    uint8_t by_mod;
    uint8_t seen;
    uint32_t loads;
} deletions[KEYS_MAX];

static int killing; /* rac1_world_moby_kill's DeleteMoby under way */

static float eyes_xyz[EYES_MAX * 3];
static int eyes_count;

/* The stand-in: whether one is on, and what it put aside. */
static struct {
    int on;
    uint8_t motion[3][16];
    gaddr moby;
} stand;

static int in_level(void) {
    return rac1_level_running();
}

/* Whether the level's mobys are the game's current ones (not the pause menu's). */
static int main_context(void) {
    return GREF(int, CONTEXT) == 0;
}

/* The level's moby array and the end of its placed part, whichever context is up. */
static gaddr array_base(void) {
    return main_context() ? GREF(gaddr, OPENRAC_DATA(MOBY_ARRAY)) : GREF(gaddr, ALT_ARRAY);
}

static int32_t placed_count(void) {
    const gaddr base = array_base();
    const gaddr end =
        main_context() ? GREF(gaddr, OPENRAC_DATA(MOBY_PLACED_END)) : GREF(gaddr, ALT_PLACED_END);
    return base != 0 && end > base ? (int32_t)((end - base) / MOBY_SIZE) : 0;
}

/* Cache the active context's actual moby-array length. The end marker is a state byte of 0xFF;
 * keeping this bound prevents host APIs from treating memory after a short level/pause array as
 * additional mobys when expanded mode raises the public slot ceiling. */
static int32_t array_slots(void) {
    /* The console's arena: the fixed 4096 slots as before, the mobys the game spawns past the end
     * marker included. */
    if (openrac_guest_ram_size <= GUEST_RAM_SIZE) {
        return 4096;
    }
    const gaddr base = array_base();
    const uint32_t load = rac1_moby_generation();
    if (base != array_limit_base || load != array_load) {
        array_limit_base = base;
        array_load = load;
        array_limit_slots = 0;
        if (base != 0) {
            while (array_limit_slots < KEYS_MAX
                   && GREF(uint8_t, base + (gaddr)array_limit_slots * MOBY_SIZE + MOBY_STATE)
                          != 0xFF) {
                ++array_limit_slots;
            }
        }
    }
    return array_limit_slots;
}

/* The game's moby calls (CreateMoby, DeleteMoby) work on the current context: the level's is put
 * up for them while the pause menu's is (and put back after). */
static int enter_level_context(void) {
    if (main_context()) {
        return 0;
    }
    func_L00_00235FF8(0);
    return 1;
}

static void leave_level_context(int entered) {
    if (entered) {
        func_L00_00235FF8(1);
    }
}

/* The slots' keys for the level loaded now, read once a load. */
static void read_keys(void) {
    if (keys_load == rac1_moby_generation() || array_base() == 0) {
        return;
    }
    keys_load = rac1_moby_generation();
    memset(key_of, 0xFF, sizeof key_of);
    memset(slot_of, 0xFF, sizeof slot_of);
    const int32_t placed = placed_count();
    int32_t found = 0;
    /* Every placed moby has its record; the table runs on past the last for none. */
    for (int32_t record = 0; record < KEYS_MAX && found < placed; ++record) {
        const int16_t slot = GREF(int16_t, FILE_ORDER + 2u * (unsigned)record);
        if (slot >= 0 && slot < placed) {
            key_of[slot] = (int16_t)record;
            slot_of[record] = slot;
            ++found;
        }
    }
}

int32_t rac1_moby_slot(gaddr moby) {
    const gaddr base = array_base();
    if (moby < base || base == 0 || (moby - base) % MOBY_SIZE != 0) {
        return -1;
    }
    const int32_t slot = (int32_t)((moby - base) / MOBY_SIZE);
    return slot < array_slots() ? slot : -1;
}

gaddr rac1_world_moby_at(int32_t slot) {
    if (!in_level() || slot < 0 || slot >= array_slots()) {
        return 0;
    }
    const gaddr base = array_base();
    const gaddr m = base + (gaddr)slot * MOBY_SIZE;
    /* Gone or free. */
    if (GREF(uint8_t, m + MOBY_STATE) >= 0x80) {
        return 0;
    }
    /* Past the array's end, which is the first 0xFF: a respawn leaves the spawned mobys of before
     * there as they were, looking alive, their grid cells cleared under them (DeleteMoby traps). */
    for (int32_t s = placed_count(); s < slot; ++s) {
        if (GREF(uint8_t, base + (gaddr)s * MOBY_SIZE + MOBY_STATE) == 0xFF) {
            return 0;
        }
    }
    return m;
}

int rac1_world_mobys(openrac_game_moby_state* out, int max) {
    if (!in_level()) {
        return 0;
    }
    read_keys();
    const gaddr base = array_base();
    const int32_t placed = placed_count();
    const int32_t slots = array_slots();
    int n = 0;
    for (int32_t slot = 0; slot < slots; ++slot) {
        const gaddr m = base + (gaddr)slot * MOBY_SIZE;
        const uint8_t state = GREF(uint8_t, m + MOBY_STATE);
        /* 0xFF ends the array; other states past 0x80 are free or dead slots, of which a placed
         * one deleted is listed (dead: the mods learn of a kill from it, never from an absence). */
        if (state == 0xFF) {
            break;
        }
        if (state >= 0x80 && !(state == STATE_DELETED && slot < placed)) {
            continue;
        }
        if (n < max) {
            openrac_game_moby_state* s = &out[n];
            const gaddr cls = GREF(gaddr, m + MOBY_CLASS);
            s->slot = slot;
            s->key = slot < placed ? key_of[slot] : -1;
            s->oclass = GREF(int16_t, m + MOBY_OCLASS);
            s->flags =
                (GREF(gaddr, m + MOBY_UPDATE) != 0 ? FLAG_UPDATES : 0u)
                | (slot < placed ? FLAG_PLACED : 0u) | (state >= 0x80 ? FLAG_DEAD : 0u)
                | (cls != 0 && GREF(uint16_t, cls + CLASS_TYPE) == TYPE_CREATURE ? FLAG_CREATURE
                                                                                 : 0u)
                | (rac1_scene_actor(m) ? FLAG_SCENE : 0u)
                | ((GREF(uint16_t, m + MOBY_MODE) & MODE_HIDDEN) != 0
                           || GREF(uint8_t, m + MOBY_ALPHA) == 0
                       ? FLAG_HIDDEN
                       : 0u)
                | (state < 0x80 && GREF(uint8_t, m + MOBY_DRAWN) != 0 ? FLAG_DRAWN : 0u);
            s->state = state;
            for (unsigned i = 0; i < 3; ++i) {
                s->position[i] = GREF(float, m + MOBY_POS + 4u * i);
                s->rotation[i] = GREF(float, m + MOBY_ROT + 4u * i);
            }
            s->scale = GREF(float, m + MOBY_SCALE);
            /* As the hero's pose: a blend's target seq and the key it arrives at. */
            const uint8_t seq = GREF(uint8_t, m + MOBY_SEQ);
            const uint8_t target = GREF(uint8_t, m + MOBY_TARGET_SEQ);
            s->seq = target;
            s->next_frame = GREF(uint8_t, m + MOBY_NEXT_FRAME);
            s->frame = seq != target ? s->next_frame : GREF(uint8_t, m + MOBY_FRAME);
            s->anim_time = seq != target ? 0.0f : GREF(float, m + MOBY_ANIM_TIME);
            s->anim_speed = GREF(float, m + MOBY_ANIM_SPEED);
        }
        ++n;
    }
    return n;
}

int32_t rac1_world_moby_key(gaddr moby) {
    const int32_t slot = in_level() ? rac1_moby_slot(moby) : -1;
    if (slot < 0 || slot >= KEYS_MAX) {
        return -1;
    }
    read_keys();
    return key_of[slot];
}

int32_t rac1_world_moby_slot(int32_t key) {
    if (!in_level() || key < 0 || key >= KEYS_MAX) {
        return -1;
    }
    read_keys();
    const int32_t slot = slot_of[key];
    return slot >= 0 && rac1_world_moby_at(slot) != 0 ? slot : -1;
}

void rac1_world_moby_kill(gaddr moby) {
    if (moby != 0 && GREF(uint8_t, moby + MOBY_STATE) < 0x80) {
        const int entered = enter_level_context();
        killing = 1;
        func_0020D678(moby);
        killing = 0;
        leave_level_context(entered);
    }
}

void rac1_world_moby_hide(gaddr moby, int hidden) {
    if (moby == 0 || GREF(uint8_t, moby + MOBY_STATE) >= 0x80) {
        return;
    }
    const uint16_t mode = GREF(uint16_t, moby + MOBY_MODE);
    GREF(uint16_t, moby + MOBY_MODE) =
        (uint16_t)(hidden ? mode | 0x1u : mode & (uint16_t)~MODE_HIDDEN);
}

/* DeleteMoby: the game's, its slot's deletion noted first (whose update deletes it, or a mod), for
 * a mod that wants to know why a moby left the level (interface 1.9, moby_deletion). */
void func_0020D678(gaddr moby) {
    const int32_t slot = in_level() && main_context() ? rac1_moby_slot(moby) : -1;
    if (slot >= 0 && slot < KEYS_MAX && GREF(uint8_t, moby + MOBY_STATE) < 0x80) {
        const gaddr by = rac1_updating_moby();
        const int32_t by_slot = by != 0 ? rac1_moby_slot(by) : -1;
        deletions[slot].by = by_slot;
        deletions[slot].by_class = by_slot >= 0 ? GREF(int16_t, by + MOBY_OCLASS) : -1;
        deletions[slot].oclass = GREF(int16_t, moby + MOBY_OCLASS);
        deletions[slot].by_mod = (uint8_t)killing;
        deletions[slot].seen = 1;
        deletions[slot].loads = rac1_moby_generation();
    }
    func_0020D678__game(moby);
}

int rac1_world_moby_deletion(int32_t slot, openrac_game_moby_deletion* out) {
    if (slot < 0 || slot >= KEYS_MAX || !deletions[slot].seen
        || deletions[slot].loads != rac1_moby_generation()) {
        return 0;
    }
    out->by = deletions[slot].by;
    out->by_class = deletions[slot].by_class;
    out->by_mod = deletions[slot].by_mod;
    out->oclass = deletions[slot].oclass;
    out->loads = deletions[slot].loads;
    return 1;
}

gaddr rac1_world_create_moby(int32_t oclass) {
    if (!in_level() || oclass < 0) {
        return 0;
    }
    const int entered = enter_level_context();
    const gaddr m = func_0020D348(oclass);
    leave_level_context(entered);
    return m;
}

int32_t rac1_world_moby_spawn(int32_t oclass) {
    if (!in_level() || oclass < 0) {
        return -1;
    }
    const int entered = enter_level_context();
    const gaddr m = func_0020D348(oclass);
    /* No room, or a class the level has not loaded (InitMobyInstance leaves it undrawn). */
    if (m != 0 && GREF(gaddr, m + MOBY_CLASS) == 0) {
        func_0020D678(m);
        leave_level_context(entered);
        return -1;
    }
    /* CreateMoby leaves the distances it is updated and drawn within at 0 (never in range): the
     * game's spawners set them (spawn_bolt_by_value 0x40, others 0xFF), its visible byte and its
     * occlusion bits. A mod's moby gets what the spawners give where CreateMoby left nothing: a
     * driven one is drawn, one let go runs (its update's own first state may set them again). */
    if (m != 0) {
        if (GREF(uint8_t, m + MOBY_DIST) == 0) {
            GREF(uint8_t, m + MOBY_DIST) = 0xFFu;
        }
        if (GREF(int16_t, m + MOBY_DRAW_DIST) == 0) {
            GREF(int16_t, m + MOBY_DRAW_DIST) = 0xFF;
        }
        if (GREF(uint16_t, m + MOBY_OCCLUSION) == 0) {
            GREF(uint16_t, m + MOBY_OCCLUSION) = 0x7F80u;
        }
        GREF(uint8_t, m + MOBY_DRAWN) = 1;
    }
    leave_level_context(entered);
    return m != 0 ? rac1_moby_slot(m) : -1;
}

int32_t rac1_world_hero_slot(void) {
    return in_level() ? rac1_moby_slot(GREF(gaddr, HERO_MOBY)) : -1;
}

void rac1_world_stand_in(const float position[3], gaddr moby) {
    if (stand.on || !in_level() || !main_context()) {
        return;
    }
    stand.on = 1;
    /* The position, the ground point and the body's centre, moved by the same step. */
    static const unsigned offsets[3] = {0x80u, 0xC0u, 0xD0u};
    float step[3];
    for (unsigned c = 0; c < 3; ++c) {
        step[c] = position[c] - GREF(float, HERO + 0x80u + 4u * c);
    }
    for (unsigned i = 0; i < 3; ++i) {
        memcpy(stand.motion[i], G(HERO + offsets[i]), 16);
        for (unsigned c = 0; c < 3; ++c) {
            GREF(float, HERO + offsets[i] + 4u * c) += step[c];
        }
    }
    stand.moby = GREF(gaddr, HERO_MOBY);
    if (moby != 0) {
        GREF(gaddr, HERO_MOBY) = moby;
    }
}

void rac1_stand_in_end(void) {
    if (!stand.on) {
        return;
    }
    static const unsigned offsets[3] = {0x80u, 0xC0u, 0xD0u};
    for (unsigned i = 0; i < 3; ++i) {
        memcpy(G(HERO + offsets[i]), stand.motion[i], 16);
    }
    GREF(gaddr, HERO_MOBY) = stand.moby;
    stand.on = 0;
}

/* The player's bolts after adding `add` (a mod moves the bolts a stand-in's update gave to the
 * player who stood in). */
int32_t rac1_world_bolts(int32_t add) {
    GREF(int32_t, BOLTS) += add;
    return GREF(int32_t, BOLTS);
}

int rac1_moby_gate(gaddr moby, int phase) {
    /* The pause menu's own mobys are none of the mods'. */
    if (openrac_game_moby_update == NULL || !main_context()) {
        return 0;
    }
    return openrac_game_moby_update(rac1_moby_slot(moby), phase);
}

void rac1_world_eyes(const float* xyz, int count) {
    eyes_count = count < 0 ? 0 : count > EYES_MAX ? EYES_MAX : count;
    if (eyes_count > 0) {
        memcpy(eyes_xyz, xyz, sizeof(float) * 3u * (unsigned)eyes_count);
    }
}

int rac1_moby_eyes(const float** xyz) {
    *xyz = eyes_xyz;
    return in_level() ? eyes_count : 0;
}

/* A hit record written to `victim` by the call just made (its index set, the record new or
 * changed), told to the mods; taken back when they say so. */
static void hit_written(gaddr victim, uint8_t before, const uint8_t* old) {
    const uint8_t index = GREF(uint8_t, victim + MOBY_HIT);
    if (index == 0xFF || index >= 64 || !main_context()) {
        return;
    }
    const gaddr record = HIT_RING + (gaddr)index * HIT_SIZE;
    if (index == before && memcmp(old, G(record), HIT_SIZE) == 0) {
        return; /* refused: a fresher hit stands */
    }
    /* A figure's shot hits nothing here: the shooter's game tells its hits (figure_items.c). */
    if (rac1_figure_owned(rac1_updating_moby())
        || rac1_figure_owned(GREF(gaddr, record + HIT_ATTACKER))) {
        GREF(uint8_t, victim + MOBY_HIT) = 0xFF;
        rac1_figure_hit_dropped("hit");
        return;
    }
    if (openrac_game_moby_hit == NULL) {
        return;
    }
    float position[3], velocity[3];
    for (unsigned c = 0; c < 3; ++c) {
        position[c] = GREF(float, record + 4u * c);
        velocity[c] = GREF(float, record + 0x10u + 4u * c);
    }
    const gaddr attacker = GREF(gaddr, record + HIT_ATTACKER);
    if (openrac_game_moby_hit(
            rac1_moby_slot(victim),
            attacker != 0 ? rac1_moby_slot(attacker) : -1,
            GREF(uint32_t, record + HIT_MASK),
            GREF(float, record + HIT_DAMAGE),
            position,
            velocity
        )) {
        GREF(uint8_t, victim + MOBY_HIT) = 0xFF;
    }
}

/* What a hit call's victim had before it: its index and that record. */
static uint8_t hit_before(gaddr victim, uint8_t old[HIT_SIZE]) {
    const uint8_t index = GREF(uint8_t, victim + MOBY_HIT);
    if (index < 64) {
        memcpy(old, G(HIT_RING + (gaddr)index * HIT_SIZE), HIT_SIZE);
    }
    return index;
}

/* PostHitMessage: the game's, then the mods told. The two discs order its arguments apart
 * (RAC1_POST_HIT_DAMAGE_LAST: the US C has the damage last). */
#ifdef RAC1_POST_HIT_DAMAGE_LAST
void func_L00_0025AC00(gaddr victim, int attacker, int mask, gaddr pos, gaddr vel, float damage) {
    uint8_t old[HIT_SIZE];
    const uint8_t before = hit_before(victim, old);
    func_L00_0025AC00__game(victim, attacker, mask, pos, vel, damage);
    hit_written(victim, before, old);
}
#else
void func_L00_0025AC00(gaddr victim, float damage, int attacker, int mask, gaddr pos, gaddr vel) {
    uint8_t old[HIT_SIZE];
    const uint8_t before = hit_before(victim, old);
    func_L00_0025AC00__game(victim, damage, attacker, mask, pos, vel);
    hit_written(victim, before, old);
}
#endif

/* record_moby_hit: the same for a hit copied from a record (explosions, contact). */
void func_L00_0025AAC0(gaddr victim, gaddr source) {
    uint8_t old[HIT_SIZE];
    const uint8_t before = hit_before(victim, old);
    func_L00_0025AAC0__game(victim, source);
    hit_written(victim, before, old);
}

void rac1_world_post_hit(
    gaddr victim,
    gaddr attacker,
    uint32_t mask,
    float damage,
    const float position[3],
    const float velocity[3]
) {
    if (victim == 0 || !in_level()) {
        return;
    }
    GFRAME(vectors, 32);
    for (unsigned c = 0; c < 4; ++c) {
        GREF(float, vectors + 4u * c) = c < 3 ? position[c] : 1.0f;
        GREF(float, vectors + 0x10u + 4u * c) = c < 3 ? velocity[c] : 0.0f;
    }
#ifdef RAC1_POST_HIT_DAMAGE_LAST
    func_L00_0025AC00__game(victim, (int)attacker, (int)mask, vectors, vectors + 0x10u, damage);
#else
    func_L00_0025AC00__game(victim, damage, (int)attacker, (int)mask, vectors, vectors + 0x10u);
#endif
}

/* The world going on under a menu (interface 1.9, play_overlay_pause): while a mod asks for it,
 * the level's mobys are updated each tick under the pause menu (its update, func_L00_00277A88)
 * and under a freeze dialog (update_mode_freeze, func_001FD3E8), both wrapped (libraries.tsv), in
 * the level's own moby context; the hero and the camera stay as the menu left them.
 * A game in a party then never stops the others' world while its player looks at a menu.
 *
 * Under the pause menu the level's camera is put up around that update too, and the level is
 * drawn. The menu puts a camera of its own in the camera block for its frame objects (at
 * (256, 256, 64)): the run list (moby_pool.c) and the mobys' own updates measured their distances
 * from it, so the mobys around the player stopped as out of reach, and those that leave when the
 * camera is far went away, in the game that runs them and so in the others'. The menu also draws
 * over a picture of the frame it opened on (transfer_draw_buffer_chunks, which the window answers
 * with its last world) and runs no world renderer, so the paused player saw the world stand
 * still. Each of its ticks, before its own update and drawing, the world renderers run here with
 * the level's camera: TfragProc puts the world's marker in the display list (the menu's 2D goes
 * over it) and has the window take this tick's world camera and the level's mobys
 * (openrac_game_world_under_menu); the other layers are flagged as DrawWorld would, and PartProc
 * hands over the particles. The window draws the menu's picture of the frame from that live world
 * (frontend scene_hud.cpp). The menu's own camera, fog and mobys are put back after: its frame
 * objects are drawn with them as before.
 *
 * The level's camera: the camera block (D_00187040 up to +0x400: MobyProc's rows at +0, the
 * matrices, the position at +0x140 the run list reads, the view's rows at +0x350 the window
 * reads) and the view context (D_0018CE00: the tangents, focal lengths and fog), as they are on
 * the menu's first tick, before initialize_pause_menu puts its own there; swapped with the
 * menu's, so what the level's update writes there is kept for the next tick.
 *
 * The level's collision: initialize_pause_menu turns the collision queries off while the menu is
 * open (bit 31 of the query stamp, CollOutput +0x10, D_L00_00173F40; a query whose stamp is negative
 * hits nothing, collision.c's new_stamp) and close_pause_menu turns them back on. Nothing ran
 * under the menu in the game, but the level's mobys do here: with every query missing, the
 * creatures that stand on the ground (Veldin's class 749) found none and were put at height 0,
 * under the level, where they stayed after the menu closed. The queries are put back on around
 * the level's update and off again after it, under the pause menu and under a freeze dialog
 * (which the pause menu can open). */
static int keep_running;

void rac1_world_keep_running(int on) {
    keep_running = on != 0;
}

/* The collision queries' stamp (CollOutput +0x10) and its bit for "off". */
#define COLL_STAMP (OPENRAC_LDATA(0, RAC1_LADDR(00, 0x00173F40u)) + 0x10u)
#define COLL_OFF 0x80000000u

#define CAMERA_BLOCK RAC1_ADDR(0x00187040u)
#define VIEW_CONTEXT RAC1_ADDR(0x0018CE00u)

enum {
    CAMERA_BYTES = 0x400,
    VIEW_BYTES = 0x240
};

void func_002352C8(void);
void func_00218B10(void);

static struct {
    uint32_t load; /* the level load it was saved in; ~0 none */
    uint8_t camera[CAMERA_BYTES];
    uint8_t view[VIEW_BYTES];
} level_view = {0xFFFFFFFFu, {0}, {0}};

static void swap_view(void) {
    uint8_t held[CAMERA_BYTES];
    memcpy(held, G(OPENRAC_DATA(CAMERA_BLOCK)), CAMERA_BYTES);
    memcpy(G(OPENRAC_DATA(CAMERA_BLOCK)), level_view.camera, CAMERA_BYTES);
    memcpy(level_view.camera, held, CAMERA_BYTES);
    memcpy(held, G(OPENRAC_DATA(VIEW_CONTEXT)), VIEW_BYTES);
    memcpy(G(OPENRAC_DATA(VIEW_CONTEXT)), level_view.view, VIEW_BYTES);
    memcpy(level_view.view, held, VIEW_BYTES);
}

/* `pause_menu`: under the pause menu (the level's camera put up, the level drawn), else under a
 * freeze dialog, which keeps the level's camera and draws the level itself. */
/* What the menu updates of this level load did with the world, for moby_run (interface 1.9):
 * how many, and the last one's OPENRAC_RUN_MENU_* value. */
static struct {
    uint32_t load;
    uint32_t ticks;
    int32_t last;
} menu_runs = {0xFFFFFFFFu, 0, 0};

static void note_menu_run(int32_t how) {
    const uint32_t load = rac1_moby_generation();
    if (menu_runs.load != load) {
        menu_runs.load = load;
        menu_runs.ticks = 0;
    }
    ++menu_runs.ticks;
    menu_runs.last = how;
}

static void run_world_under_menu(int pause_menu) {
    if (!keep_running || !in_level()) {
        note_menu_run(1); /* OPENRAC_RUN_MENU_HELD */
        return;
    }
    /* The menu's first tick (it sets itself up in this update): the camera is still the level's. */
    const int first = main_context();
    const uint32_t load = rac1_moby_generation();
    int swapped = 0;
    if (pause_menu && first) {
        memcpy(level_view.camera, G(OPENRAC_DATA(CAMERA_BLOCK)), CAMERA_BYTES);
        memcpy(level_view.view, G(OPENRAC_DATA(VIEW_CONTEXT)), VIEW_BYTES);
        level_view.load = load;
    } else if (pause_menu && level_view.load == load) {
        swap_view();
        swapped = 1;
    }
    /* OPENRAC_RUN_MENU_FIRST, _LEVEL_CAMERA, _OWN_CAMERA, _FREEZE */
    note_menu_run(!pause_menu ? 5 : first ? 2 : swapped ? 3 : 4);
    const int entered = enter_level_context();
    const uint32_t coll_off = GREF(uint32_t, COLL_STAMP) & COLL_OFF;
    GREF(uint32_t, COLL_STAMP) &= ~COLL_OFF;
    func_00213C78();
    GREF(uint32_t, COLL_STAMP) |= coll_off;
    if (pause_menu && (first || swapped)) {
        openrac_game_world_under_menu();
        openrac_game_draw(OPENRAC_DRAW_SKY);
        func_002352C8();
        openrac_game_draw(OPENRAC_DRAW_TIES | OPENRAC_DRAW_SHRUBS | OPENRAC_DRAW_MOBYS);
        func_00218B10();
    }
    leave_level_context(entered);
    if (swapped) {
        swap_view();
    }
}

void func_L00_00277A88(void) {
    run_world_under_menu(1);
    func_L00_00277A88__game();
}

void func_001FD3E8(void) {
    run_world_under_menu(0);
    func_001FD3E8__game();
}

/* The camera: the level's, also while the pause menu's is up (its camera at 256,256,64 is no
 * place in the level), as the run list and the drawing take it under the menu. */
int rac1_world_camera_position(float out[3]) {
    if (!in_level()) {
        return 0;
    }
    if (!main_context() && level_view.load == rac1_moby_generation()) {
        memcpy(out, level_view.camera + 0x140, 3 * sizeof(float));
    } else {
        memcpy(out, G(OPENRAC_DATA(CAMERA_POSITION)), 3 * sizeof(float));
    }
    return 1;
}

int rac1_level_mobys_current(void) {
    return main_context();
}

/* The level's game mode (play.c's), for moby_run. */
#define LEVEL_MODE OPENRAC_LDATA(0, RAC1_LADDR(00, 0x0015F6A8u))

int rac1_world_moby_run(int32_t slot, openrac_game_moby_run* out) {
    if (!in_level()) {
        return 0;
    }
    out->ran = rac1_moby_run(slot, &out->runs, out->camera);
    const uint32_t load = rac1_moby_generation();
    out->menu_ticks = menu_runs.load == load ? menu_runs.ticks : 0;
    out->menu = menu_runs.load == load ? menu_runs.last : 0;
    if (level_view.load == load) {
        memcpy(out->level_camera, level_view.camera + 0x140, sizeof out->level_camera);
    }
    out->level_mode = GREF(int, LEVEL_MODE);
    out->context = main_context() ? 0 : 1;
    const gaddr base = array_base();
    if (slot < 0 || slot >= KEYS_MAX || base == 0) {
        return 1;
    }
    const gaddr m = base + (gaddr)slot * MOBY_SIZE;
    /* A free or dead slot, or the array's end (a slot moby_list gave is read, never one past). */
    if (GREF(uint8_t, m + MOBY_STATE) >= 0x80) {
        return 1;
    }
    out->update_dist = GREF(uint8_t, m + MOBY_DIST);
    out->draw_dist = GREF(int16_t, m + MOBY_DRAW_DIST);
    out->drawn = GREF(uint8_t, m + MOBY_DRAWN);
    float sum = 0.0f;
    for (unsigned i = 0; i < 3; ++i) {
        const float d = GREF(float, m + MOBY_POS + 4u * i) - out->camera[i];
        sum += d * d;
    }
    out->distance = sqrtf(sum);
    return 1;
}
