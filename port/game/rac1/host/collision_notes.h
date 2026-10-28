/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Developer tools: the collision queries (collision.c) near a watched point, for a diagnostics
 * recording. collision.c keeps the watch (OpenracCollWatch) and hands it out as
 * openrac_game_diagnostics (game_host.h); the profile (game/rac1/profile.cpp) points it at Ratchet
 * each tick and reads back the queries that started near him: which entry, its flags, what it
 * ignored, its segment or volume, its result and the CollOutput it left, so a query that hits what
 * it should not can be told from the code that reads its answer.
 */
#ifndef OPENRAC_RAC1_COLLISION_NOTES_H
#define OPENRAC_RAC1_COLLISION_NOTES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* One query. A line's a and b are its ends; a volume's a is its centre and b its radius and height.
 * entry: 0 line, 1 sphere, 2 capsule, 5 the executable's line. ignore: the record it leaves out, with
 * that record's first word. tmpl: a line's hit template. out: where CollOutput is; output: its
 * +0x18..+0x7F after the query (moby, kind, point, +0x30, normal, triangle). */
typedef struct {
    uint32_t entry, flags, ignore, ignore_word, tmpl, result, out;
    float a[3], b[3];
    uint32_t output[26];
} OpenracCollNote;

/* The watch: the point and squared radius (0: off), and what queries saw since it was set. */
typedef struct {
    float point[3], rr;
    uint32_t calls, hits; /* every query */
    uint32_t count;       /* the notes kept, the first so many near the point */
    OpenracCollNote notes[16];
} OpenracCollWatch;

#ifdef __cplusplus
}
#endif

#endif
