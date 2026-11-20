/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/*
 * The 2D quads and GUI sprite elements Going Commando drew in its last whole frame, apart from
 * text (ui2d.c records them as they go out): each quad's box in the game's screen (512 x 416), a
 * sprite element's centre (a box of no size), texture and colour. The keyboard and mouse follows a
 * grid's selection frame by the sprites (rac2 keyboard_mouse).
 */
#ifndef OPENRAC_RAC2_UI_QUADS_H
#define OPENRAC_RAC2_UI_QUADS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OPENRAC_RAC2_UI_QUADS_MAX 256

typedef struct openrac_rac2_ui_quad {
    float x0, y0, x1, y1;
    uint64_t tex0;    /* the texture (TEX0; a sprite's shape), 0 for a flat colour */
    uint32_t colour;  /* ABGR, alpha 0x80 full */
    uint32_t element; /* a GUI sprite element (its guest address), 0 for a quad */
} openrac_rac2_ui_quad;

/* The last whole frame's quads into out (at most OPENRAC_RAC2_UI_QUADS_MAX): how many. */
int openrac_rac2_take_ui_quads(openrac_rac2_ui_quad* out);

/* A frame starts (picture.c, the frame's set-up): the quads drawn so far are the last frame's. */
void openrac_rac2_ui_quads_frame(void);

#ifdef __cplusplus
}
#endif

#endif
