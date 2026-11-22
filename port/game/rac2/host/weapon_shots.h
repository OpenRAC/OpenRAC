/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Going Commando: the hero's weapons and their shots for the mods (openrac_api on_weapon_fire,
 * weapon_refire, on_shot_end, moby_spawn_live, weapon_look, moby_status, weapon_record; interface
 * 1.29), and the moby loop that runs the mods' gate and the statuses (weapon_shots.c says how). */
#ifndef OPENRAC_RAC2_WEAPON_SHOTS_H
#define OPENRAC_RAC2_WEAPON_SHOTS_H

#include <stdint.h>

#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "openrac/openrac_mod.h"

/* Once a tick, before the mods' tick (play.c's rac2_play_frame): the shots fired and ended since
 * the last are told to the mods. */
void rac2_shots_frame(void);

/* The play table's calls (play.c). */
int32_t rac2_shot_refire(uint32_t serial, float yaw, float pitch);
int32_t rac2_shot_spawn_live(const openrac_game_spawn_live* spawn);
int rac2_moby_status(int32_t slot, uint32_t status, int32_t ticks);
gaddr rac2_weapon_record(int32_t item);
/* A hit posted to `victim` as the game posts its own (QueueMobyImpact through a request as
 * make_hit_descriptor fills one); the velocity is not a field of the game's request. */
void rac2_post_hit(
    gaddr victim,
    gaddr attacker,
    uint32_t mask,
    float damage,
    const float position[3],
    const float velocity[3]
);

/* The shots' kinds, by the game's spawn routine (weapon_shots.c's table). */
enum {
    RAC2_SHOT_MINIROCKET,
    RAC2_SHOT_RYNO,
    RAC2_SHOT_SEEKER,
    RAC2_SHOT_HOVERBOMB,
    RAC2_SHOT_BOMB,
    RAC2_SHOT_GRAVITY_BOMB,
    RAC2_SHOT_BOUNCER,
    RAC2_SHOT_DECOY,
    RAC2_SHOT_MINITURRET,
    RAC2_SHOT_KINDS
};

/* A shot kind's class (the moby its spawn makes), or -1; and the kind of class `oclass`, or -1. */
int rac2_shot_class(int kind);
int rac2_shot_kind(int oclass);

/* Turns `v` (3 floats) by `yaw` radians about the up axis (+z), then `pitch` radians up from
 * the horizontal plane, about the axis across it; its length kept. */
void rac2_shot_turn(float v[3], const float forward[3], float yaw, float pitch);

#endif /* OPENRAC_RAC2_WEAPON_SHOTS_H */
