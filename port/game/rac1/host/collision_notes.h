/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Developer tools: the collision queries (collision.c) near a watched point, for a diagnostics
 * recording. The profile watches Ratchet each tick and reads back the queries that started near him:
 * which entry, its flags, what it ignored, its segment or volume, its result and the CollOutput it
 * left, so a query that hits what it should not can be told from the code that reads its answer.
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

/* Each tick: what the last one saw. */
typedef struct {
    uint32_t calls, hits; /* every query of the tick */
    uint32_t count;       /* the notes kept, the first so many near the point */
    OpenracCollNote notes[16];
} OpenracCollNotes;

/* Watches the queries starting within `radius` of `point` (0 stops), and starts a new tick's notes. */
void openrac_rac1_coll_watch(const float point[3], float radius);

/* The notes since the last watch. */
const OpenracCollNotes* openrac_rac1_coll_notes(void);

#ifdef __cplusplus
}
#endif

#endif
