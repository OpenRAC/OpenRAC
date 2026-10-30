/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Ratchet & Clank: what the follow camera's right stick turn will be, for the picture's late
 * camera latch (settings camera_latch, frontend/camera_latch.cpp, docs/port/SETTINGS.md).
 *
 * Each tick the follow camera's view (FollowCameraBuildView) reads the right stick
 * (FollowCamera_ReadStickInput, func_L00_002E9E60; US FUN_L00_002e89b0): its x, the pad's
 * analog[0] (+0x100: the report's byte with a dead zone of 0x30 about 0x7F, the rest over 0x4C,
 * mirrored with the left/right option, negated unless the camera is inverted), is the target of
 * a damped spring, the turn factor at the camera's +0x1A8 (its speed at +0x1AC, stiffness and
 * damping the level's D_L00_00161D7C and D_L00_00161D80). Cam_SteerHeadingFromStick then turns
 * the camera's offset from the hero (+0x130) about the camera block's axis (D_L00_00166F10 +
 * 0x20, the world's up) by that factor times the turn speed (+0x1BC, set from D_L00_0015F044
 * whenever the stick is moved). The flags at +0x1B8 bit 0 lock that turn (a camera volume's).
 *
 * The read runs under a host function that only notes the camera it read for (the game's C runs
 * as it is, the option on or off), so that the frontend can ask between ticks what the next ticks
 * would turn with the stick as it is now: the same spring stepped from the camera's state, never
 * written back. The vertical stick is left out: the game eases the pitch toward an angle the
 * stick holds, not at a speed, and a picture ahead of it would have to guess where it stops.
 */
#include <math.h>
#include <string.h>

#include "rac1.h"

#define PAD RAC1_ADDR(0x0013CA40u)
#define PAD_MIRROR RAC1_ADDR(0x0015EEB4u)
#define CAM_INVERT RAC1_ADDR(0x0015EEE0u)
#define LEVEL_MODE OPENRAC_LDATA(0, RAC1_LADDR(00, 0x0015F6A8u))
#define TURN_SPEED OPENRAC_LDATA(0, RAC1_LADDR(00, 0x0015F044u))
#define SPRING_STIFFNESS OPENRAC_LDATA(0, RAC1_LADDR(00, 0x00161D7Cu))
#define SPRING_DAMPING OPENRAC_LDATA(0, RAC1_LADDR(00, 0x00161D80u))
#define CAMERA_BLOCK OPENRAC_LDATA(0, RAC1_LADDR(00, 0x00166F10u))

static gaddr s_camera;   /* the follow camera's data (its moby's +0x70) the last read was for */
static unsigned s_reads; /* how many reads so far: the frontend sees whether one ran this tick */

static float get_float(gaddr at) {
    float v;
    memcpy(&v, G(at), 4);
    return v;
}

void func_L00_002E9E60(gaddr camera, gaddr yaw, gaddr pitch) {
    func_L00_002E9E60__game(camera, yaw, pitch);
    s_camera = GREF(gaddr, camera + 0x70u);
    ++s_reads;
}

/* The pad's analog[0] for a report byte (Pad_ProcessReport), then as the camera reads it. */
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
    if (GREF(uint8_t, PAD_MIRROR) != 0) {
        f = -f;
    }
    return GREF(int32_t, CAM_INVERT) == 0 ? -f : f;
}

static int camera_turn(
    uint8_t right_x, float ticks, float* radians, float axis[3], unsigned* reads
) {
    *reads = s_reads;
    *radians = 0.0f;
    if (openrac_game_loaded_overlay() < 0 || GREF(int32_t, LEVEL_MODE) != 0 || s_camera == 0
        || s_camera >= 0x2000000u || (GREF(int32_t, s_camera + 0x1B8u) & 1) != 0) {
        return 0;
    }
    const float target = stick_turn(right_x);
    if (target == 0.0f) {
        return 0;
    }
    const gaddr block = CAMERA_BLOCK;
    for (int i = 0; i < 3; ++i) {
        axis[i] = get_float(block + 0x20u + 4u * (unsigned)i);
    }
    /* Cam_InterpValues, a tick at a time, the last one in part. */
    const float speed = get_float(TURN_SPEED);
    const float stiffness = get_float(SPRING_STIFFNESS);
    const float damping = get_float(SPRING_DAMPING);
    float r = get_float(s_camera + 0x1A8u);
    float v = get_float(s_camera + 0x1ACu);
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

__attribute__((constructor)) static void install_camera_latch(void) {
    openrac_game_camera_turn = camera_turn;
}
