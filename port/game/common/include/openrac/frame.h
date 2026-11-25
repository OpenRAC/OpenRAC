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

/* openrac_frame_screen_effect's kinds: native post passes over the target as drawn so far
 * (docs/port/SCREEN_EFFECTS.md). */
#define OPENRAC_SCREEN_GLOW 1   /* the frame's glowing pixels copied, tinted, blurred, added back */
#define OPENRAC_SCREEN_SOFTEN 2 /* the frame blurred and mixed back over itself */

/* A screen effect's parameters, the game's own values (colours 0xAABBGGRR, 0x80 = 1.0). */
typedef struct openrac_screen_effect {
    int kind; /* OPENRAC_SCREEN_* */
    /* The copy's colour: each channel of the frame times it; its alpha times the frame's alpha. */
    uint32_t tint;
    /* GLOW: a pixel glows when the frame's alpha times the tint's alpha is above this (0x80 = 1.0,
     * the game's units); the rest of the copy is black. SOFTEN: not read. */
    uint32_t threshold;
    /* How much of the blurred copy goes back, 0x80 = all of it: added (GLOW) or mixed (SOFTEN). */
    uint32_t strength;
    /* Extra blur passes at the copy's largest size (0 to 2). */
    uint32_t blur;
} openrac_screen_effect;
/* A view's rectangle in the game's 2D screen pixels (openrac_frame_screen's), from the top left. */
typedef struct openrac_frame_viewport {
    float x0, y0, x1, y1;
} openrac_frame_viewport;

/* The views a split screen has at most (openrac_frame_view_begin's `view`). */
#define OPENRAC_FRAME_VIEWS 4

/* Starts a new recording (optional: the window closes one at each tick it captures). */
void openrac_frame_begin(void);
/* Closes the recording (optional). */
void openrac_frame_end(void);

/* Announces a fresh construction in the display-list buffer at `root`, after its real reset/swap
 * and before its first recorded command. Previous commands belonging to that buffer are retired
 * even if the new build records nothing; untouched buffers remain valid for late/resent lists.
 * This is not a tick boundary or a DMA send. Native commands still require reached anchors.
 * Producers that never call this retain the transitional tag-only matching behavior. */
void openrac_frame_list_begin(gaddr root);
/* The game's 2D screen the coordinates of the commands that follow are in (512 x 448 until set):
 * a game whose 2D is drawn on another size (NTSC's 512 x 416) says so once, and its rectangles,
 * scissors and views are taken as they are. Kept across recordings. */
void openrac_frame_screen(float width, float height);

/* Split screen: the world recorded between view_begin and view_end is local player view `view`'s
 * (0 to OPENRAC_FRAME_VIEWS - 1), drawn into `rect` with the camera the game holds when the view's
 * openrac_frame_world is recorded (the one it draws that view with). The window keeps each view's last picture and puts every view's on
 * the screen each frame, so a game that draws one view a frame (alternating) and one that draws
 * all of them in one frame both show every view. A recording with no view is a full-screen frame:
 * the views are forgotten. view_begin while a view is open ends it first. */
void openrac_frame_view_begin(int view, const openrac_frame_viewport* rect);
void openrac_frame_view_end(void);

/* Transitional, while the game still sends residual 2D as packets in its own display list: the
 * list's write pointer (its value, a game address) the commands that follow belong at. The window
 * finds the list's tag at that address and draws them there; 0 for none. Goes when the game sends
 * no residual packets (P7, R3). */
void openrac_frame_anchor(gaddr list_cursor);

/* Normalizes a complete aligned RAM list cursor, or returns UINT32_MAX (unreachable) for invalid
 * input. Shared by the cursor-word helper and explicit list ownership at the C boundary. */
static inline gaddr openrac_frame_ram_cursor(gaddr at) {
    const int at_in_ram =
        at < openrac_guest_ram_size
        || (at >= GUEST_RAM_UNCACHED && at - GUEST_RAM_UNCACHED < openrac_guest_ram_size)
        || (at >= GUEST_RAM_UNCACHED_ACCEL
            && at - GUEST_RAM_UNCACHED_ACCEL < openrac_guest_ram_size);
    at = openrac_guest_ram_address(at);

    /* List tags occupy aligned quadwords, including the eventual terminating tag. */
    if (!at_in_ram || at == 0 || (at & 15u) != 0 || at > openrac_guest_ram_size
        || 16u > openrac_guest_ram_size - at) {
        return UINT32_MAX;
    }

    return at;
}

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
    openrac_frame_anchor(openrac_frame_ram_cursor(at));
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

/* 1 when the window draws what is recorded natively; 0 without a window or under OPENRAC_UI_REPLAY,
 * where the caller draws the game's way: for the calls below that return nothing. */
int openrac_frame_native(void);

/* A flat rectangle (x0, y0, x1, y1; NULL: the whole target) in colour `rgba`, blended. */
void openrac_frame_fill(const float* rect, uint32_t rgba, int blend);

/* openrac_frame_fill_depth's tests, against the target's depth (bigger is nearer, cleared to 0). */
#define OPENRAC_DEPTH_ALWAYS 0
#define OPENRAC_DEPTH_GEQUAL 1  /* where the rectangle is as near as what is there, or nearer */
#define OPENRAC_DEPTH_GREATER 2 /* only where it is nearer */
/* openrac_frame_fill_depth's `writes`: what it writes where it passes. */
#define OPENRAC_WRITE_RED 1u
#define OPENRAC_WRITE_GREEN 2u
#define OPENRAC_WRITE_BLUE 4u
#define OPENRAC_WRITE_ALPHA 8u
#define OPENRAC_WRITE_COLOUR 15u /* the four channels above */
#define OPENRAC_WRITE_DEPTH 16u  /* the rectangle's depth */

/* A flat rectangle as openrac_frame_fill, with a depth of its own: `depth`, in the units of a
 * `depth_bits`-bit depth buffer (16, 24 or 32, bigger nearer, as the game's own), is compared with
 * the target's by `depth_test` (OPENRAC_DEPTH_*), and where it passes `writes` (OPENRAC_WRITE_*)
 * are written. It goes into the current target (the screen, or openrac_frame_target_begin's panel),
 * cut to the current scissor. 1 when the window draws it; 0 (no window, OPENRAC_UI_REPLAY, a value
 * out of range): the caller draws the game's way. */
int openrac_frame_fill_depth(
    const float* rect,
    uint32_t rgba,
    int blend,
    uint32_t depth,
    int depth_bits,
    int depth_test,
    unsigned writes
);
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
/* A screen effect over the target as drawn so far (a native post pass: the frame copied, shrunk,
 * blurred and put back), at this point of the frame. */
void openrac_frame_screen_effect(const openrac_screen_effect* effect);
/* Keeps the last frame the window drew with its world in (the world alone, before its 2D) as port
 * picture `id`: what a menu that read its frame back as it opened draws under itself. The picture
 * follows the world while the game draws one under the menu. */
void openrac_frame_snapshot_world(uint32_t id);

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
