/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/*
 * The menus' texts Going Commando drew last (font.c records them as its GUI text element draw
 * and its prints run): each one's element, place in the game's screen (512 x 416), colour and
 * first button icon. The keyboard and
 * mouse points at menu items by them (rac2 keyboard_mouse).
 */
#ifndef OPENRAC_RAC2_GUI_TEXTS_H
#define OPENRAC_RAC2_GUI_TEXTS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OPENRAC_RAC2_GUI_TEXTS_MAX 128

typedef struct openrac_rac2_gui_text {
    uint32_t element; /* the GUI text element (its guest address), 0 for a plain print */
    uint32_t colour;  /* ABGR, alpha 0x80 full */
    float x0, y0, x1, y1;
    int cell; /* its first button icon's cell along the font's row 96 (key_prompts.h), -1 none */
} openrac_rac2_gui_text;

/* The texts drawn since the last call into out (at most OPENRAC_RAC2_GUI_TEXTS_MAX): how many.
 * The list starts again empty. */
int openrac_rac2_take_gui_texts(openrac_rac2_gui_text* out);

#ifdef __cplusplus
}
#endif

#endif
