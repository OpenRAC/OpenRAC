/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/* Going Commando's weapon shots for the mods (port/game/rac2/host/weapon_shots.c, interface 1.29) on
 * a level of the test's own: a shot turned by yaw and pitch; the hero's Minirocket Tube shot told
 * once, after the tick, with its item and serial, fired again from within the tell (the game's
 * spawn called again with the first call's arguments, the copy's matrix turned, its gun kept) and
 * refused outside it; another moby's shot not told; the ends (an impact, a timer run out); a shot
 * made live, and one that goes off at once; a weapon with no shot moby told by its rounds; the moby
 * loop's statuses (a frozen moby runs only for a new hit, a slowed one one tick in two) and the
 * mods' gate; the looks given to what a shot's update makes; a hit posted as the game posts one;
 * and no word of a moby followed unless it is memory, nor anything read or told outside play. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(c)                                                                                   \
    do {                                                                                           \
        if (!(c)) {                                                                                \
            fprintf(stderr, "%d: %s\n", __LINE__, #c);                                             \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)

#include "../game/rac2/host/weapon_shots.c"

/* enemy_copies.c's: no copies in this test. */
void rac2_copy_slot_taken(gaddr moby) {
    (void)moby;
}

/* The moby loop's calls into enemy_copies.c and death_trace.c: counted, nothing else. */
static int groups_tidied, gone_calls;

void rac2_copy_groups_tidy(void) {
    ++groups_tidied;
}

void rac2_copy_gone(gaddr moby) {
    (void)moby;
    ++gone_calls;
}

void rac2_death_trace_tick(void) {}

uint8_t* openrac_guest_base;
uint32_t openrac_guest_ram_size = 0x02000000u;
gaddr openrac_relocate_low = 0, openrac_relocate_high = 0;

gaddr openrac_relocate_data(gaddr a) {
    return a;
}

gaddr openrac_relocate_level_data(int from, gaddr a) {
    (void)from;
    return a;
}

static gaddr statics = 0x01C00000u;

gaddr openrac_guest_static(uint32_t size) {
    const gaddr at = statics;
    statics += (size + 15u) & ~15u;
    return at;
}

static int overlay = 4;

int openrac_game_loaded_overlay(void) {
    return overlay;
}

static uint32_t program_loads = 1;

uint32_t rac2_program_loads(void) {
    return program_loads;
}

static int32_t pool_free_slots = 300;

int rac2_pool_status(int32_t out[5]) {
    memset(out, 0, 5 * sizeof *out);
    out[2] = pool_free_slots;
    return 1;
}

/* ---- The level: a moby table, the hero, his gun ---- */

#define BASE 0x00400000u
#define SLOTS_IN 64u
#define END (BASE + SLOTS_IN * 0x100u)
#define PVARS_AT 0x00600000u
#define GUN_SLOT 1u
#define ENEMY_SLOT 2u
#define OTHER_GUN_SLOT 3u
#define UPDATE_ENEMY 0x00900000u
#define UPDATE_SHOT 0x00900100u

static void put32(gaddr a, uint32_t v) {
    memcpy(G(a), &v, 4);
}

static void put16(gaddr a, uint16_t v) {
    memcpy(G(a), &v, 2);
}

static void putf(gaddr a, float v) {
    memcpy(G(a), &v, 4);
}

static float getf(gaddr a) {
    float v;
    memcpy(&v, G(a), 4);
    return v;
}

static gaddr moby(uint32_t slot) {
    return BASE + slot * 0x100u;
}

static void level(void) {
    memset(G(BASE), 0, SLOTS_IN * 0x100u);
    for (uint32_t s = 0; s < SLOTS_IN; ++s) {
        GREF(uint8_t, moby(s) + MOBY_STATE) = 0xFE;
        put32(moby(s) + MOBY_PVARS, PVARS_AT + s * 0x100u);
    }
    GREF(uint8_t, END + MOBY_STATE) = 0xFF;
    put32(OPENRAC_DATA(RAC2_g_mobyTableBase), BASE);
    put32(OPENRAC_DATA(RAC2_g_mobyTableEnd), END);
    put32(OPENRAC_DATA(RAC2_g_nGameState), 0);
    put32(LOOP_TABLE, BASE);
    /* The hero's moby (slot 0), the gun in his hand (slot 1), an enemy (2), another moby's gun (3). */
    for (uint32_t s = 0; s < 4; ++s) {
        GREF(uint8_t, moby(s) + MOBY_STATE) = 1;
        put16(moby(s) + MOBY_OCLASS, (uint16_t)(100 + s));
    }
    put32(moby(ENEMY_SLOT) + MOBY_UPDATE, UPDATE_ENEMY);
    put32(RAC2_g_pHeroMoby, moby(0));
    put32(HERO_ITEM_MOBY, moby(GUN_SLOT));
    put32(HERO_ITEM_ID, 27);
    putf(moby(GUN_SLOT) + MOBY_POS + 12u, 1.0f);
    put32(PARTICLE_BASE, 0x00A00000u);
}

