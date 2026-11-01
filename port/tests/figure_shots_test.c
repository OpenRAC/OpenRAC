/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * The figures' shots (rac1/host/figure_shots.c, PAL's table): the hero's noted once, with what the
 * other games need; a figure's made by the game's spawn with the figure as the owner, past the held
 * state of what a glove throws, hitting nothing; refused, with nothing made, when the class cannot
 * be had or the level has no moby to spare; the tracks of what cannot be replayed, polled from the
 * hero and shown here as particles or a moby that never runs.
 */
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
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif
#include "../game/rac1/host/figure_shots.c"

/* Guest memory: the first 4 MB at their addresses; mobys at MOBYS, their pvars at PVARS. */
enum {
    RAM = 0x400000,
    MOBYS = 0x200000,
    PVARS = 0x300000,
    STACK = 0x3F0000,
    FIGURE = MOBYS,
    OTHER_FIGURE = MOBYS + 0x100,
    HERO_M = MOBYS + 0x200,
    HAND = MOBYS + 0x300,
    SPAWNED = MOBYS + 0x400, /* the pool from here */
    POOL = 64,
    TARGET = MOBYS + 0x7000,
    TARGET_KEY = 21
};

uint8_t* openrac_guest_base;
gaddr openrac_guest_sp = STACK;
gaddr openrac_relocate_low = 0, openrac_relocate_high = 0;

gaddr openrac_guest_frame(uint32_t size) {
    openrac_guest_sp -= (size + 15u) & ~15u;
    return openrac_guest_sp;
}

gaddr openrac_relocate_data(gaddr a) {
    return a;
}

gaddr openrac_relocate_level_data(int from, gaddr a) {
    (void)from;
    return a;
}

static gaddr next_moby;
static int level_on = 1, class_ready = 1, game_calls, figure_gone, updating;
static int flames, sparks, bursts, drops, track_mobys;
static gaddr last_owner, last_target;
static float last_args[5];
static float placed[3];

int rac1_level_running(void) {
    return level_on;
}

uint32_t rac1_moby_generation(void) {
    return 5;
}

int rac1_figure_items_debug(void) {
    return 0;
}

int rac1_figure_alive(gaddr moby) {
    return !figure_gone && (moby == FIGURE || moby == OTHER_FIGURE);
}

gaddr rac1_updating_moby(void) {
    return (gaddr)updating;
}

int32_t rac1_world_moby_key(gaddr moby) {
    return moby == TARGET ? TARGET_KEY : -1;
}

int32_t rac1_world_moby_slot(int32_t key) {
    return key == TARGET_KEY ? 7 : -1;
}

gaddr rac1_world_moby_at(int32_t slot) {
    return slot == 7 ? TARGET : 0;
}

int rac1_figure_class_ready(int32_t oclass) {
    (void)oclass;
    return class_ready;
}

static gaddr make_moby(int32_t oclass) {
    const gaddr m = next_moby;
    next_moby += MOBY_STRIDE;
    GREF(uint8_t, m + MOBY_STATE) = 0;
    GREF(int16_t, m + MOBY_OCLASS) = (int16_t)oclass;
    GREF(gaddr, m + MOBY_PVARS) = PVARS + (m - MOBYS);
    GREF(float, m + MOBY_SCALE) = 0.65f;
    return m;
}

gaddr rac1_figure_track_moby(gaddr figure, int32_t oclass) {
    (void)figure;
    ++track_mobys;
    return make_moby(oclass);
}

void rac1_figure_track_moby_place(gaddr moby, const float* position) {
    (void)moby;
    memcpy(placed, position, sizeof placed);
}

void rac1_figure_track_moby_drop(gaddr moby, int32_t oclass) {
    (void)oclass;
    GREF(uint8_t, moby + MOBY_STATE) = 0xFE;
    ++drops;
}

/* The game's spawns: each a moby of its class, with its owner where the game puts it. */
static gaddr spawned(int32_t oclass, gaddr owner, unsigned at) {
    const gaddr m = make_moby(oclass);
    GREF(gaddr, GREF(gaddr, m + MOBY_PVARS) + at) = owner;
    last_owner = owner;
    return m;
}

