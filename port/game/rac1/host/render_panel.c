/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Ratchet & Clank PAL v2.00: the off-screen panels the vendor's monitors and the pause menu render
 * into, as panels of the frame recorder (docs/port/FRAME_RECORDER.md, section 3.3, and
 * docs/port/SCREEN_EFFECTS.md, section 5.1).
 *
 * setup_render_target (func_001FB608) points the draw environment at a (1 << a) x (1 << b) area of
 * the console's frame memory and clears it (the monitors' background func_001FB908 follows it);
 * everything the game then draws goes there until PutDrawBufferLarge (func_001FB498) puts the draw
 * buffer back. Here the game's own C runs unchanged, and the recorder is told where the panel opens
 * and where it closes, at the list cursor of each: the packets, the 2D quads and the moby markers
 * in between are drawn into a native render target (its id says which block it stood for), and the
 * quad that later shows the panel (a texture at that block) samples it.
 *
 * The two clears the game's packets make in a panel are recorded too. setup_render_target's packet
 * ends in a black sprite over the whole area (colour and depth, test always); the monitors'
 * background (draw_centered_capture_background, func_001FB908) is a sprite of 512 x 512 (test
 * always) that leaves the colour as it is (ALPHA Cd) and writes depth 0, over the panel's whole
 * area. Here the sprite of the first is cut off the end of its DMA packet (the draw environment and
 * the TEST before it stay: the panel's block and offset still come from them) and recorded as a
 * fill of colour 0 and depth 0 at the panel's opening, and the second is taken back whole and
 * recorded as a depth-only fill at the cursor it began at. The monitors' passes run the background
 * once more after the panel is closed, over the screen's depth, which is recorded the same way.
 * Either keeps its packet when the window does not draw it, when the packet is not the size and
 * words read from the C, or when no panel of this frame is open (the panel's own).
 *
 * OPENRAC_UI_SKIP naming a function, OPENRAC_UI_REPLAY and no window keep the game's own;
 * `panel_clear` keeps the setup's sprite, `screen_clear` the background's after the close and
 * func_001FB908 both backgrounds.
 */
#include <stdio.h>
#include <string.h>

#include "openrac/frame.h"
#include "openrac/host_env.h"
#include "rac1.h"

#if !RAC1_OMIT(func_001FB608) && !RAC1_OMIT(func_001FB498) && !RAC1_OMIT(func_001FB908)
/* The list cursor the packets are appended through (g_render_packet_cursor). */
#define LIST_CURSOR RAC1_ADDR(0x00161000u)
/* The game's vertical-sync counter (game_frame_counter): one step a displayed frame. */
#define FRAME_COUNTER RAC1_ADDR(0x0015F538u)
/* A panel's port id: this base and the page its memory starts at (c >> 13). */
#define PANEL_BASE 0x52320000u

static int panel_skipped(void) {
    const char* list = openrac_env_string("OPENRAC_UI_SKIP");
    return list != NULL
           && (strstr(list, RAC1_NAME(func_001FB608)) != NULL
               || strstr(list, RAC1_NAME(func_001FB498)) != NULL);
}

/* OPENRAC_UI_SKIP naming draw_centered_capture_background keeps the packets of both clears. */
static int skip_names(const char* name) {
    const char* list = openrac_env_string("OPENRAC_UI_SKIP");
    return list != NULL && strstr(list, name) != NULL;
}

/* The frame a panel was opened in, or -1: the close only follows an open of the same frame. */
static int s_open_frame = -1;
/* The open panel's size in pixels. */
static int s_panel_w, s_panel_h;

enum {
    SETUP_BYTES = 0xF0,        /* setup_render_target's packet: DMA tag, draw environment, sprite */
    SETUP_SPRITE_BYTES = 0x30, /* its last three quadwords: the sprite's GIF tag and two of data */
    SETUP_DMA_WORD = 0x1000000Eu, /* the packet's DMA tag (14 quadwords) and its VIF DIRECT word */
    SETUP_VIF_WORD = 0x5000000Eu,
    SETUP_SPRITE_TAG = 0x8001, /* the sprite's GIF tag: one loop, EOP; the register list below */
    SETUP_SPRITE_PRIM = 0x106, /* the sprite's PRIM word */
    BACKGROUND_SIZE = 0x200,   /* every caller of the monitors' background passes 512 x 512 */
    BACKGROUND_STRIPS = 16     /* 512 / 32 sprites a pair of corners each */
};

void func_001FB608(int a, int b, int c) {
    if (openrac_frame_native() && !panel_skipped() && a > 0 && a <= 10 && b > 0 && b <= 10) {
        openrac_frame_anchor_at(OPENRAC_DATA(LIST_CURSOR));
        openrac_frame_target_begin(PANEL_BASE | (((unsigned)c >> 13) & 0xFFFFu), 1 << a, 1 << b);
        s_open_frame = GREF(int, OPENRAC_DATA(FRAME_COUNTER));
        s_panel_w = 1 << a;
        s_panel_h = 1 << b;
    }
    const gaddr cursor = OPENRAC_DATA(LIST_CURSOR);
    const gaddr before = openrac_frame_ram_cursor(GREF(gaddr, cursor));
    func_001FB608__game(a, b, c);
    if (s_open_frame < 0 || s_open_frame != GREF(int, OPENRAC_DATA(FRAME_COUNTER))
        || before == UINT32_MAX || !openrac_frame_native() || skip_names("panel_clear")) {
        return;
    }
    /* The packet is the one the C builds, byte for byte where it matters. */
    if (openrac_frame_ram_cursor(GREF(gaddr, cursor)) != before + SETUP_BYTES
        || GREF(uint32_t, before) != SETUP_DMA_WORD
        || GREF(uint32_t, before + 12u) != SETUP_VIF_WORD
        || GREF(uint64_t, before + SETUP_BYTES - 0x30u)
               != (0x4400000000000000ull | SETUP_SPRITE_TAG)
        || GREF(uint64_t, before + SETUP_BYTES - 0x20u) != SETUP_SPRITE_PRIM) {
        return;
    }
    const gaddr sprite = before + SETUP_BYTES - SETUP_SPRITE_BYTES;
    GREF(uint32_t, before) = SETUP_DMA_WORD - 3u;
    GREF(uint32_t, before + 12u) = SETUP_VIF_WORD - 3u;
    GREF(gaddr, cursor) = sprite;
    /* At the packet's start, where the panel opened: the clear follows its TargetBegin there. */
    openrac_frame_anchor(before);
    openrac_frame_fill_depth(
        NULL,
        0,
        OPENRAC_BLEND_OPAQUE | OPENRAC_PICTURE_WHOLE,
        0,
        24,
        OPENRAC_DEPTH_ALWAYS,
        OPENRAC_WRITE_COLOUR | OPENRAC_WRITE_DEPTH
    );
}

/* draw_centered_capture_background (func_001FB908): the depth clear, as above, in a panel of this
 * frame and, after the panel is closed, over the screen (the monitors' passes clear the large
 * buffer's depth again before their quad). The game sets the ALPHA register first (a register
 * packet of three quadwords the C appends before the sprite's); that stays. */
void func_001FB908(int w, int h) {
    const gaddr cursor = OPENRAC_DATA(LIST_CURSOR);
    const gaddr before = openrac_frame_ram_cursor(GREF(gaddr, cursor));
    func_001FB908__game(w, h);
    if (before == UINT32_MAX || !openrac_frame_native() || skip_names(RAC1_NAME(func_001FB908))
        || w != BACKGROUND_SIZE || h != BACKGROUND_SIZE) {
        return;
    }
    const int in_panel =
        s_open_frame >= 0 && s_open_frame == GREF(int, OPENRAC_DATA(FRAME_COUNTER));
    if (!in_panel && skip_names("screen_clear")) {
        return;
    }
    if (in_panel && (s_panel_w > w || s_panel_h > h)) {
        return;
    }
    /* One DMA tag, then five quadwords of header and a quadword a strip: the sprite's packet is
     * the last of what the call appended. */
    const uint32_t quadwords = BACKGROUND_STRIPS + 5u;
    const gaddr after = openrac_frame_ram_cursor(GREF(gaddr, cursor));
    if (after == UINT32_MAX || after < before + 16u * (quadwords + 1u)) {
        return;
    }
    const gaddr sprite = after - 16u * (quadwords + 1u);
    if (GREF(uint32_t, sprite) != (quadwords | 0x10000000u)) {
        return;
    }
    GREF(gaddr, cursor) = sprite;
    openrac_frame_anchor_at(cursor);
    openrac_frame_fill_depth(
        NULL,
        0,
        OPENRAC_BLEND_OPAQUE | OPENRAC_PICTURE_WHOLE,
        0,
        24,
        OPENRAC_DEPTH_ALWAYS,
        OPENRAC_WRITE_DEPTH
    );
    if (openrac_env_string("OPENRAC_UI_LOG") != NULL) {
        static int logged;
        if (logged++ < 4) {
            fprintf(
                stderr,
                "render_panel: depth clear %s\n",
                in_panel ? "in a panel" : "over the screen"
            );
        }
    }
}

void func_001FB498(void) {
    if (s_open_frame >= 0) {
        if (s_open_frame == GREF(int, OPENRAC_DATA(FRAME_COUNTER))) {
            openrac_frame_anchor_at(OPENRAC_DATA(LIST_CURSOR));
            openrac_frame_target_end();
        }
        s_open_frame = -1;
    }
    func_001FB498__game();
}
#endif
