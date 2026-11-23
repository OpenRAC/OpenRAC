/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Going Commando: weapons whose targets are picked on screen or not, for the mods (openrac_api
 * weapon_ignore_culling, interface 1.30; weapon_shots.h).
 *
 * The game's auto-targeting weapons see only what the camera saw. Their picks pass over every moby
 * whose drawn flag (+0x31) is 0: the flag MobyProc sets for the mobys in the 4:3 view and draw
 * distance last frame (render.c's mark_drawn_mobys). The drones aim along the camera too. So the
 * weapons that hit all around the hero miss the enemies behind the camera or off screen. The
 * enemies themselves are there: the moby loop (BuildActiveMobyChain, func_002A08A0) runs and lists
 * as targets the mobys drawn and those within their update distance of the camera, drawn or not.
 *
 * The weapons, by the moby whose update makes the pick (its class) and how it picks:
 *
 * SelectBestTargetInCone (func_002AF580, src/game/002AF580-qcopy.c) walks the moby loop's target
 * list and passes over the mobys not drawn. With angle and vertical both at pi or more it skips its
 * cone tests and takes the nearest. It checks the line of sight (CollLine, flags 18) of the one it
 * returns, not of the list it fills.
 *   Zodiac        3784, the gun: charging_weapon_update (func_L00_00408E58), its pulse (state 3)
 *                 takes up to 32 targets in its list within 40 units, all around, 60 degrees up or
 *                 down of the camera's forward
 *   Plasma Coil   2715, its shot: thrown_shot_update (func_L00_003CA730), stuck to the enemy it
 *                 hit, its arcs jump to up to 3 more within 10 units all around
 *   Plasma Storm  2473, the shot of the Mega's gun (1482; the shot keeps its gun's class at pvars
 *                 +0x44): UpdateMovingEffectState (func_L00_003BAC30), each tick up to 15 targets
 *                 within 15 units all around
 *   Synthenoid    3250: UpdateAutoTargetFire (func_L00_003EB8E8) through moby_get_or_find_target
 *                 (func_L00_00324D18: the hero's own target first), in a cone around the camera's
 *                 forward (g_homing_yaw_degrees, g_homing_pitch_degrees) from the drone
 *   Kilonoid      2818, the Synthenoid's Mega: kilonoid_fire_shot (func_L00_003D40D8), the same with
 *                 g_kilonoid_aim_yaw_degrees and g_kilonoid_aim_pitch_degrees
 *   Seeker Gun    1497 and 3329, its seekers: UpdateHomingProjectile (func_L00_003AA708), up to 32
 *                 targets in its list within 8 units all around
 *   Miniturret    4966, the turret: projectile_launch_at_target (func_00304D38), all around
 *   Megaturret    4967, the Mega's turret: megaturret_find_target (func_00307500), all around
 * The pick is wrapped: while one of those mobys' update makes it (the moby loop runs them; the
 * hero's update runs the gun in his hand, which picks from its own position) and a mod asked for
 * its weapon, the live enemies not drawn within the pick's range and in sight of its origin (no
 * wall of the level on the line: CollLine, flags 18, as the game tries its pick) are marked drawn
 * for the pick and given back their flag after it. The drones' cone is opened all around (angle
 * and vertical at pi: their nearest enemy in range, by distance). Their range, kinds, caps and the
 * hero's own target first stay the game's.
 *
 * Two weapons test the flag in their own update:
 *   Shield Charger 2652 and its Mega, the Tesla Barrier, 4498: tesla_barrier_update
 *                 (func_L00_003C3D90) takes the enemies the area query around the hero returns
 *                 (coll_process_hit, func_00277F48: no flag of its own) and passes over the ones not
 *                 drawn, both for the shield's damage and for the barrier's arcs
 *   Tesla Claw    177: tesla_claw_update (func_L00_00392530) lets go of a held target that is not
 *                 drawn (or 120 degrees off the hero's facing, or 20 units away)
 * Those updates are wrapped the same way: around the barrier's, the enemies not drawn within
 * BARRIER_REACH of the hero and in his sight; around the claw's, its two held targets in his sight.
 *
 * Nothing else sees a flag changed: not the loop, the lock-on or the renderer. The player-aimed
 * locks (the hero's aim, the Lancer, the Minirocket Tube, the Gravity Bomb, the Bomb Glove, the
 * RYNO II, the Sheepinator's beam, the Tesla Claw's pick) follow the camera as the game has them.
 *
 * Only in play (game state 0, with the hero's moby): a scene (state 2, cutscene_update_frame,
 * func_L00_00370408) runs the mobys' updates too, the barrier's among them, over its own actors,
 * and gets the game's picks as they are. Every pointer followed from a moby is checked first: the
 * moby a slot of the table, its class, pvars and target record words of the game's RAM (a copy of
 * NG++ whose pvars are in the port's memory is passed over), the picker's own pvars in RAM or the
 * port's. Yeedil's scene before the last boss had a live enemy whose pvars word (+0x68) held a
 * float (179.7, 0x4333AC8F); moby_get_private_value_0 (func_002AB1F0) read it as a pointer.
 *
 * OPENRAC_PICK_TRACE=1 logs each widened pick that took an enemy not drawn; OPENRAC_PICK_TRACE=31,45
 * also widens those items' picks, as if a mod had asked (docs/port/DEBUGGING.md).
 */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "game_protos.h"
#include "moby_checks.h"
#include "openrac/host_env.h"
#include "rac2.h"
#include "weapon_shots.h"

#define MOBY_BASE OPENRAC_DATA(RAC2_g_mobyTableBase)
#define MOBY_END OPENRAC_DATA(RAC2_g_mobyTableEnd)
#define MOBY_SIZE 0x100u
/* The moby in the hero's hand: his item slot 0's, +0x1220 of his record (g_heroPos is its +0x80). */
#define HAND_MOBY (RAC2_g_heroPos - 0x80u + 0x1220u)
#define SLOTS 4096u
/* CollLine's result (D_001BAF80): +0x18 the moby it hit, 0 for the level. */
#define COLL_RESULT OPENRAC_DATA(RAC2_ADDR(0x001BAF80u))
#define COLL_HIT_MOBY 0x18u

enum {
    MOBY_POS = 0x10,
    MOBY_MODE = 0x34, /* 0x20: its pvars' first word is its target record (func_002AB1F0) */
    MODE_RECORD = 0x20,
    MOBY_STATE = 0x20, /* 0x80 and over: dead or free; 0xFF the table's end */
    MOBY_INFO = 0x24,  /* its class's record: +0x46 its kind, 5 an enemy */
    MOBY_DRAWN = 0x31,
    MOBY_PVARS = 0x68,
    MOBY_OCLASS = 0xAA,
    INFO_KIND = 0x46,
    RECORD_HEIGHT = 0x10, /* the target record: +0 its health, +0x10 its target height */
    KIND_ENEMY = 5,
    SHOT_GUN_CLASS = 0x44, /* the Plasma Storm shot's pvars: its gun's class */
    CLAW_TARGET = 0x20,    /* the Tesla Claw's pvars: its two held targets */
    CLAW_SECONDARY = 0x38,
    LINE_FLAGS = 18 /* SelectBestTargetInCone's line of sight */
};

enum {
    ITEM_SEEKER_GUN = 24,
    ITEM_PLASMA_COIL = 28,
    ITEM_SYNTHENOID = 31,
    ITEM_MINITURRET = 41,
    ITEM_ZODIAC = 43,
    ITEM_SHIELD_CHARGER = 45,
    ITEM_TESLA_CLAW = 18,
    CLASS_ZODIAC = 3784,
    CLASS_PLASMA_COIL_SHOT = 2715,
    CLASS_PLASMA_STORM_SHOT = 2473,
    CLASS_PLASMA_COIL = 3146,
    CLASS_PLASMA_STORM = 1482,
    CLASS_SYNTHENOID = 3250,
    CLASS_KILONOID = 2818,
    CLASS_SEEKER = 1497,
    CLASS_SEEKER_MEGA = 3329,
    CLASS_MINITURRET = 4966,
    CLASS_MEGATURRET = 4967,
    CLASS_SHIELD_CHARGER = 2652,
    CLASS_TESLA_BARRIER = 4498
};

/* A pick's cone opened all around: SelectBestTargetInCone skips its cone tests from 3.141593. */
#define ALL_AROUND 3.14159298f
/* How far from the hero the Shield Charger's and Tesla Barrier's area queries are looked over (the
 * query's own radius is smaller; it decides). */
#define BARRIER_REACH 24.0f
/* The hero's middle, over his feet, for his lines of sight. */
#define HERO_MIDDLE 1.0f

/* g_nGameState: 0 in play, 2 a scene. */
#define GAME_STATE OPENRAC_DATA(RAC2_g_nGameState)
#define STATE_PLAY 0
#define HERO_MOBY OPENRAC_DATA(RAC2_g_pHeroMoby)

/* A picker: the moby class whose update makes the pick and its weapon's item; `open` opens its
 * cone all around. */
typedef struct {
    int16_t oclass;
    int8_t item;
    uint8_t open;
} Picker;

static const Picker pickers[] = {
    {CLASS_ZODIAC, ITEM_ZODIAC, 0},
    {CLASS_PLASMA_COIL_SHOT, ITEM_PLASMA_COIL, 0},
    {CLASS_PLASMA_STORM_SHOT, ITEM_PLASMA_COIL, 0}, /* by its gun's class (weapon_of) */
    {CLASS_SYNTHENOID, ITEM_SYNTHENOID, 1},
    {CLASS_KILONOID, ITEM_SYNTHENOID, 1},
    {CLASS_SEEKER, ITEM_SEEKER_GUN, 0},
    {CLASS_SEEKER_MEGA, ITEM_SEEKER_GUN, 0},
    {CLASS_MINITURRET, ITEM_MINITURRET, 0},
    {CLASS_MEGATURRET, ITEM_MINITURRET, 0},
};

/* The weapons a mod asked for, one bit an item. */
static uint64_t held;

static int weapon_known(int32_t item) {
    if (item == ITEM_SHIELD_CHARGER || item == ITEM_TESLA_CLAW) {
        return 1;
    }
    for (unsigned i = 0; i < sizeof pickers / sizeof pickers[0]; ++i) {
        if (pickers[i].item == item) {
            return 1;
        }
    }
    return 0;
}

int rac2_weapon_ignore_culling(int32_t item, int enabled) {
    if (item < 0 || item >= 64 || !weapon_known(item)) {
        return 0;
    }
    const uint64_t bit = 1ull << item;
    held = enabled ? held | bit : held & ~bit;
    return 1;
}

/* The picker row of the moby `m`'s update, or null. */
static const Picker* picker_row(gaddr m) {
    const int16_t oclass = GREF(int16_t, m + MOBY_OCLASS);
    for (unsigned i = 0; i < sizeof pickers / sizeof pickers[0]; ++i) {
        if (pickers[i].oclass != oclass) {
            continue;
        }
        if (oclass == CLASS_PLASMA_STORM_SHOT) {
            const gaddr pvars = rac2_moby_pvars(m, 1);
            const int32_t gun = pvars != 0 ? GREF(int32_t, pvars + SHOT_GUN_CLASS) : 0;
            if (gun != CLASS_PLASMA_COIL && gun != CLASS_PLASMA_STORM) {
                return NULL;
            }
        }
        return &pickers[i];
    }
    return NULL;
}

/* The moby whose update makes the pick from `origin`: the one the moby loop runs now, else the gun
 * in the hero's hand picking from its own position (the hero's update runs the gun's, outside the
 * loop). 0 for another. */
static gaddr picker_of(gaddr origin) {
    const gaddr running = rac2_moby_updating();
    if (running != 0) {
        return rac2_moby_slot(running) ? running : 0;
    }
    const gaddr hand = GREF(gaddr, HAND_MOBY);
    return rac2_moby_slot(hand) && origin == hand + MOBY_POS ? hand : 0;
}

/* OPENRAC_PICK_TRACE: on (any value but "" and "0") logs the widened picks; the items it lists
 * ("31,45") are widened too, as if a mod had asked, for runs without one. */
static int trace_on = -1;
static uint64_t trace_held;

static void read_trace(void) {
    const char* v = openrac_env_string("OPENRAC_PICK_TRACE");
    trace_on = v != NULL && v[0] != '\0' && !(v[0] == '0' && v[1] == '\0');
    for (const char* c = v; trace_on && *c != '\0';) {
        char* next;
        const long item = strtol(c, &next, 10);
        if (next == c) {
            ++c;
            continue;
        }
        if (item >= 0 && item < 64 && weapon_known((int32_t)item)) {
            trace_held |= 1ull << item;
        }
        c = next;
    }
}

static int tracing(void) {
    return trace_on > 0;
}

/* The weapons widened: the mods' and the trace's. */
static uint64_t widened(void) {
    if (trace_on < 0) {
        read_trace();
    }
    return held | trace_held;
}

/* Whether a pick is widened now for `item`: asked for, and in play. */
static int holds(int item) {
    return item >= 0 && (widened() >> item & 1u) != 0 && rac2_in_play();
}

/* The point a line of sight to the moby `m` aims at: its position raised by its target height (its
 * target record's +0x10, as SelectBestTargetInCone raises it). 0 when it is no live target. The
 * record is its pvars' first word (moby_get_private_value_0, func_002AB1F0, with +0x34's 0x20), read
 * only from pvars in RAM and taken only when it is a record of RAM itself (moby_checks.h). */
static int aim_point(gaddr m, float to[3]) {
    const gaddr record = rac2_target_record(m);
    if (record == 0 || !(GREF(float, record) > 0.0f)) {
        return 0;
    }
    to[0] = GREF(float, m + MOBY_POS);
    to[1] = GREF(float, m + MOBY_POS + 4u);
    to[2] = GREF(float, m + MOBY_POS + 8u) + GREF(float, record + RECORD_HEIGHT);
    return isfinite(to[0]) && isfinite(to[1]) && isfinite(to[2]);
}

/* Whether a wall of the level stands between `from` and `to` (CollLine with the game's flags; the
 * target `m` passed over). A moby on the line does not block it. */
static int walled(const float from[3], const float to[3], gaddr m) {
    GFRAME(line, 32);
    for (unsigned c = 0; c < 3; ++c) {
        GREF(float, line + 4u * c) = from[c];
        GREF(float, line + 16u + 4u * c) = to[c];
    }
    GREF(float, line + 12u) = 0.0f;
    GREF(float, line + 28u) = 0.0f;
    if (func_002761F8(line, line + 16u, LINE_FLAGS, m, 0) == 0) {
        return 0;
    }
    return GREF(gaddr, COLL_RESULT + COLL_HIT_MOBY) == 0;
}

/* Whether the moby `m` is an enemy not drawn that may be marked: a slot of the table, live (not
 * dying, dead or free), its class a record of RAM of an enemy kind, its pvars in RAM, not `self`. */
static int undrawn_enemy(gaddr m, gaddr self) {
    if (m == self || !rac2_moby_slot(m)) {
        return 0;
    }
    const uint8_t state = GREF(uint8_t, m + MOBY_STATE);
    if (state >= 0x80 || GREF(uint8_t, m + MOBY_DRAWN) != 0) {
        return 0;
    }
    const gaddr info = GREF(gaddr, m + MOBY_INFO);
    return rac2_ram_at(info, INFO_KIND + 2u) && GREF(int16_t, info + INFO_KIND) == KIND_ENEMY
           && rac2_moby_pvars(m, 0) != 0;
}

/* The mobys marked drawn for one call, by slot, to give back after it. */
typedef struct {
    gaddr base;
    uint32_t n;
    uint16_t slot[SLOTS];
} Marks;

static void mark(Marks* k, gaddr m) {
    if (k->n < SLOTS) {
        GREF(uint8_t, m + MOBY_DRAWN) = 1;
        k->slot[k->n++] = (uint16_t)((m - k->base) / MOBY_SIZE);
    }
}

static int marked(const Marks* k, gaddr m) {
    for (uint32_t i = 0; i < k->n; ++i) {
        if (k->base + (gaddr)k->slot[i] * MOBY_SIZE == m) {
            return 1;
        }
    }
    return 0;
}

static void unmark(Marks* k) {
    for (uint32_t i = 0; i < k->n; ++i) {
        GREF(uint8_t, k->base + (gaddr)k->slot[i] * MOBY_SIZE + MOBY_DRAWN) = 0;
    }
    k->n = 0;
}

/* Marks drawn the enemies not drawn within `reach` of `from` and in its sight; `self` passed over. */
static void mark_in_sight(Marks* k, const float from[3], float reach, gaddr self) {
    k->n = 0;
    k->base = GREF(gaddr, MOBY_BASE);
    const gaddr end = GREF(gaddr, MOBY_END);
    if (!rac2_moby_slot(k->base) || end > openrac_guest_ram_size || !isfinite(from[0])
        || !isfinite(from[1]) || !isfinite(from[2])) {
        return;
    }
    const float reach2 = reach * reach;
    for (gaddr m = k->base; m < end && m - k->base < SLOTS * MOBY_SIZE; m += MOBY_SIZE) {
        if (GREF(uint8_t, m + MOBY_STATE) == 0xFF) {
            break;
        }
        if (!undrawn_enemy(m, self)) {
            continue;
        }
        float to[3];
        if (!aim_point(m, to)) {
            continue;
        }
        const float d[3] = {to[0] - from[0], to[1] - from[1], to[2] - from[2]};
        if (d[0] * d[0] + d[1] * d[1] + d[2] * d[2] > reach2 || walled(from, to, m)) {
            continue;
        }
        mark(k, m);
    }
}

static float distance_to(const float from[3], gaddr m) {
    const float d[3] =
        {GREF(float, m + MOBY_POS) - from[0],
         GREF(float, m + MOBY_POS + 4u) - from[1],
         GREF(float, m + MOBY_POS + 8u) - from[2]};
    return sqrtf(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
}

static void hero_middle(float at[3]) {
    at[0] = GREF(float, OPENRAC_DATA(RAC2_g_heroPos));
    at[1] = GREF(float, OPENRAC_DATA(RAC2_g_heroPos) + 4u);
    at[2] = GREF(float, OPENRAC_DATA(RAC2_g_heroPos) + 8u) + HERO_MIDDLE;
}

/* One set for each wrapper: a pick made inside the barrier's or the claw's update keeps theirs. */
static Marks pick_marks, barrier_marks, claw_marks;

/* SelectBestTargetInCone: the game's pick, with the enemies not drawn in range and in sight marked
 * drawn around it, and the drones' cone opened, for the weapons a mod asked for. */
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
    const gaddr picker = widened() != 0 && rac2_in_play() ? picker_of(origin) : 0;
    const Picker* row = picker != 0 ? picker_row(picker) : NULL;
    if (row == NULL || !holds(row->item) || origin == 0 || range <= 0.0f) {
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
    if (row->open) {
        angle = ALL_AROUND;
        vertical = ALL_AROUND;
    }
    const float from[3] = {GREF(float, origin), GREF(float, origin + 4u), GREF(float, origin + 8u)};
    /* SelectBestTargetInCone measures to the raised point: a little over the range. */
    mark_in_sight(&pick_marks, from, range + 2.0f, picker);
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
    if (tracing() && pick_marks.n != 0) {
        const int n = count != 0 ? GREF(int32_t, count) : 0;
        int off = 0;
        for (int i = 0; out != 0 && i < n && i < capacity; ++i) {
            off += marked(&pick_marks, GREF(gaddr, out + 4u * (gaddr)i));
        }
        if (off != 0 || (best != 0 && marked(&pick_marks, best))) {
            fprintf(
                stderr,
                "[pick] class %d item %d: best %08x class %d at %.1f%s, %d of %d listed off screen "
                "(%u in sight)\n",
                row->oclass,
                row->item,
                (unsigned)best,
                best != 0 ? GREF(int16_t, best + MOBY_OCLASS) : -1,
                best != 0 ? distance_to(from, best) : 0.0f,
                best != 0 && marked(&pick_marks, best) ? " off screen" : "",
                off,
                n,
                (unsigned)pick_marks.n
            );
        }
    }
    unmark(&pick_marks);
    return best;
}

/* tesla_barrier_update (the Shield Charger's and the Tesla Barrier's moby): the game's update,
 * with the enemies not drawn near the hero and in his sight marked drawn around it. */
void func_L00_003C3D90(gaddr self) {
    const int16_t oclass = self != 0 ? GREF(int16_t, self + MOBY_OCLASS) : 0;
    if (!holds(ITEM_SHIELD_CHARGER)
        || (oclass != CLASS_SHIELD_CHARGER && oclass != CLASS_TESLA_BARRIER)) {
        func_L00_003C3D90__game(self);
        return;
    }
    float from[3];
    hero_middle(from);
    mark_in_sight(&barrier_marks, from, BARRIER_REACH, self);
    if (barrier_marks.n == 0) {
        func_L00_003C3D90__game(self);
        return;
    }
    /* The barrier's arcs: its four targets (its pvars +0x20), to see which it took. */
    const gaddr pvars = rac2_moby_pvars(self, 1);
    gaddr before[4] = {0, 0, 0, 0};
    for (unsigned i = 0; pvars != 0 && i < 4; ++i) {
        before[i] = GREF(gaddr, pvars + 0x20u + 4u * i);
    }
    func_L00_003C3D90__game(self);
    for (unsigned i = 0; tracing() && pvars != 0 && i < 4; ++i) {
        const gaddr t = GREF(gaddr, pvars + 0x20u + 4u * i);
        if (t != 0 && t != before[i] && rac2_moby_slot(t) && marked(&barrier_marks, t)) {
            fprintf(
                stderr,
                "[pick] class %d item %d: arc to %08x class %d at %.1f off screen (%u in sight)\n",
                oclass,
                ITEM_SHIELD_CHARGER,
                (unsigned)t,
                GREF(int16_t, t + MOBY_OCLASS),
                distance_to(from, t),
                (unsigned)barrier_marks.n
            );
        }
    }
    unmark(&barrier_marks);
}

/* tesla_claw_update: the game's update, with its held targets not drawn and in the hero's sight
 * marked drawn around it, so that it keeps them. */
void func_L00_00392530(gaddr actor) {
    const gaddr pvars =
        holds(ITEM_TESLA_CLAW) && rac2_moby_slot(actor) ? rac2_moby_pvars(actor, 1) : 0;
    float from[3];
    hero_middle(from);
    if (pvars == 0 || !isfinite(from[0]) || !isfinite(from[1]) || !isfinite(from[2])) {
        func_L00_00392530__game(actor);
        return;
    }
    claw_marks.n = 0;
    claw_marks.base = GREF(gaddr, MOBY_BASE);
    const gaddr held_targets[2] =
        {GREF(gaddr, pvars + CLAW_TARGET), GREF(gaddr, pvars + CLAW_SECONDARY)};
    for (unsigned i = 0; i < 2; ++i) {
        const gaddr t = held_targets[i];
        float to[3];
        if (!undrawn_enemy(t, actor) || !aim_point(t, to) || walled(from, to, t)) {
            continue;
        }
        mark(&claw_marks, t);
    }
    func_L00_00392530__game(actor);
    if (tracing() && claw_marks.n != 0) {
        /* Once for each target it keeps off screen. */
        static gaddr last;
        const gaddr kept = GREF(gaddr, pvars + CLAW_TARGET);
        if (kept != 0 && kept != last && marked(&claw_marks, kept)) {
            fprintf(
                stderr,
                "[pick] class 177 item %d: holds %08x class %d at %.1f off screen\n",
                ITEM_TESLA_CLAW,
                (unsigned)kept,
                GREF(int16_t, kept + MOBY_OCLASS),
                distance_to(from, kept)
            );
            last = kept;
        }
    }
    unmark(&claw_marks);
}
