/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Going Commando: weapons whose targets are picked on screen or not, for the mods (openrac_api
 * weapon_ignore_culling, interface 1.30; weapon_shots.h).
 *
 * The game's target pick, SelectBestTargetInCone (func_002AF580, src/game/002AF580-qcopy.c), walks
 * the moby loop's target list (g_target_list, the mobys BuildActiveMobyChain, func_002A08A0, keeps
 * near the camera) and passes over every moby whose drawn flag (+0x31) is 0: the flag MobyProc
 * sets for the mobys in the 4:3 view and draw distance last frame (render.c's mark_drawn_mobys).
 * So the weapons that hit all around the hero miss the enemies behind the camera or off screen:
 *   Zodiac       charging_weapon_update (func_L00_00408E58, class 3784, the gun): its pulse
 *                (state 3) takes up to 32 targets within 40 units, any heading, 60 degrees up or
 *                down of the camera's forward
 *   Plasma Coil  thrown_shot_update (func_L00_003CA730, class 2715, the Plasma Coil's shot from
 *                SpawnParticlePoolMoby, func_L00_003CB980): stuck to the enemy it hit, its arcs
 *                jump to up to 3 more within 10 units all around
 *   Plasma Storm UpdateMovingEffectState (func_L00_003BAC30, class 2473, the shot the Mega's gun,
 *                class 1482, makes through effect_spawn_moby, func_L00_003BB080, which keeps the
 *                gun's class at pvars +0x44): each tick of its flight, up to 15 targets within 15
 *                units all around, which its orbiting arcs then strike
 * The pick is wrapped: while the moby loop runs one of those mobys' update and a mod asked for its
 * weapon, the live mobys not drawn are marked drawn for the pick and given back their flag after
 * it, so the pick takes them as the game takes the drawn ones (its range, angles, kinds and caps
 * are unchanged). Nothing else sees the flag changed: not the loop, the lock-on or the renderer.
 */
#include <stdint.h>

#include "game_protos.h"
#include "rac2.h"
#include "weapon_shots.h"

#define MOBY_BASE OPENRAC_DATA(RAC2_g_mobyTableBase)
#define MOBY_END OPENRAC_DATA(RAC2_g_mobyTableEnd)
#define MOBY_SIZE 0x100u
#define SLOTS 4096u

enum {
    MOBY_STATE = 0x20, /* 0x80 and over: dead or free; 0xFF the table's end */
    MOBY_DRAWN = 0x31,
    MOBY_PVARS = 0x68,
    MOBY_OCLASS = 0xAA,
    SHOT_GUN_CLASS = 0x44 /* the Plasma Storm shot's pvars: its gun's class */
};

enum {
    ITEM_PLASMA_COIL = 28,
    ITEM_ZODIAC = 43,
    CLASS_ZODIAC = 3784,
    CLASS_PLASMA_COIL_SHOT = 2715,
    CLASS_PLASMA_STORM_SHOT = 2473,
    CLASS_PLASMA_COIL = 3146,
    CLASS_PLASMA_STORM = 1482
};

/* The weapons a mod asked for, one bit an item. */
static uint64_t held;

int rac2_weapon_ignore_culling(int32_t item, int enabled) {
    if (item != ITEM_PLASMA_COIL && item != ITEM_ZODIAC) {
        return 0;
    }
    const uint64_t bit = 1ull << item;
    held = enabled ? held | bit : held & ~bit;
    return 1;
}

/* The weapon whose target pick the moby `m` makes in its update, -1 for none. */
static int weapon_of(gaddr m) {
    const int16_t oclass = GREF(int16_t, m + MOBY_OCLASS);
    if (oclass == CLASS_ZODIAC) {
        return ITEM_ZODIAC;
    }
    if (oclass == CLASS_PLASMA_COIL_SHOT) {
        return ITEM_PLASMA_COIL;
    }
    if (oclass == CLASS_PLASMA_STORM_SHOT) {
        const gaddr pvars = GREF(gaddr, m + MOBY_PVARS);
        const int32_t gun = pvars != 0 ? GREF(int32_t, pvars + SHOT_GUN_CLASS) : 0;
        if (gun == CLASS_PLASMA_COIL || gun == CLASS_PLASMA_STORM) {
            return ITEM_PLASMA_COIL;
        }
    }
    return -1;
}

static int ignores_culling(gaddr m) {
    if (held == 0 || m == 0) {
        return 0;
    }
    const int item = weapon_of(m);
    return item >= 0 && (held >> item & 1u) != 0;
}

/* SelectBestTargetInCone: the game's pick, with the live mobys not drawn marked drawn around it
 * for the weapons a mod asked for. */
gaddr func_002AF580(
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
    const gaddr picker = rac2_moby_updating();
    if (!ignores_culling(picker)) {
        return func_002AF580__game(
            origin,
            direction,
            angle,
            vertical,
            count,
            out,
            capacity,
            exclude,
            range,
            vertical_min,
            angle_min,
            allow_special,
            types
        );
    }
    static uint16_t marked[SLOTS];
    uint32_t n = 0;
    const gaddr base = GREF(gaddr, MOBY_BASE);
    const gaddr end = GREF(gaddr, MOBY_END);
    if (base != 0 && end > base) {
        for (gaddr m = base; m < end && n < SLOTS; m += MOBY_SIZE) {
            const uint8_t state = GREF(uint8_t, m + MOBY_STATE);
            if (state == 0xFF) {
                break;
            }
            if (state < 0x80 && m != picker && GREF(uint8_t, m + MOBY_DRAWN) == 0) {
                GREF(uint8_t, m + MOBY_DRAWN) = 1;
                marked[n++] = (uint16_t)((m - base) / MOBY_SIZE);
            }
        }
    }
    const gaddr best = func_002AF580__game(
        origin,
        direction,
        angle,
        vertical,
        count,
        out,
        capacity,
        exclude,
        range,
        vertical_min,
        angle_min,
        allow_special,
        types
    );
    for (uint32_t i = 0; i < n; ++i) {
        GREF(uint8_t, base + (gaddr)marked[i] * MOBY_SIZE + MOBY_DRAWN) = 0;
    }
    return best;
}
