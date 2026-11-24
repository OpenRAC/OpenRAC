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
#include <string.h>

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

/* Anchors at the write pointer in the readable game word at `cursor`, accepting RAM and its aliases.
 * Invalid, null or unaligned list pointers use an unreachable anchor, never unanchored drawing.
 * Only an explicit openrac_frame_anchor(0) clears the anchor. */
static inline void openrac_frame_anchor_at(gaddr cursor) {
    /* UINT32_MAX cannot name a RAM tag and keeps a bad cursor's commands eligible for dropping. */
    if (cursor == 0) {
        openrac_frame_anchor(UINT32_MAX);
        return;
    }

    gaddr at;
    memcpy(&at, G(cursor), sizeof(at));
    const int at_in_ram =
        at < openrac_guest_ram_size
        || (at >= GUEST_RAM_UNCACHED && at - GUEST_RAM_UNCACHED < openrac_guest_ram_size)
        || (at >= GUEST_RAM_UNCACHED_ACCEL
            && at - GUEST_RAM_UNCACHED_ACCEL < openrac_guest_ram_size);
    at = openrac_guest_ram_address(at);

    /* List tags occupy aligned quadwords, including the eventual terminating tag. */
    if (!at_in_ram || at == 0 || (at & 15u) != 0 || at > openrac_guest_ram_size
        || 16u > openrac_guest_ram_size - at) {
        openrac_frame_anchor(UINT32_MAX);
        return;
    }

    openrac_frame_anchor(at);
}

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

/*
 * Transitional placements, while a game still draws part of its 2D or its effect strips as its own
 * packets (Ratchet & Clank and Going Commando, until R2 and R3): each says where something starts
 * among them, as the markers host C once wrote into the list did. Nothing is written into the list.
 */
/* The world's shrub pass starts here: the effect strips sent before it are drawn before it. */
void openrac_frame_world_shrubs(void);
/* The effect strips that follow move with attachment `attachment`'s parent of the tick before (the
 * scope game_host.h's OPENRAC_PREVIOUS_EFFECT_PARENT_MARKER named); 0 ends it. */
void openrac_frame_effect_parent(uint32_t attachment);
/* The same scope following only the parent's position, preserving world-space orientation. */
void openrac_frame_effect_parent_position(uint32_t attachment);
/* With "HUD: Edges": what follows goes out to the HUD's side, `side` (openrac_game_hud_side) past
 * the picture's; 0 ends it. OPENRAC_HUD_STRETCH: an element across the middle stretches out with the
 * rest; OPENRAC_HUD_SPLIT(x): what follows parts at x (the game's 2D pixels) instead, each side as
 * one block. */
#define OPENRAC_HUD_STRETCH 1u
#define OPENRAC_HUD_SPLIT(x) (2u | (unsigned)(x) << 2)
void openrac_frame_hud_side(int side, unsigned how);
/* `on`: what follows covers the whole target on a window wider than the picture (as a fade does),
 * rather than keeping its shape in the picture's middle; 0 ends it. */
void openrac_frame_wide(int on);
/* `on`: a scissor of the screen's whole width stays the picture's, even with "HUD: Edges" (a menu
 * parks its hidden panels past the screen's sides); 0 ends it. */
void openrac_frame_picture_band(int on);
/* The frame the window last drew with its world in (kept when the game read its frame back,
 * openrac_game_store_image), over the whole target. */
void openrac_frame_last_frame(void);

#ifdef __cplusplus
}
#endif

#endif /* OPENRAC_FRAME_H */
