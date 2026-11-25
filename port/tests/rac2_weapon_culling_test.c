/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/* Going Commando's target picks on screen or not (port/game/rac2/host/weapon_culling.c, interface
 * 1.30) on a moby table of the test's own: the game's pick sees only the drawn mobys unless a mod
 * asked for the weapon whose gun, shot, drone or turret makes it (the Zodiac's gun, in the moby
 * loop or from the hero's hand outside it, the Plasma Coil's shot, the Plasma Storm's shot by its
 * gun's class, the Synthenoid's and Kilonoid's drones, the seekers, the turrets); then it sees the
 * live enemies not drawn in its range and in sight too, the drones' cone opened all around, and
 * each gets its flag back after. The Shield Charger's and Tesla Barrier's update sees the enemies
 * near the hero and in his sight, the Tesla Claw's its held targets in his sight. Other weapons
 * are refused. Nothing is widened outside play (a scene), and a moby whose class, pvars or target
 * record is no word of the game's RAM (a float there, pvars in the port's memory that are no copy's)
 * is passed over without following it; an NG++ copy's own pvars there are taken. A moby the
 * renderer could not draw (hidden, faded out) or not started yet is never marked. */
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

#include "../game/rac2/host/weapon_culling.c"

uint8_t* openrac_guest_base;
uint32_t openrac_guest_ram_size = 0x02000000u;
gaddr openrac_relocate_low = 0, openrac_relocate_high = 0;

static const char* trace_value;

const char* openrac_env_string(const char* name) {
    CHECK(strcmp(name, "OPENRAC_PICK_TRACE") == 0);
    return trace_value;
}

gaddr openrac_relocate_data(gaddr a) {
    return a;
}

gaddr openrac_relocate_level_data(int from, gaddr a) {
    (void)from;
    return a;
}

#define STACK 0x00900000u
gaddr openrac_guest_sp = STACK;

gaddr openrac_guest_frame(uint32_t size) {
    openrac_guest_sp -= (size + 15u) & ~15u;
    return openrac_guest_sp;
}

#define BASE 0x00400000u
#define SLOTS_IN 16u
#define END (BASE + SLOTS_IN * 0x100u)
#define PVARS 0x00600000u
#define INFOS 0x00700000u
#define RECORDS 0x00710000u
#define PICKER 1u
#define DEAD 5u
/* Off screen and behind a wall of the level. */
#define WALLED 7u
/* Off screen with another moby on the line: in sight. */
#define BEHIND_MOBY 8u
/* Off screen, not an enemy. */
#define NOT_ENEMY 9u
/* Off screen, out of every range. */
#define FAR 10u
/* Off screen with no live record (dead to the game). */
#define NO_HEALTH 11u

static gaddr now;

gaddr rac2_moby_updating(void) {
    return now;
}

/* enemy_copies.c's: one copy, whose pvars the port laid out in its memory, when set. */
static gaddr copy_moby, copy_pvars;

gaddr rac2_copy_pvars(gaddr moby, uint32_t* bytes) {
    if (copy_moby == 0 || moby != copy_moby || GREF(gaddr, moby + MOBY_PVARS) != copy_pvars) {
        return 0;
    }
    if (bytes != NULL) {
        *bytes = 0x400u;
    }
    return copy_pvars;
}

static gaddr moby(uint32_t slot) {
    return BASE + slot * 0x100u;
}

/* moby_get_private_value_0, as the game's: its pvars' first word when +0x34 has 0x20 (each slot's
 * record, alive but for NO_HEALTH's). It reads what it is given: an address out of the test's memory
 * faults, as the port's did. */
gaddr func_002AB1F0(gaddr m) {
    CHECK(m >= BASE && m < END);
    if ((GREF(uint16_t, m + MOBY_MODE) & MODE_RECORD) == 0) {
        return 0;
    }
    const gaddr pvars = GREF(gaddr, m + MOBY_PVARS);
    /* The copy's pvars, in the port's memory (out of the test's): its record in RAM. */
    if (copy_moby != 0 && m == copy_moby && pvars == copy_pvars) {
        return RECORDS + ((m - BASE) / 0x100u) * 0x20u;
    }
    CHECK(pvars < openrac_guest_ram_size);
    return GREF(gaddr, pvars);
}

