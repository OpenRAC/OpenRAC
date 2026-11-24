/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Going Commando: OPENRAC_RAC2_DEATH_TRACE=1 logs, on stderr, how Siberius's cart riders and their
 * claws come and go, and the hero's sinking count (docs/port/DEBUGGING.md).
 *
 * The riders: class 3369 (UpdateSiberiusEntityActions, func_L07_003E61E0, through
 * shooter_entity_base_update, func_L07_003E69B8) and the shooters, class 3370 (func_L07_003EA900).
 * A 3369 spawns two claws of class 2722 as it starts (enemy_spawn_child_parts, func_00325B70) and
 * keeps them at pvars +0x330 and +0x334; only its own update moves them
 * (enemy_attach_turn_child_parts, func_00325C50, from entity_finish_update) and only its death
 * deletes them (group_destroy_attached_parts, func_00325D30). Its start-up sets its death path (the
 * target record's +0x0E) to 1: the Zodiac's pulse then sets 2 and leaves the death to it, which
 * its next update runs (the claws and itself deleted). A rider that waits to be thrown onto the cart
 * starts hidden (+0x34's 0x41, state 11).
 *
 * Each tick, before the moby loop, every slot holding one of those classes (or that did last tick)
 * is compared with the last look: a line for its state's changes into or out of the live states
 * and into 0, 10 or 11, its hidden bits (+0x34's 0x81), its death path, its claws, its slot freed or
 * given out to another class, and a claw whose rider is gone or is no rider (once). SetDeathBits
 * (func_002A6C58, enemy_copies.c's wrap) prints one line for each death of those classes, with the
 * moby whose update records it (the Zodiac's gun, the hero, or none: a mod's).
 *
 * The hero's sinking (hero_l00_update_ground_contact, func_L00_002B4900): his ground probe stores
 * the surface's material at +0x1490 and its type at +0x149D of his record (D_00189E20); with
 * material 0 the sink height at +0x330 is the ground's height there (func_002A90F8), and while he is
 * below it the count at +0x338 runs from 1 up, every frame, until he is above it again. A line each
 * time the count starts, every 15 frames while it runs, and when it stops: his height (+0x88), the
 * sink height, the count, the material, the type, his state (+0x2294) and the moby the last
 * collision line hit (g_collision_result +0x18). */
#include "death_trace.h"

#include <stdio.h>
#include <string.h>

#include "enemy_copies.h"
#include "game_protos.h"
#include "moby_checks.h"
#include "openrac/host_env.h"
#include "rac2.h"
#include "weapon_shots.h"

#define MOBY_BASE OPENRAC_DATA(RAC2_g_mobyTableBase)
#define MOBY_END OPENRAC_DATA(RAC2_g_mobyTableEnd)
#define MOBY_SIZE 0x100u
#define SLOTS 4096u

enum {
    MOBY_STATE = 0x20,
    MOBY_MODE = 0x34,
    MODE_RECORD = 0x20,
    MODE_HIDDEN = 0x81,
    MOBY_PVARS = 0x68,
    MOBY_OCLASS = 0xAA,
    MOBY_PARENT = 0xB8, /* a part's rider (moby_init_child_from_parent, func_002B37E8) */
    RECORD_PATH = 0x0E, /* the target record's death path */
    RECORD_WEAPON = 0x17,
    RIDER_CLAWS = 0x330,
    CLASS_RIDER = 3369,
    CLASS_SHOOTER = 3370,
    CLASS_CLAW = 2722
};

/* The hero's record (D_00189E20): g_heroPos is its +0x80. */
#define HERO (OPENRAC_DATA(RAC2_g_heroPos) - 0x80u)
#define HERO_Z 0x88u
#define HERO_SINK_HEIGHT 0x330u
#define HERO_SINK_COUNT 0x338u
#define HERO_MATERIAL 0x1490u
#define HERO_SURFACE 0x149Du
#define HERO_STATE 0x2294u
/* CollLine's result (D_001BAF80): +0x18 the moby it hit, 0 for the level. */
#define COLL_RESULT OPENRAC_DATA(RAC2_ADDR(0x001BAF80u))
#define COLL_HIT_MOBY 0x18u
#define SINK_EVERY 15

static int on = -1;

int rac2_death_trace_on(void) {
    if (on < 0) {
        const char* v = openrac_env_string("OPENRAC_RAC2_DEATH_TRACE");
        on = v != NULL && v[0] != '\0' && !(v[0] == '0' && v[1] == '\0');
    }
    return on;
}

static int watched(int16_t oclass) {
    return oclass == CLASS_RIDER || oclass == CLASS_SHOOTER || oclass == CLASS_CLAW;
}

/* The target record of `m` (its pvars' first word with +0x34's 0x20), when both are memory. */
static gaddr record_of(gaddr m) {
    if ((GREF(uint16_t, m + MOBY_MODE) & MODE_RECORD) == 0) {
        return 0;
    }
    const gaddr pvars = rac2_moby_pvars(m, 1);
    const gaddr record = pvars != 0 ? GREF(gaddr, pvars) : 0;
    return rac2_memory_at(record, 0x18u) ? record : 0;
}

/* What the last look saw of a slot. */
typedef struct {
    int16_t oclass;
    uint8_t seen;
    uint8_t state;
    uint8_t hidden;
    uint8_t path;
    uint8_t orphan_told;
    gaddr claw[2];
} Look;

static Look looks[SLOTS];
static uint32_t looks_load;
static gaddr looks_base;
static uint32_t ticks;
static int32_t sink_last;

static int interesting_state(uint8_t s) {
    return s == 0 || s == 10 || s == 11;
}

static void rider_claws(gaddr m, gaddr out[2]) {
    out[0] = out[1] = 0;
    const gaddr pvars = rac2_moby_pvars(m, 1);
    if (pvars == 0 || !rac2_memory_at(pvars + RIDER_CLAWS, 8u)) {
        return;
    }
    out[0] = GREF(gaddr, pvars + RIDER_CLAWS);
    out[1] = GREF(gaddr, pvars + RIDER_CLAWS + 4u);
}

static int16_t class_at(gaddr m) {
    return rac2_moby_slot(m) ? GREF(int16_t, m + MOBY_OCLASS) : -1;
}

static void look_at(uint32_t slot, gaddr m) {
    Look* was = &looks[slot];
    const uint8_t state = GREF(uint8_t, m + MOBY_STATE);
    const int16_t oclass = GREF(int16_t, m + MOBY_OCLASS);
    const int live = state < 0x80u;
    if (!was->seen && !(live && watched(oclass))) {
        return;
    }
    /* Its slot freed, or given out to another class: one line, then forgotten. */
    if (was->seen && (oclass != was->oclass || (!live && was->state < 0x80u))) {
        fprintf(
            stderr,
            "[death] t=%u slot %u class %d %s (state %u -> %u, now class %d)%s\n",
            ticks,
            slot,
            was->oclass,
            oclass != was->oclass ? "slot given out again" : "freed",
            was->state,
            state,
            oclass,
            rac2_copy_is(m) ? " copy" : ""
        );
        memset(was, 0, sizeof *was);
        if (!(live && watched(oclass))) {
            return;
        }
    }
    const uint8_t hidden = (uint8_t)(GREF(uint16_t, m + MOBY_MODE) & MODE_HIDDEN);
    const gaddr record = record_of(m);
    const uint8_t path = record != 0 ? GREF(uint8_t, record + RECORD_PATH) : 0xFFu;
    gaddr claw[2] = {0, 0};
    if (oclass == CLASS_RIDER) {
        rider_claws(m, claw);
    }
    if (!was->seen) {
        fprintf(
            stderr,
            "[death] t=%u slot %u class %d seen: state %u hidden %02x path %u claws %08x %08x%s\n",
            ticks,
            slot,
            oclass,
            state,
            hidden,
            path,
            (unsigned)claw[0],
            (unsigned)claw[1],
            rac2_copy_is(m) ? " copy" : ""
        );
    } else {
        if (state != was->state && (interesting_state(state) || interesting_state(was->state))) {
            fprintf(
                stderr,
                "[death] t=%u slot %u class %d state %u -> %u\n",
                ticks,
                slot,
                oclass,
                was->state,
                state
            );
        }
        if (hidden != was->hidden) {
            fprintf(
                stderr,
                "[death] t=%u slot %u class %d hidden %02x -> %02x (state %u)\n",
                ticks,
                slot,
                oclass,
                was->hidden,
                hidden,
                state
            );
        }
        if (path != was->path) {
            fprintf(
                stderr,
                "[death] t=%u slot %u class %d death path %u -> %u (state %u, health %.1f)\n",
                ticks,
                slot,
                oclass,
                was->path,
                path,
                state,
                record != 0 ? (double)GREF(float, record) : 0.0
            );
        }
        if (claw[0] != was->claw[0] || claw[1] != was->claw[1]) {
            fprintf(
                stderr,
                "[death] t=%u slot %u class %d claws %08x %08x -> %08x %08x\n",
                ticks,
                slot,
                oclass,
                (unsigned)was->claw[0],
                (unsigned)was->claw[1],
                (unsigned)claw[0],
                (unsigned)claw[1]
            );
        }
    }
    /* A claw whose rider is gone or is no rider: drawn and hit with nothing to move or delete it. */
    if (oclass == CLASS_CLAW && live && !was->orphan_told) {
        const gaddr rider = GREF(gaddr, m + MOBY_PARENT);
        const int16_t rider_class = class_at(rider);
        if (rider != 0
            && (rider_class != CLASS_RIDER || GREF(uint8_t, rider + MOBY_STATE) >= 0x80u)) {
            fprintf(
                stderr,
                "[death] t=%u slot %u claw left alone: its rider %08x is class %d state %u "
                "hidden %02x\n",
                ticks,
                slot,
                (unsigned)rider,
                rider_class,
                rac2_moby_slot(rider) ? GREF(uint8_t, rider + MOBY_STATE) : 0xFFu,
                rac2_moby_slot(rider) ? GREF(uint16_t, rider + MOBY_MODE) & MODE_HIDDEN : 0u
            );
            was->orphan_told = 1;
        }
    }
    was->seen = 1;
    was->oclass = oclass;
    was->state = state;
    was->hidden = hidden;
    was->path = path;
    was->claw[0] = claw[0];
    was->claw[1] = claw[1];
}

static void hero_sinking(void) {
    const gaddr hero = HERO;
    if (!rac2_ram_at(hero, HERO_STATE + 4u)) {
        return;
    }
    const int32_t count = GREF(int32_t, hero + HERO_SINK_COUNT);
    const int starts = count != 0 && sink_last == 0;
    const int stops = count == 0 && sink_last != 0;
    if (starts || stops || (count != 0 && count % SINK_EVERY == 0)) {
        fprintf(
            stderr,
            "[death] t=%u hero sink count %d: z %.2f sink height %.2f material %d surface %u state "
            "%d line hit %08x\n",
            ticks,
            (int)count,
            (double)GREF(float, hero + HERO_Z),
            (double)GREF(float, hero + HERO_SINK_HEIGHT),
            (int)GREF(int16_t, hero + HERO_MATERIAL),
            (unsigned)GREF(uint8_t, hero + HERO_SURFACE),
            (int)GREF(int32_t, hero + HERO_STATE),
            (unsigned)GREF(gaddr, COLL_RESULT + COLL_HIT_MOBY)
        );
    }
    sink_last = count;
}

void rac2_death_trace_tick(void) {
    if (!rac2_death_trace_on()) {
        return;
    }
    ++ticks;
    const gaddr base = GREF(gaddr, MOBY_BASE);
    const gaddr end = GREF(gaddr, MOBY_END);
    if (looks_load != rac2_program_loads() || looks_base != base) {
        memset(looks, 0, sizeof looks);
        looks_load = rac2_program_loads();
        looks_base = base;
    }
    if (!rac2_moby_slot(base) || end > openrac_guest_ram_size) {
        return;
    }
    for (gaddr m = base; m < end && (m - base) / MOBY_SIZE < SLOTS; m += MOBY_SIZE) {
        if (GREF(uint8_t, m + MOBY_STATE) == 0xFFu) {
            break;
        }
        look_at((m - base) / MOBY_SIZE, m);
    }
    if (rac2_in_play()) {
        hero_sinking();
    }
}

void rac2_death_trace_death(gaddr moby, int flags, int arg) {
    if (!rac2_death_trace_on() || !rac2_moby_slot(moby)) {
        return;
    }
    const int16_t oclass = GREF(int16_t, moby + MOBY_OCLASS);
    if (!watched(oclass)) {
        return;
    }
    const gaddr record = record_of(moby);
    const gaddr by = rac2_moby_updating();
    fprintf(
        stderr,
        "[death] t=%u death %08x class %d state %u hidden %02x path %u health %.1f weapon %u "
        "flags %x arg %d by %08x class %d%s\n",
        ticks,
        (unsigned)moby,
        oclass,
        GREF(uint8_t, moby + MOBY_STATE),
        GREF(uint16_t, moby + MOBY_MODE) & MODE_HIDDEN,
        record != 0 ? GREF(uint8_t, record + RECORD_PATH) : 0xFFu,
        record != 0 ? (double)GREF(float, record) : 0.0,
        record != 0 ? GREF(uint8_t, record + RECORD_WEAPON) : 0xFFu,
        (unsigned)flags,
        arg,
        (unsigned)by,
        by != 0 ? class_at(by) : -1,
        rac2_copy_is(moby) ? " copy" : ""
    );
}