gaddr func_L00_002CC3C0__game(gaddr o, gaddr p, gaddr t, float a, float b, float c, float d) {
    (void)p, (void)a, (void)b, (void)c, (void)d;
    ++game_calls;
    last_target = t;
    return spawned(CLASS_HOMING_SHOT, o, 0x20u);
}

gaddr func_L00_002AE110(int o, gaddr p) {
    (void)p;
    ++game_calls;
    return spawned(CLASS_BOMB, (gaddr)o, 0x50u);
}

void func_L00_002AE278__game(gaddr pos, gaddr bomb, gaddr velocity) {
    /* launch_bomb adds this hero's motion. */
    const gaddr pvars = GREF(gaddr, bomb + MOBY_PVARS);
    (void)pos;
    for (unsigned c = 0; c < 3; ++c) {
        GREF(float, pvars + 4u * c) = GREF(float, velocity + 4u * c) + 100.0f;
    }
}

gaddr func_L00_002B0F58__game(
    gaddr o, gaddr p, gaddr t, float a, float b, float h, float c, float d
) {
    (void)p;
    ++game_calls;
    last_target = t;
    last_args[0] = a, last_args[1] = b, last_args[2] = h, last_args[3] = c, last_args[4] = d;
    return spawned(CLASS_MISSILE, o, 0x10u);
}

gaddr func_L00_002CE390__game(int o, gaddr p, gaddr t, float h, float a, float b) {
    (void)p;
    ++game_calls;
    last_target = t;
    last_args[0] = h, last_args[1] = a, last_args[2] = b;
    return spawned(CLASS_RYNO_ROCKET, (gaddr)o, 0x20u);
}

void func_L00_002C96D0__game(gaddr pos, gaddr moby, gaddr velocity) {
    (void)pos, (void)moby, (void)velocity;
    ++game_calls;
}

void func_L00_002AB2A8__game(gaddr pos, gaddr moby, gaddr velocity) {
    (void)pos, (void)moby, (void)velocity;
    ++game_calls;
}

void func_L00_002C4B90__game(gaddr pos, gaddr moby, gaddr velocity) {
    (void)pos, (void)moby, (void)velocity;
    ++game_calls;
}

gaddr func_L00_002C9578(int o, gaddr p, gaddr v) {
    (void)p, (void)v;
    ++game_calls;
    return spawned(CLASS_DOOM_THROWN, (gaddr)o, 0x30u);
}

gaddr func_L00_002AB170(int o, gaddr p) {
    (void)p;
    ++game_calls;
    return spawned(CLASS_MINE, (gaddr)o, 0x30u);
}

gaddr func_L00_002C48C8(int o, gaddr p, gaddr v) {
    (void)p, (void)v;
    ++game_calls;
    return spawned(CLASS_DECOY, (gaddr)o, 0x40u);
}

gaddr func_L00_0026C0D0(gaddr p, gaddr v, unsigned char flags, float size) {
    (void)p, (void)v, (void)flags, (void)size;
    ++flames;
    return 0;
}

void func_L00_0026E438(gaddr p, gaddr v, int flag) {
    (void)p, (void)v, (void)flag;
    ++sparks;
}

void func_L00_002B6FF0(gaddr moby, int flag) {
    (void)moby, (void)flag;
    ++bursts;
}

static int radial;

void func_L00_0025BA50__game(
    gaddr a, gaddr b, gaddr c, int d, gaddr e, int f, int g, int h, float i, float j, float k
) {
    (void)a, (void)b, (void)c, (void)d, (void)e, (void)f, (void)g, (void)h, (void)i, (void)j, (void)k;
    ++radial;
}

static void set_vector(gaddr at, float x, float y, float z) {
    GREF(float, at) = x;
    GREF(float, at + 4u) = y;
    GREF(float, at + 8u) = z;
}

