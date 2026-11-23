/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/*
 * Going Commando's follow camera turned by the mouse itself (rac2 keyboard_mouse): each tick, just
 * after the camera reads the right stick (camera_latch.c, func_L00_00424F98) and before it turns
 * by it, the hook gets the camera and the two stick values the camera will use, to turn the camera
 * directly and set the values. Null: nothing does.
 */
#ifndef OPENRAC_RAC2_FOLLOW_MOUSE_H
#define OPENRAC_RAC2_FOLLOW_MOUSE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* camera: the follow camera (its data at +0x70); turn, tilt: the guest addresses of the floats the
 * camera turns and tilts by. */
typedef void (*openrac_rac2_follow_mouse_fn)(uint32_t camera, uint32_t turn, uint32_t tilt);

void openrac_rac2_set_follow_mouse(openrac_rac2_follow_mouse_fn fn);

/* The first-person camera (cam_aligned_update_from_input, L1's view and the first-person mode) turned
 * by the mouse itself: each tick, just after the camera turns by the stick, the hook gets the
 * camera (its view's forward, side and up rows at +0x0/+0x10/+0x20), its state (+0x10: the view's
 * tilt in radians, > 0 looks up), the hero's up, and whether the game stops the stick turning
 * (turn_locked) or tilting it (tilt_locked) this tick. Null: nothing does. */
typedef void (*openrac_rac2_aligned_mouse_fn)(
    uint32_t camera, uint32_t state, const float up[3], int turn_locked, int tilt_locked
);

void openrac_rac2_set_aligned_mouse(openrac_rac2_aligned_mouse_fn fn);

/* The camera block's axis the follow camera turns about (the world's up), into up[3]; 0 when no
 * level is loaded. */
int openrac_rac2_follow_axis(float up[3]);

/* The follow camera's tilt range (radians): the elevation target at a full stick; 0 for none. */
float openrac_rac2_follow_tilt_range(void);

#ifdef __cplusplus
}
#endif

#endif