/* CollLine: a wall of the level before WALLED, a moby before BEHIND_MOBY, nothing else. */
static int lines;

int func_002761F8(gaddr a0, gaddr a1, int a2, gaddr a3, gaddr a4) {
    (void)a0, (void)a1, (void)a4;
    CHECK(a2 == LINE_FLAGS);
    ++lines;
    if (a3 == moby(WALLED)) {
        GREF(gaddr, COLL_RESULT + COLL_HIT_MOBY) = 0;
        return 1;
    }
    if (a3 == moby(BEHIND_MOBY)) {
        GREF(gaddr, COLL_RESULT + COLL_HIT_MOBY) = moby(3);
        return 1;
    }
    return 0;
}

/* The live mobys drawn, counted (the picker left out). */
static int drawn_count(gaddr self) {
    int n = 0;
    for (gaddr m = BASE; GREF(uint8_t, m + MOBY_STATE) != 0xFF; m += MOBY_SIZE) {
        if (GREF(uint8_t, m + MOBY_STATE) < 0x80 && GREF(uint8_t, m + MOBY_DRAWN) != 0
            && m != self) {
            ++n;
        }
    }
    return n;
}

static gaddr first_drawn(gaddr self) {
    for (gaddr m = BASE; GREF(uint8_t, m + MOBY_STATE) != 0xFF; m += MOBY_SIZE) {
        if (GREF(uint8_t, m + MOBY_STATE) < 0x80 && GREF(uint8_t, m + MOBY_DRAWN) != 0
            && m != self) {
            return m;
        }
    }
    return 0;
}

/* The game's pick, as far as the flag goes: the live mobys drawn, counted; the arguments checked
 * to come through, the cone as it came. */
static int seen;
static float seen_angle, seen_vertical;
static gaddr pick_origin = 0x100u;

gaddr func_002AF580__game(
    gaddr origin,
    gaddr direction,
    float angle,
    float vertical,
    gaddr count,
    gaddr out,
    int capacity,
    gaddr exclude,
    float range,
    float vertical_min,
    float angle_min,
    int allow_special,
    gaddr types
) {
    CHECK(origin == pick_origin && direction == 0x110u);
    CHECK(count == 0x120u && out == 0x130u && capacity == 32 && exclude == 0x140u);
    CHECK(range == 40.0f && vertical_min == 3.0f && angle_min == 4.0f && allow_special == 5);
    CHECK(types == 0x150u);
    seen_angle = angle;
    seen_vertical = vertical;
    seen = drawn_count(now);
    GREF(int32_t, count) = 0;
    return first_drawn(now);
}

static gaddr pick(void) {
    return func_002AF580(
        pick_origin, 0x110u, 1.0f, 2.0f, 0x120u, 0x130u, 32, 0x140u, 40.0f, 3.0f, 4.0f, 5, 0x150u
    );
}

/* The barrier's update: the enemies drawn it sees. */
static int barrier_seen;

void func_L00_003C3D90__game(gaddr a0) {
    barrier_seen = drawn_count(a0);
}

/* The claw's update: whether each held target is drawn. */
static int claw_first_drawn, claw_second_drawn;

void func_L00_00392530__game(gaddr a0) {
    const gaddr pvars = GREF(gaddr, a0 + MOBY_PVARS);
    claw_first_drawn = GREF(uint8_t, GREF(gaddr, pvars + CLAW_TARGET) + MOBY_DRAWN);
    claw_second_drawn = GREF(uint8_t, GREF(gaddr, pvars + CLAW_SECONDARY) + MOBY_DRAWN);
}

static void put_class(uint32_t slot, int16_t oclass) {
    memcpy(G(moby(slot) + MOBY_OCLASS), &oclass, 2);
}