/* ---- The game's functions the host C calls ---- */

/* CreateMoby: the first free slot past the test's four, of class `oclass`. */
gaddr func_0029EFD0__game(int oclass) {
    for (uint32_t s = 4; s < SLOTS_IN; ++s) {
        if (GREF(uint8_t, moby(s) + MOBY_STATE) == 0xFE) {
            const gaddr m = moby(s);
            const gaddr pvars = GREF(gaddr, m + MOBY_PVARS);
            memset(G(m), 0, 0x100);
            put32(m + MOBY_PVARS, pvars);
            memset(G(pvars), 0, 0x100);
            GREF(uint8_t, m + MOBY_STATE) = 0;
            put16(m + MOBY_OCLASS, (uint16_t)oclass);
            put32(m + MOBY_UPDATE, UPDATE_SHOT);
            return m;
        }
    }
    return 0;
}

/* The spawns: what each was called with, and a shot as the game's leaves one. */
static int rocket_calls;
static gaddr rocket_owner, rocket_target;
static float rocket_pos[4], rocket_scale;

gaddr func_L00_003AB9C0__game(
    gaddr owner, gaddr pos, gaddr target, gaddr arg, float scale, float lo, float hi, float unused
) {
    (void)arg, (void)lo, (void)hi, (void)unused;
    ++rocket_calls;
    rocket_owner = owner;
    rocket_target = target;
    rocket_scale = scale;
    memcpy(rocket_pos, G(pos), 16);
    const gaddr m = func_0029EFD0(1498);
    if (m != 0) {
        memcpy(G(m + MOBY_POS), G(pos), 16);
        /* Along +x, level. */
        putf(m + MOBY_FORWARD, 1.0f);
        putf(m + MOBY_RIGHT + 4u, -1.0f);
        putf(m + MOBY_UP + 8u, 1.0f);
        const gaddr pvars = GREF(gaddr, m + MOBY_PVARS);
        putf(pvars + 0x30u, 1.0f);
        putf(pvars + 0x18u, 0.5f);
        put16(pvars + 0x24u, 150);
        put32(pvars + 0x10u, target);
    }
    return m;
}

/* The RYNO II's: as its spawn leaves one, the pvars' target empty, a wobble, 300 ticks of life,
 * the next of the game's 32 trail rings, its first point half a unit over the rocket. Blocked (its
 * line from the gun meets a wall), it is put at the hero, here the origin. */
static int ryno_ring, ryno_blocked;

gaddr func_L00_00409E48__game(gaddr owner, gaddr pos, gaddr target, gaddr color, float height) {
    (void)owner, (void)target, (void)color;
    const gaddr m = func_0029EFD0(3799);
    if (m != 0) {
        memcpy(G(m + MOBY_POS), G(pos), 16);
        if (ryno_blocked) {
            memset(G(m + MOBY_POS), 0, 12);
        }
        putf(m + MOBY_FORWARD, 1.0f);
        const gaddr pvars = GREF(gaddr, m + MOBY_PVARS);
        memset(G(pvars), 0, 0x58);
        putf(pvars + 0x14u, height);
        putf(pvars + 0x18u, 0.9f);
        putf(pvars + 0x1Cu, 7.0f);
        put16(pvars + 0x24u, 300);
        put32(pvars + TRAIL_INDEX, (uint32_t)ryno_ring);
        const gaddr point = RYNO_TRAILS + (gaddr)ryno_ring * TRAIL_SIZE;
        memcpy(G(point), G(m + MOBY_POS), 16);
        putf(point + 8u, getf(m + MOBY_POS + 8u) + 0.5f);
        ryno_ring = (ryno_ring + 1) % 32;
    }
    return m;
}

/* The trail ring a rocket's pvars name, as its update and draw reach it. */
static gaddr trail_of(gaddr m) {
    const gaddr pvars = GREF(gaddr, m + MOBY_PVARS);
    return RYNO_TRAILS + (gaddr)GREF(int32_t, pvars + TRAIL_INDEX) * TRAIL_SIZE;
}

