/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Ratchet & Clank: the hero put somewhere by a mod (game_host.h, openrac_game_play's hero_place;
 * interface 1.12, multiplayer: a late joiner beside the host, "Go to host" on the pause page).
 *
 * How the game does it. Hero_ResetToPosition (func_L00_00217718; rac1-ntsc's FUN_L00_00216f90) is
 * what its own teleporters and level scripts call: the hero and his moby at a place and a rotation,
 * his speeds, attachments and pad state cleared, his state set, the ground under him looked for, and
 * the camera started again behind him. Called between ticks' moby loops, as a trigger's update
 * would, it needs nothing else.
 *
 * When. As a scene a mod asks for (scenes.c): after the tick's moby loop (rac1_hero_place_tick,
 * UpdateMobys), in game mode 0 with the hero bound and alive. Asked in another mode (the pause menu,
 * a scene, the ship), the request waits for game mode 0; it goes with the level. On foot only: the
 * reset does not end a ride, so a hero on a vehicle (the hoverboard, the ship's guns, the rest of
 * vehicle_mode) keeps the request waiting.
 *
 * Whether a hero stands somewhere another can be put beside him (rac1_hero_steady, the pose's
 * OPENRAC_POSE_STEADY): on the level's own ground for a moment, not on a moby (a lift, a moving
 * platform: its place means nothing a tick later), not in water, not on a ride, not hurt or dying.
 *
 * Invulnerability is the game's own count, the hurt hero's blinking one (Hero.invulnerable_frames,
 * +0x1C0), down a tick.
 */
#include <math.h>

#include "hero_place.h"
#include "openrac/host_env.h"
#include "pose_anim.h"
#include "rac1.h"

#define LEVEL_MODE OPENRAC_LDATA(0, RAC1_LADDR(00, 0x0015F6A8u))
#define MODE_PLAY 0

/* CollOutput: what a line query hit, its point at +0x20 (collision.c). */
#define COLL_OUTPUT OPENRAC_LDATA(0, RAC1_LADDR(00, 0x00173F40u))
#define COLL_POINT 0x20u
/* A line query for what stands on anything (the hero's ground probe's flags), and CollType's
 * surfaces that are no ground: water (0) and quicksand (0xD). */
#define LINE_GROUND 2
#define SURFACE_WATER 0
#define SURFACE_SINK 0xD

/* The hero (include/hero.h; the same fields on both discs): his moby, state, health, ride, and the
 * counts his ground and water checks keep. */
#define HERO RAC1_ADDR(0x0013F450u)
#define HERO_MOBY (HERO + 0x2080u)
#define HERO_STATE (HERO + 0x2084u)
#define HERO_HEALTH (HERO + 0x22A8u)
#define HERO_VEHICLE (HERO + 0x20A4u)
#define HERO_INVULNERABLE (HERO + 0x1C0u)
#define HERO_WATER_FRAMES (HERO + 0x2F8u)
#define HERO_GROUND_MOBY (HERO + 0x2FCu)
#define HERO_GROUNDED_FRAMES (HERO + 0x300u)
#define HERO_AIR_FRAMES (HERO + 0x30Eu)

/* The states of a hero not to be stood beside (hero_state.h). */
#define STATE_HURT 0x16
#define STATE_DYING 0x3D
#define STATE_FATAL_FALL 0x77
#define STATE_DEATH_DESCENT 0x7C
#define STATE_SINK 0x7F
#define STATE_DYING_SPEED_LIMITED 0x80
#define STATE_DYING_ZERO_HEALTH 0x82

/* A hero steady for this many ticks of ground; how long a place keeps him from harm. */
#define STEADY_TICKS 10
#define PLACED_TICKS (3 * POSE_TICKS_A_SECOND)
/* The flags (openrac_mod.h, OPENRAC_PLACE_*). */
#define PLACE_INVULNERABLE 0x1u
#define PLACE_GROUND 0x2u
#define PLACE_BESIDE 0x4u

/* The place a mod asked for (none: no `wanted`), and the level load it was asked in. */
static struct {
    int wanted;
    uint32_t load;
    float position[3];
    float yaw;
    uint32_t flags;
} request;

/* Room in game memory for the vectors the game's calls read (four, sixteen bytes each). */
static gaddr vectors(void) {
    static gaddr room;
    if (room == 0) {
        room = openrac_guest_static(64);
    }
    return room;
}