static void reset(void) {
    memset(openrac_guest_base + MOBYS, 0, RAM - MOBYS);
    memset(shots, 0, sizeof shots);
    memset(tracks, 0, sizeof tracks);
    memset(tracks_out, 0, sizeof tracks_out);
    memset(live, 0, sizeof live);
    shot_count = 0;
    next_moby = SPAWNED;
    game_calls = flames = sparks = bursts = drops = track_mobys = 0;
    figure_gone = 0;
    class_ready = 1;
    level_on = 1;
    updating = 0;
    radial = 0;
    last_target = 0;
    /* The hero, his hand, the figures, a free pool that nothing has used. */
    GREF(gaddr, HERO_MOBY) = HERO_M;
    GREF(gaddr, HERO_SLOTS) = HAND;
    GREF(int32_t, CONTEXT) = 0;
    GREF(uint8_t, HAND + MOBY_STATE) = 2;
    GREF(int16_t, HAND + MOBY_OCLASS) = 8;
    GREF(gaddr, HAND + MOBY_PVARS) = PVARS + 0x300;
    GREF(gaddr, HERO_VISIBOMB) = 0;
    set_vector(HERO_M + MOBY_POS, 100.0f, 200.0f, 30.0f);
    set_vector(HAND + MOBY_POS, 101.0f, 200.0f, 31.0f);
    set_vector(HERO_FACING, 1.0f, 0.0f, 0.0f);
    set_vector(FIGURE + MOBY_POS, 10.0f, 20.0f, 3.0f);
    GREF(gaddr, MOBY_END) = SPAWNED;
    GREF(gaddr, MOBY_PERM_END) = SPAWNED + POOL * MOBY_STRIDE;
    for (uint32_t i = 0; i < POOL; ++i) {
        GREF(uint8_t, SPAWNED + i * MOBY_STRIDE + MOBY_STATE) = 0xFE;
    }
    GREF(uint8_t, TARGET + MOBY_STATE) = 1;
}

static openrac_game_shot2 take_one(void) {
    openrac_game_shot2 s;
    CHECK(rac1_hero_shots2(&s, 1) == 1);
    return s;
}

static void test_the_hero_s_devastator_goes_with_its_target(void) {
    reset();
    GFRAME(v, 16);
    set_vector(v, 1.0f, 2.0f, 3.0f);
    func_L00_002B0F58(HAND, v, TARGET, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f);
    CHECK(game_calls == 1);
    const openrac_game_shot2 s = take_one();
    CHECK(s.item == ITEM_DEVASTATOR && s.flags == 0 && s.target_key == TARGET_KEY);
    CHECK(s.position[2] == 3.0f && s.aim[0] == 0.1f && s.aim[3] == 0.5f && s.aux[AUX_HEIGHT] == 0.3f);

    /* Fired by no hand of the hero's (an enemy's), not his shot. */
    func_L00_002B0F58(MOBYS + 0x5000, v, 0, 0, 0, 0, 0, 0);
    CHECK(rac1_hero_shots2(shots, 0) == 0 && shot_count == 0);

    /* A figure's, made by the same call: the figure the owner, the same target, not noted. */
    game_calls = 0;
    CHECK(rac1_figure_shot2(FIGURE, &s));
    CHECK(game_calls == 1 && last_owner == FIGURE && last_target == TARGET);
    CHECK(last_args[0] == 0.1f && last_args[2] == 0.3f && last_args[4] == 0.5f);
    CHECK(shot_count == 0);
    CHECK(GREF(float, v) == 1.0f); /* the caller's stack is not this frame */
}

static void test_a_target_gone_flies_on_its_heading(void) {
    reset();
    openrac_game_shot2 s;
    clear_shot(&s, ITEM_RYNO);
    s.target_key = TARGET_KEY;
    GREF(uint8_t, TARGET + MOBY_STATE) = 0xFE;
    CHECK(rac1_figure_shot2(FIGURE, &s));
    CHECK(last_target == 0);
}