/* Its four points all at x, y, z. */
static int trail_at(gaddr m, float x, float y, float z) {
    const gaddr ring = trail_of(m);
    for (int i = 0; i < 4; ++i) {
        const gaddr p = ring + (gaddr)i * 16u;
        if (fabsf(getf(p) - x) > 1e-5f || fabsf(getf(p + 4u) - y) > 1e-5f
            || fabsf(getf(p + 8u) - z) > 1e-5f) {
            return 0;
        }
    }
    return 1;
}

/* moby_get_private_value_0, as the game's: its pvars' first word when +0x34 has 0x20, no record
 * otherwise. It reads what it is given: pvars out of the test's RAM fault, as the port's did. */
gaddr func_002AB1F0(gaddr moby) {
    CHECK(moby >= BASE && moby < END);
    if ((GREF(uint16_t, moby + MOBY_FLAGS) & 0x20u) == 0) {
        return 0;
    }
    const gaddr pvars = GREF(gaddr, moby + MOBY_PVARS);
    CHECK(pvars < openrac_guest_ram_size);
    return GREF(gaddr, pvars);
}

gaddr func_L00_003AA4C8__game(gaddr a0, gaddr a1, gaddr a2, gaddr a3) {
    (void)a0, (void)a1, (void)a2, (void)a3;
    return 0;
}

static int hoverbomb_calls;
static float hoverbomb_velocity[4], hoverbomb_speed;

gaddr func_L00_003A98E8__game(
    gaddr pos, gaddr velocity, gaddr owner, float speed, float life, float scale, int mode
) {
    (void)owner, (void)life, (void)scale, (void)mode;
    ++hoverbomb_calls;
    memcpy(hoverbomb_velocity, G(velocity), 16);
    hoverbomb_speed = speed;
    const gaddr m = func_0029EFD0(1496);
    if (m != 0) {
        memcpy(G(m + MOBY_POS), G(pos), 16);
    }
    return m;
}

gaddr func_L00_0038D530__game(gaddr a0, gaddr a1) {
    (void)a0, (void)a1;
    return 0;
}

gaddr func_L00_003EED10__game(int a0, gaddr a1, gaddr a2) {
    (void)a0, (void)a1, (void)a2;
    return 0;
}

int func_L00_003FE2D0__game(int a0, int a1, int a2) {
    (void)a0, (void)a1, (void)a2;
    return 0;
}

gaddr func_L00_00398810__game(gaddr a0, gaddr a1) {
    (void)a0, (void)a1;
    return 0;
}

gaddr func_L00_00387CA8__game(gaddr a0, gaddr a1) {
    (void)a0, (void)a1;
    return 0;
}

static uint32_t particle_next;

gaddr func_002B9030__game(int tag) {
    (void)tag;
    return 0x00A00000u + 0x40u * (particle_next++ % 2048u);
}

void func_002A05A8(gaddr object) {
    (void)object;
}

void func_002A0F48(gaddr object) {
    (void)object;
}

static gaddr hit_victim, hit_request;

void func_002A8E20(gaddr victim, gaddr request) {
    hit_victim = victim;
    hit_request = request;
}

/* An update: the enemy's counts its runs; a shot's dies when told, making a blast first. */
static int enemy_runs;
static int shot_dies, shot_blasts;

uint64_t openrac_guest_call_regs(
    gaddr address, const uint64_t* ints, const float* floats, float* result, int float_result
) {
    (void)floats, (void)result, (void)float_result;
    const gaddr object = (gaddr)ints[0];
    if (address == UPDATE_ENEMY) {
        ++enemy_runs;
    } else if (address == UPDATE_SHOT && shot_dies) {
        if (shot_blasts) {
            func_0029EFD0(3301);
        }
        func_002B9030(0);
        GREF(uint8_t, object + MOBY_STATE) = 0xFD;
    }
    return 0;
}

/* ---- The mods' side ---- */

void (*openrac_game_weapon_fired)(const openrac_game_weapon_shot* shot) = NULL;
void (*openrac_game_shot_ended)(const openrac_game_shot_end* end) = NULL;
int (*openrac_game_moby_look)(gaddr moby, int32_t item) = NULL;
void (*openrac_game_particle_look)(gaddr record, int32_t item) = NULL;
int (*openrac_game_moby_update)(int32_t slot, int phase) = NULL;

static openrac_game_weapon_shot told[8];
static int told_count;
static int refire_in_tell;
static int32_t refired_slot = -1;