static void level(void) {
    memset(G(BASE), 0, SLOTS_IN * 0x100u + 0x100u);
    for (uint32_t s = 0; s < SLOTS_IN; ++s) {
        const gaddr m = moby(s);
        GREF(uint8_t, m + MOBY_STATE) = 1;
        GREF(gaddr, m + MOBY_PVARS) = PVARS + s * 0x100u;
        GREF(gaddr, PVARS + s * 0x100u) = RECORDS + s * 0x20u;
        GREF(uint16_t, m + MOBY_MODE) = MODE_RECORD;
        GREF(uint8_t, m + MOBY_ALPHA) = 0x80;
        GREF(gaddr, m + MOBY_INFO) = INFOS;
        put_class(s, 100);
        /* Along x, a unit apart, from the origin (the pick's and the hero's). */
        GREF(float, m + MOBY_POS) = (float)s;
        /* Slots 2 and 3 drawn; the others off screen. */
        GREF(uint8_t, m + MOBY_DRAWN) = s == 2 || s == 3;
        GREF(float, RECORDS + s * 0x20u) = s == NO_HEALTH ? 0.0f : 10.0f;
        GREF(float, RECORDS + s * 0x20u + 0x10u) = 0.5f;
    }
    GREF(int16_t, INFOS + INFO_KIND) = KIND_ENEMY;
    GREF(int16_t, INFOS + 0x100u + INFO_KIND) = 3;
    GREF(gaddr, moby(NOT_ENEMY) + MOBY_INFO) = INFOS + 0x100u;
    GREF(float, moby(FAR) + MOBY_POS) = 100.0f;
    GREF(uint8_t, moby(DEAD) + MOBY_STATE) = 0x80;
    GREF(uint8_t, END + MOBY_STATE) = 0xFF;
    GREF(gaddr, OPENRAC_DATA(RAC2_g_mobyTableBase)) = BASE;
    GREF(gaddr, OPENRAC_DATA(RAC2_g_mobyTableEnd)) = END;
    GREF(int32_t, GAME_STATE) = STATE_PLAY;
    GREF(gaddr, HERO_MOBY) = moby(0);
    for (unsigned c = 0; c < 3; ++c) {
        GREF(float, pick_origin + 4u * c) = 0.0f;
        GREF(float, OPENRAC_DATA(RAC2_g_heroPos) + 4u * c) = 0.0f;
    }
    now = moby(PICKER);
}

/* The flags are as the level left them. */
static void flags_kept(void) {
    for (uint32_t s = 0; s < SLOTS_IN; ++s) {
        CHECK(GREF(uint8_t, moby(s) + MOBY_DRAWN) == (s == 2 || s == 3));
    }
    CHECK(openrac_guest_sp == STACK);
}

/* The game's two drawn ones and the enemies off screen in range and in sight: slots 0 to 15 less
 * the picker, the dead one, the walled one, the one not an enemy, the far one and the one with no
 * health. */
#define WIDENED 10
#define ON_SCREEN 2

/* The cone came through as the game gave it, or opened all around. */
static int cone_kept(void) {
    return seen_angle == 1.0f && seen_vertical == 2.0f;
}

static int cone_open(void) {
    return seen_angle == ALL_AROUND && seen_vertical == ALL_AROUND;
}

