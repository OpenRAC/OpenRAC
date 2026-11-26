/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/*
 * The button icons the game's text showed last (ui2d.c records them as FontPrint draws them): each
 * icon's button and the place of it and the text after it on its line, in the game's screen. The
 * keyboard and mouse presses an icon's button for a click on it (rac1 keyboard_mouse).
 */
#ifndef OPENRAC_RAC1_PROMPT_SPOTS_H
#define OPENRAC_RAC1_PROMPT_SPOTS_H

#ifdef __cplusplus
extern "C" {
#endif

#define OPENRAC_RAC1_PROMPT_SPOTS_MAX 32

typedef struct openrac_rac1_prompt_spot {
    int cell; /* the icon's cell along the font's row 96 (key_prompts.h: Cross 0 .. R2 7) */
    int x0, y0, x1, y1;
} openrac_rac1_prompt_spot;

/* The spots drawn since the last call into out (at most OPENRAC_RAC1_PROMPT_SPOTS_MAX): how many.
 * The list starts again empty. */
int openrac_rac1_take_prompt_spots(openrac_rac1_prompt_spot* out);

#ifdef __cplusplus
}
#endif

#endif
