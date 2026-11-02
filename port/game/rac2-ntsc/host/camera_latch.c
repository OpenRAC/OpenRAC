/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Going Commando: what the follow camera's right stick turn will be, for the picture's late camera
 * latch (settings responsive_controls, frontend/camera_latch.cpp), as Ratchet & Clank's
 * (rac1/host/camera_latch.c says what the latch does with it).
 *
 * Each tick the follow camera (func_L00_00426340) reads the sticks (func_L00_00424F98, Ratchet &
 * Clank's FollowCamera_ReadStickInput, paired at 0.48): its x, the pad's analog float at +0x100
 * (0x00138280: the report's byte with a dead zone of 0x30 about 0x7F, the rest over 0x4C, as the
 * profile's stick_dead_zone says), negated unless the camera's left/right option (0x001A7BA8) is
 * set, and 0 while the camera's flags (its data +0x1C0, bit 0) lock it. func_L00_00425118 then
 * eases a spring towards that (func_002702C8: the value at +0x1C0, its speed at +0x1C4, stiffness
 * and damping the level's 0x001B14AC and 0x001B14B0, no limit) and turns the camera's offset from
 * the hero (+0x160) about the camera block's axis (0x001B8E20 + 0x20) by that value times the turn
 * speed at +0x1D4 (set from 0x001A8904 whenever the stick is moved).
 *
 * The read runs under a host function that only notes the camera it read for, so that the frontend
 * can ask between ticks what the next ticks would turn with the stick as it is now: the same spring
 * stepped from the camera's state, never written back. The vertical stick is left out, as in
 * Ratchet & Clank.
 */
#include <math.h>
#include <stdint.h>

#include "game_protos.h"
#include "openrac/game_host.h"

#define CAM_LEFT_RIGHT 0x001A7BA8u
#define SPRING_STIFFNESS OPENRAC_LDATA(0, 0x001B14ACu)
#define SPRING_DAMPING OPENRAC_LDATA(0, 0x001B14B0u)
#define CAMERA_BLOCK OPENRAC_LDATA(0, 0x001B8E20u)

/* The camera's stick state: its data (the moby's +0x70) + 0x1C0. */
#define STATE_FLAGS 0x10u
#define STATE_VALUE 0x00u
#define STATE_SPEED 0x04u
#define STATE_TURN_SPEED 0x14u
#define STATE_SIZE 0x30u

static gaddr s_state;    /* the stick state the last read was for */
static unsigned s_reads; /* how many reads so far: the frontend sees whether one ran this tick */

void func_L00_00424F98(gaddr camera, gaddr x, gaddr y) {
    func_L00_00424F98__game(camera, x, y);
    s_state = GREF(gaddr, camera + 0x70u) + 0x1C0u;
    ++s_reads;
}

/* The pad's analog x for a report byte (func_002B88F0), then as the camera reads it. */
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
    return GREF(int32_t, CAM_LEFT_RIGHT) == 0 ? -f : f;
}

static int camera_turn(uint8_t right_x, float ticks, float* radians, float axis[3], unsigned* reads) {
    *reads = s_reads;
    *radians = 0.0f;
    if (openrac_game_loaded_overlay() < 0 || s_state == 0 || openrac_guest_ram_size < STATE_SIZE
        || s_state > openrac_guest_ram_size - STATE_SIZE || (GREF(int32_t, s_state + STATE_FLAGS) & 1) != 0) {
        return 0;
    }
    const float target = stick_turn(right_x);
    if (target == 0.0f) {
        return 0;
    }
    const gaddr block = CAMERA_BLOCK;
    for (int i = 0; i < 3; ++i) {
        axis[i] = GREF(float, block + 0x20u + 4u * (unsigned)i);
    }
    /* func_002702C8, a tick at a time, the last one in part. */
    const float speed = GREF(float, s_state + STATE_TURN_SPEED);
    const float stiffness = GREF(float, SPRING_STIFFNESS);
    const float damping = GREF(float, SPRING_DAMPING);
    float r = GREF(float, s_state + STATE_VALUE);
    float v = GREF(float, s_state + STATE_SPEED);
    float turned = 0.0f;
    for (float left = ticks; left > 0.0f; left -= 1.0f) {
        const float diff = target - r;
        v = v + (stiffness * diff - damping * v);
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
void rac2_install_camera_latch(void) {
    openrac_game_camera_turn = camera_turn;
}
