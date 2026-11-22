/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/* Going Commando's target picks on screen or not (port/game/rac2/host/weapon_culling.c, interface
 * 1.30) on a moby table of the test's own: the game's pick sees only the drawn mobys unless a mod
 * asked for the weapon whose gun or shot runs its update now (the Zodiac's gun, the Plasma Coil's
 * shot, the Plasma Storm's shot by its gun's class); then it sees the live mobys not drawn too, and
 * each gets its flag back after; other weapons are refused. */
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

gaddr openrac_relocate_data(gaddr a) {
    return a;
}

gaddr openrac_relocate_level_data(int from, gaddr a) {
    (void)from;
    return a;
}

#define BASE 0x00400000u
#define SLOTS_IN 16u
#define END (BASE + SLOTS_IN * 0x100u)
#define PVARS 0x00600000u
#define PICKER 1u
#define DEAD 5u

static gaddr now;

gaddr rac2_moby_updating(void) {
    return now;
}

static gaddr moby(uint32_t slot) {
    return BASE + slot * 0x100u;
}

/* The game's pick, as far as the flag goes: the live mobys drawn, counted; the arguments checked
 * to come through. */
static int seen;

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
    CHECK(origin == 0x100u && direction == 0x110u && angle == 1.0f && vertical == 2.0f);
    CHECK(count == 0x120u && out == 0x130u && capacity == 32 && exclude == 0x140u);
    CHECK(range == 40.0f && vertical_min == 3.0f && angle_min == 4.0f && allow_special == 5);
    CHECK(types == 0x150u);
    seen = 0;
    gaddr first = 0;
    for (gaddr m = BASE; GREF(uint8_t, m + MOBY_STATE) != 0xFF; m += MOBY_SIZE) {
        if (GREF(uint8_t, m + MOBY_STATE) < 0x80 && GREF(uint8_t, m + MOBY_DRAWN) != 0
            && m != now) {
            ++seen;
            first = first != 0 ? first : m;
        }
    }
    return first;
}

static gaddr pick(void) {
    return func_002AF580(
        0x100u, 0x110u, 1.0f, 2.0f, 0x120u, 0x130u, 32, 0x140u, 40.0f, 3.0f, 4.0f, 5, 0x150u
    );
}

static void put_class(uint32_t slot, int16_t oclass) {
    memcpy(G(moby(slot) + MOBY_OCLASS), &oclass, 2);
}

static void level(void) {
    memset(G(BASE), 0, SLOTS_IN * 0x100u + 0x100u);
    for (uint32_t s = 0; s < SLOTS_IN; ++s) {
        GREF(uint8_t, moby(s) + MOBY_STATE) = 1;
        GREF(gaddr, moby(s) + MOBY_PVARS) = PVARS + s * 0x100u;
        put_class(s, 100);
        /* Slots 2 and 3 drawn; the others off screen. */
        GREF(uint8_t, moby(s) + MOBY_DRAWN) = s == 2 || s == 3;
    }
    GREF(uint8_t, moby(DEAD) + MOBY_STATE) = 0x80;
    GREF(uint8_t, END + MOBY_STATE) = 0xFF;
    GREF(gaddr, OPENRAC_DATA(RAC2_g_mobyTableBase)) = BASE;
    GREF(gaddr, OPENRAC_DATA(RAC2_g_mobyTableEnd)) = END;
    now = moby(PICKER);
}

/* The flags are as the level left them. */
static void flags_kept(void) {
    for (uint32_t s = 0; s < SLOTS_IN; ++s) {
        CHECK(GREF(uint8_t, moby(s) + MOBY_DRAWN) == (s == 2 || s == 3));
    }
}

/* All live mobys but the picker: slots 0 to 15 less the picker and the dead one. */
#define ALL_LIVE ((int)SLOTS_IN - 2)

int main(void) {
    openrac_guest_base = calloc(1, openrac_guest_ram_size);
    CHECK(openrac_guest_base != NULL);
    level();

    /* Nothing asked: the game's drawn mobys only. */
    put_class(PICKER, CLASS_ZODIAC);
    CHECK(pick() == moby(2) && seen == 2);
    flags_kept();

    /* Other weapons are refused. */
    CHECK(rac2_weapon_ignore_culling(27, 1) == 0);
    CHECK(rac2_weapon_ignore_culling(-1, 1) == 0);
    CHECK(rac2_weapon_ignore_culling(64, 1) == 0);
    CHECK(held == 0);

    /* The Zodiac asked: its gun's pick sees every live moby, then each flag is given back. */
    CHECK(rac2_weapon_ignore_culling(ITEM_ZODIAC, 1) == 1);
    CHECK(pick() == moby(0) && seen == ALL_LIVE);
    flags_kept();
    CHECK(GREF(uint8_t, moby(DEAD) + MOBY_DRAWN) == 0);

    /* Only within an update: between updates the pick is the game's. */
    now = 0;
    CHECK(pick() == moby(2) && seen == 2);
    now = moby(PICKER);

    /* The Plasma Storm's shot is the Plasma Coil's item, not the Zodiac's. */
    put_class(PICKER, CLASS_PLASMA_STORM_SHOT);
    GREF(int32_t, PVARS + PICKER * 0x100u + SHOT_GUN_CLASS) = CLASS_PLASMA_STORM;
    CHECK(pick() == moby(2) && seen == 2);
    CHECK(rac2_weapon_ignore_culling(ITEM_PLASMA_COIL, 1) == 1);
    CHECK(pick() == moby(0) && seen == ALL_LIVE);
    flags_kept();

    /* A 2473 shot another moby's gun made is left as it is. */
    GREF(int32_t, PVARS + PICKER * 0x100u + SHOT_GUN_CLASS) = 999;
    CHECK(pick() == moby(2) && seen == 2);

    /* The Plasma Coil's own shot. */
    put_class(PICKER, CLASS_PLASMA_COIL_SHOT);
    CHECK(pick() == moby(0) && seen == ALL_LIVE);
    flags_kept();

    /* Given back: the game's pick again for that weapon, the other one still on. */
    CHECK(rac2_weapon_ignore_culling(ITEM_PLASMA_COIL, 0) == 1);
    CHECK(pick() == moby(2) && seen == 2);
    put_class(PICKER, CLASS_ZODIAC);
    CHECK(pick() == moby(0) && seen == ALL_LIVE);
    CHECK(rac2_weapon_ignore_culling(ITEM_ZODIAC, 0) == 1);
    CHECK(held == 0);
    CHECK(pick() == moby(2) && seen == 2);
    flags_kept();

    free(openrac_guest_base);
    puts("rac2_weapon_culling: ok");
    return 0;
}
