/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/* A posed moby's animation (rac1/host/pose_anim.h), against a moby that animates as the game's do
 * (its time to the next key on by its speed times the key's length each tick, round the seq):
 * - a new seq is blended into, a blend plays on at its own speed, a moby far from the pose is put
 *   there;
 * - while the poses stop coming (a stall of the sender's), the moby stops within a key of the
 *   last one, and is never put back (it was, every few frames: rubberbanding);
 * - at the end of a seq that plays once, the moby ahead of the poses (as it comes out of its
 *   blend) does not run round into the seq's start;
 * - while the sender blends, the moby waits on the key the blend goes to;
 * - running with the poses, it keeps to them within a fraction of a key;
 * - a pose's velocity is a tick's move by the port's 60 ticks a second on every disc. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "../game/rac1/host/pose_anim.h"

#define CHECK(c)                                                                                   \
    do {                                                                                           \
        if (!(c)) {                                                                                \
            fprintf(stderr, "%d: %s\n", __LINE__, #c);                                             \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)

/* A key's length at speed 1, in keys a tick (the hero's walk and stop seqs). */
#define KEY 0.25f

typedef struct {
    int frames;
    int frame;
    float time;
    float speed;
    int wrapped; /* times it came round to its first key */
    int set;     /* times it was put at a pose */
} Moby;

/* The pose's place `place` keys into seq 3 at `speed`; one tick of the moby after it. */
static int tick(Moby* m, float place, float speed, int blending) {
    pose_anim_in in = {
        .frames = m->frames,
        .moby_seq = 3,
        .moby_target = 3,
        .moby_frame = m->frame,
        .moby_time = m->time,
        .pose_seq = 3,
        .pose_frame = (int)place,
        .pose_time = place - floorf(place),
        .pose_speed = speed,
        .pose_blending = blending,
    };
    float s = 0.0f;
    const int action = pose_anim_step(&in, &s);
    if (action == POSE_ANIM_SET) {
        m->frame = in.pose_frame;
        m->time = blending ? 0.0f : in.pose_time;
        ++m->set;
    }
    m->speed = s;
    m->time += s * KEY;
    while (m->time >= 1.0f) {
        m->time -= 1.0f;
        if (++m->frame == m->frames) {
            m->frame = 0;
            ++m->wrapped;
        }
    }
    return action;
}

/* The keys the moby is ahead of `place`, the short way round. */
static float ahead(const Moby* m, float place) {
    float d = (float)m->frame + m->time - place;
    while (d > 0.5f * (float)m->frames) {
        d -= (float)m->frames;
    }
    while (d < -0.5f * (float)m->frames) {
        d += (float)m->frames;
    }
    return d;
}

static void the_actions(void) {
    pose_anim_in in =
        {.frames = 13,
         .moby_seq = 3,
         .moby_target = 3,
         .moby_frame = 4,
         .pose_seq = 5,
         .pose_frame = 4,
         .pose_speed = 0.55f};
    float s = 0.0f;
    CHECK(pose_anim_step(&in, &s) == POSE_ANIM_BLEND);
    in.pose_seq = 3;
    in.moby_seq = 0xFF; /* in its blend */
    CHECK(pose_anim_step(&in, &s) == POSE_ANIM_BLENDING);
    in.moby_seq = 3;
    in.pose_frame = 9;
    CHECK(pose_anim_step(&in, &s) == POSE_ANIM_SET && s == 0.55f);
    /* Round the seq the short way: key 12 to key 0 is one key on, not twelve back. */
    in.moby_frame = 12;
    in.pose_frame = 0;
    CHECK(fabsf(pose_anim_error(&in) - 1.0f) < 1e-6f);
    CHECK(pose_anim_step(&in, &s) == POSE_ANIM_RUN && s > 0.55f);
}

static void a_stall(void) {
    Moby m = {.frames = 22};
    float place = 0.0f;
    for (int t = 0; t < 600; ++t) {
        /* Running at 1.33 but for ticks 200 to 260 (the poses held at the last one). */
        const int stalled = t >= 200 && t < 260;
        if (!stalled) {
            place = fmodf(place + 1.33f * KEY, 22.0f);
        }
        tick(&m, place, 1.33f, 0);
        CHECK(ahead(&m, place) <= 1.0f + 1e-4f);
        if (t > 30 && t < 200) {
            CHECK(fabsf(ahead(&m, place)) < 0.6f);
        }
    }
    CHECK(m.set == 0);
    /* Back with the poses: a tick on from the last (its own tick follows the pose's). */
    CHECK(fabsf(ahead(&m, place) - 1.33f * KEY) < 0.05f);
}

static void the_end_of_a_seq_that_plays_once(void) {
    /* The stop seq: 13 keys, from key 4 at 0.55 then 1.0, the last pose at 11.59, held there
     * until the next seq comes (12 ticks later); the moby out of its blend 0.9 of a key ahead. */
    Moby m = {.frames = 13, .frame = 4, .time = 0.9f};
    float place = 4.0f;
    for (int t = 0; t < 60; ++t) {
        const float speed = t < 20 ? 0.55f : 1.0f;
        place = fminf(place + speed * KEY, 11.59f);
        tick(&m, place, speed, 0);
        CHECK(m.frame >= 4 && ahead(&m, place) <= 1.0f + 1e-4f);
    }
    CHECK(m.wrapped == 0 && m.set == 0);
}

static void the_sender_blends(void) {
    /* Out of its own (shorter) blend on key 8, the moby waits there while the sender's goes on. */
    Moby m = {.frames = 22, .frame = 8};
    for (int t = 0; t < 10; ++t) {
        tick(&m, 8.0f, 0.6f, 1);
        CHECK(m.frame == 8 && m.time == 0.0f && m.speed == 0.0f);
    }
    /* Behind the key, it runs to it. */
    m.frame = 7;
    m.time = 0.5f;
    tick(&m, 8.0f, 0.6f, 1);
    CHECK(m.speed > 0.0f);
}

/* A tick's move at 60 ticks a second, whichever disc's video rate. */
static void a_velocity(void) {
    CHECK(POSE_TICKS_A_SECOND == 60);
    CHECK(pose_velocity(0.25f) == 15.0f);
    CHECK(pose_velocity(-0.1f) == -0.1f * 60.0f);
}

int main(void) {
    a_velocity();
    the_actions();
    a_stall();
    the_end_of_a_seq_that_plays_once();
    the_sender_blends();
    puts("pose_anim: all passed");
    return 0;
}