static void fired(const openrac_game_weapon_shot* shot) {
    if (told_count < 8) {
        told[told_count] = *shot;
    }
    ++told_count;
    if (refire_in_tell && shot->serial != 0) {
        refired_slot = rac2_shot_refire(shot->serial, 3.14159265f / 2.0f, 0.0f);
    }
}

static openrac_game_shot_end ends[8];
static int end_count;

static void ended(const openrac_game_shot_end* end) {
    if (end_count < 8) {
        ends[end_count] = *end;
    }
    ++end_count;
}

static int looked_mobys;
static gaddr looked_last;
static int32_t looked_item;

static int moby_look(gaddr m, int32_t item) {
    ++looked_mobys;
    looked_last = m;
    looked_item = item;
    return 1;
}

static int tagged, untagged;

static void particle_look(gaddr record, int32_t item) {
    (void)record;
    if (item >= 0) {
        ++tagged;
    } else {
        ++untagged;
    }
}

static int gate_calls;
static int32_t gate_skip = -1;

static int gate(int32_t slot, int phase) {
    ++gate_calls;
    return phase == 0 && slot == gate_skip;
}

/* ---- The checks ---- */

static void turns(void) {
    const float x[3] = {1.0f, 0.0f, 0.0f};
    float v[3] = {2.0f, 0.0f, 0.0f};
    rac2_shot_turn(v, x, 3.14159265f / 2.0f, 0.0f);
    CHECK(fabsf(v[0]) < 1e-5f && fabsf(v[1] - 2.0f) < 1e-5f && fabsf(v[2]) < 1e-5f);
    float u[3] = {1.0f, 0.0f, 0.0f};
    rac2_shot_turn(u, x, 0.0f, 3.14159265f / 6.0f);
    CHECK(fabsf(u[0] - cosf(3.14159265f / 6.0f)) < 1e-5f && fabsf(u[2] - 0.5f) < 1e-5f);
    /* Straight up has no axis across it: pitch leaves it. */
    const float z[3] = {0.0f, 0.0f, 1.0f};
    float w[3] = {0.0f, 0.0f, 1.0f};
    rac2_shot_turn(w, z, 0.3f, 0.5f);
    CHECK(fabsf(w[2] - 1.0f) < 1e-5f);
    CHECK(rac2_shot_kind(1498) == RAC2_SHOT_MINIROCKET && rac2_shot_kind(5) == -1);
    CHECK(rac2_shot_class(RAC2_SHOT_DECOY) == 203 && rac2_shot_class(99) == -1);
}

/* The gun in the hero's hand fires one rocket at the enemy, as weapon_spawn_muzzle_effect does. */
static gaddr fire(gaddr gun) {
    const gaddr pos = 0x00700000u;
    const float at[4] = {10.0f, 20.0f, 3.0f, 1.0f};
    memcpy(G(pos), at, 16);
    memset(G(pos + 0x10u), 0, 16);
    const gaddr m =
        func_L00_003AB9C0(gun, pos, moby(ENEMY_SLOT), pos + 0x10u, 0.25f, 2.0f, 5.0f, 0.6f);
    put32(m + MOBY_OWNER, gun);
    /* The caller's stack goes on: the noted call keeps its own copy. */
    memset(G(pos), 0, 16);
    return m;
}

static void tells_and_refires(void) {
    level();
    openrac_game_weapon_fired = fired;
    openrac_game_shot_ended = ended;
    rac2_shots_frame();
    told_count = 0;
    refire_in_tell = 1;
    rocket_calls = 0;
    const gaddr m = fire(moby(GUN_SLOT));
    /* Another moby's gun firing through the same spawn is not the hero's. */
    fire(moby(OTHER_GUN_SLOT));
    CHECK(told_count == 0);
    rac2_shots_frame();
    CHECK(told_count == 1);
    CHECK(told[0].item == 27 && told[0].oclass == 1498 && told[0].slot == slot_of(m));
    CHECK(told[0].owner == (int32_t)GUN_SLOT && told[0].target == (int32_t)ENEMY_SLOT);
    CHECK(told[0].serial != 0 && told[0].flags == 0 && told[0].position[0] == 10.0f);
    CHECK(fabsf(told[0].direction[0] - 1.0f) < 1e-5f && told[0].speed == 0.5f);
    /* The refire: the spawn again with what the first was given, its matrix turned a quarter. */
    CHECK(rocket_calls == 3 && refired_slot >= 0 && refired_slot != told[0].slot);
    const gaddr copy = moby((uint32_t)refired_slot);
    CHECK(rocket_owner == moby(GUN_SLOT) && rocket_target == moby(ENEMY_SLOT));
    CHECK(rocket_pos[0] == 10.0f && rocket_pos[1] == 20.0f && rocket_scale == 0.25f);
    CHECK(
        fabsf(getf(copy + MOBY_FORWARD)) < 1e-5f
        && fabsf(getf(copy + MOBY_FORWARD + 4u) - 1.0f) < 1e-5f
    );
    const gaddr pvars = GREF(gaddr, copy + MOBY_PVARS);
    CHECK(fabsf(getf(pvars + 0x34u) - 1.0f) < 1e-5f);
    CHECK(GREF(gaddr, copy + MOBY_OWNER) == moby(GUN_SLOT));
    /* Not told again, and no refire outside a tell. */
    told_count = 0;
    rac2_shots_frame();
    CHECK(told_count == 0);
    CHECK(rac2_shot_refire(told[0].serial, 0.1f, 0.0f) == -1);
    refire_in_tell = 0;
}

