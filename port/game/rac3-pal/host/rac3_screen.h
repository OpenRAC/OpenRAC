/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * The screen glow and the second screen effect as native post passes (screen.c,
 * docs/port/SCREEN_EFFECTS.md): the host C of each copy's rows calls these with the copy's own
 * globals, relocated to the program loaded now. */
#ifndef RAC3_SCREEN_H
#define RAC3_SCREEN_H

#include "openrac/guest.h"

/* The glow's globals in one program: `word` the word the glow sets for the frame ($gp - 0xC0C in
 * the single-player levels; the tint's four words, the add-back weight follow it), `twice` the
 * byte that blurs the largest copy once more, `list` the display list's write pointer. */
typedef struct Rac3Glow {
    gaddr word;
    gaddr twice;
    gaddr list;
} Rac3Glow;

/* The glow's first half: the word set and the whole glow recorded at the list's place. */
int rac3_screen_glow(const Rac3Glow* glow);
/* The glow's second half: the word cleared (the add-back is part of the recorded pass). */
int rac3_screen_glow_end(const Rac3Glow* glow);
/* The second effect: the frame blurred and mixed back, at the list's place. */
int rac3_screen_soften(gaddr list);

#endif
