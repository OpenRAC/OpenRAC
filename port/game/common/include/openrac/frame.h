/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * The native frame recorder (docs/port/FRAME_RECORDER.md): a game's host C hands the window its
 * frame as an ordered list of commands, appended directly. The order of the calls is the draw
 * order. Nothing is written into the game's display list or memory, and no command carries a word
 * of the console's graphics chip. Every call is a no-op without a window.
 *
 * 2D coordinates are the game's 2D screen pixels (512 x 448 on PAL), floats, from the top left;
 * colours 0xAABBGGRR with 0x80 full (the games' units). Off-screen panels are named by port ids
 * (OPENRAC_FRAME_TARGETS and up), never by the console's memory.
 */
#ifndef OPENRAC_FRAME_H
#define OPENRAC_FRAME_H

#include <stdint.h>

#include "openrac/guest.h"
#include "openrac/ui_item.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Blends (renderer/direct/draw_state.h BlendMode). */
#define OPENRAC_BLEND_OPAQUE 0
#define OPENRAC_BLEND_MIX 1      /* drawn over, by the source's alpha */
#define OPENRAC_BLEND_ADD 2      /* the source times its alpha, added */
#define OPENRAC_BLEND_SUBTRACT 3 /* the source times its alpha, taken away */
#define OPENRAC_BLEND_MULTIPLY 4 /* the destination darkened by the source's alpha */
/* Or-ed into a picture's or a fill's blend: on a window wider than the game's picture, cover the
 * whole window (a backdrop, a fade) rather than the picture's middle. */
#define OPENRAC_PICTURE_WHOLE 0x100

/* openrac_frame_clear's `what`. */
#define OPENRAC_CLEAR_COLOUR 1
#define OPENRAC_CLEAR_DEPTH 2

/* The first port id of an off-screen panel. */
#define OPENRAC_FRAME_TARGETS 0x10000u

/* Starts a new recording (optional: the window closes one at each tick it captures). */
void openrac_frame_begin(void);
/* Closes the recording (optional). */
void openrac_frame_end(void);

/* Transitional, while the game still sends residual 2D as packets in its own display list: the
 * list's write pointer (its value, a game address) the commands that follow belong at. The window
 * finds the list's tag at that address and draws them there; 0 for none. Goes when the game sends
 * no residual packets (P7, R3). */
void openrac_frame_anchor(gaddr list_cursor);

/* The world (OPENRAC_DRAW_* layers) is drawn here: the 2D before it goes under the world. */
void openrac_frame_world(unsigned layers);
/* The world's moby pass starts here: effect strips sent before it are drawn before the mobys. */
void openrac_frame_world_mobys(void);
/* `count` mobys from `list`, drawn with the camera the game holds now, into the current target (a
 * panel, or the screen over a menu's world). Nonzero when the window draws them. */
int openrac_frame_mobys(gaddr list, int count);

/* A quad of the 2D layer from the game's data alone (openrac/ui_item.h). 1 when the window draws it
 * natively; 0 (no window, OPENRAC_UI_REPLAY): the caller draws the game's way. */
int openrac_frame_ui_item(const openrac_game_ui_item* item);

/* Transitional: the older 2D record (openrac_game_ui_rect, with the chip's TEX0), drawn in the 2D
 * state in effect where it lands. 1 drawn, 0 the caller draws the game's way. Retired by P4. */
struct openrac_game_ui_rect;
int openrac_frame_ui_quad(const struct openrac_game_ui_rect* quad);

/* Transitional: the world effect quad openrac_game_effect_quad_marker named takes the alpha test and
 * depth mode in effect here among the residual packets. Retired by P4. */
void openrac_frame_effect_quad(uint32_t marker);

/* 1 when the window draws what is recorded natively; 0 without a window or under OPENRAC_UI_REPLAY,
 * where the caller draws the game's way: for the calls below that return nothing. */
int openrac_frame_native(void);

/* A flat rectangle (x0, y0, x1, y1; NULL: the whole target) in colour `rgba`, blended. */
void openrac_frame_fill(const float* rect, uint32_t rgba, int blend);
/* Clears the target's colour to `rgba` and/or its depth (OPENRAC_CLEAR_*). */
void openrac_frame_clear(uint32_t rgba, int what);

/* Port picture `id` (a snapshot, or pixels given with openrac_frame_picture_set) over `rect` (NULL:
 * the picture's area), modulated by `rgba`, blended. */
void openrac_frame_picture(uint32_t id, const float* rect, uint32_t rgba, int blend);
/* A picture straight from the game's own data (a menu background, a loading picture): decoded once
 * per content into a normal texture, drawn over `rect` (NULL: the picture's area). */
void openrac_frame_picture_image(
    const openrac_game_ui_image* image, const float* rect, uint32_t rgba, int blend
);
/* Gives port picture `id` RGBA pixels (rows top first, alpha 0x80 opaque) the host decoded or
 * assembled itself; kept until set again. */
void openrac_frame_picture_set(uint32_t id, const uint8_t* rgba, int width, int height);
/* Keeps the frame drawn so far as port picture `id`. */
void openrac_frame_snapshot(uint32_t id);

/* Draws into off-screen panel `id` (OPENRAC_FRAME_TARGETS and up), `width` x `height` of the game's
 * 2D pixels, until openrac_frame_target_end. Effect quads and pictures can sample it by id. */
void openrac_frame_target_begin(uint32_t id, int width, int height);
void openrac_frame_target_end(void);

/* What follows is cut to `rect` (x0, y0, x1, y1; NULL: the whole target). */
void openrac_frame_scissor(const float* rect);

#ifdef __cplusplus
}
#endif

#endif /* OPENRAC_FRAME_H */
