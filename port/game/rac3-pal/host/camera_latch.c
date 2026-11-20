/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Up Your Arsenal: what the follow camera's right stick turn will be, for the picture's late camera
 * latch (settings responsive_controls, frontend/camera_latch.cpp), as Going Commando's
 * (rac2/host/camera_latch.c) and Ratchet & Clank's (rac1/host/camera_latch.c says what the latch
 * does with it).
 *
 * Each tick the follow camera's update (Veldin's func_L01_003EA648, Going Commando's
 * func_L00_00426340) reads the sticks (func_L01_003E92F8, Going Commando's func_L00_00424F98, the
 * same steps): its x, the pad's analog float at +0x100 of the player's pad block (the pointer at
 * +0x134 of the player's camera entry, 0x001E2B90 + 0x460 a player in Veldin; the pad code's dead
 * zone of 0x30 about 0x7F, the rest over 0x4C, as the profile's stick_dead_zone says), negated
 * unless the horizontal invert option of the camera setting in use is set (0x00143A70, a row of 8
 * players' words for each setting 0x001439F4 picks; the read takes the row's first word), and 0 while the camera's stick state (its
 * data, the moby's +0x70, + 0x1E0; Going Commando's + 0x1C0) has bit 0 of its flags (+0x10) set.
 * func_L01_003E9500 (Going Commando's func_L00_00425118) then eases the turn towards that through
 * the shared spring (0x00382BC8, Going Commando's func_002702C8: the value at +0x00, its speed at
 * +0x04; here with a stiffness of 2 and a damping of 4, constants, where Going Commando's are the
 * level's) and turns the camera's offset from the hero (+0x180) about the axis of the player's
 * camera entry (0x001E2AA0 + 0x20 + 0x460 a player) by that value times the turn speed at +0x14
 * (set from 0x001D5FF0 by player whenever the stick is moved).
 *
 * The read runs under a host function that only notes the camera it read for, so that the frontend
 * can ask between ticks what the next ticks would turn with the stick as it is now: the same spring
 * stepped from the camera's state, never written back. The vertical stick is left out, as in
 * Ratchet & Clank and Going Commando. The multiplayer maps' cameras (levels 39 to 53, their own
 * copies) are not latched.
 */
#include <math.h>
#include <stdint.h>

#include "game_protos.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"

/* A global of Veldin's program, in the program loaded now. */
#define VELDIN(a) OPENRAC_LDATA(1, (gaddr)(a))

#define CAMERA_SETTING 0x001439F4u      /* byte: 0 third person, 1 lock-strafe, 2 first person */
#define CAMERA_INVERT_H 0x00143A70u     /* 3 settings x 8 players' words */
#define CAMERA_PADS VELDIN(0x001E2B90u) /* +0x134 the player's pad block */
#define CAMERA_AXES VELDIN(0x001E2AA0u) /* +0x20 the axis the offset turns about */
#define CAMERA_ENTRY 0x460u

/* The camera's stick state: its data (the moby's +0x70) + 0x1E0. */
#define STATE_VALUE 0x00u
#define STATE_SPEED 0x04u
#define STATE_FLAGS 0x10u
#define STATE_TURN_SPEED 0x14u
#define STATE_SIZE 0x30u

/* The spring the turn eases through (0x00382BC8's arguments at func_L01_003E9500's call). */
#define SPRING_STIFFNESS 2.0f
#define SPRING_DAMPING 4.0f

static gaddr s_state;     /* the stick state the last read was for */
static unsigned s_player; /* the camera's player (the moby's +0x94) */
static unsigned s_reads;  /* how many reads so far: the frontend sees whether one ran this tick */

static int locked(gaddr state) {
    return (GREF(uint32_t, state + STATE_FLAGS) & 1u) != 0;
}

void func_L01_003E92F8(gaddr camera, gaddr x, gaddr y) {
    func_L01_003E92F8__game(camera, x, y);
    s_state = GREF(gaddr, camera + 0x70u) + 0x1E0u;
    s_player = GREF(uint32_t, camera + 0x94u);
    ++s_reads;
    /* Sensitivity above 100%: scale the stick's turn, as in rac1 and rac2 (not the turn the camera
     * keeps from its state's +0x1C while the stick is centred). */
    if (openrac_game_camera_speed != 1.0f && s_player < 8 && !locked(s_state)) {
        const gaddr pad = GREF(gaddr, CAMERA_PADS + s_player * CAMERA_ENTRY + 0x134u);
        if (pad != 0 && pad < openrac_guest_ram_size - 0x104u
            && GREF(float, pad + 0x100u) != 0.0f) {
            GREF(float, x) *= openrac_game_camera_speed;
        }
    }
}

/* The pad's analog x for a report byte (the pad code's, 0x003A31C8), then as the camera reads it. */
static float stick_turn(uint8_t x) {
    const int d = x < 0x7F ? 0x7F - x : x - 0x7F;
    if (d < 0x30) {
        return 0.0f;
    }
    float f = (float)(d - 0x30) / (float)0x4C;
    f = f > 1.0f ? 1.0f : f;
    if (x < 0x7F) {
        f = -f;
    }
    f *= openrac_game_camera_speed;
    const unsigned setting = GREF(uint8_t, CAMERA_SETTING);
    if (setting >= 3) {
        return 0.0f;
    }
    /* The first player's word of the setting's row, whichever player's camera reads it. */
    return GREF(int32_t, CAMERA_INVERT_H + setting * 0x20u) == 0 ? -f : f;
}

static int camera_turn(
    uint8_t right_x, float ticks, float* radians, float axis[3], unsigned* reads
) {
    *reads = s_reads;
    *radians = 0.0f;
    if (openrac_game_loaded_overlay() <= 0 || openrac_game_loaded_overlay() >= 39 || s_state == 0
        || s_player >= 8 || openrac_guest_ram_size < STATE_SIZE
        || s_state > openrac_guest_ram_size - STATE_SIZE || locked(s_state)) {
        return 0;
    }
    const float target = stick_turn(right_x);
    if (target == 0.0f) {
        return 0;
    }
    const gaddr entry = CAMERA_AXES + s_player * CAMERA_ENTRY;
    for (int i = 0; i < 3; ++i) {
        axis[i] = GREF(float, entry + 0x20u + 4u * (unsigned)i);
    }
    /* 0x00382BC8, a tick at a time, the last one in part. */
    const float speed = GREF(float, s_state + STATE_TURN_SPEED);
    float r = GREF(float, s_state + STATE_VALUE);
    float v = GREF(float, s_state + STATE_SPEED);
    float turned = 0.0f;
    for (float left = ticks; left > 0.0f; left -= 1.0f) {
        const float diff = target - r;
        v = v + (SPRING_STIFFNESS * diff - SPRING_DAMPING * v);
        if (fabsf(diff) < v) {
            v = fabsf(diff);
        } else if (v < -fabsf(diff)) {
            v = -fabsf(diff);
        }
        r += v;
        turned += speed * r * (left < 1.0f ? left : 1.0f);
    }
    *radians = turned;
    return isfinite(turned) ? 1 : 0;
}

/* From crt0 (entry.c), with the other host hooks: nothing else names this file. */
void rac3_install_camera_latch(void) {
    openrac_game_camera_turn = camera_turn;
}