int main(void) {
    openrac_guest_base = calloc(1, openrac_guest_ram_size);
    CHECK(openrac_guest_base != NULL);
    level();

    /* Nothing asked: the game's drawn mobys only. */
    put_class(PICKER, CLASS_ZODIAC);
    CHECK(pick() == moby(2) && seen == ON_SCREEN && cone_kept());
    flags_kept();

    /* Other weapons are refused: the Lancer, the RYNO II, the Gravity Bomb, out of range. */
    CHECK(rac2_weapon_ignore_culling(30, 1) == 0);
    CHECK(rac2_weapon_ignore_culling(44, 1) == 0);
    CHECK(rac2_weapon_ignore_culling(42, 1) == 0);
    CHECK(rac2_weapon_ignore_culling(-1, 1) == 0);
    CHECK(rac2_weapon_ignore_culling(64, 1) == 0);
    CHECK(held == 0);

    /* The Zodiac asked: its gun's pick sees every live enemy in range and in sight, its cone the
     * game's, then each flag is given back. */
    CHECK(rac2_weapon_ignore_culling(ITEM_ZODIAC, 1) == 1);
    lines = 0;
    CHECK(pick() == moby(0) && seen == WIDENED && cone_kept());
    CHECK(lines > 0);
    flags_kept();
    CHECK(GREF(uint8_t, moby(DEAD) + MOBY_DRAWN) == 0);

    /* Between the loop's updates the pick is the game's, but for the gun in the hero's hand picking
     * from its own position (the hero's update runs it). */
    now = 0;
    CHECK(pick() == moby(2) && seen == ON_SCREEN);
    GREF(gaddr, HAND_MOBY) = moby(PICKER);
    pick_origin = moby(PICKER) + MOBY_POS;
    CHECK(pick() == moby(0) && seen == WIDENED);
    flags_kept();
    pick_origin = 0x100u;
    CHECK(pick() == moby(2) && seen == ON_SCREEN);
    GREF(gaddr, HAND_MOBY) = 0;
    now = moby(PICKER);

    /* The Plasma Storm's shot is the Plasma Coil's item, not the Zodiac's. */
    put_class(PICKER, CLASS_PLASMA_STORM_SHOT);
    GREF(int32_t, PVARS + PICKER * 0x100u + SHOT_GUN_CLASS) = CLASS_PLASMA_STORM;
    CHECK(pick() == moby(2) && seen == ON_SCREEN);
    CHECK(rac2_weapon_ignore_culling(ITEM_PLASMA_COIL, 1) == 1);
    CHECK(pick() == moby(0) && seen == WIDENED && cone_kept());
    flags_kept();

    /* A 2473 shot another moby's gun made is left as it is. */
    GREF(int32_t, PVARS + PICKER * 0x100u + SHOT_GUN_CLASS) = 999;
    CHECK(pick() == moby(2) && seen == ON_SCREEN);

    /* The Plasma Coil's own shot. */
    put_class(PICKER, CLASS_PLASMA_COIL_SHOT);
    CHECK(pick() == moby(0) && seen == WIDENED);
    flags_kept();

    /* The drones: the Synthenoid's and the Kilonoid's (its Mega) are item 31, their cone opened
     * all around. */
    put_class(PICKER, CLASS_SYNTHENOID);
    CHECK(pick() == moby(2) && seen == ON_SCREEN && cone_kept());
    CHECK(rac2_weapon_ignore_culling(ITEM_SYNTHENOID, 1) == 1);
    CHECK(pick() == moby(0) && seen == WIDENED && cone_open());
    flags_kept();
    put_class(PICKER, CLASS_KILONOID);
    CHECK(pick() == moby(0) && seen == WIDENED && cone_open());
    flags_kept();

    /* The seekers (item 24) and the turrets (item 41): their cone the game's. */
    const int16_t widened_only[] =
        {CLASS_SEEKER, CLASS_SEEKER_MEGA, CLASS_MINITURRET, CLASS_MEGATURRET};
    const int32_t widened_item[] =
        {ITEM_SEEKER_GUN, ITEM_SEEKER_GUN, ITEM_MINITURRET, ITEM_MINITURRET};
    for (unsigned i = 0; i < 4; ++i) {
        put_class(PICKER, widened_only[i]);
        CHECK(rac2_weapon_ignore_culling(widened_item[i], 0) == 1);
        CHECK(pick() == moby(2) && seen == ON_SCREEN);
        CHECK(rac2_weapon_ignore_culling(widened_item[i], 1) == 1);
        CHECK(pick() == moby(0) && seen == WIDENED && cone_kept());
        flags_kept();
    }

    /* The Shield Charger and the Tesla Barrier (item 45): their update sees the enemies near the
     * hero and in his sight; the update of another class the game's. */
    const gaddr barrier = moby(PICKER);
    put_class(PICKER, CLASS_TESLA_BARRIER);
    now = barrier;
    func_L00_003C3D90(barrier);
    CHECK(barrier_seen == ON_SCREEN);
    CHECK(rac2_weapon_ignore_culling(ITEM_SHIELD_CHARGER, 1) == 1);
    func_L00_003C3D90(barrier);
    CHECK(barrier_seen == WIDENED);
    flags_kept();
    put_class(PICKER, CLASS_SHIELD_CHARGER);
    func_L00_003C3D90(barrier);
    CHECK(barrier_seen == WIDENED);
    flags_kept();
    put_class(PICKER, 100);
    func_L00_003C3D90(barrier);
    CHECK(barrier_seen == ON_SCREEN);

    /* The Tesla Claw (item 18): its held targets off screen kept drawn for its update when in the
     * hero's sight, not behind a wall. */
    const gaddr claw = moby(PICKER);
    put_class(PICKER, 177);
    GREF(gaddr, PVARS + PICKER * 0x100u + CLAW_TARGET) = moby(4);
    GREF(gaddr, PVARS + PICKER * 0x100u + CLAW_SECONDARY) = moby(WALLED);
    func_L00_00392530(claw);
    CHECK(claw_first_drawn == 0 && claw_second_drawn == 0);
    CHECK(rac2_weapon_ignore_culling(ITEM_TESLA_CLAW, 1) == 1);
    func_L00_00392530(claw);
    CHECK(claw_first_drawn == 1 && claw_second_drawn == 0);
    flags_kept();
    /* A held moby out of the table is left alone. */
    GREF(gaddr, PVARS + PICKER * 0x100u + CLAW_TARGET) = 0x00123450u;
    func_L00_00392530(claw);
    flags_kept();

    /* In a scene (game state 2), or with no hero, every pick and update is the game's. */
    put_class(PICKER, CLASS_ZODIAC);
    CHECK(rac2_weapon_ignore_culling(ITEM_SHIELD_CHARGER, 1) == 1);
    GREF(int32_t, GAME_STATE) = 2;
    lines = 0;
    CHECK(pick() == moby(2) && seen == ON_SCREEN);
    put_class(PICKER, CLASS_TESLA_BARRIER);
    func_L00_003C3D90(moby(PICKER));
    CHECK(barrier_seen == ON_SCREEN);
    put_class(PICKER, 177);
    GREF(gaddr, PVARS + PICKER * 0x100u + CLAW_TARGET) = moby(4);
    func_L00_00392530(moby(PICKER));
    CHECK(claw_first_drawn == 0);
    CHECK(lines == 0);
    flags_kept();
    GREF(int32_t, GAME_STATE) = STATE_PLAY;
    GREF(gaddr, HERO_MOBY) = 0;
    CHECK(pick() == moby(2) && seen == ON_SCREEN);
    GREF(gaddr, HERO_MOBY) = moby(0);
    put_class(PICKER, CLASS_ZODIAC);
    CHECK(pick() == moby(0) && seen == WIDENED);
    flags_kept();

    /* Yeedil's crash: an enemy's pvars word a float (179.7) is not followed, nor its record word,
     * nor its class word; a copy's pvars in the port's memory are passed over; a moby with no
     * target record (+0x34 without 0x20) is not taken. Each is one fewer seen, and nothing faults
     * (the stub checks what it is given). */
    const gaddr float_word = 0x4333AC8Fu;
    const gaddr bad_words[] = {float_word, GUEST_STATICS + 0x100u};
    for (unsigned i = 0; i < 2; ++i) {
        GREF(gaddr, moby(4) + MOBY_PVARS) = bad_words[i];
        CHECK(pick() == moby(0) && seen == WIDENED - 1);
        flags_kept();
    }
    GREF(gaddr, moby(4) + MOBY_PVARS) = PVARS + 4u * 0x100u;
    GREF(gaddr, PVARS + 4u * 0x100u) = float_word;
    CHECK(pick() == moby(0) && seen == WIDENED - 1);
    GREF(gaddr, PVARS + 4u * 0x100u) = RECORDS + 4u * 0x20u;
    GREF(gaddr, moby(4) + MOBY_INFO) = float_word;
    CHECK(pick() == moby(0) && seen == WIDENED - 1);
    GREF(gaddr, moby(4) + MOBY_INFO) = INFOS;
    GREF(uint16_t, moby(4) + MOBY_MODE) = 0;
    CHECK(pick() == moby(0) && seen == WIDENED - 1);
    GREF(uint16_t, moby(4) + MOBY_MODE) = MODE_RECORD;
    CHECK(pick() == moby(0) && seen == WIDENED);
    /* NG++'s copy: its pvars in the port's memory are its own (enemy_copies.c laid them out), so
     * it is taken as the originals are; the same word on another moby is still passed over. */
    copy_moby = moby(4);
    copy_pvars = GUEST_STATICS + 0x100u;
    GREF(gaddr, moby(4) + MOBY_PVARS) = copy_pvars;
    CHECK(pick() == moby(0) && seen == WIDENED);
    flags_kept();
    copy_moby = moby(6);
    CHECK(pick() == moby(0) && seen == WIDENED - 1);
    copy_moby = 0;
    GREF(gaddr, moby(4) + MOBY_PVARS) = PVARS + 4u * 0x100u;
    /* What the renderer could not draw is never marked: held hidden (+0x34's 0x01 or 0x80), faded
     * out (alpha under 8), or not started (state 0: its start-up would set its death path again, as
     * Siberius's cart riders do). */
    const uint16_t hidden_modes[] = {MODE_RECORD | 0x01u, MODE_RECORD | 0x80u, MODE_RECORD | 0x41u};
    for (unsigned i = 0; i < 3; ++i) {
        GREF(uint16_t, moby(4) + MOBY_MODE) = hidden_modes[i];
        CHECK(pick() == moby(0) && seen == WIDENED - 1);
        flags_kept();
    }
    GREF(uint16_t, moby(4) + MOBY_MODE) = MODE_RECORD;
    GREF(uint8_t, moby(4) + MOBY_ALPHA) = 7;
    CHECK(pick() == moby(0) && seen == WIDENED - 1);
    GREF(uint8_t, moby(4) + MOBY_ALPHA) = 0x80;
    GREF(uint8_t, moby(4) + MOBY_STATE) = 0;
    CHECK(pick() == moby(0) && seen == WIDENED - 1);
    GREF(uint8_t, moby(4) + MOBY_STATE) = 1;
    CHECK(pick() == moby(0) && seen == WIDENED);
    /* The barrier's update over the same: no fault, the float one passed over. */
    GREF(gaddr, moby(4) + MOBY_PVARS) = float_word;
    put_class(PICKER, CLASS_TESLA_BARRIER);
    func_L00_003C3D90(moby(PICKER));
    CHECK(barrier_seen == WIDENED - 1);
    flags_kept();
    GREF(gaddr, moby(4) + MOBY_PVARS) = PVARS + 4u * 0x100u;
    CHECK(rac2_weapon_ignore_culling(ITEM_SHIELD_CHARGER, 0) == 1);

    /* Given back: the game's pick again for that weapon, the other one still on. */
    put_class(PICKER, CLASS_PLASMA_COIL_SHOT);
    CHECK(rac2_weapon_ignore_culling(ITEM_PLASMA_COIL, 0) == 1);
    CHECK(pick() == moby(2) && seen == ON_SCREEN);
    put_class(PICKER, CLASS_ZODIAC);
    CHECK(pick() == moby(0) && seen == WIDENED);
    const int32_t all[] =
        {ITEM_ZODIAC,
         ITEM_SYNTHENOID,
         ITEM_SEEKER_GUN,
         ITEM_MINITURRET,
         ITEM_SHIELD_CHARGER,
         ITEM_TESLA_CLAW};
    for (unsigned i = 0; i < sizeof all / sizeof all[0]; ++i) {
        CHECK(rac2_weapon_ignore_culling(all[i], 0) == 1);
    }
    CHECK(held == 0);
    CHECK(pick() == moby(2) && seen == ON_SCREEN);
    flags_kept();

    /* OPENRAC_PICK_TRACE: on, and the weapons it lists widened without a mod (unknown ones not). */
    trace_value = "31,45,27,x18";
    read_trace();
    CHECK(tracing() && held == 0);
    CHECK(widened() == (1ull << 31 | 1ull << 45 | 1ull << 18));
    put_class(PICKER, CLASS_KILONOID);
    CHECK(pick() == moby(0) && seen == WIDENED && cone_open());
    flags_kept();
    trace_value = "0";
    read_trace();
    CHECK(!tracing());
    trace_held = 0;

    free(openrac_guest_base);
    puts("rac2_weapon_culling: ok");
    return 0;
}
