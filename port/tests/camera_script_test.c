/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Who a camera script belongs to (camera_script.h). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../game/rac1/host/camera_script.h"
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "%d: %s\n", __LINE__, #c); exit(1); } } while (0)

enum { DOOR = 0x1000, LIFT = 0x2000 };

static int next(rac1_camera_note* n, int* id, int* started) {
    if (n->count == 0) {
        return 0;
    }
    *id = n->events[n->first].id;
    *started = n->events[n->first].started;
    n->first = (n->first + 1u) % RAC1_CAMERA_EVENTS;
    --n->count;
    return 1;
}

static void told(rac1_camera_note* n, int id, int started) {
    int a = -2, b = -2;
    CHECK(next(n, &a, &b));
    CHECK(a == id && b == started);
}

int main(void) {
    rac1_camera_note n;
    int a, b;

    /* This game's own: the game's C runs, once told, ended by its moby. */
    memset(&n, 0, sizeof n);
    CHECK(rac1_camera_note_start(&n, DOOR, 737, 0));
    told(&n, 737, 1);
    CHECK(rac1_camera_note_start(&n, DOOR, 737, 0));
    CHECK(!next(&n, &a, &b));
    CHECK(rac1_camera_note_view(&n, DOOR, 0) == RAC1_CAMERA_VIEW_RUN);
    CHECK(rac1_camera_note_reset(&n, DOOR, 0x72, 0));
    /* A member comes near as it plays: another moby's stand-in leaves it alone, and its own moby
     * still ends it and lets the hero go. */
    CHECK(rac1_camera_note_view(&n, LIFT, 1) == RAC1_CAMERA_VIEW_DROP);
    CHECK(!rac1_camera_note_stop(&n, LIFT, 1));
    CHECK(!rac1_camera_note_start(&n, LIFT, 877, 1));
    CHECK(!next(&n, &a, &b));
    CHECK(n.moby == DOOR);
    CHECK(rac1_camera_note_reset(&n, DOOR, 0, 1));
    CHECK(!rac1_camera_note_reset(&n, DOOR, 0x72, 1));
    CHECK(rac1_camera_note_stop(&n, DOOR, 1));
    told(&n, 737, 0);
    CHECK(!n.on);

    /* A stand-in's: this game's camera and hero are not touched, the views kept. */
    memset(&n, 0, sizeof n);
    CHECK(!rac1_camera_note_start(&n, LIFT, 877, 1));
    told(&n, 877, 1);
    CHECK(n.on && n.stand_in);
    CHECK(rac1_camera_note_view(&n, LIFT, 1) == RAC1_CAMERA_VIEW_KEEP);
    /* The member walks off: still the stand-in's, whoever is nearest. */
    CHECK(rac1_camera_note_view(&n, LIFT, 0) == RAC1_CAMERA_VIEW_KEEP);
    CHECK(!rac1_camera_note_start(&n, LIFT, 877, 0));
    CHECK(!rac1_camera_note_reset(&n, LIFT, 0, 0));
    CHECK(!next(&n, &a, &b));
    /* This game's hero and another moby of this game's are free meanwhile. */
    CHECK(rac1_camera_note_view(&n, DOOR, 0) == RAC1_CAMERA_VIEW_RUN);
    CHECK(rac1_camera_note_reset(&n, DOOR, 0, 0));
    CHECK(rac1_camera_note_stop(&n, DOOR, 0));
    CHECK(n.on);
    /* Its end ends it here without touching this game's camera. */
    CHECK(!rac1_camera_note_stop(&n, LIFT, 0));
    told(&n, 877, 0);
    CHECK(!n.on);

    /* Nothing runs: a stand-in's end or teleport touches nothing, this game's own passes. */
    memset(&n, 0, sizeof n);
    CHECK(!rac1_camera_note_stop(&n, LIFT, 1));
    CHECK(rac1_camera_note_stop(&n, LIFT, 0));
    CHECK(!rac1_camera_note_reset(&n, LIFT, 0, 1));
    CHECK(!rac1_camera_note_reset(&n, LIFT, 0x72, 1));
    CHECK(rac1_camera_note_reset(&n, LIFT, 0x72, 0));
    CHECK(!next(&n, &a, &b));

    /* This game's script takes over from a stand-in's: one end, one start. */
    memset(&n, 0, sizeof n);
    CHECK(!rac1_camera_note_start(&n, LIFT, 877, 1));
    told(&n, 877, 1);
    CHECK(rac1_camera_note_start(&n, DOOR, 737, 0));
    told(&n, 877, 0);
    told(&n, 737, 1);
    CHECK(!n.stand_in);

    /* A full queue keeps the newest. */
    memset(&n, 0, sizeof n);
    for (int i = 0; i < RAC1_CAMERA_EVENTS + 2; ++i) {
        rac1_camera_note_event(&n, i, 1);
    }
    CHECK(n.count == RAC1_CAMERA_EVENTS);
    told(&n, 2, 1);

    CHECK(rac1_camera_hold_state(0x72));
    CHECK(!rac1_camera_hold_state(0));

    puts("camera_script: ok");
    return 0;
}
