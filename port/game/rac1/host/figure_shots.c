/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Ratchet & Clank: the shots a figure fires (game_host.h, openrac_game_play; interfaces 1.9, 1.12).
 * Shots are shown only: damage is the shooter's game's to tell, and a late shot is dropped by the
 * mod. figure_tracks.h has the table of what each item shows.
 *
 * What flies by itself is the game's own spawn, called with the figure as the owner: the hero's are
 * noted by a wrapper, a figure's made by the same call. A moby whose owner is a figure is a figure's
 * (OWNED below): while its update runs (rac1_figure_owned of the moby being updated), the collision
 * queries post no hit records (collision.c), its blast sends no radial message
 * (SendRadialMessageToMobys, func_L00_0025BA50, wrapped) and the hits posted are dropped (world.c).
 *   Blaster       spawn_homing_shot, func_L00_002CC3C0 (class 305)
 *   Bomb Glove    launch_bomb, func_L00_002AE278, of spawn_bomb's (121)
 *   Devastator    spawn_devastator_missile, func_L00_002B0F58 (153)
 *   R.Y.N.O.      spawn_ryno_rocket, func_L00_002CE390 (457): it flies to its target by itself; the
 *                 player does not steer it
 * The three gloves hold what they throw from the moment they are drawn, so the throw is the release,
 * not the spawn: release_glove_of_doom_held_moby (func_L00_002C96D0, class 230, which makes its bots
 * itself where it lands), the Mine Glove's release (func_L00_002AB2A8, class 74) and the Decoy
 * Glove's (func_L00_002C4B90, classes 203 and 1900). The hero's are noted there. A figure's is the
 * glove's spawn, then what the release sets, past the held state: not the release itself, which
 * tests a line from this game's hero to the hand and would put the mine against a wall here. Past
 * the held state nothing of theirs asks about the hero's hand. A figure's decoy draws the enemies of
 * the game it is in: spawn_decoy lists it, as the hero's; the host's game has the enemies.
 *
 * The rest are tracks (figure_tracks.h). Their state is in the hero's hand and in globals, so
 * nothing of the game's own can be called for a figure; rac1_hero_shots2 polls them and the figure's
 * are drawn here, each tick (rac1_figure_items_tick):
 *   Pyrocitor     its hand moby firing (state 3): the game's flame particles from the figure's hand
 *                 (pyrocitor_update, func_L00_0026C0D0). The flame is not stopped by walls here
 *   Tesla Claw    its hand moby firing (3): sparks (func_L00_0026E438) along the shooter's line, the
 *                 beam's points being one set of globals shared with the hero's own
 *   Morph-O-Ray   its timer running: as the claw's
 *   Suck Cannon   its hand moby sucking (3): sparks going to the hand
 *   Visibomb      hero +0x1FE0 alive, class 172: a moby of the class where the shooter's is. Its
 *                 spawn takes the hero's camera and state (hero_set_state, the camera mode), so it
 *                 cannot be replayed; the end is its burst (func_L00_002B6FF0)
 *   Drones        hero +0x2020, six, class 479: mobys of the class, where the shooter's are about
 *                 him. spawn_hero_drone fills the hero's own six slots
 *
 * The classes. Measured on both discs, all 19 levels: every class a shot makes (74, 121, 153, 172,
 * 186, 203, 230, 457, 479, 1900) is in the level's table with its data, 305 is a table entry with
 * no model of its own, and none is one of the 21 that share the decode buffers (figure_items.c). A
 * shot of a class that cannot be had all the same (a mod's level, say) is refused and nothing shows.
 * So is one that would take the level's last free moby slots: a full pool is the game's crash
 * (moby_pool.c).
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "figure_items.h"
#include "figure_tracks.h"
#include "openrac/host_env.h"
#include "rac1.h"

/* The classes of the hand, and of the shots. */
#define CLASS_PYROCITOR 0xB0
#define CLASS_TESLA_CLAW 0xB1
#define CLASS_MORPH_O_RAY 0xB9
#define CLASS_SUCK_CANNON 0x351
#define CLASS_HOMING_SHOT 0x131
#define CLASS_BOMB 0x79
#define CLASS_MISSILE 0x99
#define CLASS_VISIBOMB_MISSILE 0xAC
#define CLASS_RYNO_ROCKET 0x1C9
#define CLASS_DOOM_THROWN 0xE6
#define CLASS_DOOM_BOT 0xBA
#define CLASS_MINE 0x4A
#define CLASS_DECOY 0xCB
#define CLASS_GOLD_DECOY 0x76C
#define CLASS_DRONE 0x1DF

/* Where each class of shot keeps its owner, in its pvars. */
static const struct {
    int16_t oclass;
    uint16_t owner;
} OWNED[] = {
    {CLASS_HOMING_SHOT, 0x20u},
    {CLASS_BOMB, 0x50u},
    {CLASS_MISSILE, 0x10u},
    {CLASS_RYNO_ROCKET, 0x20u},
    {CLASS_DOOM_THROWN, 0x30u},
    {CLASS_DOOM_BOT, 0x60u},
    {CLASS_MINE, 0x30u},
    {CLASS_DECOY, 0x40u},
    {CLASS_GOLD_DECOY, 0x40u},
};

/* The hero: the way he faces, his Visibomb in flight, his drones. */
#define HERO_FACING (HERO + 0x640u)
#define HERO_VISIBOMB (HERO + 0x1FE0u)
#define HERO_DRONES (HERO + 0x2020u)
#define DRONES 6
/* The hand moby's states and pvars. */
#define STATE_FIRING 3
#define DECOY_VELOCITY 0x10u
#define MINE_TIMER 0x34u
#define TESLA_TIP 0x10u
#define MORPH_TIMER 0x1Cu
#define MORPH_YAW 0x48u
#define MORPH_PITCH 0x4Cu
/* What the game's flame and beams reach, and a flame particle's speed a tick. */
#define FLAME_REACH 12.0f
#define FLAME_SPEED 0.27f
#define BEAM_REACH 12.0f
#define SUCK_REACH 8.0f
#define BEAM_DRIFT 0.02f
#define SUCK_SPEED 0.08f
#define FLAME_PARTICLES 2
#define BEAM_SPARKS 3
/* The pool: first free and past the last. Spawned mobys are 0x100 bytes. */
#define MOBY_END OPENRAC_DATA(RAC1_ADDR(0x0016001Cu))
#define MOBY_PERM_END OPENRAC_DATA(RAC1_ADDR(0x00160020u))
#define MOBY_STRIDE 0x100u
#define MOBY_FREE 0xFE
#define MOBY_LAST 0xFF
/* The free slots a figure's shot leaves to the game. */
#define SPARE_LEFT 24

/* The hero's tracks: the hand's, the Visibomb's, the drones'. A MOVE every this many ticks. */
#define TRACK_HAND 0
#define TRACK_VISIBOMB 1
#define TRACK_DRONE 2
#define TRACKS_OUT (TRACK_DRONE + DRONES)
#define HAND_EVERY 6u
#define VISIBOMB_EVERY 6u
#define DRONE_EVERY 12u
/* aux: the track's id, the class of its moby, whether its place is from its figure. */
#define AUX_TRACK 0
#define AUX_CLASS 1
#define AUX_FROM_FIGURE 2
/* aux of a thrown glove's: how long a mine waits. */
#define AUX_MINE_TIMER 0
/* aux of a missile's or rocket's: the height it aims at, over its target. */
#define AUX_HEIGHT 0

/* The shots the hero fired since hero_shots last took them. */
static openrac_game_shot2 shots[32];
static int shot_count;
/* Set while a figure's shot is made: not the hero's. */
static int firing_for_figure;

static ShotTrackOut tracks_out[TRACKS_OUT];
static float drone_was[DRONES][3];
static uint32_t ticks_out;

/* The tracks shown here. */
#define TRACKS 16
static ShotTrack tracks[TRACKS];
static uint32_t ticks_in;

/* The figures' mobys made here that have outlived their figure's say: still a figure's. */
#define LIVE 48
static struct {
    gaddr moby;
    int32_t oclass;
    uint32_t load;
} live[LIVE];

static int in_level(void) {
    return rac1_level_running();
}

static void clear_shot(openrac_game_shot2* s, int32_t item) {
    memset(s, 0, sizeof *s);
    s->item = item;
    s->target_key = -1;
}

static void note_shot(const openrac_game_shot2* s) {
    const int max = (int)(sizeof shots / sizeof shots[0]);
    if (shot_count == max) {
        memmove(&shots[0], &shots[1], sizeof shots[0] * (size_t)(max - 1));
        --shot_count;
    }
    shots[shot_count++] = *s;
}

/* The moby in the hero's hand. */
static gaddr hand_moby(void) {
    return GREF(gaddr, HERO_SLOTS);
}

static int alive_as(gaddr moby, int32_t oclass) {
    return moby != 0 && GREF(uint8_t, moby + MOBY_STATE) < 0x80
           && GREF(int16_t, moby + MOBY_OCLASS) == oclass;
}

static void read_vector(float* out, gaddr from) {
    for (unsigned c = 0; c < 3; ++c) {
        out[c] = GREF(float, from + 4u * c);
    }
}

/* The moby the hero aims at, for the others' games, -1 for none or a spawned one: placed mobys have
 * the same key in every game. */
static int32_t key_of_target(gaddr target) {
    return target != 0 ? rac1_world_moby_key(target) : -1;
}

/* Back from a key, 0 for one gone. */
static gaddr target_of_key(int32_t key) {
    const int32_t slot = key >= 0 ? rac1_world_moby_slot(key) : -1;
    const gaddr moby = slot >= 0 ? rac1_world_moby_at(slot) : 0;
    return moby != 0 && GREF(uint8_t, moby + MOBY_STATE) < 0x80 ? moby : 0;
}

/* --- The hero's shots --- */

/* spawn_homing_shot (the Blaster's): the hero's noted. */
gaddr func_L00_002CC3C0(
    gaddr owner, gaddr start, gaddr target, float yaw, float pitch, float yaw0, float pitch0
) {
    if (!firing_for_figure && owner != 0 && owner == GREF(gaddr, HERO_MOBY)) {
        openrac_game_shot2 s;
        clear_shot(&s, ITEM_BLASTER);
        read_vector(s.position, start);
        s.aim[0] = yaw;
        s.aim[1] = pitch;
        s.aim[2] = yaw0;
        s.aim[3] = pitch0;
        s.target_key = key_of_target(target);
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
    const gaddr hand = hand_moby();
    if (pvars == 0 || hand == 0 || GREF(gaddr, pvars + 0x50u) != hand) {
        return;
    }
    openrac_game_shot2 s;
    clear_shot(&s, ITEM_BOMB_GLOVE);
    read_vector(s.position, bomb + MOBY_POS);
    read_vector(s.velocity, pvars);
    note_shot(&s);
}

/* spawn_devastator_missile: the hero's noted, with his target. */
gaddr func_L00_002B0F58(
    gaddr owner,
    gaddr start,
    gaddr target,
    float yaw,
    float pitch,
    float height,
    float yaw0,
    float pitch0
) {
    if (!firing_for_figure && owner != 0 && owner == hand_moby()) {
        openrac_game_shot2 s;
        clear_shot(&s, ITEM_DEVASTATOR);
        read_vector(s.position, start);
        s.aim[0] = yaw;
        s.aim[1] = pitch;
        s.aim[2] = yaw0;
        s.aim[3] = pitch0;
        s.aux[AUX_HEIGHT] = height;
        s.target_key = key_of_target(target);
        note_shot(&s);
    }
    return func_L00_002B0F58__game(owner, start, target, yaw, pitch, height, yaw0, pitch0);
}

/* spawn_ryno_rocket: the hero's noted, with his target. */
gaddr func_L00_002CE390(int owner, gaddr start, gaddr target, float height, float yaw, float pitch) {
    if (!firing_for_figure && owner != 0 && (gaddr)owner == hand_moby()) {
        openrac_game_shot2 s;
        clear_shot(&s, ITEM_RYNO);
        read_vector(s.position, start);
        s.aim[0] = yaw;
        s.aim[1] = pitch;
        s.aux[AUX_HEIGHT] = height;
        s.target_key = key_of_target(target);
        note_shot(&s);
    }
    return func_L00_002CE390__game(owner, start, target, height, yaw, pitch);
}

/* A thrown thing noted as its release left it. */
static void note_thrown(int32_t item, gaddr moby, unsigned velocity) {
    const gaddr pvars = GREF(gaddr, moby + MOBY_PVARS);
    if (pvars == 0) {
        return;
    }
    openrac_game_shot2 s;
    clear_shot(&s, item);
    read_vector(s.position, moby + MOBY_POS);
    read_vector(s.velocity, pvars + velocity);
    if (item == ITEM_MINE_GLOVE) {
        s.aux[AUX_MINE_TIMER] = (float)GREF(int16_t, pvars + MINE_TIMER);
    }
    note_shot(&s);
}

/* release_glove_of_doom_held_moby. */
void func_L00_002C96D0(gaddr pos, gaddr moby, gaddr velocity) {
    func_L00_002C96D0__game(pos, moby, velocity);
    note_thrown(ITEM_GLOVE_OF_DOOM, moby, 0);
}

/* The Mine Glove's release (initialize_moby_motion_at_position). */
void func_L00_002AB2A8(gaddr pos, gaddr moby, gaddr velocity) {
    func_L00_002AB2A8__game(pos, moby, velocity);
    note_thrown(ITEM_MINE_GLOVE, moby, 0);
}

/* The Decoy Glove's release (release_moby_with_collision_check). */
void func_L00_002C4B90(gaddr pos, gaddr moby, gaddr velocity) {
    func_L00_002C4B90__game(pos, moby, velocity);
    note_thrown(ITEM_DECOY_GLOVE, moby, DECOY_VELOCITY);
}

/* --- The hero's tracks --- */

static void sent_track(
    int32_t item,
    int track,
    uint32_t flags,
    const float* position,
    const float* velocity,
    int32_t oclass,
    int from_figure
) {
    openrac_game_shot2 s;
    clear_shot(&s, item);
    s.flags = flags;
    memcpy(s.position, position, sizeof s.position);
    memcpy(s.velocity, velocity, sizeof s.velocity);
    s.aux[AUX_TRACK] = (float)track;
    s.aux[AUX_CLASS] = (float)oclass;
    s.aux[AUX_FROM_FIGURE] = from_figure ? 1.0f : 0.0f;
    note_shot(&s);
}

/* What the hand does now: its item for a firing weapon (0 for none), its line from the hero. */
static int32_t hand_firing(float* start, float* line) {
    const gaddr hand = hand_moby();
    if (hand == 0 || GREF(uint8_t, hand + MOBY_STATE) >= 0x80) {
        return 0;
    }
    const gaddr hero = GREF(gaddr, HERO_MOBY);
    const gaddr pvars = GREF(gaddr, hand + MOBY_PVARS);
    const int firing = GREF(uint8_t, hand + MOBY_STATE) == STATE_FIRING;
    float facing[3], from[3], here[3];
    read_vector(facing, HERO_FACING);
    read_vector(from, hand + MOBY_POS);
    read_vector(here, hero + MOBY_POS);
    const float length = sqrtf(facing[0] * facing[0] + facing[1] * facing[1] + facing[2] * facing[2]);
    const float unit = length > 0.0f ? 1.0f / length : 0.0f;
    int32_t item = 0;
    float reach = 0.0f;
    switch (GREF(int16_t, hand + MOBY_OCLASS)) {
        case CLASS_PYROCITOR:
            if (firing) {
                item = ITEM_PYROCITOR;
                reach = FLAME_REACH;
            }
            break;
        case CLASS_TESLA_CLAW:
            if (firing && pvars != 0) {
                item = ITEM_TESLA_CLAW;
                float tip[3];
                read_vector(from, pvars);
                read_vector(tip, pvars + TESLA_TIP);
                for (unsigned c = 0; c < 3; ++c) {
                    start[c] = from[c] - here[c];
                    line[c] = tip[c] - from[c];
                }
                return item;
            }
            break;
        case CLASS_MORPH_O_RAY:
            if (pvars != 0 && GREF(int16_t, pvars + MORPH_TIMER) != 0) {
                item = ITEM_MORPH_O_RAY;
                const float yaw = GREF(float, pvars + MORPH_YAW);
                const float pitch = GREF(float, pvars + MORPH_PITCH);
                read_vector(from, pvars);
                facing[0] = cosf(pitch) * cosf(yaw);
                facing[1] = cosf(pitch) * sinf(yaw);
                facing[2] = sinf(pitch);
                reach = BEAM_REACH;
                for (unsigned c = 0; c < 3; ++c) {
                    start[c] = from[c] - here[c];
                    line[c] = facing[c] * reach;
                }
                return item;
            }
            break;
        case CLASS_SUCK_CANNON:
            if (firing) {
                item = ITEM_SUCK_CANNON;
                reach = SUCK_REACH;
            }
            break;
        default:
            break;
    }
    for (unsigned c = 0; c < 3; ++c) {
        start[c] = from[c] - here[c];
        line[c] = facing[c] * unit * reach;
    }
    return item;
}

static void poll_hand(void) {
    static int32_t item_on;
    float start[3] = {0.0f, 0.0f, 0.0f}, line[3] = {0.0f, 0.0f, 0.0f};
    const int32_t item = hand_firing(start, line);
    /* The weapon changed under a track: the old one ends first. */
    if (item_on != 0 && item != item_on) {
        const uint32_t flags = shot_track_out(&tracks_out[TRACK_HAND], 0, ticks_out, HAND_EVERY);
        if (flags != 0) {
            sent_track(item_on, TRACK_HAND, flags, start, line, 0, 1);
        }
        item_on = 0;
    }
    const uint32_t flags = shot_track_out(&tracks_out[TRACK_HAND], item != 0, ticks_out, HAND_EVERY);
    if (flags != 0 && item != 0) {
        sent_track(item, TRACK_HAND, flags, start, line, 0, 1);
    }
    item_on = item;
}

static void poll_visibomb(void) {
    const gaddr missile = GREF(gaddr, HERO_VISIBOMB);
    const int on = alive_as(missile, CLASS_VISIBOMB_MISSILE);
    static float was[3], step[3];
    float now[3] = {was[0], was[1], was[2]};
    if (on) {
        read_vector(now, missile + MOBY_POS);
    }
    const uint32_t flags = shot_track_out(&tracks_out[TRACK_VISIBOMB], on, ticks_out, VISIBOMB_EVERY);
    if (flags == 0) {
        return;
    }
    for (unsigned c = 0; c < 3; ++c) {
        step[c] = (flags & OPENRAC_GAME_SHOT_MOVE) != 0 ? (now[c] - was[c]) / VISIBOMB_EVERY : 0.0f;
        was[c] = now[c];
    }
    const uint32_t burst = (flags & OPENRAC_GAME_SHOT_STOP) != 0 ? OPENRAC_GAME_SHOT_BURST : 0u;
    sent_track(ITEM_VISIBOMB, TRACK_VISIBOMB, flags | burst, now, step, CLASS_VISIBOMB_MISSILE, 0);
}

static void poll_drones(void) {
    const gaddr hero = GREF(gaddr, HERO_MOBY);
    float here[3];
    read_vector(here, hero + MOBY_POS);
    for (int i = 0; i < DRONES; ++i) {
        const gaddr drone = GREF(gaddr, HERO_DRONES + 4u * (uint32_t)i);
        const int on = alive_as(drone, CLASS_DRONE);
        ShotTrackOut* out = &tracks_out[TRACK_DRONE + i];
        float now[3] = {drone_was[i][0], drone_was[i][1], drone_was[i][2]};
        if (on) {
            read_vector(now, drone + MOBY_POS);
            for (unsigned c = 0; c < 3; ++c) {
                now[c] -= here[c];
            }
        }
        const uint32_t flags = shot_track_out(out, on, ticks_out, DRONE_EVERY);
        if (flags == 0) {
            continue;
        }
        float step[3] = {0.0f, 0.0f, 0.0f};
        for (unsigned c = 0; c < 3; ++c) {
            if ((flags & OPENRAC_GAME_SHOT_MOVE) != 0) {
                step[c] = (now[c] - drone_was[i][c]) / DRONE_EVERY;
            }
            drone_was[i][c] = now[c];
        }
        sent_track(ITEM_DRONE_DEVICE, TRACK_DRONE + i, flags, now, step, CLASS_DRONE, 1);
    }
}

static void poll_tracks(void) {
    ++ticks_out;
    /* Out of a level none of the hero's are on: each ends. */
    if (!in_level() || GREF(gaddr, HERO_MOBY) == 0 || GREF(int32_t, CONTEXT) != 0) {
        memset(tracks_out, 0, sizeof tracks_out);
        return;
    }
    poll_hand();
    poll_visibomb();
    poll_drones();
}

int rac1_hero_shots2(openrac_game_shot2* out, int max) {
    poll_tracks();
    const int n = shot_count < max ? shot_count : max;
    memcpy(out, shots, sizeof shots[0] * (size_t)n);
    memmove(&shots[0], &shots[n], sizeof shots[0] * (size_t)(shot_count - n));
    shot_count -= n;
    return n;
}

/* The two kinds interface 1.9 has, for a mod of that time: what it cannot show is dropped. */
int rac1_hero_shots(openrac_game_shot* out, int max) {
    int n = 0;
    while (n < max && shot_count > 0) {
        const openrac_game_shot2 s = shots[0];
        --shot_count;
        memmove(&shots[0], &shots[1], sizeof shots[0] * (size_t)shot_count);
        if (s.item == ITEM_BLASTER || s.item == ITEM_BOMB_GLOVE) {
            memcpy(&out[n++], &s, sizeof out[0]);
        }
    }
    return n;
}

/* --- A figure's --- */

/* Whether the level's pool has room to spare for a figure's shot. */
static int room_for_shot(void) {
    gaddr slot = GREF(gaddr, MOBY_END);
    const gaddr end = GREF(gaddr, MOBY_PERM_END);
    int spare = 0;
    while (slot != 0 && slot < end) {
        const uint8_t state = GREF(uint8_t, slot + MOBY_STATE);
        if (state == MOBY_LAST) {
            /* From here on none has been used yet. */
            return (end - slot) / MOBY_STRIDE + (uint32_t)spare >= SPARE_LEFT;
        }
        spare += state >= MOBY_FREE;
        if (spare >= SPARE_LEFT) {
            return 1;
        }
        slot += MOBY_STRIDE;
    }
    return 0;
}

static void note_live(gaddr moby) {
    const uint32_t load = rac1_moby_generation();
    int at = -1;
    for (int i = 0; i < LIVE; ++i) {
        if (live[i].moby == 0 || live[i].load != load || !alive_as(live[i].moby, live[i].oclass)) {
            live[i].moby = 0;
            at = at < 0 ? i : at;
        }
    }
    if (at >= 0) {
        live[at].moby = moby;
        live[at].oclass = GREF(int16_t, moby + MOBY_OCLASS);
        live[at].load = load;
    }
}

static int is_live(gaddr moby) {
    const uint32_t load = rac1_moby_generation();
    const int32_t oclass = GREF(int16_t, moby + MOBY_OCLASS);
    for (int i = 0; i < LIVE; ++i) {
        if (live[i].moby == moby && live[i].load == load && live[i].oclass == oclass) {
            return 1;
        }
    }
    return 0;
}

/* The classes the game's spawn for this item makes. */
static int shot_class_ready(int32_t item) {
    switch (item) {
        case ITEM_BOMB_GLOVE:
            return rac1_figure_class_ready(CLASS_BOMB);
        case ITEM_DEVASTATOR:
            return rac1_figure_class_ready(CLASS_MISSILE);
        case ITEM_RYNO:
            return rac1_figure_class_ready(CLASS_RYNO_ROCKET);
        case ITEM_GLOVE_OF_DOOM:
            return rac1_figure_class_ready(CLASS_DOOM_THROWN)
                   && rac1_figure_class_ready(CLASS_DOOM_BOT);
        case ITEM_MINE_GLOVE:
            return rac1_figure_class_ready(CLASS_MINE);
        case ITEM_DECOY_GLOVE:
            return rac1_figure_class_ready(CLASS_DECOY)
                   && rac1_figure_class_ready(CLASS_GOLD_DECOY);
        default:
            return 1;
    }
}

/* A glove's throw past its held state: where and how the shooter's went, as its release sets. */
static void place_thrown(gaddr moby, const openrac_game_shot2* shot, unsigned velocity) {
    const gaddr pvars = GREF(gaddr, moby + MOBY_PVARS);
    for (unsigned c = 0; c < 3; ++c) {
        GREF(float, moby + MOBY_POS + 4u * c) = shot->position[c];
        GREF(float, pvars + velocity + 4u * c) = shot->velocity[c];
    }
}

/* The game's spawn for `shot`, for `figure`: 0 for none made. */
static gaddr spawn_shot(gaddr figure, const openrac_game_shot2* shot, gaddr v) {
    const gaddr target = target_of_key(shot->target_key);
    gaddr moby = 0;
    switch (shot->item) {
        case ITEM_BLASTER:
            return func_L00_002CC3C0(
                figure, v, target, shot->aim[0], shot->aim[1], shot->aim[2], shot->aim[3]
            );
        case ITEM_BOMB_GLOVE:
            moby = func_L00_002AE110((int)figure, v);
            if (moby != 0) {
                func_L00_002AE278(v, moby, v + 0x10u);
                /* As the shooter's game launched it (launch_bomb adds this hero's motion). */
                place_thrown(moby, shot, 0);
            }
            return moby;
        case ITEM_DEVASTATOR:
            return func_L00_002B0F58(
                figure,
                v,
                target,
                shot->aim[0],
                shot->aim[1],
                shot->aux[AUX_HEIGHT],
                shot->aim[2],
                shot->aim[3]
            );
        case ITEM_RYNO:
            return func_L00_002CE390(
                (int)figure, v, target, shot->aux[AUX_HEIGHT], shot->aim[0], shot->aim[1]
            );
        case ITEM_GLOVE_OF_DOOM:
            moby = func_L00_002C9578((int)figure, v, v + 0x10u);
            if (moby != 0) {
                place_thrown(moby, shot, 0);
                GREF(uint8_t, moby + MOBY_STATE) = 2;
            }
            return moby;
        case ITEM_MINE_GLOVE:
            moby = func_L00_002AB170((int)figure, v);
            if (moby != 0) {
                place_thrown(moby, shot, 0);
                GREF(uint8_t, moby + MOBY_STATE) = 1;
                GREF(float, moby + MOBY_SCALE) = GREF(float, moby + MOBY_SCALE) / 0.65f;
                GREF(int16_t, GREF(gaddr, moby + MOBY_PVARS) + MINE_TIMER) =
                    (int16_t)shot->aux[AUX_MINE_TIMER];
            }
            return moby;
        case ITEM_DECOY_GLOVE:
            moby = func_L00_002C48C8((int)figure, v, v + 0x10u);
            if (moby != 0) {
                place_thrown(moby, shot, DECOY_VELOCITY);
                GREF(uint8_t, moby + MOBY_STATE) = 2;
            }
            return moby;
        default:
            return 0;
    }
}

static void fire_flame(const ShotTrack* t, const float* from) {
    GFRAME(v, 32);
    for (int i = 0; i < FLAME_PARTICLES; ++i) {
        const float along = (float)((ticks_in + (uint32_t)i) % 4u) / 4.0f;
        for (unsigned c = 0; c < 3; ++c) {
            const float step = t->velocity[c] / FLAME_REACH * FLAME_SPEED;
            GREF(float, v + 4u * c) = from[c] + step * along;
            GREF(float, v + 0x10u + 4u * c) = step;
        }
        GREF(float, v + 12u) = 1.0f;
        GREF(float, v + 0x1Cu) = 0.0f;
        func_L00_0026C0D0(v, v + 0x10u, i == 0 ? 5 : 4, FLAME_REACH);
    }
}

static void fire_beam(const ShotTrack* t, const float* from) {
    if ((ticks_in & 1u) != 0) {
        return;
    }
    const int suck = t->item == ITEM_SUCK_CANNON;
    GFRAME(v, 32);
    for (int i = 0; i < BEAM_SPARKS; ++i) {
        const float along = (float)((ticks_in * 7u + (uint32_t)i * 5u) % 16u) / 16.0f;
        for (unsigned c = 0; c < 3; ++c) {
            GREF(float, v + 4u * c) = from[c] + t->velocity[c] * along;
            GREF(float, v + 0x10u + 4u * c) =
                t->velocity[c] * (suck ? -SUCK_SPEED / SUCK_REACH : BEAM_DRIFT / BEAM_REACH);
        }
        GREF(float, v + 12u) = 1.0f;
        GREF(float, v + 0x1Cu) = 0.0f;
        func_L00_0026E438(v, v + 0x10u, t->item == ITEM_MORPH_O_RAY);
    }
}

static void track_free(ShotTrack* t, int burst) {
    if (t->moby != 0 && t->load == rac1_moby_generation()) {
        if (burst && alive_as(t->moby, (int32_t)t->aim[3])) {
            func_L00_002B6FF0(t->moby, 0);
        }
        rac1_figure_track_moby_drop(t->moby, (int32_t)t->aim[3]);
    }
    t->moby = 0;
}

static int track_shot(gaddr figure, const openrac_game_shot2* shot) {
    const ShotShown shown = shot_shown(shot->item);
    if (shown != SHOT_SHOWN_FLAME && shown != SHOT_SHOWN_BEAM && shown != SHOT_SHOWN_MOBY) {
        return 0;
    }
    const uint32_t load = rac1_moby_generation();
    const int32_t id = (int32_t)shot->aux[AUX_TRACK];
    ShotTrack* t = shot_track_for(tracks, TRACKS, figure, id, shot->flags, load);
    if (t == NULL) {
        return 0;
    }
    if (t->owner != figure || t->id != id || t->load != load) {
        track_free(t, 0);
        memset(t, 0, sizeof *t);
        t->owner = figure;
        t->id = id;
        t->load = load;
    }
    t->item = shot->item;
    t->seen = ticks_in;
    t->stopped = (shot->flags & OPENRAC_GAME_SHOT_STOP) != 0;
    memcpy(t->position, shot->position, sizeof t->position);
    memcpy(t->velocity, shot->velocity, sizeof t->velocity);
    t->aim[0] = shot->aux[AUX_FROM_FIGURE];
    t->aim[3] = shot->aux[AUX_CLASS];
    if (t->stopped) {
        track_free(t, (shot->flags & OPENRAC_GAME_SHOT_BURST) != 0);
        return 1;
    }
    if (shown == SHOT_SHOWN_MOBY && t->moby == 0) {
        t->moby = rac1_figure_track_moby(figure, (int32_t)shot->aux[AUX_CLASS]);
    }
    return 1;
}

int rac1_figure_shot2(gaddr figure, const openrac_game_shot2* shot) {
    if (!in_level() || !rac1_figure_alive(figure) || GREF(int32_t, CONTEXT) != 0) {
        return 0;
    }
    int made = 0;
    if ((shot->flags
         & (OPENRAC_GAME_SHOT_START | OPENRAC_GAME_SHOT_MOVE | OPENRAC_GAME_SHOT_STOP))
        != 0) {
        made = track_shot(figure, shot);
    } else if (shot_shown(shot->item) == SHOT_SHOWN_SPAWN && shot_class_ready(shot->item)
               && room_for_shot()) {
        GFRAME(v, 32);
        for (unsigned c = 0; c < 4; ++c) {
            GREF(float, v + 4u * c) = c < 3 ? shot->position[c] : 1.0f;
            GREF(float, v + 0x10u + 4u * c) = c < 3 ? shot->velocity[c] : 0.0f;
        }
        firing_for_figure = 1;
        const gaddr moby = spawn_shot(figure, shot, v);
        firing_for_figure = 0;
        if (moby != 0) {
            note_live(moby);
            made = 1;
        }
    }
    if (rac1_figure_items_debug()) {
        fprintf(
            stderr,
            "[debug] figure items: shot of item %d flags %u %s\n",
            shot->item,
            shot->flags,
            made ? "made" : "refused"
        );
    }
    return made;
}

int rac1_figure_shot(gaddr figure, const openrac_game_shot* shot) {
    openrac_game_shot2 s;
    clear_shot(&s, shot->item);
    memcpy(&s, shot, sizeof *shot);
    return rac1_figure_shot2(figure, &s);
}

/* Each tick, after the moby loop: the figures' tracks drawn, the silent ones ended. */
void rac1_figure_shots_tick(void) {
    ++ticks_in;
    const uint32_t load = rac1_moby_generation();
    for (int i = 0; i < TRACKS; ++i) {
        ShotTrack* t = &tracks[i];
        if (t->owner == 0) {
            continue;
        }
        if (t->load != load || !rac1_figure_alive(t->owner) || shot_track_silent(t, ticks_in)) {
            track_free(t, 0);
            t->owner = 0;
            continue;
        }
        if (t->stopped) {
            continue;
        }
        float from[3];
        for (unsigned c = 0; c < 3; ++c) {
            from[c] = t->position[c];
            if (t->aim[0] != 0.0f) {
                from[c] += GREF(float, t->owner + MOBY_POS + 4u * c);
            }
        }
        switch (shot_shown(t->item)) {
            case SHOT_SHOWN_FLAME:
                fire_flame(t, from);
                break;
            case SHOT_SHOWN_BEAM:
                fire_beam(t, from);
                break;
            case SHOT_SHOWN_MOBY:
                if (t->moby != 0 && alive_as(t->moby, (int32_t)t->aim[3])) {
                    /* On along its step until the next word. */
                    const float ran = (float)(ticks_in - t->seen);
                    for (unsigned c = 0; c < 3; ++c) {
                        from[c] += t->velocity[c] * ran;
                    }
                    rac1_figure_track_moby_place(t->moby, from);
                }
                break;
            default:
                break;
        }
    }
}

/* A figure gone: what it was showing goes with it. */
void rac1_figure_shots_remove(gaddr figure) {
    for (int i = 0; i < TRACKS; ++i) {
        if (tracks[i].owner == figure) {
            track_free(&tracks[i], 0);
            tracks[i].owner = 0;
        }
    }
}

/* A figure's moby's hit or blast not let through, said with OPENRAC_ITEMS_DEBUG. */
void rac1_figure_hit_dropped(const char* what) {
    if (rac1_figure_items_debug()) {
        fprintf(stderr, "[debug] figure items: a figure's %s dropped\n", what);
    }
}

/* Whether `moby` is a figure's: what it does hits nothing here. A shot left behind by a figure gone
 * is one still. */
int rac1_figure_owned(gaddr moby) {
    if (moby == 0 || !in_level()) {
        return 0;
    }
    const gaddr pvars = GREF(gaddr, moby + MOBY_PVARS);
    if (pvars == 0) {
        return 0;
    }
    const int32_t oclass = GREF(int16_t, moby + MOBY_OCLASS);
    for (size_t i = 0; i < sizeof OWNED / sizeof OWNED[0]; ++i) {
        if (OWNED[i].oclass == oclass) {
            return rac1_figure_alive(GREF(gaddr, pvars + OWNED[i].owner)) || is_live(moby);
        }
    }
    return 0;
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
