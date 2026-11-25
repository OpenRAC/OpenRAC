/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Going Commando: the hero's weapons and their shots for the mods (interface 1.29; weapon_shots.h).
 * rac1/host/figure_shots.c is the model: the game's own spawn of each shot is wrapped, the hero's
 * shots are noted there, and a shot made for a mod is made by the same call, so the game runs it.
 *
 * The spawns (rac2-decomp, src/levels/shared; one function in every level's program):
 *   Minirocket Tube  projectile_spawn_targeted, func_L00_003AB9C0 (class 1498), from the gun's
 *                    weapon_spawn_muzzle_effect (func_L00_003A5370); its owner is the gun
 *   RYNO II          SpawnAimedProjectile, func_L00_00409E48 (3799), from UpdateLockOnWeapon
 *   Seeker Gun       effect_spawn_owned_moby, func_L00_003AA4C8 (3329), from
 *                    hero_fire_effect_from_facing (func_L00_003AB8B8); its last argument is the
 *                    moby it homes on
 *   Hoverbomb Gun    SpawnProjectile, func_L00_003A98E8 (1496), from moby_fire_shot
 *   Bomb Glove       part_l00_spawn_spinning_debris, func_L00_0038D530 (121)
 *   Gravity Bomb     gravity_bomb_spawn_projectile, func_L00_003EED10 (3300)
 *   Bouncer          bouncer_spawn_debris, func_L00_003FE2D0 (3684 through
 *                    bouncer_spawn_debris_object); it sets the bomblet's +0xB8 to the gun itself
 *   Decoy Glove      decoy_create_projectile, func_L00_00398810 (203): it retires the oldest of six
 *   Miniturret Glove miniturret_spawn_shot, func_L00_00387CA8 (4966)
 * Every caller but the Bouncer's writes the gun into the new moby's +0xB8 after the spawn returns,
 * so a spawn is noted where it happens and told at the tick's end (rac2_shots_frame): a shot is the
 * hero's when its +0xB8, or the gun its spawn names, is the moby in his hand (his item slot 0,
 * +0x1220 of his record). The other callers (the Megarocket Cannon's sequenced_child_spawn through
 * projectile_spawn_targeted, the HK-22 through effect_spawn_owned_moby) are not the hero's.
 * Left out: the Chopper (guided_rocket_create keeps a per-level slot for each blade), the Spiderbot
 * Glove (spider_bot_spawn keeps one bot in globals), the Visibomb (its missile takes the hero and
 * the camera), the Synthenoid (SpawnCB2Moby spends ammo itself). A weapon of those, or one with no
 * shot moby of its own (the Blitz Gun's pellets, the Lancer, the Lava Gun, the Pulse Rifle's beam,
 * the Plasma Coil), is told once a tick it spends rounds (its ammo at 0x139688 + item * 4 down).
 *
 * A shot fired again (rac2_shot_refire) is its spawn called again with what the first call was
 * given (vectors copied, as the caller's were on its stack), turned: the velocity argument where
 * the spawn takes one, otherwise the rows of the new moby's matrix (+0xC0 forward, +0xD0 right,
 * +0xE0 up, as the spawns write them) and the direction in its pvars (+0x30). Its +0xB8 is the
 * first's.
 *
 * The two rockets' trails: each rocket keeps its last four points (0x40 bytes) in a ring of the
 * level's (the Minirocket Tube's eight at D_L00_0023B710, RYNO II's 32 at D_L00_00255BA0, round
 * robin), named by its pvars +0x2C; its update shifts them in each tick and its draw
 * (moby_start_animation_from_table, its +0x2C << 6 plus the ring) lays the streak through them.
 * Past eight or 32 alive, two rockets write one ring, and the streak runs from one to the other
 * across the scene. So every rocket gets a ring of its moby slot's in the port's memory, named by
 * the +0x2C that reaches it from the game's ring, and its four points start where it is: a rocket
 * a mod made away from the gun (a split) is put there after its spawn wrote the first point.
 *
 * The moby loop (moby_update_live_mobys, func_L00_0032CF60) is the game's, written here with
 * three additions: the mods' gate before and after each update (openrac_game_moby_update, as
 * rac1/host/world.c calls it), the statuses (moby_status: a frozen moby's update and animation are
 * left out but for one update for each hit posted to it, which the impact queue says: its +0xA8
 * names an entry of g_impact_queue (0x1BF200, 0x40 bytes) whose +0x38 is the moby; a slowed one
 * runs one tick in two or three), and the moby whose update runs now, so that what it makes takes
 * its weapon's look: a moby CreateMoby makes (func_0029EFD0: its blast, its debris) and a particle
 * part_alloc makes (func_002B9030: its trail, sparks and smoke, also those that outlive it). The
 * gun in the hero's hand gives its item's look to what its update makes (the shot, the muzzle's).
 *
 * A shot ends when its moby dies (+0x20 at 0x80 or over). The loop sees it die in its own update:
 * its end is an impact when that update made a moby (its blast) or, for the two rockets, set +0xBC
 * (the hit they explode on); an end with the shot's own timer at 0 (pvars) is its time running out.
 */
#include "weapon_shots.h"

#include <math.h>
#include <string.h>

#include "death_trace.h"
#include "enemy_copies.h"
#include "game_protos.h"
#define RAC2_MOBY_TARGET_RECORD func_002AB1F0
#include "moby_checks.h"
#include "moby_pool.h"
#include "rac2.h"

/* The hero's record (D_00189E20 on v1.01): g_heroPos is its +0x80. His item slots, 0x50 bytes each:
 * slot 0 the hand, its moby at +0x1220, its item at +0x1248 (func_L00_002AFBF0). */
#define HERO (RAC2_g_heroPos - 0x80u)
#define HERO_ITEM_MOBY (HERO + 0x1220u)
#define HERO_ITEM_ID (HERO + 0x1248u)

/* Each item's ammo (weapon_spend_ammo, func_L00_002D3F30), its record in the level's weapon table
 * (g_item_slot_to_record) and the table, 0xE0 bytes a record. */
#define AMMO RAC2_ADDR(0x00139688u)
#define ITEM_RECORDS RAC2_ADDR(0x00139568u)
#define WEAPON_TABLE OPENRAC_LDATA(0, RAC2_LADDR(00, 0x00262570u))
#define WEAPON_RECORD_SIZE 0xE0u
#define ITEMS 64

/* The moby loop's words: the target list it clears, the head of the list it links, and the table
 * it walks (moby_update_live_mobys). */
#define LOOP_TARGETS OPENRAC_LDATA(0, RAC2_LADDR(00, 0x001FACE0u))
#define LOOP_HEAD OPENRAC_LDATA(0, RAC2_LADDR(00, 0x001B1EA8u))
#define LOOP_TABLE OPENRAC_LDATA(0, RAC2_LADDR(00, 0x001B1E9Cu))

/* The impact queue (QueueMobyImpact, func_002A8E20): 64 entries of 0x40 bytes. */
#define IMPACT_QUEUE OPENRAC_DATA(RAC2_ADDR(0x001BF200u))
#define IMPACT_SIZE 0x40u
#define IMPACT_ENTRIES 64u
#define IMPACT_OWNER 0x38u
#define IMPACT_DAMAGE 0x2Cu

/* The particle pool (part_alloc_from_pool, func_002B9038): 2048 records of 0x40 bytes. */
#define PARTICLE_BASE OPENRAC_DATA(RAC2_ADDR(0x001B1D90u))
#define PARTICLES 2048
#define PARTICLE_SIZE 0x40u

/* The rockets' trail rings (the file's head): 0x40 bytes a rocket, four points. */
#define MINIROCKET_TRAILS OPENRAC_LDATA(0, RAC2_LADDR(00, 0x0023B710u))
#define RYNO_TRAILS OPENRAC_LDATA(0, RAC2_LADDR(00, 0x00255BA0u))
#define TRAIL_SIZE 0x40u
#define TRAIL_INDEX 0x2Cu /* in the pvars */

/* The moby table (play.c's). */
#define MOBY_BASE OPENRAC_DATA(RAC2_g_mobyTableBase)
#define MOBY_END OPENRAC_DATA(RAC2_g_mobyTableEnd)
#define MOBY_SIZE 0x100u
#define SLOTS 4096

enum {
    MOBY_POS = 0x10,
    MOBY_STATE = 0x20, /* 0x80 and over: dead or free */
    MOBY_NEXT = 0x28,  /* the moby loop's list */
    MOBY_FLAGS = 0x34,
    MOBY_UPDATE = 0x64,
    MOBY_PVARS = 0x68,
    MOBY_IMPACT = 0xA8,
    MOBY_OCLASS = 0xAA,
    MOBY_OWNER = 0xB8,
    MOBY_HIT = 0xBC, /* the rockets' */
    MOBY_FORWARD = 0xC0,
    MOBY_RIGHT = 0xD0,
    MOBY_UP = 0xE0
};

#define FLAG_NOT_RUN 0x2u
#define FLAG_KEEP_MATRIX 0x4u
#define FLAG_NO_ANIMATION 0x40u

/* What each kind is (the spawns above): its class, its weapon, whether its spawn takes the
 * velocity (else the matrix is turned), its own timer in its pvars (an s16, -1 none), the moby it
 * homes on in its pvars (-1 none). */
static const struct {
    int16_t oclass;
    int16_t item;
    int8_t takes_velocity;
    int8_t timer;
    int8_t target;
} KINDS[RAC2_SHOT_KINDS] = {
    [RAC2_SHOT_MINIROCKET] = {1498, 27, 0, 0x24, 0x10},
    [RAC2_SHOT_RYNO] = {3799, 44, 0, 0x24, 0x10},
    [RAC2_SHOT_SEEKER] = {3329, 24, 1, 0x18, 0x14},
    [RAC2_SHOT_HOVERBOMB] = {1496, 25, 1, -1, -1},
    [RAC2_SHOT_BOMB] = {121, 12, 1, -1, -1},
    [RAC2_SHOT_GRAVITY_BOMB] = {3300, 42, 1, -1, -1},
    [RAC2_SHOT_BOUNCER] = {3684, 37, 1, -1, -1},
    [RAC2_SHOT_DECOY] = {203, 17, 1, -1, -1},
    [RAC2_SHOT_MINITURRET] = {4966, 41, 1, -1, -1},
};

int rac2_shot_class(int kind) {
    return kind >= 0 && kind < RAC2_SHOT_KINDS ? KINDS[kind].oclass : -1;
}

int rac2_shot_kind(int oclass) {
    for (int k = 0; k < RAC2_SHOT_KINDS; ++k) {
        if (KINDS[k].oclass == oclass) {
            return k;
        }
    }
    return -1;
}

/* ---- Vectors ---- */

static void rotate_about(float v[3], const float axis[3], float angle) {
    const float c = cosf(angle), s = sinf(angle);
    const float d = axis[0] * v[0] + axis[1] * v[1] + axis[2] * v[2];
    const float x[3] = {
        axis[1] * v[2] - axis[2] * v[1],
        axis[2] * v[0] - axis[0] * v[2],
        axis[0] * v[1] - axis[1] * v[0],
    };
    for (int i = 0; i < 3; ++i) {
        v[i] = v[i] * c + x[i] * s + axis[i] * d * (1.0f - c);
    }
}

void rac2_shot_turn(float v[3], const float forward[3], float yaw, float pitch) {
    static const float up[3] = {0.0f, 0.0f, 1.0f};
    float f[3] = {forward[0], forward[1], forward[2]};
    rotate_about(v, up, yaw);
    rotate_about(f, up, yaw);
    /* Across the turned forward, level: forward x up. A shot straight up or down has none. */
    float across[3] = {f[1], -f[0], 0.0f};
    const float length = sqrtf(across[0] * across[0] + across[1] * across[1]);
    if (pitch == 0.0f || length < 1e-6f) {
        return;
    }
    across[0] /= length;
    across[1] /= length;
    /* About forward x up, a positive angle lifts forward toward up. */
    rotate_about(v, across, pitch);
}

static void read3(float out[3], gaddr a) {
    memcpy(out, G(a), 12);
}

static void write3(gaddr a, const float v[3]) {
    memcpy(G(a), v, 12);
}

static float length3(const float v[3]) {
    return sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

/* ---- The level ---- */

static int in_level(void) {
    const int level = openrac_game_loaded_overlay();
    return level >= 0 && level < RAC2_LEVELS;
}

static int table(gaddr* base, gaddr* end) {
    if (!in_level()) {
        return 0;
    }
    *base = GREF(gaddr, MOBY_BASE);
    *end = GREF(gaddr, MOBY_END);
    return *base != 0 && *end > *base && (*end - *base) % MOBY_SIZE == 0;
}

static int32_t slot_of(gaddr moby) {
    gaddr base, end;
    if (!table(&base, &end) || !rac2_moby_slot(moby)) {
        return -1;
    }
    const uint32_t slot = (moby - base) / MOBY_SIZE;
    return slot < SLOTS ? (int32_t)slot : -1;
}

static gaddr moby_at(int32_t slot) {
    gaddr base, end;
    if (slot < 0 || !table(&base, &end)) {
        return 0;
    }
    const gaddr m = base + (gaddr)slot * MOBY_SIZE;
    return m < end && rac2_moby_live(m) ? m : 0;
}

/* Whether `moby` is a live moby of the table, still of class `oclass` (moby_checks.h). */
static int alive(gaddr moby, int16_t oclass) {
    return rac2_moby_live(moby) && GREF(int16_t, moby + MOBY_OCLASS) == oclass;
}

/* The moby in the hero's hand, when it is a slot of the table. */
static gaddr hand(void) {
    const gaddr m = in_level() ? GREF(gaddr, HERO_ITEM_MOBY) : 0;
    return rac2_moby_slot(m) ? m : 0;
}

static int32_t hand_item(void) {
    const int32_t item = GREF(int32_t, HERO_ITEM_ID);
    return item >= 0 && item < ITEMS ? item : -1;
}

/* ---- What a spawn was given, and the shots told ---- */

/* A spawn's call: the moby made, the gun and target it names (0 none), its vectors (position,
 * then velocity, direction or the rockets' argument), its floats and its int. */
typedef struct {
    int kind;
    gaddr moby;
    gaddr owner;
    gaddr target;
    float v0[4];
    float v1[4];
    float f[4];
    int32_t i;
} Call;

/* The spawns made in the tick (rac2_shots_frame tells them), and the last real call of each kind
 * (what moby_spawn_live passes where a mod gives nothing). */
#define CALLS_MAX 128
static Call calls[CALLS_MAX];
static int call_count;
static Call last_call[RAC2_SHOT_KINDS];
static int have_last_call[RAC2_SHOT_KINDS];
/* The spawns are the port's own while it fires one again or makes one for a mod. */
static int own_spawn;

/* A shot told to the mods, until it ends. */
typedef struct {
    gaddr moby;
    int16_t oclass;
    int8_t kind;
    int8_t ended;
    int32_t item;
    uint32_t serial;
    uint32_t flags;
    gaddr owner;
    gaddr target;
    int32_t reason;
    float last[3];
} Shot;

#define SHOTS_MAX 1024
static Shot shots[SHOTS_MAX];
static int shot_count;
static uint32_t next_serial = 1;
static uint32_t shots_loads;

/* The tick's refires and live spawns, against OPENRAC_SHOTS_PER_TICK_MAX, and the pool's free
 * slots read once a tick. */
#define PER_TICK_MAX 128
#define POOL_RESERVE 32
static int made_this_tick;
static int pool_free = -1;

/* The look each slot's moby takes (its weapon's item, -1 none) and its class when given. */
static int8_t look_item[SLOTS];
static int16_t look_class[SLOTS];
static int looks_ready;

/* The particle records tied to a look. */
static uint8_t particle_tagged[PARTICLES / 8];

/* The moby whose update the loop runs now, and whether it made a moby in it. */
static gaddr updating;
static int updating_made;

gaddr rac2_moby_updating(void) {
    return updating;
}

/* The tick's ammo watch: the item in hand and its ammo after the last tick. */
static int32_t ammo_item = -1;
static int32_t ammo_last;

static void looks_reset(void) {
    for (int i = 0; i < SLOTS; ++i) {
        look_item[i] = -1;
        look_class[i] = -1;
    }
    memset(particle_tagged, 0, sizeof particle_tagged);
    looks_ready = 1;
}

/* The look item of `moby` (the weapon whose look it takes), -1 none. */
static int32_t look_of(gaddr moby) {
    if (moby == 0) {
        return -1;
    }
    if (moby == hand()) {
        return hand_item();
    }
    const int32_t slot = slot_of(moby);
    if (slot < 0 || !looks_ready || look_item[slot] < 0
        || look_class[slot] != GREF(int16_t, moby + MOBY_OCLASS)) {
        return -1;
    }
    return look_item[slot];
}

static void give_look(gaddr moby, int32_t item) {
    const int32_t slot = slot_of(moby);
    if (slot < 0 || item < 0 || openrac_game_moby_look == NULL) {
        return;
    }
    if (!looks_ready) {
        looks_reset();
    }
    if (openrac_game_moby_look(moby, item)) {
        look_item[slot] = (int8_t)item;
        look_class[slot] = GREF(int16_t, moby + MOBY_OCLASS);
    }
}

static void note_call(const Call* c) {
    if (own_spawn || c->moby == 0) {
        return;
    }
    last_call[c->kind] = *c;
    have_last_call[c->kind] = 1;
    if (call_count < CALLS_MAX) {
        calls[call_count++] = *c;
    }
}

static void copy4(float out[4], gaddr a) {
    if (a != 0) {
        memcpy(out, G(a), 16);
    } else {
        memset(out, 0, 16);
    }
}

/* ---- The rockets' trails ---- */

/* A ring for each moby slot, one block for each rocket (their rings start at different places). */
static gaddr trail_block[2];

/* The trail ring of the game's for a rocket of `kind`, 0 for another kind. */
static gaddr trail_rings(int kind) {
    if (kind == RAC2_SHOT_MINIROCKET) {
        return MINIROCKET_TRAILS;
    }
    return kind == RAC2_SHOT_RYNO ? RYNO_TRAILS : 0;
}

/* Gives the rocket `m` of `kind`, just made, the ring of its slot, its four points the one its
 * spawn wrote. */
static void trail_own(gaddr m, int kind) {
    const gaddr rings = trail_rings(kind);
    const int32_t slot = slot_of(m);
    const gaddr pvars = slot >= 0 ? rac2_moby_pvars(m, 1) : 0;
    if (rings == 0 || slot < 0 || pvars == 0) {
        return;
    }
    const int b = kind == RAC2_SHOT_RYNO;
    if (trail_block[b] == 0) {
        trail_block[b] = openrac_guest_static(SLOTS * TRAIL_SIZE + TRAIL_SIZE);
        if (trail_block[b] == 0) {
            return;
        }
    }
    /* The block's first ring at the game ring's distance from it, a whole number of rings. */
    const gaddr first = trail_block[b] + ((rings - trail_block[b]) & (TRAIL_SIZE - 1u));
    const gaddr ring = first + (gaddr)slot * TRAIL_SIZE;
    const gaddr made = rings + (gaddr)GREF(int32_t, pvars + TRAIL_INDEX) * TRAIL_SIZE;
    if (!rac2_memory_at(made, TRAIL_SIZE)) {
        return;
    }
    float point[4];
    memcpy(point, G(made), 16);
    for (int i = 0; i < 4; ++i) {
        memcpy(G(ring + (gaddr)i * 16u), point, 16);
    }
    GREF(int32_t, pvars + TRAIL_INDEX) = (int32_t)((ring - rings) / TRAIL_SIZE);
}

/* The rocket `m` of `kind` was moved by `by` after its spawn: its trail's points move with it. */
static void trail_moved(gaddr m, int kind, const float by[3]) {
    const gaddr rings = trail_rings(kind);
    const gaddr pvars = rac2_moby_slot(m) ? rac2_moby_pvars(m, 1) : 0;
    if (rings == 0 || pvars == 0) {
        return;
    }
    const gaddr ring = rings + (gaddr)GREF(int32_t, pvars + TRAIL_INDEX) * TRAIL_SIZE;
    if (!rac2_memory_at(ring, TRAIL_SIZE)) {
        return;
    }
    for (int i = 0; i < 4; ++i) {
        float p[3];
        read3(p, ring + (gaddr)i * 16u);
        for (int k = 0; k < 3; ++k) {
            p[k] += by[k];
        }
        write3(ring + (gaddr)i * 16u, p);
    }
}

/* ---- The spawns, wrapped ---- */

gaddr func_L00_003AB9C0(
    gaddr owner, gaddr pos, gaddr target, gaddr arg, float scale, float lo, float hi, float unused
) {
    const gaddr m = func_L00_003AB9C0__game(owner, pos, target, arg, scale, lo, hi, unused);
    trail_own(m, RAC2_SHOT_MINIROCKET);
    if (m != 0 && !own_spawn) {
        Call c = {RAC2_SHOT_MINIROCKET, m, owner, target, {0}, {0}, {scale, lo, hi, unused}, 0};
        copy4(c.v0, pos);
        copy4(c.v1, arg);
        note_call(&c);
    }
    return m;
}

gaddr func_L00_00409E48(gaddr owner, gaddr pos, gaddr target, gaddr color, float height) {
    const gaddr m = func_L00_00409E48__game(owner, pos, target, color, height);
    trail_own(m, RAC2_SHOT_RYNO);
    if (m != 0 && !own_spawn) {
        Call c = {RAC2_SHOT_RYNO, m, owner, target, {0}, {0}, {height, 0, 0, 0}, 0};
        copy4(c.v0, pos);
        copy4(c.v1, color);
        note_call(&c);
    }
    return m;
}

/* v2.00's takes a fifth argument (func_US2_L00_003AB8F8: the 5 v1.01 stores at pvars +0x78, which
 * its caller passes). */
#define SEEKER_ARGS RAC2_K(SEEKER_SPAWN_ARGS, 4, func_L00_003AA4C8)
#if SEEKER_ARGS == 5
#define SEEKER_SPAWN(pos, velocity, target, mode)                                                  \
    func_L00_003AA4C8__game((pos), (velocity), 0, (target), (mode))
gaddr func_L00_003AA4C8(gaddr pos, gaddr velocity, gaddr unused, gaddr target, int mode) {
    const gaddr m = func_L00_003AA4C8__game(pos, velocity, unused, target, mode);
#else
#define SEEKER_SPAWN(pos, velocity, target, mode)                                                  \
    ((void)(mode), func_L00_003AA4C8__game((pos), (velocity), 0, (target)))
gaddr func_L00_003AA4C8(gaddr pos, gaddr velocity, gaddr unused, gaddr target) {
    const int mode = 5;
    const gaddr m = func_L00_003AA4C8__game(pos, velocity, unused, target);
#endif
    if (m != 0 && !own_spawn) {
        Call c = {RAC2_SHOT_SEEKER, m, 0, target, {0}, {0}, {0}, mode};
        copy4(c.v0, pos);
        copy4(c.v1, velocity);
        note_call(&c);
    }
    return m;
}

gaddr func_L00_003A98E8(
    gaddr pos, gaddr velocity, gaddr owner, float speed, float life, float scale, int mode
) {
    const gaddr m = func_L00_003A98E8__game(pos, velocity, owner, speed, life, scale, mode);
    if (m != 0 && !own_spawn) {
        Call c = {RAC2_SHOT_HOVERBOMB, m, owner, 0, {0}, {0}, {speed, life, scale, 0}, mode};
        copy4(c.v0, pos);
        copy4(c.v1, velocity);
        note_call(&c);
    }
    return m;
}

gaddr func_L00_0038D530(gaddr pos, gaddr velocity) {
    const gaddr m = func_L00_0038D530__game(pos, velocity);
    if (m != 0 && !own_spawn) {
        Call c = {RAC2_SHOT_BOMB, m, 0, 0, {0}, {0}, {0}, 0};
        copy4(c.v0, pos);
        copy4(c.v1, velocity);
        note_call(&c);
    }
    return m;
}

gaddr func_L00_003EED10(int owner, gaddr pos, gaddr direction) {
    const gaddr m = func_L00_003EED10__game(owner, pos, direction);
    if (m != 0 && !own_spawn) {
        Call c = {RAC2_SHOT_GRAVITY_BOMB, m, (gaddr)owner, 0, {0}, {0}, {0}, 0};
        copy4(c.v0, pos);
        copy4(c.v1, direction);
        note_call(&c);
    }
    return m;
}

int func_L00_003FE2D0(int owner, int pos, int velocity) {
    const int m = func_L00_003FE2D0__game(owner, pos, velocity);
    if (m != 0 && !own_spawn) {
        Call c = {RAC2_SHOT_BOUNCER, (gaddr)m, (gaddr)owner, 0, {0}, {0}, {0}, 0};
        copy4(c.v0, (gaddr)pos);
        copy4(c.v1, (gaddr)velocity);
        note_call(&c);
    }
    return m;
}

gaddr func_L00_00398810(gaddr pos, gaddr direction) {
    const gaddr m = func_L00_00398810__game(pos, direction);
    if (m != 0 && !own_spawn) {
        Call c = {RAC2_SHOT_DECOY, m, 0, 0, {0}, {0}, {0}, 0};
        copy4(c.v0, pos);
        copy4(c.v1, direction);
        note_call(&c);
    }
    return m;
}

gaddr func_L00_00387CA8(gaddr pos, gaddr velocity) {
    const gaddr m = func_L00_00387CA8__game(pos, velocity);
    if (m != 0 && !own_spawn) {
        Call c = {RAC2_SHOT_MINITURRET, m, 0, 0, {0}, {0}, {0}, 0};
        copy4(c.v0, pos);
        copy4(c.v1, velocity);
        note_call(&c);
    }
    return m;
}

/* CreateMoby: what a moby's update makes takes that moby's look (its blast, its debris). */
gaddr func_0029EFD0(int oclass) {
    const gaddr m = func_0029EFD0__game(oclass);
    if (m != 0) {
        rac2_copy_slot_taken(m);
    }
    /* A new moby in a slot: the look of the one before is not its own. */
    if (m != 0 && looks_ready) {
        const int32_t slot = slot_of(m);
        if (slot >= 0) {
            look_item[slot] = -1;
        }
    }
    if (m != 0 && updating != 0) {
        updating_made = 1;
        if (openrac_game_moby_look != NULL) {
            give_look(m, look_of(updating));
        }
    }
    return m;
}

/* part_alloc: so does a particle (its trail, sparks and smoke, those that outlive it too). */
gaddr func_002B9030(int tag) {
    const gaddr record = func_002B9030__game(tag);
    if (record == 0 || openrac_game_particle_look == NULL) {
        return record;
    }
    const gaddr base = GREF(gaddr, PARTICLE_BASE);
    const uint32_t index = (record - base) / PARTICLE_SIZE;
    const int indexed = record >= base && index < PARTICLES;
    const int32_t item = updating != 0 ? look_of(updating) : -1;
    if (item >= 0) {
        openrac_game_particle_look(record, item);
        if (indexed) {
            particle_tagged[index / 8] |= (uint8_t)(1u << (index % 8));
        }
    } else if (!indexed || (particle_tagged[index / 8] & (1u << (index % 8))) != 0) {
        openrac_game_particle_look(record, -1);
        if (indexed) {
            particle_tagged[index / 8] &= (uint8_t)~(1u << (index % 8));
        }
    }
    return record;
}

/* ---- The statuses and the moby loop ---- */

typedef struct {
    gaddr moby;
    int16_t oclass;
    uint8_t status;
    int32_t ticks;
    int32_t impact; /* the impact entry it last ran for: index << 16 | damage's top bits, -1 none */
} Status;

static Status statuses[OPENRAC_MOBY_STATUS_MAX];
static uint32_t loop_ticks;

static Status* status_of(gaddr moby) {
    for (int i = 0; i < OPENRAC_MOBY_STATUS_MAX; ++i) {
        if (statuses[i].moby == moby && statuses[i].status != OPENRAC_MOBY_STATUS_NONE) {
            return &statuses[i];
        }
    }
    return NULL;
}

int rac2_moby_status(int32_t slot, uint32_t status, int32_t ticks) {
    const gaddr m = moby_at(slot);
    if (m == 0 || m == GREF(gaddr, RAC2_g_pHeroMoby) || status > OPENRAC_MOBY_STATUS_SLOW_THIRD) {
        return 0;
    }
    Status* s = status_of(m);
    if (status == OPENRAC_MOBY_STATUS_NONE || ticks <= 0) {
        if (s != NULL) {
            s->status = OPENRAC_MOBY_STATUS_NONE;
            s->moby = 0;
        }
        return 1;
    }
    if (s == NULL) {
        for (int i = 0; i < OPENRAC_MOBY_STATUS_MAX && s == NULL; ++i) {
            if (statuses[i].status == OPENRAC_MOBY_STATUS_NONE) {
                s = &statuses[i];
            }
        }
        if (s == NULL) {
            return 0;
        }
        s->impact = -1;
    }
    s->moby = m;
    s->oclass = GREF(int16_t, m + MOBY_OCLASS);
    s->status = (uint8_t)status;
    s->ticks = ticks;
    return 1;
}

/* The hit the impact queue holds for `moby`: its entry and damage, -1 for none. */
static int32_t pending_impact(gaddr moby) {
    const uint8_t index = GREF(uint8_t, moby + MOBY_IMPACT);
    if (index >= IMPACT_ENTRIES) {
        return -1;
    }
    const gaddr entry = IMPACT_QUEUE + (gaddr)index * IMPACT_SIZE;
    if (GREF(gaddr, entry + IMPACT_OWNER) != moby) {
        return -1;
    }
    return (int32_t)((uint32_t)index << 16 | (GREF(uint32_t, entry + IMPACT_DAMAGE) >> 16));
}

/* Whether a moby under a status runs this tick: a frozen one for a hit posted to it, a slowed one
 * one tick in two or three, or for a hit. */
static int status_runs(gaddr moby) {
    Status* s = status_of(moby);
    if (s == NULL) {
        return 1;
    }
    if (s->oclass != GREF(int16_t, moby + MOBY_OCLASS) || --s->ticks < 0) {
        s->status = OPENRAC_MOBY_STATUS_NONE;
        s->moby = 0;
        return 1;
    }
    const int32_t impact = pending_impact(moby);
    if (impact >= 0 && impact != s->impact) {
        s->impact = impact;
        return 1;
    }
    switch (s->status) {
        case OPENRAC_MOBY_STATUS_SLOW_HALF:
            return loop_ticks % 2u == 0;
        case OPENRAC_MOBY_STATUS_SLOW_THIRD:
            return loop_ticks % 3u == 0;
        default:
            return 0;
    }
}

static int any_status(void) {
    for (int i = 0; i < OPENRAC_MOBY_STATUS_MAX; ++i) {
        if (statuses[i].status != OPENRAC_MOBY_STATUS_NONE) {
            return 1;
        }
    }
    return 0;
}

static Shot* shot_of(gaddr moby) {
    for (int i = 0; i < shot_count; ++i) {
        if (shots[i].moby == moby && !shots[i].ended) {
            return &shots[i];
        }
    }
    return NULL;
}

/* A shot's moby died: why, and where. */
static void shot_died(Shot* s, int made) {
    s->ended = 1;
    const int kind = s->kind;
    s->reason = made ? OPENRAC_SHOT_END_IMPACT : OPENRAC_SHOT_END_GONE;
    /* No slot of the table now (a load moved it): gone, where it was last told. */
    if (!rac2_moby_slot(s->moby)) {
        return;
    }
    read3(s->last, s->moby + MOBY_POS);
    /* Its slot taken by a moby of another class: its words are the new one's, never read as the
     * shot's. A dead one's pvars word may be anything (a float, 0x4C58xxxx, on a new game started
     * from Yeedil): its timer is read only from pvars in memory. */
    const int same = GREF(int16_t, s->moby + MOBY_OCLASS) == s->oclass;
    const gaddr pvars = same ? rac2_moby_pvars(s->moby, 1) : 0;
    if (made
        || ((kind == RAC2_SHOT_MINIROCKET || kind == RAC2_SHOT_RYNO) && same
            && GREF(uint8_t, s->moby + MOBY_HIT) != 0)) {
        s->reason = OPENRAC_SHOT_END_IMPACT;
    } else if (KINDS[kind].timer >= 0 && pvars != 0
               && GREF(int16_t, pvars + (gaddr)KINDS[kind].timer) <= 0) {
        s->reason = OPENRAC_SHOT_END_EXPIRED;
    } else {
        s->reason = OPENRAC_SHOT_END_GONE;
    }
}

/* moby_update_live_mobys: links the live mobys that run into the loop's list, then runs each one:
 * its animation (unless flag 0x40), its update, its cells (unless flag 4). As the game's, with the
 * gate, the statuses and the moby that runs now. */
void func_L00_0032CF60(void) {
    gaddr last = 0;
    /* The copies' squads lists hold only live copies (enemy_copies.h); the death trace's look. */
    rac2_copy_groups_tidy();
    rac2_death_trace_tick();
    GREF(int32_t, LOOP_TARGETS) = 0;
    GREF(gaddr, LOOP_HEAD) = 0;
    for (gaddr scan = GREF(gaddr, LOOP_TABLE); GREF(uint8_t, scan + MOBY_STATE) != 0xFF;
         scan += MOBY_SIZE) {
        if ((GREF(uint8_t, scan + MOBY_STATE) & 0x80) == 0
            && (GREF(uint16_t, scan + MOBY_FLAGS) & FLAG_NOT_RUN) == 0) {
            if (last != 0) {
                GREF(gaddr, last + MOBY_NEXT) = scan;
            } else {
                GREF(gaddr, LOOP_HEAD) = scan;
            }
            last = scan;
        }
    }
    if (last != 0) {
        GREF(gaddr, last + MOBY_NEXT) = 0;
    }
    ++loop_ticks;
    const int statused = any_status();
    const int tracking = shot_count > 0;
    for (gaddr object = GREF(gaddr, LOOP_HEAD); object != 0;
         object = GREF(gaddr, object + MOBY_NEXT)) {
        if (GREF(int8_t, object + MOBY_STATE) < 0) {
            continue;
        }
        if (statused && !status_runs(object)) {
            continue;
        }
        int32_t slot = -1;
        if (openrac_game_moby_update != NULL) {
            slot = slot_of(object);
            if (slot >= 0 && openrac_game_moby_update(slot, 0) != 0) {
                continue;
            }
        }
        if ((GREF(uint16_t, object + MOBY_FLAGS) & FLAG_NO_ANIMATION) == 0) {
            func_002A05A8(object);
        }
        updating = object;
        updating_made = 0;
        const gaddr update = GREF(gaddr, object + MOBY_UPDATE);
        if (update != 0) {
            openrac_guest_call_a0(update, object);
        }
        updating = 0;
        if (GREF(int8_t, object + MOBY_STATE) < 0) {
            rac2_copy_gone(object);
        }
        if (tracking && GREF(int8_t, object + MOBY_STATE) < 0) {
            Shot* s = shot_of(object);
            if (s != NULL) {
                shot_died(s, updating_made);
            }
        }
        if ((GREF(uint16_t, object + MOBY_FLAGS) & FLAG_KEEP_MATRIX) == 0) {
            func_002A0F48(object);
        }
        if (slot >= 0) {
            openrac_game_moby_update(slot, 1);
        }
    }
}

/* ---- Firing again, and shots for a mod ---- */

/* Room for the vectors a spawn reads (two quadwords), in game memory. */
static gaddr scratch(void) {
    static gaddr room;
    if (room == 0) {
        room = openrac_guest_static(0x40);
    }
    return room;
}

static int may_make(void) {
    if (made_this_tick >= PER_TICK_MAX) {
        return 0;
    }
    if (pool_free < 0) {
        int32_t pool[5];
        pool_free = rac2_pool_status(pool) > 0 ? pool[2] : 0;
    }
    return pool_free > POOL_RESERVE;
}

/* Calls kind `c->kind`'s spawn with `c`'s arguments (its vectors put in game memory first). */
static gaddr spawn(const Call* c) {
    const gaddr room = scratch();
    const gaddr a0 = room, a1 = room + 0x10u;
    memcpy(G(a0), c->v0, 16);
    memcpy(G(a1), c->v1, 16);
    own_spawn = 1;
    gaddr m = 0;
    switch (c->kind) {
        case RAC2_SHOT_MINIROCKET:
            m = func_L00_003AB9C0__game(
                c->owner, a0, c->target, a1, c->f[0], c->f[1], c->f[2], c->f[3]
            );
            break;
        case RAC2_SHOT_RYNO:
            m = func_L00_00409E48__game(c->owner, a0, c->target, a1, c->f[0]);
            break;
        case RAC2_SHOT_SEEKER:
            m = SEEKER_SPAWN(a0, a1, c->target, c->i);
            break;
        case RAC2_SHOT_HOVERBOMB:
            m = func_L00_003A98E8__game(a0, a1, c->owner, c->f[0], c->f[1], c->f[2], c->i);
            break;
        case RAC2_SHOT_BOMB:
            m = func_L00_0038D530__game(a0, a1);
            break;
        case RAC2_SHOT_GRAVITY_BOMB:
            m = func_L00_003EED10__game((int)c->owner, a0, a1);
            break;
        case RAC2_SHOT_BOUNCER:
            m = (gaddr)func_L00_003FE2D0__game((int)c->owner, (int)a0, (int)a1);
            break;
        case RAC2_SHOT_DECOY:
            m = func_L00_00398810__game(a0, a1);
            break;
        case RAC2_SHOT_MINITURRET:
            m = func_L00_00387CA8__game(a0, a1);
            break;
        default:
            break;
    }
    own_spawn = 0;
    trail_own(m, c->kind);
    if (m != 0) {
        ++made_this_tick;
        if (pool_free > 0) {
            --pool_free;
        }
    }
    return m;
}

/* Turns a new shot's matrix rows and its pvars' direction (+0x30) by yaw and pitch. */
static void turn_matrix(gaddr m, float yaw, float pitch) {
    float forward[3];
    read3(forward, m + MOBY_FORWARD);
    static const unsigned rows[3] = {MOBY_FORWARD, MOBY_RIGHT, MOBY_UP};
    for (int r = 0; r < 3; ++r) {
        float v[3];
        read3(v, m + rows[r]);
        rac2_shot_turn(v, forward, yaw, pitch);
        write3(m + rows[r], v);
    }
    const gaddr pvars = rac2_moby_pvars(m, 1);
    if (pvars != 0) {
        float v[3];
        read3(v, pvars + 0x30u);
        rac2_shot_turn(v, forward, yaw, pitch);
        write3(pvars + 0x30u, v);
    }
}

/* A new shot's matrix and pvars' direction along `velocity`, and its speed (pvars +0x18) its
 * length: the rockets', which their spawn aims itself. */
static void aim_matrix(gaddr m, const float velocity[3]) {
    const float speed = length3(velocity);
    if (speed < 1e-6f) {
        return;
    }
    const float f[3] = {velocity[0] / speed, velocity[1] / speed, velocity[2] / speed};
    float r[3] = {f[1], -f[0], 0.0f};
    float rl = sqrtf(r[0] * r[0] + r[1] * r[1]);
    if (rl < 1e-6f) {
        r[0] = 1.0f;
        r[1] = 0.0f;
        rl = 1.0f;
    }
    r[0] /= rl;
    r[1] /= rl;
    const float u[3] =
        {r[1] * f[2] - r[2] * f[1], r[2] * f[0] - r[0] * f[2], r[0] * f[1] - r[1] * f[0]};
    write3(m + MOBY_FORWARD, f);
    write3(m + MOBY_RIGHT, r);
    write3(m + MOBY_UP, u);
    const gaddr pvars = rac2_moby_pvars(m, 1);
    if (pvars != 0) {
        write3(pvars + 0x30u, f);
        GREF(float, pvars + 0x18u) = speed;
    }
}

/* The shot told now (rac2_shots_frame), which rac2_shot_refire fires again. */
static const Call* firing;
static const Shot* firing_shot;

static Shot* track(gaddr m, int kind, int32_t item, uint32_t flags, gaddr owner, gaddr target) {
    if (shot_count >= SHOTS_MAX) {
        /* Drop the ended ones first; past that the oldest is no longer followed. */
        int k = 0;
        for (int i = 0; i < shot_count; ++i) {
            if (!shots[i].ended) {
                shots[k++] = shots[i];
            }
        }
        shot_count = k;
        if (shot_count >= SHOTS_MAX) {
            memmove(&shots[0], &shots[1], sizeof shots[0] * (SHOTS_MAX - 1));
            --shot_count;
        }
    }
    Shot* s = &shots[shot_count++];
    memset(s, 0, sizeof *s);
    s->moby = m;
    s->oclass = GREF(int16_t, m + MOBY_OCLASS);
    s->kind = (int8_t)kind;
    s->item = item;
    s->serial = next_serial++;
    if (next_serial == 0) {
        next_serial = 1;
    }
    s->flags = flags;
    s->owner = owner;
    s->target = target;
    return s;
}

int32_t rac2_shot_refire(uint32_t serial, float yaw, float pitch) {
    if (firing == NULL || firing_shot == NULL || firing_shot->serial != serial || !may_make()) {
        return -1;
    }
    Call c = *firing;
    if (KINDS[c.kind].takes_velocity) {
        rac2_shot_turn(c.v1, c.v1, yaw, pitch);
    }
    const gaddr m = spawn(&c);
    if (m == 0) {
        return -1;
    }
    if (!KINDS[c.kind].takes_velocity) {
        turn_matrix(m, yaw, pitch);
    }
    GREF(gaddr, m + MOBY_OWNER) = firing_shot->owner;
    track(m, c.kind, firing_shot->item, OPENRAC_WEAPON_SHOT_REFIRED, firing_shot->owner, c.target);
    give_look(m, firing_shot->item);
    return slot_of(m);
}

/* A RYNO II rocket made for a mod with a target homes on it from its first tick. Its update
 * (homing_l00_shot_update_3) steers only once its life (pvars +0x24, 300 at the spawn) is under
 * 295, and then only by the lead the target's motion asks for: a still target never turns it, so a
 * rocket not already headed at the point it seeks (the target, up by the height at +0x14, plus the
 * offset at +0x40) flies on past and turns only when the target moves, which looks like a rocket
 * waiting to choose. So it is aimed at that point at its speed, its height is the target's own
 * (pvars +0x10, as UpdateLockOnWeapon passes it), the point is its last seen (+0x00), its wobble
 * (+0x1C) is off and its life starts under the gate. */
#define RYNO_HOMING_LIFE 294

static void ryno_lock(gaddr m, gaddr pvars, gaddr target, const float velocity[3]) {
    /* Its target record only when it is one of RAM (an NG++ copy's pvars are the port's, and a
     * pvars word may hold a float: moby_checks.h); else the rocket's own height. */
    const gaddr info = rac2_target_record(target);
    const float height = info != 0 ? GREF(float, info + 0x10u) : GREF(float, pvars + 0x14u);
    float pos[3], up[3], offset[3], at[3];
    read3(pos, target + MOBY_POS);
    read3(up, target + MOBY_UP);
    read3(offset, pvars + 0x40u);
    float last[3];
    for (int i = 0; i < 3; ++i) {
        last[i] = pos[i] + up[i] * height;
        at[i] = last[i] + offset[i];
    }
    float from[3];
    read3(from, m + MOBY_POS);
    float speed = length3(velocity);
    if (speed < 1e-6f) {
        speed = GREF(float, pvars + 0x18u);
    }
    const float way[3] = {at[0] - from[0], at[1] - from[1], at[2] - from[2]};
    const float d = length3(way);
    if (d > 1e-4f && speed > 1e-6f) {
        const float v[3] = {way[0] / d * speed, way[1] / d * speed, way[2] / d * speed};
        aim_matrix(m, v);
    }
    write3(pvars + 0x00u, last);
    GREF(float, pvars + 0x14u) = height;
    GREF(float, pvars + 0x1Cu) = 0.0f;
    GREF(uint8_t, pvars + 0x26u) = 1;
    if (GREF(int16_t, pvars + 0x24u) > RYNO_HOMING_LIFE) {
        GREF(int16_t, pvars + 0x24u) = RYNO_HOMING_LIFE;
    }
}

int32_t rac2_shot_spawn_live(const openrac_game_spawn_live* in) {
    const int kind = rac2_shot_kind(in->oclass);
    if (kind < 0 || !rac2_in_play()) {
        return -1;
    }
    const gaddr gun = in->owner >= 0 ? moby_at(in->owner) : hand();
    if (gun == 0 || !may_make()) {
        return -1;
    }
    const gaddr target = in->target >= 0 ? moby_at(in->target) : 0;
    Call c;
    if (have_last_call[kind]) {
        c = last_call[kind];
    } else {
        memset(&c, 0, sizeof c);
        c.kind = kind;
        /* The two rockets' guns call with these (weapon_spawn_muzzle_effect, UpdateLockOnWeapon). */
        if (kind == RAC2_SHOT_MINIROCKET || kind == RAC2_SHOT_RYNO) {
            c.f[0] = 0.5f;
            c.f[1] = 2.0f;
            c.f[2] = 5.0f;
            c.f[3] = 0.6f;
        } else if (kind == RAC2_SHOT_SEEKER) {
            c.i = 5;
        } else if (kind == RAC2_SHOT_HOVERBOMB) {
            const gaddr record = rac2_weapon_record(KINDS[kind].item);
            c.f[1] = record != 0 ? GREF(float, record + 0x70u) : 1.0f;
            c.f[2] = 1.0f;
        }
    }
    c.moby = 0;
    c.owner = gun;
    c.target = target;
    float w = GREF(float, gun + MOBY_POS + 12u);
    memcpy(c.v0, in->position, 12);
    c.v0[3] = w;
    if (KINDS[kind].takes_velocity) {
        memcpy(c.v1, in->velocity, 12);
        c.v1[3] = 0.0f;
    }
    if (kind == RAC2_SHOT_HOVERBOMB) {
        c.f[0] = length3(in->velocity);
    }
    const gaddr m = spawn(&c);
    if (m == 0) {
        return -1;
    }
    if (!KINDS[kind].takes_velocity) {
        aim_matrix(m, in->velocity);
        /* The rockets' spawn tests a line from the gun to the place and, blocked, puts the rocket at
         * the hero to go off there: a rocket made away from the gun (a split, a burst) is where the
         * mod asked, and goes off only when asked. */
        float was[3];
        read3(was, m + MOBY_POS);
        write3(m + MOBY_POS, in->position);
        const float by[3] =
            {in->position[0] - was[0], in->position[1] - was[1], in->position[2] - was[2]};
        trail_moved(m, kind, by);
        GREF(uint8_t, m + MOBY_HIT) = (in->flags & OPENRAC_SPAWN_LIVE_DETONATE) != 0 ? 1 : 0;
        /* Their spawn only aims at the target and leaves the pvars' target (+0x10) empty, and their
         * update steers only after one (homing_l00_shot_update_3, from its sixth tick): a rocket the
         * mod gives a target is one that homes on it. */
        const gaddr pvars = rac2_moby_pvars(m, 1);
        if (target != 0 && pvars != 0 && KINDS[kind].target >= 0
            && GREF(gaddr, pvars + (gaddr)KINDS[kind].target) == 0) {
            GREF(gaddr, pvars + (gaddr)KINDS[kind].target) = target;
        }
        if (kind == RAC2_SHOT_RYNO && target != 0 && pvars != 0) {
            ryno_lock(m, pvars, target, in->velocity);
        }
    }
    GREF(gaddr, m + MOBY_OWNER) = gun;
    const int32_t item = in->item >= 0 && in->item < ITEMS ? in->item : hand_item();
    track(m, kind, item, OPENRAC_WEAPON_SHOT_LIVE, gun, target);
    give_look(m, item);
    return slot_of(m);
}

gaddr rac2_weapon_record(int32_t item) {
    if (!in_level() || item < 0 || item >= ITEMS) {
        return 0;
    }
    const uint8_t record = GREF(uint8_t, ITEM_RECORDS + (gaddr)item);
    return WEAPON_TABLE + (gaddr)record * WEAPON_RECORD_SIZE;
}

/* ---- A hit for a mod ---- */

void rac2_post_hit(
    gaddr victim,
    gaddr attacker,
    uint32_t mask,
    float damage,
    const float position[3],
    const float velocity[3]
) {
    static gaddr request;
    (void)velocity;
    if (!in_level() || !rac2_moby_live(victim)) {
        return;
    }
    if (request == 0) {
        request = openrac_guest_static(0x40);
    }
    /* make_hit_descriptor (func_002A8C00): the source quadword, the attacker, the mask, the damage
     * and the one at +0x20; the rest zero. */
    memset(G(request), 0, 0x40);
    write3(request, position);
    GREF(gaddr, request + 0x10u) = attacker;
    GREF(uint32_t, request + 0x14u) = mask;
    GREF(float, request + 0x1Cu) = damage;
    GREF(int32_t, request + 0x20u) = 1;
    func_002A8E20(victim, request);
}

/* ---- Once a tick ---- */

static void tell_end(Shot* s) {
    if (openrac_game_shot_ended == NULL) {
        return;
    }
    openrac_game_shot_end e;
    memset(&e, 0, sizeof e);
    e.item = s->item;
    e.slot = slot_of(s->moby);
    e.oclass = s->oclass;
    e.owner = slot_of(s->owner);
    e.target = slot_of(s->target);
    e.serial = s->serial;
    e.flags = s->flags;
    e.reason = s->reason;
    memcpy(e.position, s->last, sizeof e.position);
    openrac_game_shot_ended(&e);
}

static void tell_fire(const Shot* s, const Call* c, const float direction[3], float speed) {
    openrac_game_weapon_shot shot;
    memset(&shot, 0, sizeof shot);
    shot.item = s->item;
    shot.slot = slot_of(s->moby);
    shot.oclass = s->oclass;
    shot.owner = slot_of(s->owner);
    shot.target = slot_of(s->target);
    shot.serial = s->serial;
    shot.flags = s->flags;
    read3(shot.position, s->moby + MOBY_POS);
    memcpy(shot.direction, direction, sizeof shot.direction);
    shot.speed = speed;
    /* A refire adds to the table, which may move it: the callback reads a copy. */
    const Shot told = *s;
    firing = c;
    firing_shot = &told;
    openrac_game_weapon_fired(&shot);
    firing = NULL;
    firing_shot = NULL;
}

/* A shot's direction (a unit vector) and speed (units a tick), as its spawn left it. */
static void heading(const Call* c, gaddr m, float direction[3], float* speed) {
    float v[3];
    if (KINDS[c->kind].takes_velocity) {
        memcpy(v, c->v1, sizeof v);
    } else {
        read3(v, m + MOBY_FORWARD);
    }
    const float length = length3(v);
    for (int i = 0; i < 3; ++i) {
        direction[i] = length > 1e-6f ? v[i] / length : 0.0f;
    }
    *speed = KINDS[c->kind].takes_velocity ? length : 0.0f;
    if (!KINDS[c->kind].takes_velocity) {
        const gaddr pvars = rac2_moby_pvars(m, 1);
        *speed = pvars != 0 ? GREF(float, pvars + 0x18u) : 0.0f;
    }
}

static void level_left(void) {
    for (int i = 0; i < shot_count; ++i) {
        if (!shots[i].ended) {
            shots[i].ended = 1;
            shots[i].reason = OPENRAC_SHOT_END_LEVEL;
        }
        tell_end(&shots[i]);
    }
    shot_count = 0;
    call_count = 0;
    memset(have_last_call, 0, sizeof have_last_call);
    memset(statuses, 0, sizeof statuses);
    ammo_item = -1;
    looks_ready = 0;
}

void rac2_shots_frame(void) {
    const uint32_t loads = rac2_program_loads();
    if (loads != shots_loads || !in_level()) {
        shots_loads = loads;
        level_left();
        if (!in_level()) {
            return;
        }
    }
    made_this_tick = 0;
    pool_free = -1;
    /* Only in play: in a scene, a menu or a program's load (g_nGameState 6, a new game's from a
     * level among them) the table is the game's to rewrite and a shot's slot may hold anything. The
     * shots wait for play; the spawns noted meanwhile are not the hero's shots. */
    if (!rac2_in_play()) {
        call_count = 0;
        return;
    }
    /* The ends first: a shot can end in the tick it was made. Then any shot dead by other hands. */
    int k = 0;
    for (int i = 0; i < shot_count; ++i) {
        Shot* s = &shots[i];
        if (!s->ended && !alive(s->moby, s->oclass)) {
            shot_died(s, 0);
        }
        if (s->ended) {
            tell_end(s);
            continue;
        }
        shots[k++] = *s;
    }
    shot_count = k;
    /* The spawns of the tick: the hero's are told, and followed. */
    const gaddr gun = hand();
    const int32_t item = hand_item();
    int told_item = 0;
    const int count = call_count;
    call_count = 0;
    for (int i = 0; i < count; ++i) {
        const Call* c = &calls[i];
        const gaddr m = c->moby;
        if (gun == 0 || !alive(m, KINDS[c->kind].oclass)) {
            continue;
        }
        const gaddr owner = GREF(gaddr, m + MOBY_OWNER);
        if (owner != gun && c->owner != gun) {
            continue;
        }
        Shot* s = track(m, c->kind, item >= 0 ? item : KINDS[c->kind].item, 0, gun, c->target);
        if (s->target == 0 && KINDS[c->kind].target >= 0) {
            const gaddr pvars = rac2_moby_pvars(m, 1);
            const gaddr t = pvars != 0 ? GREF(gaddr, pvars + (gaddr)KINDS[c->kind].target) : 0;
            s->target = rac2_moby_slot(t) ? t : 0;
        }
        if (openrac_game_moby_look != NULL && look_of(m) < 0) {
            give_look(m, s->item);
        }
        told_item = 1;
        if (openrac_game_weapon_fired != NULL) {
            float direction[3], speed;
            heading(c, m, direction, &speed);
            tell_fire(s, c, direction, speed);
        }
    }
    /* A weapon with no shot of its own: its rounds spent this tick. */
    if (item >= 0) {
        const int32_t ammo = GREF(int32_t, AMMO + (gaddr)item * 4u);
        if (item == ammo_item && ammo < ammo_last && !told_item
            && openrac_game_weapon_fired != NULL) {
            openrac_game_weapon_shot shot;
            memset(&shot, 0, sizeof shot);
            shot.item = item;
            shot.slot = -1;
            shot.oclass = -1;
            shot.owner = slot_of(gun);
            shot.target = -1;
            shot.flags = OPENRAC_WEAPON_SHOT_ROUNDS;
            if (gun != 0) {
                read3(shot.position, gun + MOBY_POS);
                read3(shot.direction, HERO + 0x760u);
            }
            openrac_game_weapon_fired(&shot);
        }
        ammo_item = item;
        ammo_last = ammo;
    } else {
        ammo_item = -1;
    }
}
