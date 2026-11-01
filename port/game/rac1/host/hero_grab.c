/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Ratchet & Clank: a moby's update that acts on the hero, when the hero it acts on is another
 * player's (game_host.h, openrac_game_play's stand_in_as and stand_in_read, hero_grab, hero_hold_at
 * and hero_release; interface 1.12, multiplayer: enemies that grab, in the game that runs them, and
 * the grabbed player's own game).
 *
 * What the game's mobys do. Most reach the hero with a hit, which a stand-in sends to the figure
 * (world.c). A few write him directly (both discs, read from the update):
 *   the snagglebeast (1106) puts him in state 0x78, which only looks at him, and pulls him along its
 *     rope for as long as it likes, turning him: position, rotation and a manipulator record each
 *     tick; state 0 lets him go;
 *   the sharkigator of Umbris (1059) leaps at him, sets his health to 0 and state 0x3D, stops the
 *     camera (+0x20A5) and from then on copies its own position to his, each tick, for as long as
 *     its jump's blend (pvars +0x14C) has run out: in a game of one, the respawn ends that;
 *   kill volumes and the like set his health to 0, or call Hero_OnKilled;
 *   triggers put him in a state of theirs (0x72, 0x1F, 0x77).
 *
 * The guard. Under a stand-in none of it may reach this game's hero. Each of the game's fourteen
 * copies of the state setter (hero_set_state; a level's hero code is its own) is wrapped, and only
 * writes the state the update reads; the two of the ride's start do nothing (a pad or a hatch would
 * make him Clank); Hero_OnKilled (hero_down.c) only counts. What else the update
 * can write is put back when it ends (rac1_hero_guard_end): the state, the health, the rotation, the
 * manipulator record, the air counts, the camera's stop, the fall's fade. Before that the mod reads what the update
 * did (rac1_hero_guard_read: the gate's call after an update).
 *
 * What the update sees. An update that looks at the hero's state, health or whether he is in the air
 * must see the player it acts on, or a grab's own test ("not yet in state 0x78") would pass or fail
 * by what this game's hero is doing. stand_in_as writes those for the update; plain stand_in leaves
 * them as they are.
 *
 * The other side. The held player's game holds its own hero, the way the game does: into the state
 * by Hero_ResetToPosition with no camera reset, as the Rilgar sharkigator takes him, at the end of the
 * tick's moby loop (rac1_hero_grab_tick); 0x78 for a hold, and for a fatal grab what the Umbris
 * sharkigator does, 0x3D at health 0 with the camera stopped. Each tick he goes on towards the
 * place asked (hero_grab.h). A new state ends it: the game's own end, a death or a respawn. Let go, he
 * stands up in state 0 where he is, unless he has no health: then what the game does with a dead
 * hero goes on.
 */
#include <string.h>

#include "hero_grab.h"
#include "rac1.h"

#define LEVEL_MODE OPENRAC_LDATA(0, RAC1_LADDR(00, 0x0015F6A8u))
#define MODE_PLAY 0

/* The hero (include/hero.h; the same fields on both discs). */
#define HERO RAC1_ADDR(0x0013F450u)
#define HERO_POSITION (HERO + 0x80u)
#define HERO_ROTATION (HERO + 0x90u)
#define HERO_GROUND_POINT (HERO + 0xC0u)
#define HERO_BODY_CENTRE (HERO + 0xD0u)
#define HERO_FRAMES_OFF_GROUND (HERO + 0x30Cu)
#define HERO_AIR_FRAMES (HERO + 0x30Eu)
/* The manipulator record the rope moves with him (+0x1D30, its yaw at +0x1D48). */
#define HERO_HAND (HERO + 0x1D30u)
#define HERO_MOBY (HERO + 0x2080u)
#define HERO_STATE (HERO + 0x2084u)
#define HERO_VEHICLE (HERO + 0x20A4u)
#define HERO_CAMERA_STOPPED (HERO + 0x20A5u)
#define HERO_FADE_PENDING (HERO + 0x20B1u)
#define HERO_HEALTH (HERO + 0x22A8u)

/* A word of the hero's moby the Umbris sharkigator's kill sets to -1. */
#define MOBY_KILL_WORD 0x98u
#define MOBY_STATE 0x20u
#define MOBY_CLASS_ID 0xA6u
#define MOBY_PVARS 0x78u

/* The states of a hold and of a fatal grab (hero_state.h; 0x78 is CLEAR_VELOCITY there). */
#define STATE_STANDING 0
#define STATE_HELD 0x78
#define STATE_DYING 0x3D

/* The Umbris sharkigator: its class, its state after the kill, its jump's blend. */
#define UMBRIS_SHARKIGATOR 1059
#define UMBRIS_SHARKIGATOR_FED 6
#define UMBRIS_SHARKIGATOR_BLEND 0x14Cu

/* How fast a held hero turns, as the snagglebeast turns him (25.13 a second, a 60th a tick). */
#define SPIN_A_TICK 0.41887903f

/* The flags (openrac_mod.h). */
#define STAND_IN_AIRBORNE 0x1u
#define STAND_IN_KILLED 0x2u
#define GRAB_FATAL 0x1u
#define GRAB_SPIN 0x2u

static struct {
    int on;
    int killed;
    int32_t state;
    int32_t health;
    int16_t off_ground;
    int16_t air;
    uint8_t rotation[16];
    uint8_t hand[32];
    uint8_t camera_stopped;
    uint8_t fade_pending;
} guard;

static struct {
    int asked;
    int held;
    int let_go;
    int placed;
    int state;
    int animation;
    uint32_t flags;
    uint32_t load;
    float wanted[3];
    float yaw;
} grab;

int rac1_hero_guarded(void) {
    return guard.on;
}

void rac1_hero_guard_killed(void) {
    guard.killed = 1;
}

void rac1_hero_guard_begin(void) {
    guard.on = 1;
    guard.killed = 0;
    guard.state = GREF(int32_t, HERO_STATE);
    guard.health = GREF(int32_t, HERO_HEALTH);
    guard.off_ground = GREF(int16_t, HERO_FRAMES_OFF_GROUND);
    guard.air = GREF(int16_t, HERO_AIR_FRAMES);
    guard.camera_stopped = GREF(uint8_t, HERO_CAMERA_STOPPED);
    guard.fade_pending = GREF(uint8_t, HERO_FADE_PENDING);
    memcpy(guard.rotation, G(HERO_ROTATION), sizeof guard.rotation);
    memcpy(guard.hand, G(HERO_HAND), sizeof guard.hand);
}

void rac1_hero_guard_show(int32_t state, int32_t health, uint32_t flags) {
    GREF(int32_t, HERO_STATE) = state;
    GREF(int32_t, HERO_HEALTH) = health;
    const int16_t air = (flags & STAND_IN_AIRBORNE) != 0 ? 1 : 0;
    GREF(int16_t, HERO_FRAMES_OFF_GROUND) = air;
    GREF(int16_t, HERO_AIR_FRAMES) = air;
}

int rac1_hero_guard_read(
    float position[3], float* yaw, int32_t* state, int32_t* health, uint32_t* flags
) {
    if (!guard.on) {
        return 0;
    }
    memcpy(position, G(HERO_POSITION), 3 * sizeof(float));
    *yaw = GREF(float, HERO_ROTATION + 8u);
    *state = GREF(int32_t, HERO_STATE);
    *health = GREF(int32_t, HERO_HEALTH);
    *flags = guard.killed ? STAND_IN_KILLED : 0;
    return 1;
}

void rac1_hero_guard_end(gaddr moby) {
    if (!guard.on) {
        return;
    }
    guard.on = 0;
    GREF(int32_t, HERO_STATE) = guard.state;
    GREF(int32_t, HERO_HEALTH) = guard.health;
    GREF(int16_t, HERO_FRAMES_OFF_GROUND) = guard.off_ground;
    GREF(int16_t, HERO_AIR_FRAMES) = guard.air;
    GREF(uint8_t, HERO_CAMERA_STOPPED) = guard.camera_stopped;
    GREF(uint8_t, HERO_FADE_PENDING) = guard.fade_pending;
    memcpy(G(HERO_ROTATION), guard.rotation, sizeof guard.rotation);
    memcpy(G(HERO_HAND), guard.hand, sizeof guard.hand);
    /* Its meal was another game's hero, so no respawn of this game's will end the copy to his
     * position: without this it would carry this game's hero off at its next update with no
     * stand-in. */
    if (moby != 0 && GREF(int16_t, moby + MOBY_CLASS_ID) == UMBRIS_SHARKIGATOR
        && GREF(uint8_t, moby + MOBY_STATE) == UMBRIS_SHARKIGATOR_FED) {
        GREF(float, GREF(gaddr, moby + MOBY_PVARS) + UMBRIS_SHARKIGATOR_BLEND) = 0.0f;
    }
}

/* hero_set_state, each copy. */
#define GUARDED_SETTER(name, game)                                                                 \
    int name(int state, int animate) {                                                             \
        if (guard.on) {                                                                            \
            GREF(int32_t, HERO_STATE) = state;                                                     \
            return 1;                                                                              \
        }                                                                                          \
        return game(state, animate);                                                               \
    }

GUARDED_SETTER(func_L00_00222B80, func_L00_00222B80__game)
GUARDED_SETTER(func_L01_0023D688, func_L01_0023D688__game)
GUARDED_SETTER(func_L02_0022BE40, func_L02_0022BE40__game)
GUARDED_SETTER(func_L03_00216D38, func_L03_00216D38__game)
GUARDED_SETTER(func_L05_0024D680, func_L05_0024D680__game)
GUARDED_SETTER(func_L06_00235E08, func_L06_00235E08__game)
GUARDED_SETTER(func_L08_00231228, func_L08_00231228__game)
GUARDED_SETTER(func_L11_0024E240, func_L11_0024E240__game)
GUARDED_SETTER(func_L12_002407C0, func_L12_002407C0__game)
GUARDED_SETTER(func_L14_002306C8, func_L14_002306C8__game)
GUARDED_SETTER(func_L15_00217348, func_L15_00217348__game)
GUARDED_SETTER(func_L16_0021EA88, func_L16_0021EA88__game)
GUARDED_SETTER(func_L17_0021ED38, func_L17_0021ED38__game)
GUARDED_SETTER(func_L18_002284E0, func_L18_002284E0__game)

/* The ride or Clank taken up, each copy: nothing of it under a stand-in. */
void func_L00_002110C0(int mode, int state, gaddr moby) {
    if (!guard.on) {
        func_L00_002110C0__game(mode, state, moby);
    }
}

void func_L01_00231960(int mode, int state, int moby) {
    if (!guard.on) {
        func_L01_00231960__game(mode, state, moby);
    }
}

int rac1_hero_grab(int animation, uint32_t flags) {
    if (!rac1_level_running() || GREF(gaddr, HERO_MOBY) == 0) {
        return 0;
    }
    if (grab.held) {
        return 1;
    }
    grab.asked = 1;
    grab.let_go = 0;
    grab.placed = 0;
    grab.animation = animation;
    grab.flags = flags;
    grab.state = (flags & GRAB_FATAL) != 0 ? STATE_DYING : STATE_HELD;
    grab.load = rac1_moby_generation();
    return 1;
}

int rac1_hero_hold_at(const float position[3], float yaw) {
    if (!grab.asked && !grab.held) {
        return 0;
    }
    grab.placed = 1;
    memcpy(grab.wanted, position, sizeof grab.wanted);
    grab.yaw = yaw;
    return 1;
}

int rac1_hero_let_go(void) {
    if (grab.asked) {
        grab.asked = 0;
        return 1;
    }
    if (!grab.held) {
        return 0;
    }
    grab.let_go = 1;
    return 1;
}

/* Room in game memory for the vectors the game's call reads. */
static gaddr vectors(void) {
    static gaddr room;
    if (room == 0) {
        room = openrac_guest_static(32);
    }
    return room;
}

/* Hero_ResetToPosition where he is, in `state`, the camera left alone. */
static void reset_in_state(int state) {
    const gaddr room = vectors();
    memcpy(G(room), G(HERO_POSITION), 16);
    memcpy(G(room + 16u), G(HERO_ROTATION), 16);
    func_L00_00217718(room, room + 16u, state, 0);
}

static void take(void) {
    grab.asked = 0;
    grab.held = 1;
    if ((grab.flags & GRAB_FATAL) != 0) {
        GREF(int32_t, HERO_HEALTH) = 0;
    }
    reset_in_state(grab.state);
    if ((grab.flags & GRAB_FATAL) != 0) {
        GREF(int32_t, GREF(gaddr, HERO_MOBY) + MOBY_KILL_WORD) = -1;
        GREF(uint8_t, HERO_CAMERA_STOPPED) = 1;
    }
    if (grab.animation >= 0) {
        func_L00_00232C10(grab.animation, 0, -1.0f);
    }
}

static void hold(void) {
    if (!grab.placed) {
        return;
    }
    float was[3];
    float at[3];
    memcpy(was, G(HERO_POSITION), sizeof was);
    memcpy(at, was, sizeof at);
    grab_step(at, grab.wanted);
    for (unsigned c = 0; c < 3; ++c) {
        const float step = at[c] - was[c];
        GREF(float, HERO_POSITION + 4u * c) += step;
        GREF(float, HERO_GROUND_POINT + 4u * c) += step;
        GREF(float, HERO_BODY_CENTRE + 4u * c) += step;
    }
    if ((grab.flags & GRAB_SPIN) != 0) {
        GREF(float, HERO_ROTATION + 8u) += SPIN_A_TICK;
    } else {
        GREF(float, HERO_ROTATION + 8u) = grab.yaw;
    }
}

void rac1_hero_grab_tick(void) {
    if (!grab.asked && !grab.held) {
        return;
    }
    if (!rac1_level_running() || grab.load != rac1_moby_generation()) {
        grab.asked = grab.held = grab.let_go = 0;
        return;
    }
    if (GREF(int, LEVEL_MODE) != MODE_PLAY) {
        return;
    }
    if (grab.asked) {
        /* A ride, and a hero already dead, are the game's to end. */
        if (GREF(uint8_t, HERO_VEHICLE) != 0 || GREF(int32_t, HERO_HEALTH) <= 0) {
            return;
        }
        take();
        return;
    }
    if (GREF(int32_t, HERO_STATE) != grab.state) {
        grab.held = grab.let_go = 0;
        return;
    }
    if (grab.let_go) {
        grab.held = grab.let_go = 0;
        if (GREF(int32_t, HERO_HEALTH) > 0) {
            reset_in_state(STATE_STANDING);
        }
        return;
    }
    hold();
}