static void put_vector(gaddr at, float x, float y, float z) {
    GREF(float, at) = x;
    GREF(float, at + 4u) = y;
    GREF(float, at + 8u) = z;
    GREF(float, at + 12u) = 0.0f;
}

static int line(const float from[3], const float to[3], int flags) {
    const gaddr room = vectors();
    put_vector(room, from[0], from[1], from[2]);
    put_vector(room + 16u, to[0], to[1], to[2]);
    return func_L00_001EFFF0(room, room + 16u, flags, (int)GREF(gaddr, HERO_MOBY), 0);
}

static int clear_between(void* user, const float from[3], const float to[3]) {
    (void)user;
    return !line(from, to, LINE_GROUND);
}

static int ground_under(void* user, const float at[3], float* z) {
    (void)user;
    const float top[3] = {at[0], at[1], at[2] + PLACE_STEP};
    const float bottom[3] = {at[0], at[1], at[2] - PLACE_STEP};
    if (!line(top, bottom, LINE_GROUND)) {
        return 0;
    }
    const int surface = func_L00_001F3958();
    if (surface == SURFACE_WATER || surface == SURFACE_SINK) {
        return 0;
    }
    *z = GREF(float, COLL_OUTPUT + COLL_POINT + 8u);
    return 1;
}

static int hero_ready(void) {
    return rac1_level_running() && GREF(gaddr, HERO_MOBY) != 0 && GREF(int32_t, HERO_HEALTH) > 0;
}

int rac1_hero_steady(void) {
    if (!hero_ready() || GREF(int, LEVEL_MODE) != MODE_PLAY) {
        return 0;
    }
    switch (GREF(int32_t, HERO_STATE)) {
        case STATE_HURT:
        case STATE_DYING:
        case STATE_FATAL_FALL:
        case STATE_DEATH_DESCENT:
        case STATE_SINK:
        case STATE_DYING_SPEED_LIMITED:
        case STATE_DYING_ZERO_HEALTH:
            return 0;
        default:
            break;
    }
    return GREF(uint8_t, HERO_VEHICLE) == 0 && GREF(int16_t, HERO_AIR_FRAMES) == 0
           && GREF(int32_t, HERO_GROUNDED_FRAMES) >= STEADY_TICKS
           && GREF(int32_t, HERO_WATER_FRAMES) == 0 && GREF(gaddr, HERO_GROUND_MOBY) == 0;
}

int rac1_hero_place(const float position[3], float yaw, uint32_t flags) {
    if (!hero_ready()) {
        return 0;
    }
    request.wanted = 1;
    request.load = rac1_moby_generation();
    request.position[0] = position[0];
    request.position[1] = position[1];
    request.position[2] = position[2];
    request.yaw = yaw;
    request.flags = flags;
    return 1;
}

void rac1_hero_place_tick(void) {
    if (!request.wanted) {
        return;
    }
    if (!rac1_level_running() || request.load != rac1_moby_generation()) {
        request.wanted = 0;
        return;
    }
    if (GREF(int, LEVEL_MODE) != MODE_PLAY || !hero_ready() || GREF(uint8_t, HERO_VEHICLE) != 0) {
        return;
    }
    request.wanted = 0;
    float at[3] = {request.position[0], request.position[1], request.position[2]};
    float yaw = request.yaw;
    if (request.flags & PLACE_BESIDE) {
        float spot[3];
        if (!place_beside(at, yaw, clear_between, ground_under, NULL, spot, &yaw)) {
            return;
        }
        at[0] = spot[0];
        at[1] = spot[1];
        at[2] = spot[2];
    } else if (request.flags & PLACE_GROUND) {
        float z;
        if (ground_under(NULL, at, &z)) {
            at[2] = z;
        }
    }
    const gaddr room = vectors();
    put_vector(room + 32u, at[0], at[1], at[2]);
    put_vector(room + 48u, 0.0f, 0.0f, yaw);
    /* State 0, standing; the camera started again behind him. */
    func_L00_00217718(room + 32u, room + 48u, 0, 1);
    if (request.flags & PLACE_INVULNERABLE) {
        GREF(int32_t, HERO_INVULNERABLE) = PLACED_TICKS;
    }
}
