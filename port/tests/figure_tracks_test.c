/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * What rac1/host/figure_shots.c decides about shots (figure_tracks.h): which items show what, a
 * track's life when any of its shots may be lost, and who gets the decode buffer.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/rac1/host/figure_tracks.h"

#define CHECK(c)                                                                                   \
    do {                                                                                           \
        if (!(c)) {                                                                                \
            fprintf(stderr, "%d: %s\n", __LINE__, #c);                                             \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)

#define FIGURE_A 0x1000u
#define FIGURE_B 0x2000u

static void test_every_weapon_shows(void) {
    /* The arsenal has a way of showing but the Walloper, whose swing is the figure's hand. */
    static const int spawned[] = {
        ITEM_BOMB_GLOVE,
        ITEM_DEVASTATOR,
        ITEM_BLASTER,
        ITEM_MINE_GLOVE,
        ITEM_GLOVE_OF_DOOM,
        ITEM_RYNO,
        ITEM_DECOY_GLOVE,
    };
    for (size_t i = 0; i < sizeof spawned / sizeof spawned[0]; ++i) {
        CHECK(shot_shown(spawned[i]) == SHOT_SHOWN_SPAWN);
    }
    CHECK(shot_shown(ITEM_PYROCITOR) == SHOT_SHOWN_FLAME);
    CHECK(shot_shown(ITEM_TESLA_CLAW) == SHOT_SHOWN_BEAM);
    CHECK(shot_shown(ITEM_MORPH_O_RAY) == SHOT_SHOWN_BEAM);
    CHECK(shot_shown(ITEM_SUCK_CANNON) == SHOT_SHOWN_BEAM);
    CHECK(shot_shown(ITEM_VISIBOMB) == SHOT_SHOWN_MOBY);
    CHECK(shot_shown(ITEM_DRONE_DEVICE) == SHOT_SHOWN_MOBY);
    CHECK(shot_shown(0x12) == SHOT_SHOWN_NONE); /* the Walloper */
    CHECK(shot_shown(8) == SHOT_SHOWN_NONE);    /* the wrench: a held piece */
    CHECK(shot_shown(-1) == SHOT_SHOWN_NONE);
    CHECK(shot_shown(0x7F) == SHOT_SHOWN_NONE);
}

static void test_track_out(void) {
    ShotTrackOut t;
    memset(&t, 0, sizeof t);
    CHECK(shot_track_out(&t, 0, 1, 6) == 0);
    CHECK(shot_track_out(&t, 1, 2, 6) == OPENRAC_GAME_SHOT_START);
    /* A MOVE each six ticks, so a held trigger costs ten shots a second, not sixty. */
    int moves = 0;
    for (uint32_t tick = 3; tick < 63; ++tick) {
        moves += shot_track_out(&t, 1, tick, 6) == OPENRAC_GAME_SHOT_MOVE;
    }
    CHECK(moves == 10);
    CHECK(shot_track_out(&t, 0, 63, 6) == OPENRAC_GAME_SHOT_STOP);
    CHECK(shot_track_out(&t, 0, 64, 6) == 0);
    CHECK(shot_track_out(&t, 1, 65, 6) == OPENRAC_GAME_SHOT_START);
}

static ShotTrack* begin(ShotTrack* tracks, int n, uint32_t owner, int id, uint32_t flags, uint32_t tick) {
    ShotTrack* t = shot_track_for(tracks, n, owner, id, flags, 7);
    CHECK(t != NULL);
    t->owner = owner;
    t->id = id;
    t->load = 7;
    t->seen = tick;
    t->stopped = (flags & OPENRAC_GAME_SHOT_STOP) != 0;
    return t;
}

static void test_a_track_survives_losses(void) {
    ShotTrack tracks[4];
    memset(tracks, 0, sizeof tracks);
    /* Its START lost: the first MOVE begins it. */
    ShotTrack* t = begin(tracks, 4, FIGURE_A, 0, OPENRAC_GAME_SHOT_MOVE, 10);
    CHECK(t == &tracks[0]);
    /* The same again: its own slot. */
    CHECK(shot_track_for(tracks, 4, FIGURE_A, 0, OPENRAC_GAME_SHOT_MOVE, 7) == t);
    /* Another player's, or another track of his, a slot of its own. */
    CHECK(shot_track_for(tracks, 4, FIGURE_B, 0, OPENRAC_GAME_SHOT_MOVE, 7) == &tracks[1]);
    CHECK(shot_track_for(tracks, 4, FIGURE_A, 1, OPENRAC_GAME_SHOT_MOVE, 7) == &tracks[1]);
    /* Its STOP lost: over by itself. */
    CHECK(!shot_track_silent(t, 10 + SHOT_TRACK_SILENCE));
    CHECK(shot_track_silent(t, 11 + SHOT_TRACK_SILENCE));
}

static void test_a_late_move_does_not_bring_it_back(void) {
    ShotTrack tracks[2];
    memset(tracks, 0, sizeof tracks);
    begin(tracks, 2, FIGURE_A, 0, OPENRAC_GAME_SHOT_START, 10);
    ShotTrack* t = begin(tracks, 2, FIGURE_A, 0, OPENRAC_GAME_SHOT_STOP, 12);
    CHECK(t->stopped);
    CHECK(shot_track_for(tracks, 2, FIGURE_A, 0, OPENRAC_GAME_SHOT_MOVE, 7) == NULL);
    CHECK(shot_track_for(tracks, 2, FIGURE_A, 0, OPENRAC_GAME_SHOT_STOP, 7) == NULL);
    /* A new one does. */
    CHECK(shot_track_for(tracks, 2, FIGURE_A, 0, OPENRAC_GAME_SHOT_START, 7) == t);
    /* A stopped one keeps its slot longer than a live one, to keep the late MOVE out. */
    CHECK(!shot_track_silent(t, 12 + SHOT_TRACK_STOPPED));
    CHECK(shot_track_silent(t, 13 + SHOT_TRACK_STOPPED));
}

static void test_a_stop_of_none_and_an_old_load(void) {
    ShotTrack tracks[2];
    memset(tracks, 0, sizeof tracks);
    /* A STOP of a track never shown makes none. */
    CHECK(shot_track_for(tracks, 2, FIGURE_A, 0, OPENRAC_GAME_SHOT_STOP, 7) == NULL);
    begin(tracks, 2, FIGURE_A, 0, OPENRAC_GAME_SHOT_START, 10);
    /* The last level's is not this one's. */
    CHECK(shot_track_for(tracks, 2, FIGURE_A, 0, OPENRAC_GAME_SHOT_STOP, 8) == NULL);
}

static void test_full_takes_the_oldest(void) {
    ShotTrack tracks[2];
    memset(tracks, 0, sizeof tracks);
    begin(tracks, 2, FIGURE_A, 0, OPENRAC_GAME_SHOT_START, 20);
    begin(tracks, 2, FIGURE_B, 0, OPENRAC_GAME_SHOT_START, 10);
    CHECK(shot_track_for(tracks, 2, FIGURE_A, 3, OPENRAC_GAME_SHOT_MOVE, 7) == &tracks[1]);
}

static void test_nearest_figure_keeps_the_buffer(void) {
    /* Nearer by enough, after the holder has had it a while. */
    CHECK(figure_takes_buffer(10.0f, 40.0f, FIGURE_BUFFER_HOLD));
    /* Not by a step: the two would trade it as they walk. */
    CHECK(!figure_takes_buffer(10.0f, 10.0f + FIGURE_BUFFER_MARGIN, FIGURE_BUFFER_HOLD));
    CHECK(!figure_takes_buffer(40.0f, 10.0f, FIGURE_BUFFER_HOLD));
    /* Not at once: a decode is not free. */
    CHECK(!figure_takes_buffer(10.0f, 40.0f, FIGURE_BUFFER_HOLD - 1));
}

int main(void) {
    test_every_weapon_shows();
    test_track_out();
    test_a_track_survives_losses();
    test_a_late_move_does_not_bring_it_back();
    test_a_stop_of_none_and_an_old_load();
    test_full_takes_the_oldest();
    test_nearest_figure_keeps_the_buffer();
    puts("figure_tracks ok");
    return 0;
}
