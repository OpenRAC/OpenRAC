/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Ratchet & Clank: who a camera script belongs to (camera_script.c), with no game memory, so its
 * tests run alone (port/tests/camera_script_test.c).
 *
 * A script is the moby that calls CameraScript. The game knows one hero, so a moby the mods run for
 * a player of another game (the hero standing in elsewhere: world.c) would take this game's camera
 * for what that player did. Such a script is noted as the stand-in's: the game's C does not run, its
 * views are kept for the mods, and this game's camera is left alone, until that moby ends it.
 */
#ifndef RAC1_CAMERA_SCRIPT_H
#define RAC1_CAMERA_SCRIPT_H

#include <stdint.h>

#define RAC1_CAMERA_EVENTS 8

typedef struct rac1_camera_note {
    int on;
    int stand_in; /* the script is a stand-in's: this game's camera is not in it */
    uint32_t moby;
    int32_t id;
    /* Starts and ends the mods are still to hear of, oldest at `first`. */
    struct {
        int32_t id;
        int started;
    } events[RAC1_CAMERA_EVENTS];
    unsigned first;
    unsigned count;
} rac1_camera_note;

/* What a call that sets the script's view does. */
enum {
    RAC1_CAMERA_VIEW_RUN,  /* the game's C runs */
    RAC1_CAMERA_VIEW_KEEP, /* kept for the mods, not run */
    RAC1_CAMERA_VIEW_DROP  /* neither: another's, and this game has a script of its own */
};

static inline void rac1_camera_note_event(rac1_camera_note* n, int32_t id, int started) {
    /* The mods hear once a tick: a full queue loses the oldest pair's start, not the news. */
    if (n->count == RAC1_CAMERA_EVENTS) {
        n->first = (n->first + 1u) % RAC1_CAMERA_EVENTS;
        --n->count;
    }
    const unsigned at = (n->first + n->count) % RAC1_CAMERA_EVENTS;
    n->events[at].id = id;
    n->events[at].started = started;
    ++n->count;
}

static inline void rac1_camera_note_end(rac1_camera_note* n) {
    if (n->on) {
        rac1_camera_note_event(n, n->id, 0);
    }
    n->on = 0;
    n->stand_in = 0;
    n->moby = 0;
}

static inline void rac1_camera_note_begin(
    rac1_camera_note* n, uint32_t moby, int32_t id, int stand_in
) {
    n->on = 1;
    n->stand_in = stand_in;
    n->moby = moby;
    n->id = id;
    rac1_camera_note_event(n, id, 1);
}

/* CameraScript called by `moby` of class `id`, the hero standing in elsewhere or not: 1 when the
 * game's C runs. */
static inline int rac1_camera_note_start(
    rac1_camera_note* n, uint32_t moby, int32_t id, int standing
) {
    if (n->on && n->moby == moby) {
        /* Asked again by the moby that runs it (several do so whenever the camera is not theirs). */
        return !n->stand_in;
    }
    if (standing) {
        if (n->on && !n->stand_in) {
            return 0; /* this game's own goes on */
        }
        rac1_camera_note_end(n);
        rac1_camera_note_begin(n, moby, id, 1);
        return 0;
    }
    rac1_camera_note_end(n);
    rac1_camera_note_begin(n, moby, id, 0);
    return 1;
}

/* CameraScript2 called by `moby`: 1 when the game's C runs. */
static inline int rac1_camera_note_stop(rac1_camera_note* n, uint32_t moby, int standing) {
    if (!n->on) {
        return !standing;
    }
    const int theirs = n->moby == moby;
    if (n->stand_in) {
        if (!theirs && !standing) {
            return 1;
        }
        rac1_camera_note_end(n);
        return 0;
    }
    if (standing && !theirs) {
        return 0;
    }
    rac1_camera_note_end(n);
    return 1;
}

/* set_camera_position or set_camera_rotation called by `moby`. */
static inline int rac1_camera_note_view(const rac1_camera_note* n, uint32_t moby, int standing) {
    if (n->on && n->stand_in && (n->moby == moby || standing)) {
        return RAC1_CAMERA_VIEW_KEEP;
    }
    if (n->on && !n->stand_in && standing && n->moby != moby) {
        return RAC1_CAMERA_VIEW_DROP;
    }
    return RAC1_CAMERA_VIEW_RUN;
}

/* The hero put in place by `moby` (Hero_ResetToPosition) in `state`, 0 his own control: 1 when the
 * game's C runs. For a stand-in it would be another player's hero, in this game, with the camera
 * started again behind him. A moby that holds this game's hero by a script of this game's still lets
 * him go, a member near or not. */
static inline int rac1_camera_note_reset(
    const rac1_camera_note* n, uint32_t moby, int state, int standing
) {
    if (n->on && n->stand_in && n->moby == moby) {
        return 0;
    }
    if (!standing) {
        return 1;
    }
    return state == 0 && n->on && n->moby == moby;
}

/* The states a moby's script holds the hero in, standing still for it: entered for a stand-in, they
 * are let go at once (rac1_stand_in_end). */
static inline int rac1_camera_hold_state(int state) {
    return state == 0x72 || state == 0x1F || state == 0x3B || state == 0x32;
}

#endif
