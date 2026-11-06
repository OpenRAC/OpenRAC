/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Ratchet & Clank: what figure_shots.c decides without touching the game, so a test can run it. A
 * weapon that is not one shot (the Pyrocitor's flame, the Tesla Claw's beam, a Visibomb in flight,
 * the drones) is a track: START when it begins, a MOVE every few ticks, STOP when it ends, each a
 * shot that may be lost. So a MOVE stands in for a lost START, a track that goes silent ends by
 * itself, and a MOVE that comes after its STOP does not bring it back.
 */
#ifndef RAC1_FIGURE_TRACKS_H
#define RAC1_FIGURE_TRACKS_H

#include <stdint.h>

#include "openrac/game_host.h"

/* The items (save tables; docs/kb/datamining.md). */
enum {
    ITEM_SUCK_CANNON = 0x09,
    ITEM_BOMB_GLOVE = 0x0A,
    ITEM_DEVASTATOR = 0x0B,
    ITEM_VISIBOMB = 0x0D,
    ITEM_BLASTER = 0x0F,
    ITEM_PYROCITOR = 0x10,
    ITEM_MINE_GLOVE = 0x11,
    ITEM_TESLA_CLAW = 0x13,
    ITEM_GLOVE_OF_DOOM = 0x14,
    ITEM_MORPH_O_RAY = 0x15,
    ITEM_RYNO = 0x17,
    ITEM_DRONE_DEVICE = 0x18,
    ITEM_DECOY_GLOVE = 0x19
};

/* How a figure shows an item's shot. */
typedef enum {
    SHOT_SHOWN_NONE, /* nothing to show but what the figure's hand does (the Walloper, a gadget) */
    SHOT_SHOWN_SPAWN, /* the game's own spawn, once: the shot then flies by itself */
    SHOT_SHOWN_FLAME, /* a track drawn as flame */
    SHOT_SHOWN_BEAM,  /* a track drawn as sparks along a line */
    SHOT_SHOWN_MOBY   /* a track drawn as a moby of the shot's class, never run */
} ShotShown;

static inline ShotShown shot_shown(int32_t item) {
    switch (item) {
        case ITEM_BOMB_GLOVE:
        case ITEM_DEVASTATOR:
        case ITEM_BLASTER:
        case ITEM_MINE_GLOVE:
        case ITEM_GLOVE_OF_DOOM:
        case ITEM_RYNO:
        case ITEM_DECOY_GLOVE:
            return SHOT_SHOWN_SPAWN;
        case ITEM_PYROCITOR:
            return SHOT_SHOWN_FLAME;
        case ITEM_SUCK_CANNON:
        case ITEM_TESLA_CLAW:
        case ITEM_MORPH_O_RAY:
            return SHOT_SHOWN_BEAM;
        case ITEM_VISIBOMB:
        case ITEM_DRONE_DEVICE:
            return SHOT_SHOWN_MOBY;
        default:
            return SHOT_SHOWN_NONE;
    }
}

/* A track of the hero's, as sent. */
typedef struct {
    uint8_t on;
    uint32_t sent;
} ShotTrackOut;

/* What goes for a track now on or off at `tick`, a MOVE each `every` ticks: 0 for nothing. */
static inline uint32_t shot_track_out(ShotTrackOut* t, int on, uint32_t tick, uint32_t every) {
    if (on && !t->on) {
        t->on = 1;
        t->sent = tick;
        return OPENRAC_GAME_SHOT_START;
    }
    if (!on && t->on) {
        t->on = 0;
        return OPENRAC_GAME_SHOT_STOP;
    }
    if (on && tick - t->sent >= every) {
        t->sent = tick;
        return OPENRAC_GAME_SHOT_MOVE;
    }
    return 0;
}

/* A track shown here. `owner` 0: a free slot. */
typedef struct {
    uint32_t owner;
    int32_t id;
    int32_t item;
    uint32_t load;
    uint32_t seen;
    uint8_t stopped;
    uint32_t moby;
    float position[3];
    float velocity[3];
    float aim[4];
} ShotTrack;

/* Ticks a track lasts with nothing heard, and how long a stopped one's slot keeps a late MOVE out. */
#define SHOT_TRACK_SILENCE 30u
#define SHOT_TRACK_STOPPED 60u

/* The track for this shot of `owner`'s: the slot it has, a free one for one that begins, the
 * longest silent otherwise; NULL for a late MOVE of a track that has stopped, and for a STOP of one
 * not shown. */
static inline ShotTrack* shot_track_for(
    ShotTrack* tracks, int count, uint32_t owner, int32_t id, uint32_t flags, uint32_t load
) {
    ShotTrack* slot = 0;
    ShotTrack* unused = 0;
    ShotTrack* oldest = 0;
    for (int i = 0; i < count; ++i) {
        ShotTrack* t = &tracks[i];
        if (t->owner == owner && t->id == id && t->load == load) {
            slot = t;
        }
        if (t->owner == 0) {
            unused = unused != 0 ? unused : t;
        } else if (oldest == 0 || t->seen < oldest->seen) {
            oldest = t;
        }
    }
    if (slot != 0) {
        return slot->stopped && (flags & OPENRAC_GAME_SHOT_START) == 0 ? 0 : slot;
    }
    if ((flags & OPENRAC_GAME_SHOT_STOP) != 0) {
        return 0;
    }
    return unused != 0 ? unused : oldest;
}

/* Whether a track is over: nothing heard for long, or stopped long ago. */
static inline int shot_track_silent(const ShotTrack* t, uint32_t tick) {
    return tick - t->seen > (t->stopped ? SHOT_TRACK_STOPPED : SHOT_TRACK_SILENCE);
}

/* Whether a figure at `mine` units from the hero takes the decode buffer from one at `theirs`, which
 * has held it `held` ticks: the nearer player's weapon is the one worth showing, by enough of a
 * margin, and not so often that two figures trade it back and forth. */
#define FIGURE_BUFFER_MARGIN 4.0f
#define FIGURE_BUFFER_HOLD 120u

static inline int figure_takes_buffer(float mine, float theirs, uint32_t held) {
    return mine + FIGURE_BUFFER_MARGIN < theirs && held >= FIGURE_BUFFER_HOLD;
}

#endif
