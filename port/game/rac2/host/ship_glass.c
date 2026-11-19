/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Going Commando: the ship's cockpit glass (render_environment_mapped_object, func_002ECB70). It is
 * not in the ship's model: the game writes it each tick as 38 world quads at the place joint 0 of
 * the ship has that tick (the flight between planets, func_002E8BA8; the parked ship's draw
 * callback). A frame between ticks draws the hull part way from the last tick, so the quads get
 * ids to move with it; without them the glass stayed at the tick's place while the ship rolled.
 * Ratchet & Clank's ship glass is the same (rac1-pal/host/ship_glass.c).
 */
#include "draw_owner.h"
#include "game_protos.h"

void func_002ECB70(gaddr a0) {
    const uint32_t outer = rac2_effect_quad_id;
    rac2_effect_quad_id = a0 + 1; /* a moby is 0x100-aligned: its 38 quads count up from here */
    func_002ECB70__game(a0);
    rac2_effect_quad_id = outer;
}