static void test_the_blaster_homes_as_the_shooter_s(void) {
    reset();
    GFRAME(v, 16);
    func_L00_002CC3C0(HERO_M, v, TARGET, 0, 0, 0, 0);
    const openrac_game_shot2 s = take_one();
    CHECK(s.item == ITEM_BLASTER && s.target_key == TARGET_KEY);
    last_target = 0;
    CHECK(rac1_figure_shot2(FIGURE, &s));
    CHECK(last_target == TARGET && last_owner == FIGURE && shot_count == 0);
    /* The old interface's shot has no target. */
    openrac_game_shot old;
    memset(&old, 0, sizeof old);
    old.item = ITEM_BLASTER;
    last_target = 123;
    CHECK(rac1_figure_shot(FIGURE, &old));
    CHECK(last_target == 0);
}

static void test_a_glove_s_throw_is_past_its_held_state(void) {
    struct {
        int32_t item;
        void (*release)(gaddr, gaddr, gaddr);
        int32_t oclass;
        unsigned velocity;
        uint8_t state;
    } gloves[] = {
        {ITEM_GLOVE_OF_DOOM, func_L00_002C96D0, CLASS_DOOM_THROWN, 0, 2},
        {ITEM_MINE_GLOVE, func_L00_002AB2A8, CLASS_MINE, 0, 1},
        {ITEM_DECOY_GLOVE, func_L00_002C4B90, CLASS_DECOY, DECOY_VELOCITY, 2},
    };
    for (size_t i = 0; i < sizeof gloves / sizeof gloves[0]; ++i) {
        reset();
        /* What the hero's release left. */
        const gaddr held = make_moby(gloves[i].oclass);
        const gaddr pvars = GREF(gaddr, held + MOBY_PVARS);
        set_vector(held + MOBY_POS, 5.0f, 6.0f, 7.0f);
        set_vector(pvars + gloves[i].velocity, 0.5f, 0.25f, 0.125f);
        GREF(int16_t, pvars + MINE_TIMER) = 120;
        gloves[i].release(0, held, 0);
        const openrac_game_shot2 s = take_one();
        CHECK(s.item == gloves[i].item && s.position[1] == 6.0f && s.velocity[2] == 0.125f);

        game_calls = 0;
        CHECK(rac1_figure_shot2(FIGURE, &s));
        CHECK(game_calls == 1 && last_owner == FIGURE && shot_count == 0);
        const gaddr made = next_moby - MOBY_STRIDE;
        CHECK(GREF(uint8_t, made + MOBY_STATE) == gloves[i].state);
        CHECK(GREF(float, made + MOBY_POS + 4u) == 6.0f);
        const gaddr made_pvars = GREF(gaddr, made + MOBY_PVARS);
        CHECK(GREF(float, made_pvars + gloves[i].velocity) == 0.5f);
        if (gloves[i].item == ITEM_MINE_GLOVE) {
            CHECK(GREF(int16_t, made_pvars + MINE_TIMER) == 120);
            CHECK(GREF(float, made + MOBY_SCALE) == 1.0f);
        }
    }
}

static void test_a_bomb_goes_as_the_shooter_s_went(void) {
    reset();
    openrac_game_shot2 s;
    clear_shot(&s, ITEM_BOMB_GLOVE);
    s.position[0] = 8.0f;
    s.velocity[0] = 0.5f;
    CHECK(rac1_figure_shot2(FIGURE, &s));
    const gaddr made = next_moby - MOBY_STRIDE;
    CHECK(GREF(float, GREF(gaddr, made + MOBY_PVARS)) == 0.5f);
    CHECK(GREF(float, made + MOBY_POS) == 8.0f);
    CHECK(shot_count == 0);
}

