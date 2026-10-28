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
 * are read once a level load (boot.c's count).
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
#include <string.h>

#include "rac1.h"

/* The moby array and the end of its placed part (each level's copy), the file-order table. */
#define MOBY_ARRAY RAC1_ADDR(0x00160018u)
#define MOBY_PLACED_END RAC1_ADDR(0x0016001Cu)
#define FILE_ORDER OPENRAC_LDATA(0, RAC1_LADDR(00, 0x001ACB00u))
/* The hit ring. */
#define HIT_RING OPENRAC_LDATA(0, RAC1_LADDR(00, 0x00178200u))
/* The hero (play.c): his motion's position, ground point and body centre, his moby; the bolts. */
#define HERO RAC1_ADDR(0x0013F450u)
#define HERO_MOBY (HERO + 0x2080u)
#define BOLTS RAC1_ADDR(0x0015EE98u)

enum {
    MOBY_POS = 0x10,
    MOBY_STATE = 0x20,
    MOBY_CLASS = 0x24,
    MOBY_SCALE = 0x2C,
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
/* The most records a level's file has that the keys follow, and eyes. */
#define KEYS_MAX 4096
#define EYES_MAX 8

static int16_t key_of[KEYS_MAX];  /* by slot: its key, -1 none */
static int16_t slot_of[KEYS_MAX]; /* by key: its slot, -1 none */
static uint32_t keys_load = 0xFFFFFFFFu;

static float eyes_xyz[EYES_MAX * 3];
static int eyes_count;

/* The stand-in: whether one is on, and what it put aside. */
static struct {
    int on;
    uint8_t motion[3][16];
    gaddr moby;
} stand;

static int in_level(void) {
    const int level = openrac_game_loaded_overlay();
    return level >= 0 && level < 19;
}

static gaddr array_base(void) {
    return GREF(gaddr, OPENRAC_DATA(MOBY_ARRAY));
}

static int32_t placed_count(void) {
    const gaddr base = array_base();
    const gaddr end = GREF(gaddr, OPENRAC_DATA(MOBY_PLACED_END));
    return base != 0 && end > base ? (int32_t)((end - base) / MOBY_SIZE) : 0;
}

/* The slots' keys for the level loaded now, read once a load. */
static void read_keys(void) {
    if (keys_load == rac1_program_loads()) {
        return;
    }
    keys_load = rac1_program_loads();
    memset(key_of, 0xFF, sizeof key_of);
    memset(slot_of, 0xFF, sizeof slot_of);
    const int32_t placed = placed_count();
    int32_t found = 0;
    /* Every placed moby has its record; the table runs on past the last for none. */
    for (int32_t record = 0; record < KEYS_MAX && found < placed; ++record) {
        const int16_t slot = GREF(int16_t, FILE_ORDER + 2u * (unsigned)record);
        if (slot >= 0 && slot < placed && slot < KEYS_MAX) {
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
    return (int32_t)((moby - base) / MOBY_SIZE);
}

gaddr rac1_world_moby_at(int32_t slot) {
    if (!in_level() || slot < 0 || slot >= KEYS_MAX) {
        return 0;
    }
    const gaddr m = array_base() + (gaddr)slot * MOBY_SIZE;
    /* Gone, free, or past the array's end. */
    return GREF(uint8_t, m + MOBY_STATE) < 0x80 ? m : 0;
}

int rac1_world_mobys(openrac_game_moby_state* out, int max) {
    if (!in_level()) {
        return 0;
    }
    read_keys();
    const gaddr base = array_base();
    const int32_t placed = placed_count();
    int n = 0;
    for (int32_t slot = 0; slot < KEYS_MAX; ++slot) {
        const gaddr m = base + (gaddr)slot * MOBY_SIZE;
        const uint8_t state = GREF(uint8_t, m + MOBY_STATE);
        /* 0xFF ends the array; other states past 0x80 are free or dead slots. */
        if (state == 0xFF) {
            break;
        }
        if (state >= 0x80) {
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
                | (slot < placed ? FLAG_PLACED : 0u)
                | (cls != 0 && GREF(uint16_t, cls + CLASS_TYPE) == TYPE_CREATURE ? FLAG_CREATURE
                                                                                 : 0u);
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
        func_0020D678(moby);
    }
}

int32_t rac1_world_moby_spawn(int32_t oclass) {
    if (!in_level() || oclass < 0) {
        return -1;
    }
    const gaddr m = func_0020D348(oclass);
    /* No room, or a class the level has not loaded (InitMobyInstance leaves it undrawn). */
    if (m == 0) {
        return -1;
    }
    if (GREF(gaddr, m + MOBY_CLASS) == 0) {
        func_0020D678(m);
        return -1;
    }
    return rac1_moby_slot(m);
}

int32_t rac1_world_hero_slot(void) {
    return in_level() ? rac1_moby_slot(GREF(gaddr, HERO_MOBY)) : -1;
}

void rac1_world_stand_in(const float position[3], gaddr moby) {
    if (stand.on || !in_level()) {
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
    if (openrac_game_moby_update == NULL) {
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
    if (openrac_game_moby_hit == NULL || index == 0xFF || index >= 64) {
        return;
    }
    const gaddr record = HIT_RING + (gaddr)index * HIT_SIZE;
    if (index == before && memcmp(old, G(record), HIT_SIZE) == 0) {
        return; /* refused: a fresher hit stands */
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