static void ends_told(void) {
    level();
    openrac_game_weapon_fired = fired;
    openrac_game_shot_ended = ended;
    rac2_shots_frame();
    told_count = 0;
    end_count = 0;
    const gaddr m = fire(moby(GUN_SLOT));
    rac2_shots_frame();
    CHECK(told_count == 1);
    /* Its update makes its blast and dies: an impact, where it was. */
    putf(m + MOBY_POS, 11.0f);
    shot_dies = 1;
    shot_blasts = 1;
    groups_tidied = 0;
    gone_calls = 0;
    func_L00_0032CF60();
    shot_dies = 0;
    CHECK(end_count == 0);
    /* The copies' squad lists tidied once before the loop; the shot that died told to
     * enemy_copies.c at once (a copy would come off its list then). */
    CHECK(groups_tidied == 1 && gone_calls == 1);
    rac2_shots_frame();
    CHECK(
        end_count == 1 && ends[0].reason == OPENRAC_SHOT_END_IMPACT && ends[0].position[0] == 11.0f
    );
    CHECK(ends[0].serial == told[0].serial && ends[0].item == 27 && ends[0].oclass == 1498);
    /* Another runs out of time: dies with its timer at 0 and no blast. */
    end_count = 0;
    const gaddr n = fire(moby(GUN_SLOT));
    rac2_shots_frame();
    put16(GREF(gaddr, n + MOBY_PVARS) + 0x24u, 0);
    shot_dies = 1;
    shot_blasts = 0;
    func_L00_0032CF60();
    shot_dies = 0;
    rac2_shots_frame();
    CHECK(end_count == 1 && ends[0].reason == OPENRAC_SHOT_END_EXPIRED);
    /* One left when the level goes is told as the level's. */
    end_count = 0;
    fire(moby(GUN_SLOT));
    rac2_shots_frame();
    ++program_loads;
    rac2_shots_frame();
    CHECK(end_count == 1 && ends[0].reason == OPENRAC_SHOT_END_LEVEL);
}

