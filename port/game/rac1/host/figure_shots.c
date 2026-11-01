/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Ratchet & Clank: the shots a figure fires (game_host.h, openrac_game_play; interface 1.9): the
 * hero's Blaster shots are spawn_homing_shot (func_L00_002CC3C0) with the hero's moby as their
 * owner, his bombs launch_bomb (func_L00_002AE278) of a bomb the glove holds; both wrapped and
 * noted for hero_shots. A figure's shot is the same call with the figure as the owner. A moby
 * whose owner is a figure (a shot's pvars +0x20, a bomb's +0x50) is a figure's: while its update
 * runs (rac1_figure_owned of the moby being updated), the collision queries post no hit records
 * (collision.c), its blast sends no radial message (SendRadialMessageToMobys, func_L00_0025BA50,
 * wrapped) and the hits posted are dropped (world.c). The pieces a figure wears are figure_items.c.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "figure_items.h"
#include "openrac/host_env.h"
#include "rac1.h"

/* The Blaster and the Bomb Glove, and the classes of their shots. */
#define ITEM_BOMB_GLOVE 0x0A
#define ITEM_BLASTER 0x0F
#define CLASS_HOMING_SHOT 0x131
#define CLASS_BOMB 0x79
#define SHOT_OWNER 0x20u
#define BOMB_OWNER 0x50u

/* The shots the hero fired since hero_shots last took them. */
static openrac_game_shot shots[16];
static int shot_count;
/* Set while a figure's shot is made: not the hero's. */
static int firing_for_figure;

static int in_level(void) {
    return rac1_level_running();
}

static void note_shot(const openrac_game_shot* s) {
    const int max = (int)(sizeof shots / sizeof shots[0]);
    if (shot_count == max) {
        memmove(&shots[0], &shots[1], sizeof shots[0] * (size_t)(max - 1));
        --shot_count;
    }
    shots[shot_count++] = *s;
}

int rac1_hero_shots(openrac_game_shot* out, int max) {
    const int n = shot_count < max ? shot_count : max;
    memcpy(out, shots, sizeof shots[0] * (size_t)n);
    memmove(&shots[0], &shots[n], sizeof shots[0] * (size_t)(shot_count - n));
    shot_count -= n;
    return n;
}

/* spawn_homing_shot (the Blaster's): the hero's noted. */
gaddr func_L00_002CC3C0(
    gaddr owner, gaddr start, gaddr target, float yaw, float pitch, float yaw0, float pitch0
) {
    if (!firing_for_figure && owner != 0 && owner == GREF(gaddr, HERO_MOBY)) {
        openrac_game_shot s;
        memset(&s, 0, sizeof s);
        s.item = ITEM_BLASTER;
        for (unsigned c = 0; c < 3; ++c) {
            s.position[c] = GREF(float, start + 4u * c);
        }
        s.aim[0] = yaw;
        s.aim[1] = pitch;
        s.aim[2] = yaw0;
        s.aim[3] = pitch0;
        note_shot(&s);
    }
    return func_L00_002CC3C0__game(owner, start, target, yaw, pitch, yaw0, pitch0);
}

/* launch_bomb: a bomb the hero's glove throws noted, with its velocity as launched. */
void func_L00_002AE278(gaddr pos, gaddr bomb, gaddr velocity) {
    func_L00_002AE278__game(pos, bomb, velocity);
    if (firing_for_figure || bomb == 0) {
        return;
    }
    const gaddr pvars = GREF(gaddr, bomb + MOBY_PVARS);
    const gaddr hand = GREF(gaddr, HERO_SLOTS);
    if (pvars == 0 || hand == 0 || GREF(gaddr, pvars + BOMB_OWNER) != hand) {
        return;
    }
    openrac_game_shot s;
    memset(&s, 0, sizeof s);
    s.item = ITEM_BOMB_GLOVE;
    for (unsigned c = 0; c < 3; ++c) {
        s.position[c] = GREF(float, bomb + MOBY_POS + 4u * c);
        s.velocity[c] = GREF(float, pvars + 4u * c);
    }
    note_shot(&s);
}

int rac1_figure_shot(gaddr figure, const openrac_game_shot* shot) {
    if (!in_level() || !rac1_figure_alive(figure) || GREF(int32_t, CONTEXT) != 0) {
        return 0;
    }
    GFRAME(v, 32);
    for (unsigned c = 0; c < 4; ++c) {
        GREF(float, v + 4u * c) = c < 3 ? shot->position[c] : 1.0f;
        GREF(float, v + 0x10u + 4u * c) = c < 3 ? shot->velocity[c] : 0.0f;
    }
    int made = 0;
    firing_for_figure = 1;
    switch (shot->item) {
        case ITEM_BLASTER:
            made = func_L00_002CC3C0(
                       figure, v, 0, shot->aim[0], shot->aim[1], shot->aim[2], shot->aim[3]
                   )
                   != 0;
            break;
        case ITEM_BOMB_GLOVE: {
            const gaddr bomb = func_L00_002AE110((int)figure, v);
            if (bomb != 0) {
                func_L00_002AE278(v, bomb, v + 0x10u);
                /* As the shooter's game launched it (launch_bomb adds this hero's motion). */
                const gaddr pvars = GREF(gaddr, bomb + MOBY_PVARS);
                if (pvars != 0) {
                    for (unsigned c = 0; c < 3; ++c) {
                        GREF(float, pvars + 4u * c) = shot->velocity[c];
                    }
                }
                made = 1;
            }
            break;
        }
        default:
            break;
    }
    firing_for_figure = 0;
    if (rac1_figure_items_debug()) {
        fprintf(
            stderr,
            "[debug] figure items: shot of item %d %s\n",
            shot->item,
            made ? "made" : "refused"
        );
    }
    return made;
}

/* A figure's moby's hit or blast not let through, said with OPENRAC_ITEMS_DEBUG. */
void rac1_figure_hit_dropped(const char* what) {
    if (rac1_figure_items_debug()) {
        fprintf(stderr, "[debug] figure items: a figure's %s dropped\n", what);
    }
}

/* Whether `moby` is a figure's (its owner a figure): what it does hits nothing here. */
int rac1_figure_owned(gaddr moby) {
    if (moby == 0 || !in_level()) {
        return 0;
    }
    const gaddr pvars = GREF(gaddr, moby + MOBY_PVARS);
    if (pvars == 0) {
        return 0;
    }
    switch (GREF(int16_t, moby + MOBY_OCLASS)) {
        case CLASS_HOMING_SHOT:
            return rac1_figure_alive(GREF(gaddr, pvars + SHOT_OWNER));
        case CLASS_BOMB:
            return rac1_figure_alive(GREF(gaddr, pvars + BOMB_OWNER));
        default:
            return 0;
    }
}

/* SendRadialMessageToMobys: a figure's blast sends none. */
void func_L00_0025BA50(
    gaddr source,
    gaddr centre,
    gaddr list,
    int count,
    gaddr template_,
    int mask,
    int a6,
    int a7,
    float a8,
    float a9,
    float a10
) {
    if (rac1_figure_owned(rac1_updating_moby()) || rac1_figure_owned(source)) {
        rac1_figure_hit_dropped("blast");
        return;
    }
    func_L00_0025BA50__game(source, centre, list, count, template_, mask, a6, a7, a8, a9, a10);
}
