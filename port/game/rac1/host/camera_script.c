/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Ratchet & Clank: the game's camera scripts for the mods (game_host.h, openrac_game_play's
 * camera_script_*; interface 1.12, multiplayer).
 *
 * How the game plays one. There is no list of scripts: any moby's update calls CameraScript
 * (func_L00_002EBF50, US FUN_L00_002eaaa0) with an eye, a rotation and a mode, which puts the camera
 * of id 5 up and gives it the view; it may then call set_camera_position and set_camera_rotation
 * (func_L00_002EBE88, func_L00_002EBEE0) each tick for a shot that moves, and CameraScript2
 * (func_L00_002EC0C8) hands back to the follow camera. Mode 1 takes the view as it is. Nothing of it
 * is in the level's data under a number, so a script's `id` here is the class of the moby that runs
 * it: the same on both discs. Its views are the running game's, sent as they are (a level's camera
 * points differ a little between discs, and only the host's are in play).
 *
 * Noted: what starts or ends (camera_script.h), for the mods once a tick (camera_script_next), and
 * the view of the script running (camera_script_view). A script's end the game does not call
 * (another camera taking over, the moby gone) is noted here at the end of the moby loop.
 *
 * Shown: a mod shows another game's script here (camera_script_play, camera_script_show): at the end
 * of the moby loop, in game mode 0 with the hero alive, in the follow camera (first person, a script
 * of this game's and a scene keep the camera), the camera of id 5 is put up by the game's own calls
 * with the view it is given, and the game's own end hands it back. The player's control stays his:
 * only the camera moves. The game taking the camera away ends it, and a script of this game's that
 * starts meanwhile wins.
 */
#include <string.h>

#include "camera_script.h"
#include "rac1.h"

#define LEVEL_MODE OPENRAC_LDATA(0, RAC1_LADDR(00, 0x0015F6A8u))
#define MODE_PLAY 0

/* The camera up now (g_current_follow_cam), and its id, eye and rotation. */
#define CURRENT_CAMERA OPENRAC_LDATA(0, RAC1_LADDR(00, 0x00166F00u))
#define CAMERA_ID 0x86u
#define CAMERA_EYE 0x30u
#define CAMERA_ROTATION 0x40u
#define CAMERA_SCRIPT 5
#define CAMERA_FIRST_PERSON 4

#define MOBY_POSITION 0x10u
#define MOBY_STATE 0x20u
#define MOBY_OCLASS 0xA6u

#define HERO RAC1_ADDR(0x0013F450u)
#define HERO_POSITION 0x80u
#define HERO_MOBY (HERO + 0x2080u)
#define HERO_HEALTH (HERO + 0x22A8u)

/* The game's end of a script shown here: an eased hand-back, or at once behind the hero. */
#define END_EASED 2
#define END_CUT 0

/* How many ticks a script of this game's may have no camera up before it counts as over: the ones
 * that fade in call CameraScript before the camera has changed. */
#define LOST_TICKS 30

static rac1_camera_note note;
static uint32_t note_load;
static int lost_ticks;
static float kept_eye[3], kept_rotation[3];

/* The script a mod shows here. */
static struct {
    int wanted; /* asked, or up */
    int up;
    int end; /* the game's end to call, -1 none */
    int32_t id;
    uint32_t load;
    float eye[3], rotation[3];
} shown = {.end = -1};

static gaddr room_for_vectors(void) {
    static gaddr room;
    if (room == 0) {
        room = openrac_guest_static(64);
    }
    return room;
}

static void put_vector(gaddr at, const float v[3]) {
    GREF(float, at) = v[0];
    GREF(float, at + 4u) = v[1];
    GREF(float, at + 8u) = v[2];
    GREF(float, at + 12u) = 0.0f;
}

static void take_vector(float out[3], gaddr at) {
    for (unsigned i = 0; i < 3; ++i) {
        out[i] = GREF(float, at + 4u * i);
    }
}

static int current_camera_id(void) {
    const gaddr camera = GREF(gaddr, CURRENT_CAMERA);
    return camera != 0 ? GREF(int16_t, camera + CAMERA_ID) : -1;
}

static int32_t class_of(gaddr moby) {
    return moby != 0 ? GREF(int16_t, moby + MOBY_OCLASS) : -1;
}

/* CameraScript. */
void func_L00_002EBF50(gaddr eye, gaddr rotation, int mode, int w, int x) {
    const gaddr moby = rac1_updating_moby();
    const int standing = rac1_stand_in_on();
    if (rac1_camera_note_start(&note, moby, class_of(moby), standing)) {
        note_load = rac1_moby_generation();
        lost_ticks = 0;
        /* A script of this game's takes the camera from one shown. */
        shown.wanted = shown.up = 0;
        shown.end = -1;
        func_L00_002EBF50__game(eye, rotation, mode, w, x);
    } else if (note.on && note.stand_in) {
        note_load = rac1_moby_generation();
        take_vector(kept_eye, eye);
        take_vector(kept_rotation, rotation);
    }
}

/* CameraScript2. */
void func_L00_002EC0C8(int mode) {
    if (rac1_camera_note_stop(&note, rac1_updating_moby(), rac1_stand_in_on())) {
        func_L00_002EC0C8__game(mode);
    }
}

/* set_camera_position. */
void func_L00_002EBE88(gaddr from) {
    const int what = rac1_camera_note_view(&note, rac1_updating_moby(), rac1_stand_in_on());
    if (what == RAC1_CAMERA_VIEW_RUN) {
        func_L00_002EBE88__game(from);
    } else if (what == RAC1_CAMERA_VIEW_KEEP) {
        take_vector(kept_eye, from);
    }
}

/* set_camera_rotation. */
void func_L00_002EBEE0(gaddr from) {
    const int what = rac1_camera_note_view(&note, rac1_updating_moby(), rac1_stand_in_on());
    if (what == RAC1_CAMERA_VIEW_RUN) {
        func_L00_002EBEE0__game(from);
    } else if (what == RAC1_CAMERA_VIEW_KEEP) {
        take_vector(kept_rotation, from);
    }
}

/* Hero_ResetToPosition. */
void func_L00_00217718(gaddr position, gaddr rotation, int state, int flags) {
    if (rac1_camera_note_reset(&note, rac1_updating_moby(), state, rac1_stand_in_on())) {
        func_L00_00217718__game(position, rotation, state, flags);
    }
}

void rac1_camera_script_free_hero(void) {
    func_L00_00222B80(0, 1); /* HeroEnterMoveState */
}

static void note_over(void) {
    if (!note.on) {
        lost_ticks = 0;
        return;
    }
    const int gone = !rac1_level_running() || note_load != rac1_moby_generation();
    if (note.stand_in) {
        const gaddr m = note.moby;
        if (gone || m == 0 || GREF(signed char, m + MOBY_STATE) < 0 || class_of(m) != note.id) {
            rac1_camera_note_end(&note);
        }
        return;
    }
    lost_ticks = current_camera_id() == CAMERA_SCRIPT ? 0 : lost_ticks + 1;
    if (gone || lost_ticks > LOST_TICKS) {
        rac1_camera_note_end(&note);
    }
}

static void shown_tick(void) {
    if (!shown.wanted && !shown.up) {
        return;
    }
    const int running = rac1_level_running() && shown.load == rac1_moby_generation();
    if (!running) {
        shown.wanted = shown.up = 0;
        shown.end = -1;
        return;
    }
    if (shown.up && shown.end >= 0) {
        func_L00_002EC0C8__game(shown.end);
        shown.up = shown.wanted = 0;
        shown.end = -1;
        return;
    }
    const gaddr room = room_for_vectors();
    put_vector(room, shown.eye);
    put_vector(room + 16u, shown.rotation);
    if (!shown.up) {
        const int camera = current_camera_id();
        if (shown.end >= 0 || GREF(int, LEVEL_MODE) != MODE_PLAY || GREF(gaddr, HERO_MOBY) == 0
            || GREF(int32_t, HERO_HEALTH) <= 0 || (note.on && !note.stand_in) || camera < 0 || camera == CAMERA_SCRIPT
            || camera == CAMERA_FIRST_PERSON) {
            shown.wanted = 0;
            shown.end = -1;
            return;
        }
        func_L00_002EBF50__game(room, room + 16u, 1, 0, 0);
        shown.up = 1;
        return;
    }
    if (current_camera_id() != CAMERA_SCRIPT || GREF(int, LEVEL_MODE) != MODE_PLAY) {
        shown.up = shown.wanted = 0;
        return;
    }
    func_L00_002EBE88__game(room);
    func_L00_002EBEE0__game(room + 16u);
}

void rac1_camera_script_tick(void) {
    note_over();
    shown_tick();
}

int rac1_camera_script_next(int32_t* id, int32_t* started) {
    if (note.count == 0) {
        return 0;
    }
    *id = note.events[note.first].id;
    *started = note.events[note.first].started;
    note.first = (note.first + 1u) % RAC1_CAMERA_EVENTS;
    --note.count;
    return 1;
}

int rac1_camera_script_view(float eye[3], float rotation[3], float where[3], int32_t* stand_in) {
    if (!note.on || !rac1_level_running()) {
        return 0;
    }
    *stand_in = note.stand_in;
    if (note.moby != 0) {
        take_vector(where, note.moby + MOBY_POSITION);
    } else {
        take_vector(where, HERO + HERO_POSITION);
    }
    if (note.stand_in) {
        memcpy(eye, kept_eye, sizeof kept_eye);
        memcpy(rotation, kept_rotation, sizeof kept_rotation);
        return 1;
    }
    if (current_camera_id() != CAMERA_SCRIPT) {
        return 0;
    }
    const gaddr camera = GREF(gaddr, CURRENT_CAMERA);
    take_vector(eye, camera + CAMERA_EYE);
    take_vector(rotation, camera + CAMERA_ROTATION);
    return 1;
}

int rac1_camera_script_play(int32_t id) {
    if (id < 0) {
        if (shown.up) {
            shown.end = id == OPENRAC_GAME_CAMERA_END_CUT ? END_CUT : END_EASED;
        } else {
            shown.wanted = 0;
        }
        return 1;
    }
    if (!rac1_level_running() || shown.wanted) {
        return 0;
    }
    shown.wanted = 1;
    shown.end = -1;
    shown.id = id;
    shown.load = rac1_moby_generation();
    /* Until a view comes: where the camera is. */
    const gaddr camera = GREF(gaddr, CURRENT_CAMERA);
    if (camera == 0) {
        shown.wanted = 0;
        return 0;
    }
    take_vector(shown.eye, camera + CAMERA_EYE);
    take_vector(shown.rotation, camera + CAMERA_ROTATION);
    return 1;
}

int rac1_camera_script_show(const float eye[3], const float rotation[3]) {
    if (!shown.wanted) {
        return 0;
    }
    memcpy(shown.eye, eye, sizeof shown.eye);
    memcpy(shown.rotation, rotation, sizeof shown.rotation);
    return 1;
}