static void test_a_figure_s_shot_hits_nothing(void) {
    static const int items[] = {
        ITEM_BLASTER,
        ITEM_BOMB_GLOVE,
        ITEM_DEVASTATOR,
        ITEM_RYNO,
        ITEM_GLOVE_OF_DOOM,
        ITEM_MINE_GLOVE,
        ITEM_DECOY_GLOVE,
    };
    for (size_t i = 0; i < sizeof items / sizeof items[0]; ++i) {
        reset();
        openrac_game_shot2 s;
        clear_shot(&s, items[i]);
        CHECK(rac1_figure_shot2(FIGURE, &s));
        const gaddr made = next_moby - MOBY_STRIDE;
        CHECK(rac1_figure_owned(made));
        /* Its blast sends none. */
        updating = (int)made;
        func_L00_0025BA50(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        CHECK(radial == 0);
        /* Still so when its player has gone: it must not start to count. */
        figure_gone = 1;
        CHECK(rac1_figure_owned(made));
    }
    /* The hero's own is not a figure's, and hits. */
    reset();
    const gaddr own = make_moby(CLASS_MISSILE);
    GREF(gaddr, GREF(gaddr, own + MOBY_PVARS) + 0x10u) = HAND;
    CHECK(!rac1_figure_owned(own));
    updating = (int)own;
    func_L00_0025BA50(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    CHECK(radial == 1);
    /* Nor is a moby of any other class. */
    const gaddr crate = make_moby(500);
    GREF(gaddr, GREF(gaddr, crate + MOBY_PVARS) + 0x10u) = FIGURE;
    CHECK(!rac1_figure_owned(crate));
}

static void test_refused_with_nothing_made(void) {
    openrac_game_shot2 s;
    /* The class cannot be had. */
    reset();
    clear_shot(&s, ITEM_DEVASTATOR);
    class_ready = 0;
    CHECK(!rac1_figure_shot2(FIGURE, &s));
    CHECK(game_calls == 0);

    /* The pool has too few free slots left for the game. */
    reset();
    clear_shot(&s, ITEM_DEVASTATOR);
    for (uint32_t i = 0; i < POOL - (SPARE_LEFT - 1); ++i) {
        GREF(uint8_t, SPAWNED + i * MOBY_STRIDE + MOBY_STATE) = 1;
    }
    CHECK(!rac1_figure_shot2(FIGURE, &s));
    CHECK(game_calls == 0);
    GREF(uint8_t, SPAWNED + MOBY_STATE) = 0xFE;
    CHECK(rac1_figure_shot2(FIGURE, &s));

    /* Never used from here on: free, every one. */
    reset();
    clear_shot(&s, ITEM_DEVASTATOR);
    GREF(uint8_t, SPAWNED + MOBY_STATE) = MOBY_LAST;
    CHECK(rac1_figure_shot2(FIGURE, &s));

    /* No figure, a menu's mobys up, out of a level, an item with nothing to show. */
    reset();
    clear_shot(&s, ITEM_DEVASTATOR);
    CHECK(!rac1_figure_shot2(MOBYS + 0x5000, &s));
    GREF(int32_t, CONTEXT) = 1;
    CHECK(!rac1_figure_shot2(FIGURE, &s));
    GREF(int32_t, CONTEXT) = 0;
    level_on = 0;
    CHECK(!rac1_figure_shot2(FIGURE, &s));
    level_on = 1;
    clear_shot(&s, 0x12);
    CHECK(!rac1_figure_shot2(FIGURE, &s));
    CHECK(game_calls == 0);
    /* A shot of an item that is a track but says nothing of one: never a spawn. */
    clear_shot(&s, ITEM_PYROCITOR);
    CHECK(!rac1_figure_shot2(FIGURE, &s));
    CHECK(game_calls == 0 && flames == 0);
}

static void test_the_pyrocitor_is_a_track(void) {
    reset();
    openrac_game_shot2 out[4];
    CHECK(rac1_hero_shots2(out, 4) == 0);
    GREF(int16_t, HAND + MOBY_OCLASS) = CLASS_PYROCITOR;
    GREF(uint8_t, HAND + MOBY_STATE) = STATE_FIRING;
    CHECK(rac1_hero_shots2(out, 4) == 1);
    CHECK(out[0].item == ITEM_PYROCITOR && out[0].flags == OPENRAC_GAME_SHOT_START);
    /* From the hero, for a figure's hand: so it stays on the figure between two words. */
    CHECK(out[0].position[0] == 1.0f && out[0].position[2] == 1.0f && out[0].velocity[0] == FLAME_REACH);
    CHECK(out[0].aux[AUX_FROM_FIGURE] == 1.0f);

    /* Shown here: flame from the figure's hand each tick, nothing spawned that can hit. */
    const openrac_game_shot2 start = out[0];
    CHECK(rac1_figure_shot2(FIGURE, &start));
    rac1_figure_shots_tick();
    CHECK(flames == FLAME_PARTICLES && game_calls == 0);

    /* A MOVE each 6 ticks. */
    int moves = 0;
    for (int i = 0; i < 12; ++i) {
        moves += rac1_hero_shots2(out, 4);
    }
    CHECK(moves == 2 && out[0].flags == OPENRAC_GAME_SHOT_MOVE);

    GREF(uint8_t, HAND + MOBY_STATE) = 2;
    CHECK(rac1_hero_shots2(out, 4) == 1 && out[0].flags == OPENRAC_GAME_SHOT_STOP);
    CHECK(out[0].item == ITEM_PYROCITOR);
    const openrac_game_shot2 stop = out[0];
    CHECK(rac1_figure_shot2(FIGURE, &stop));
    const int burned = flames;
    rac1_figure_shots_tick();
    CHECK(flames == burned);
    CHECK(rac1_hero_shots2(out, 4) == 0);
}

static void test_a_track_whose_stop_is_lost_ends(void) {
    reset();
    openrac_game_shot2 s;
    clear_shot(&s, ITEM_TESLA_CLAW);
    s.flags = OPENRAC_GAME_SHOT_MOVE;
    s.velocity[0] = 8.0f;
    CHECK(rac1_figure_shot2(FIGURE, &s));
    for (uint32_t i = 0; i < SHOT_TRACK_SILENCE; ++i) {
        rac1_figure_shots_tick();
    }
    CHECK(sparks > 0 && tracks[0].owner == FIGURE);
    rac1_figure_shots_tick();
    rac1_figure_shots_tick();
    CHECK(tracks[0].owner == 0);
    const int was = sparks;
    rac1_figure_shots_tick();
    rac1_figure_shots_tick();
    CHECK(sparks == was);
}

static void test_the_tesla_claw_s_line(void) {
    reset();
    openrac_game_shot2 out[4];
    GREF(int16_t, HAND + MOBY_OCLASS) = CLASS_TESLA_CLAW;
    GREF(uint8_t, HAND + MOBY_STATE) = STATE_FIRING;
    set_vector(GREF(gaddr, HAND + MOBY_PVARS), 102.0f, 200.0f, 31.0f);
    set_vector(GREF(gaddr, HAND + MOBY_PVARS) + TESLA_TIP, 112.0f, 200.0f, 31.0f);
    CHECK(rac1_hero_shots2(out, 4) == 1);
    CHECK(out[0].item == ITEM_TESLA_CLAW && out[0].position[0] == 2.0f && out[0].velocity[0] == 10.0f);
}

static void test_the_morph_o_ray_while_its_timer_runs(void) {
    reset();
    openrac_game_shot2 out[4];
    GREF(int16_t, HAND + MOBY_OCLASS) = CLASS_MORPH_O_RAY;
    GREF(uint8_t, HAND + MOBY_STATE) = 1;
    CHECK(rac1_hero_shots2(out, 4) == 0);
    GREF(int16_t, GREF(gaddr, HAND + MOBY_PVARS) + MORPH_TIMER) = 9;
    CHECK(rac1_hero_shots2(out, 4) == 1 && out[0].item == ITEM_MORPH_O_RAY);
    CHECK(out[0].velocity[0] == BEAM_REACH);
}

static void test_the_visibomb_in_flight(void) {
    reset();
    openrac_game_shot2 out[4];
    const gaddr missile = make_moby(CLASS_VISIBOMB_MISSILE);
    GREF(uint8_t, missile + MOBY_STATE) = 1;
    set_vector(missile + MOBY_POS, 50.0f, 60.0f, 70.0f);
    GREF(gaddr, HERO_VISIBOMB) = missile;
    CHECK(rac1_hero_shots2(out, 4) == 1);
    CHECK(out[0].item == ITEM_VISIBOMB && out[0].flags == OPENRAC_GAME_SHOT_START);
    CHECK(out[0].position[0] == 50.0f && out[0].aux[AUX_FROM_FIGURE] == 0.0f);
    CHECK((int32_t)out[0].aux[AUX_CLASS] == CLASS_VISIBOMB_MISSILE);

    const openrac_game_shot2 start = out[0];
    CHECK(rac1_figure_shot2(FIGURE, &start));
    CHECK(track_mobys == 1);
    rac1_figure_shots_tick();
    CHECK(placed[0] == 50.0f);

    /* It flies; the next word says how fast, and it goes on along that. */
    set_vector(missile + MOBY_POS, 56.0f, 60.0f, 70.0f);
    int n = 0;
    for (int i = 0; i < 6 && n == 0; ++i) {
        n = rac1_hero_shots2(out, 4);
    }
    CHECK(n == 1 && out[0].flags == OPENRAC_GAME_SHOT_MOVE && out[0].velocity[0] == 1.0f);
    const openrac_game_shot2 move = out[0];
    CHECK(rac1_figure_shot2(FIGURE, &move));
    rac1_figure_shots_tick();
    CHECK(placed[0] == 57.0f && track_mobys == 1);

    /* It ends in its burst, and its moby goes. */
    GREF(uint8_t, missile + MOBY_STATE) = 0xFE;
    CHECK(rac1_hero_shots2(out, 4) == 1);
    CHECK(out[0].flags == (OPENRAC_GAME_SHOT_STOP | OPENRAC_GAME_SHOT_BURST));
    const openrac_game_shot2 stop = out[0];
    CHECK(rac1_figure_shot2(FIGURE, &stop));
    CHECK(bursts == 1 && drops == 1);
}

static void test_a_track_goes_with_its_figure(void) {
    reset();
    openrac_game_shot2 s;
    clear_shot(&s, ITEM_DRONE_DEVICE);
    s.flags = OPENRAC_GAME_SHOT_START;
    s.aux[AUX_TRACK] = TRACK_DRONE;
    s.aux[AUX_CLASS] = CLASS_DRONE;
    s.aux[AUX_FROM_FIGURE] = 1.0f;
    s.position[0] = 1.0f;
    CHECK(rac1_figure_shot2(FIGURE, &s));
    rac1_figure_shots_tick();
    CHECK(placed[0] == 11.0f); /* about the figure, not where the shooter's hero stands */
    rac1_figure_shots_remove(FIGURE);
    CHECK(drops == 1 && bursts == 0 && tracks[0].owner == 0);
}

static void test_the_old_interface_takes_what_it_can_show(void) {
    reset();
    GFRAME(v, 16);
    func_L00_002B0F58(HAND, v, 0, 0, 0, 0, 0, 0);
    func_L00_002CC3C0(HERO_M, v, 0, 0.5f, 0, 0, 0);
    openrac_game_shot out[4];
    CHECK(rac1_hero_shots(out, 4) == 1 && out[0].item == ITEM_BLASTER && out[0].aim[0] == 0.5f);
    CHECK(shot_count == 0);
}

int main(void) {
#ifdef _WIN32
    openrac_guest_base = VirtualAlloc(NULL, RAM, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    CHECK(openrac_guest_base != NULL);
#else
    uint8_t* p = mmap(NULL, RAM, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    CHECK(p != MAP_FAILED);
    openrac_guest_base = p;
#endif
    test_the_hero_s_devastator_goes_with_its_target();
    test_a_target_gone_flies_on_its_heading();
    test_the_blaster_homes_as_the_shooter_s();
    test_a_glove_s_throw_is_past_its_held_state();
    test_a_bomb_goes_as_the_shooter_s_went();
    test_a_figure_s_shot_hits_nothing();
    test_refused_with_nothing_made();
    test_the_pyrocitor_is_a_track();
    test_a_track_whose_stop_is_lost_ends();
    test_the_tesla_claw_s_line();
    test_the_morph_o_ray_while_its_timer_runs();
    test_the_visibomb_in_flight();
    test_a_track_goes_with_its_figure();
    test_the_old_interface_takes_what_it_can_show();
    puts("figure_shots ok");
    return 0;
}