static void live(void) {
    level();
    rac2_shots_frame();
    /* A hoverbomb: the spawn takes the velocity and its length as the speed. */
    const openrac_game_spawn_live bomb =
        {1496, 25, -1, -1, 0, {1.0f, 2.0f, 3.0f}, {0.0f, 0.4f, 0.3f}};
    hoverbomb_calls = 0;
    const int32_t slot = rac2_shot_spawn_live(&bomb);
    CHECK(slot >= 0 && hoverbomb_calls == 1 && fabsf(hoverbomb_speed - 0.5f) < 1e-5f);
    CHECK(
        hoverbomb_velocity[1] == 0.4f
        && GREF(gaddr, moby((uint32_t)slot) + MOBY_OWNER) == moby(GUN_SLOT)
    );
    /* A rocket that goes off at once, aimed along its velocity, its length its speed. */
    const openrac_game_spawn_live burst =
        {1498,
         29,
         -1,
         (int32_t)ENEMY_SLOT,
         OPENRAC_SPAWN_LIVE_DETONATE,
         {5.0f, 5.0f, 1.0f},
         {0.0f, 0.0f, -0.2f}};
    const int32_t r = rac2_shot_spawn_live(&burst);
    CHECK(r >= 0 && GREF(uint8_t, moby((uint32_t)r) + MOBY_HIT) == 1);
    CHECK(fabsf(getf(moby((uint32_t)r) + MOBY_FORWARD + 8u) + 1.0f) < 1e-5f);
    CHECK(fabsf(getf(GREF(gaddr, moby((uint32_t)r) + MOBY_PVARS) + 0x18u) - 0.2f) < 1e-5f);
    /* A RYNO II rocket with a target homes from its first tick, aimed at the point it seeks. */
    const gaddr enemy = moby(ENEMY_SLOT);
    const float enemy_at[3] = {20.0f, 5.0f, 1.0f};
    memcpy(G(enemy + MOBY_POS), enemy_at, 12);
    memset(G(enemy + MOBY_UP), 0, 16);
    putf(enemy + MOBY_UP + 8u, 1.0f);
    const openrac_game_spawn_live split =
        {3799, 44, -1, (int32_t)ENEMY_SLOT, 0, {10.0f, 5.0f, 1.0f}, {0.0f, 1.2f, 0.0f}};
    const int32_t y = rac2_shot_spawn_live(&split);
    CHECK(y >= 0);
    const gaddr ym = moby((uint32_t)y);
    const gaddr yp = GREF(gaddr, ym + MOBY_PVARS);
    CHECK(GREF(gaddr, yp + 0x10u) == enemy && GREF(int16_t, yp + 0x24u) < 295);
    CHECK(GREF(uint8_t, yp + 0x26u) == 1 && getf(yp + 0x1Cu) == 0.0f);
    CHECK(fabsf(getf(ym + MOBY_FORWARD) - 1.0f) < 2e-3f && fabsf(getf(yp + 0x18u) - 1.2f) < 1e-5f);
    CHECK(fabsf(getf(yp + 0x30u) - 1.0f) < 2e-3f && getf(yp + 0x00u) == 20.0f);
    /* Without one it flies on as the game's unguided rockets do. Its spawn found the way from
     * the gun blocked and put it at the hero: it is where the mod asked, and so is its trail. */
    const openrac_game_spawn_live loose =
        {3799, 44, -1, -1, 0, {10.0f, 5.0f, 1.0f}, {0.0f, 1.2f, 0.0f}};
    ryno_blocked = 1;
    const int32_t z = rac2_shot_spawn_live(&loose);
    ryno_blocked = 0;
    CHECK(z >= 0);
    const gaddr zp = GREF(gaddr, moby((uint32_t)z) + MOBY_PVARS);
    CHECK(GREF(int16_t, zp + 0x24u) == 300 && fabsf(getf(zp + 0x34u) - 1.0f) < 1e-4f);
    /* Each rocket draws its own trail, of its slot's ring outside the game's 32, its four points
     * where it starts. */
    CHECK(trail_of(ym) != trail_of(moby((uint32_t)z)));
    CHECK(trail_of(ym) >= RYNO_TRAILS + 32u * TRAIL_SIZE);
    CHECK(trail_at(ym, 10.0f, 5.0f, 1.5f) && trail_at(moby((uint32_t)z), 10.0f, 5.0f, 1.5f));
    /* Past 32 alive, two rockets still never share one. */
    int32_t many[40];
    for (int i = 0; i < 40; ++i) {
        many[i] = rac2_shot_spawn_live(&loose);
        CHECK(many[i] >= 0);
    }
    for (int i = 0; i < 40; ++i) {
        for (int j = i + 1; j < 40; ++j) {
            CHECK(trail_of(moby((uint32_t)many[i])) != trail_of(moby((uint32_t)many[j])));
        }
    }
    /* Another class, and a pool too full. */
    const openrac_game_spawn_live other = {42, -1, -1, -1, 0, {0}, {0}};
    CHECK(rac2_shot_spawn_live(&other) == -1);
    rac2_shots_frame();
    pool_free_slots = POOL_RESERVE;
    CHECK(rac2_shot_spawn_live(&bomb) == -1);
    pool_free_slots = 300;
}

/* Words that are no pointers of memory, or no play: a dead shot's pvars word a float (a new game
 * started from Yeedil, 0x4C58xxxx), a target whose pvars are a copy's in the port's memory or a
 * float, the hand a float; and a program's load (g_nGameState 6), when nothing is read or told. Every
 * address the port would follow past the test's RAM faults, as the game's did. */
