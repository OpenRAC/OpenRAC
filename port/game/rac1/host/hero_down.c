/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Ratchet & Clank: a dead hero kept down, brought back, or sent to his checkpoint by a mod
 * (game_host.h, openrac_game_play's hero_revive; interface 1.12, multiplayer: the party's stand-over
 * revive and a full wipe).
 *
 * How the game does it. A dying state, once its animation has ended, calls Hero_OnKilled
 * (func_L00_00211908; rac1-ntsc's FUN_L00_00211250) each tick until something happens. That counts
 * the death, fades out and sets fall_fade_pending, and at the end of that frame the level loop makes
 * the level again from the checkpoint (a respawn).
 *
 * Down. While a mod holds (OPENRAC_REVIVE_HOLD), the call is left out for a hero who died on his
 * feet: he lies in the dying state's last frame, his health 0, until a mod says what comes next.
 * One who drowned, sank, fell or died on a ride goes as he always does: nobody could stand over him.
 *
 * Back. Hero_ResetToPosition (hero_place.c) takes him out of the dying state where he lies, or at
 * the place asked, between ticks' moby loops, with half his health and the hurt hero's blinking.
 *
 * Checkpoint. OPENRAC_REVIVE_RESPAWN calls the game's Hero_OnKilled as a death would, whatever state
 * the hero is in, so a game goes back by the only way the game has.
 */
#include "rac1.h"

#define LEVEL_MODE OPENRAC_LDATA(0, RAC1_LADDR(00, 0x0015F6A8u))
#define MODE_PLAY 0

/* The hero (include/hero.h; the same fields on both discs). */
#define HERO RAC1_ADDR(0x0013F450u)
#define HERO_POSITION (HERO + 0x80u)
#define HERO_ROTATION (HERO + 0x90u)
#define HERO_INVULNERABLE (HERO + 0x1C0u)
#define HERO_MOBY (HERO + 0x2080u)
#define HERO_STATE (HERO + 0x2084u)
#define HERO_VEHICLE (HERO + 0x20A4u)
#define HERO_FADE_PENDING (HERO + 0x20B1u)
#define HERO_HEALTH (HERO + 0x22A8u)
#define HERO_MAX_HEALTH RAC1_ADDR(0x0015EEA0u)

/* The dying states of a hero on his feet (hero_state.h). */
#define STATE_DYING 0x3D
#define STATE_DYING_SPEED_LIMITED 0x80
#define STATE_DYING_ZERO_HEALTH 0x82

/* How long the blinking lasts (three seconds of ticks). */
#define REVIVED_TICKS 180
/* The flags (openrac_mod.h, OPENRAC_REVIVE_*). */
#define REVIVE_HOLD 0x1u
#define REVIVE_LET_GO 0x2u
#define REVIVE_RESPAWN 0x4u

static struct {
    int hold;
    int held;
    int back;
    int respawn;
    int placed;
    uint32_t load;
    float position[3];
} down;

/* Hero_OnKilled: left out while the hero stays down, and for a player standing in, who is not this
 * game's hero (hero_grab.c). */
void func_L00_00211908(void) {
    if (rac1_hero_guarded()) {
        rac1_hero_guard_killed();
        return;
    }
    if (down.hold && GREF(uint8_t, HERO_VEHICLE) == 0) {
        switch (GREF(int32_t, HERO_STATE)) {
            case STATE_DYING:
            case STATE_DYING_SPEED_LIMITED:
            case STATE_DYING_ZERO_HEALTH:
                down.held = 1;
                return;
            default:
                break;
        }
    }
    down.held = 0;
    func_L00_00211908__game();
}

int rac1_hero_revive(const float position[3], uint32_t flags) {
    if (!rac1_level_running() || GREF(gaddr, HERO_MOBY) == 0) {
        return 0;
    }
    if (flags & (REVIVE_HOLD | REVIVE_LET_GO)) {
        down.hold = (flags & REVIVE_HOLD) != 0;
        return 1;
    }
    down.load = rac1_moby_generation();
    if (flags & REVIVE_RESPAWN) {
        down.respawn = 1;
        return 1;
    }
    if (GREF(int32_t, HERO_HEALTH) > 0) {
        return 0;
    }
    down.back = 1;
    down.placed = position != NULL;
    if (position != NULL) {
        down.position[0] = position[0];
        down.position[1] = position[1];
        down.position[2] = position[2];
    }
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

static void bring_back(void) {
    const gaddr room = vectors();
    memcpy(G(room), G(HERO_POSITION), 16);
    memcpy(G(room + 16u), G(HERO_ROTATION), 16);
    if (down.placed) {
        for (unsigned i = 0; i < 3; ++i) {
            GREF(float, room + 4u * i) = down.position[i];
        }
    }
    GREF(int32_t, HERO_HEALTH) = (GREF(int32_t, HERO_MAX_HEALTH) + 1) / 2;
    /* State 0, standing; the camera started again behind him. */
    func_L00_00217718(room, room + 16u, 0, 1);
    GREF(int32_t, HERO_INVULNERABLE) = REVIVED_TICKS;
}

void rac1_hero_down_tick(void) {
    if (!down.back && !down.respawn) {
        return;
    }
    if (!rac1_level_running() || down.load != rac1_moby_generation()) {
        down.back = down.respawn = 0;
        return;
    }
    if (GREF(int, LEVEL_MODE) != MODE_PLAY) {
        return;
    }
    if (down.respawn) {
        down.back = down.respawn = down.held = 0;
        if (!GREF(uint8_t, HERO_FADE_PENDING)) {
            func_L00_00211908__game();
        }
        return;
    }
    down.back = down.held = 0;
    bring_back();
}
