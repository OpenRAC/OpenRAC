/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/* What a posed moby's animation does at a pose (play.c's rac1_pose_moby): the decision alone, on
 * plain numbers, so a test can run it without the game.
 *
 * A pose comes some ticks behind its sender and a little ahead or behind of the moby's own clock.
 * The moby animates by itself (advance_moby_animation), at a speed nudged so its place in the seq
 * (key + time to the next key) keeps to the pose's: a little faster when behind, slower when ahead,
 * and still while far enough ahead, or while the pose holds the key its own blend goes to. Only a
 * moby far from the pose's place is put there (MobySetAnimation). Put back every few frames
 * instead, as it was, the moby ran ahead on its clock while the poses held (a stall of the
 * sender's), and was snapped back again and again; and it ran off the end of a seq that plays
 * once (a stop) into its start again before the pose's next seq came. */
#ifndef RAC1_POSE_ANIM_H
#define RAC1_POSE_ANIM_H

/* How far, in keys, the moby's place may be from the pose's before it is put there; how much
 * faster or slower it runs for each key it is behind or ahead; its fastest, in times the pose's
 * speed. */
#define POSE_ANIM_DRIFT 3.0f
#define POSE_ANIM_GAIN 1.0f
#define POSE_ANIM_FASTEST 2.0f

enum {
    POSE_ANIM_RUN,      /* runs on at *speed */
    POSE_ANIM_BLEND,    /* blends into the pose's seq (MobyBlendAnimation) */
    POSE_ANIM_BLENDING, /* its blend plays on, at the blend's own speed */
    POSE_ANIM_SET       /* is put at the pose's key and time (MobySetAnimation), runs at *speed */
};

/* The game's ticks a second. The port ticks every disc at 60, whatever the disc's video mode, so a
 * tick's move means the same on every disc. */
#define POSE_TICKS_A_SECOND 60

/* A hero's move in a tick (HERO_VELOCITY), a pose's velocity in units a second. */
static inline float pose_velocity(float move_a_tick) {
    return move_a_tick * (float)POSE_TICKS_A_SECOND;
}

typedef struct {
    int frames;   /* the pose's seq's keys */
    int moby_seq; /* the moby's seq, its blend's target seq, key and time to the next key */
    int moby_target;
    int moby_frame;
    float moby_time;
    int pose_seq; /* the pose's seq, key, time, speed, and whether its sender blends into it */
    int pose_frame;
    float pose_time;
    float pose_speed;
    int pose_blending;
} pose_anim_in;

/* The keys from the moby's place to the pose's, the shorter way round the seq: more than 0 when
 * the moby is behind. */
static inline float pose_anim_error(const pose_anim_in* in) {
    const float want = (float)in->pose_frame + (in->pose_blending ? 0.0f : in->pose_time);
    float e = want - ((float)in->moby_frame + in->moby_time);
    const float frames = (float)in->frames;
    while (e > 0.5f * frames) {
        e -= frames;
    }
    while (e < -0.5f * frames) {
        e += frames;
    }
    return e;
}

static inline int pose_anim_step(const pose_anim_in* in, float* speed) {
    *speed = in->pose_speed;
    if (in->pose_seq != in->moby_target) {
        return POSE_ANIM_BLEND;
    }
    if (in->moby_seq != in->moby_target) {
        return POSE_ANIM_BLENDING;
    }
    const float e = pose_anim_error(in);
    if (e > POSE_ANIM_DRIFT || e < -POSE_ANIM_DRIFT) {
        return POSE_ANIM_SET;
    }
    /* While its sender blends, the pose holds the key the blend goes to: the moby waits there (it
     * only runs to catch up); otherwise it runs at the pose's speed, nudged. */
    float f = (in->pose_blending ? 0.0f : 1.0f) + POSE_ANIM_GAIN * e;
    if (f < 0.0f) {
        f = 0.0f;
    } else if (f > POSE_ANIM_FASTEST) {
        f = POSE_ANIM_FASTEST;
    }
    *speed = in->pose_speed * f;
    return POSE_ANIM_RUN;
}

#endif /* RAC1_POSE_ANIM_H */