static void unchecked_words(void) {
    level();
    openrac_game_weapon_fired = fired;
    openrac_game_shot_ended = ended;
    rac2_shots_frame();
    /* A shot dies and its slot's pvars word is a float by the next frame: told gone, not read. */
    told_count = 0;
    end_count = 0;
    const gaddr a = fire(moby(GUN_SLOT));
    const gaddr b = fire(moby(GUN_SLOT));
    rac2_shots_frame();
    CHECK(told_count == 2);
    GREF(uint8_t, a + MOBY_STATE) = 0xFD;
    put32(a + MOBY_PVARS, 0x4C580168u);
    GREF(uint8_t, b + MOBY_STATE) = 0xFD;
    put16(b + MOBY_OCLASS, 77); /* taken by another class: its words are not the shot's */
    put32(b + MOBY_PVARS, 0x4C580168u);
    rac2_shots_frame();
    CHECK(end_count == 2 && ends[0].reason == OPENRAC_SHOT_END_GONE);
    CHECK(ends[1].reason == OPENRAC_SHOT_END_GONE);
    /* A RYNO II rocket made at an enemy whose pvars are a copy's, in the port's memory, or a float:
     * its target record is never read from them; the rocket takes its own height. */
    const gaddr enemy = moby(ENEMY_SLOT);
    put16(enemy + MOBY_FLAGS, 0x20u);
    const openrac_game_spawn_live split =
        {3799, 44, -1, (int32_t)ENEMY_SLOT, 0, {10.0f, 5.0f, 1.0f}, {0.0f, 1.2f, 0.0f}};
    put32(enemy + MOBY_PVARS, GUEST_PORT + 0x1000u);
    CHECK(rac2_shot_spawn_live(&split) >= 0);
    put32(enemy + MOBY_PVARS, 0x4333AC8Fu);
    CHECK(rac2_shot_spawn_live(&split) >= 0);
    put16(enemy + MOBY_FLAGS, 0);
    put32(enemy + MOBY_PVARS, PVARS_AT + ENEMY_SLOT * 0x100u);
    /* A program's load: a shot's slot word and the hand hold anything, nothing is read or told, and
     * no shot is made for a mod. */
    end_count = 0;
    told_count = 0;
    const gaddr c = fire(moby(GUN_SLOT));
    rac2_shots_frame();
    CHECK(told_count == 1);
    put32(OPENRAC_DATA(RAC2_g_nGameState), 6);
    GREF(uint8_t, c + MOBY_STATE) = 0xFD;
    put32(c + MOBY_PVARS, 0x4C580168u);
    put32(HERO_ITEM_MOBY, 0x4C580000u);
    fire(moby(GUN_SLOT));
    rac2_shots_frame();
    CHECK(end_count == 0 && told_count == 1);
    CHECK(rac2_shot_spawn_live(&split) == -1);
    /* Back in play: the shot is told gone; the hand a float is no gun, so nothing is told fired. */
    put32(OPENRAC_DATA(RAC2_g_nGameState), 0);
    fire(moby(GUN_SLOT));
    rac2_shots_frame();
    CHECK(end_count == 1 && ends[0].reason == OPENRAC_SHOT_END_GONE && told_count == 1);
    /* No hero in his table: no play either. */
    put32(HERO_ITEM_MOBY, moby(GUN_SLOT));
    put32(RAC2_g_pHeroMoby, 0);
    CHECK(rac2_shot_spawn_live(&split) == -1);
    put32(RAC2_g_pHeroMoby, moby(0));
    openrac_game_weapon_fired = NULL;
    openrac_game_shot_ended = NULL;
}

static void rounds(void) {
    level();
    openrac_game_weapon_fired = fired;
    put32(HERO_ITEM_ID, 30); /* the Lancer: no shot moby */
    put32(AMMO + 30u * 4u, 50);
    rac2_shots_frame();
    told_count = 0;
    put32(AMMO + 30u * 4u, 49);
    rac2_shots_frame();
    CHECK(told_count == 1 && told[0].item == 30 && told[0].slot == -1 && told[0].serial == 0);
    CHECK(told[0].flags == OPENRAC_WEAPON_SHOT_ROUNDS && told[0].owner == (int32_t)GUN_SLOT);
    rac2_shots_frame();
    CHECK(told_count == 1);
}

/* A hit for the frozen enemy, as QueueMobyImpact leaves one: its entry names it. */
static void hit_enemy(uint8_t index, float damage) {
    GREF(uint8_t, moby(ENEMY_SLOT) + MOBY_IMPACT) = index;
    const gaddr entry = IMPACT_QUEUE + index * IMPACT_SIZE;
    put32(entry + IMPACT_OWNER, moby(ENEMY_SLOT));
    putf(entry + IMPACT_DAMAGE, damage);
}

static void statuses_and_gate(void) {
    level();
    rac2_shots_frame();
    GREF(uint8_t, moby(ENEMY_SLOT) + MOBY_IMPACT) = 0xFF;
    enemy_runs = 0;
    func_L00_0032CF60();
    CHECK(enemy_runs == 1);
    /* Frozen: no update, no animation, but one for each new hit. */
    CHECK(rac2_moby_status((int32_t)ENEMY_SLOT, OPENRAC_MOBY_STATUS_FREEZE, 10) == 1);
    CHECK(rac2_moby_status(0, OPENRAC_MOBY_STATUS_FREEZE, 10) == 0); /* the hero */
    enemy_runs = 0;
    func_L00_0032CF60();
    func_L00_0032CF60();
    CHECK(enemy_runs == 0);
    hit_enemy(5, 10.0f);
    func_L00_0032CF60();
    func_L00_0032CF60();
    CHECK(enemy_runs == 1);
    hit_enemy(6, 12.0f);
    func_L00_0032CF60();
    CHECK(enemy_runs == 2);
    /* Its time runs out. */
    for (int i = 0; i < 10; ++i) {
        func_L00_0032CF60();
    }
    const int before = enemy_runs;
    func_L00_0032CF60();
    CHECK(enemy_runs == before + 1);
    /* Slowed to half: one tick in two. */
    GREF(uint8_t, moby(ENEMY_SLOT) + MOBY_IMPACT) = 0xFF;
    CHECK(rac2_moby_status((int32_t)ENEMY_SLOT, OPENRAC_MOBY_STATUS_SLOW_HALF, 100) == 1);
    enemy_runs = 0;
    for (int i = 0; i < 10; ++i) {
        func_L00_0032CF60();
    }
    CHECK(enemy_runs == 5);
    CHECK(rac2_moby_status((int32_t)ENEMY_SLOT, OPENRAC_MOBY_STATUS_NONE, 0) == 1);
    enemy_runs = 0;
    func_L00_0032CF60();
    CHECK(enemy_runs == 1);
    /* The gate: before and after each update; 1 before skips it. */
    openrac_game_moby_update = gate;
    gate_calls = 0;
    gate_skip = (int32_t)ENEMY_SLOT;
    enemy_runs = 0;
    func_L00_0032CF60();
    CHECK(enemy_runs == 0 && gate_calls > 0);
    gate_skip = -1;
    func_L00_0032CF60();
    CHECK(enemy_runs == 1);
    openrac_game_moby_update = NULL;
}

static void looks(void) {
    level();
    rac2_shots_frame();
    particle_next = 0;
    openrac_game_moby_look = moby_look;
    openrac_game_particle_look = particle_look;
    looked_mobys = 0;
    tagged = untagged = 0;
    /* A shot the gun in hand makes in its update takes the item's look, and so does its blast. */
    updating = moby(GUN_SLOT);
    const gaddr m = fire(moby(GUN_SLOT));
    func_002B9030(0);
    updating = 0;
    CHECK(looked_mobys == 1 && looked_last == m && looked_item == 27 && tagged == 1);
    rac2_shots_frame();
    CHECK(looked_mobys == 1);
    shot_dies = 1;
    shot_blasts = 1;
    func_L00_0032CF60();
    shot_dies = 0;
    CHECK(looked_mobys == 2 && GREF(int16_t, looked_last + MOBY_OCLASS) == 3301 && tagged == 2);
    /* A particle of no look's moby in a tagged record unties it. */
    particle_next = 0;
    func_002B9030(0);
    CHECK(untagged == 1);
    openrac_game_moby_look = NULL;
    openrac_game_particle_look = NULL;
}

static void posts_hits(void) {
    level();
    const float at[3] = {1.0f, 2.0f, 3.0f};
    const float velocity[3] = {0};
    rac2_post_hit(moby(ENEMY_SLOT), moby(GUN_SLOT), 0x830000u, 7.5f, at, velocity);
    CHECK(hit_victim == moby(ENEMY_SLOT) && hit_request != 0);
    CHECK(getf(hit_request + 4u) == 2.0f && GREF(gaddr, hit_request + 0x10u) == moby(GUN_SLOT));
    CHECK(GREF(uint32_t, hit_request + 0x14u) == 0x830000u && getf(hit_request + 0x1Cu) == 7.5f);
    CHECK(GREF(int32_t, hit_request + 0x20u) == 1);
    CHECK(rac2_weapon_record(27) == WEAPON_TABLE + GREF(uint8_t, ITEM_RECORDS + 27u) * 0xE0u);
    CHECK(rac2_weapon_record(64) == 0);
}

int main(void) {
    openrac_guest_base = calloc(1, openrac_guest_ram_size);
    CHECK(openrac_guest_base != NULL);
    turns();
    tells_and_refires();
    ends_told();
    live();
    unchecked_words();
    rounds();
    statuses_and_gate();
    looks();
    posts_hits();
    puts("RAC2 weapon shots: OK");
    return 0;
}
