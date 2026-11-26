/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Going Commando: the cutscenes' letterbox bars (DrawLetterboxBars, func_0027D900) as two fills of
 * the frame recorder instead of a GS packet (docs/port/SCREEN_EFFECTS.md, section 5.2).
 *
 * RenderFrame (func_0027BD70) calls it from one place only, straight after
 * VU1_addGSregister(0x47, 0x33001): TEST_1 with the alpha test on, ATST NEVER and AFAIL RGB_ONLY (every
 * pixel fails the test and only its colour channels are written, no depth and no alpha) and the
 * depth test ALWAYS. The bars' packet sets PRIM 0x104 (an untextured triangle strip, no alpha
 * blending, no fog) and RGBAQ 0x80000000, then two bars of eight vertices: from the left to the right
 * edge of the picture, the first `h` pixels of its top and the last `h` of its bottom (h = the
 * cutscene's progress, 1 to 52). So each bar is an opaque black rectangle of colour only, whatever
 * the frame had there. Here the game's own C runs, and when the list ends in exactly
 * { the TEST 0x33001 register packet, the bars' 128-byte packet } the packet is taken back and the two
 * bars are recorded at the cursor it began at, over the whole window's width, rows taken from the
 * packet's own vertices (h and the picture's height are differences of them, scaled to the
 * recorder's 448 rows). Anything else keeps its packet.
 *
 * Not exercised: the bars seen in cutscenes (levels 6 and 8 in hidden runs, game mode 2) come from the
 * other caller, the level's frame loop (func_L00_002A3B88's case 2, DrawLetterboxBars after
 * gfx_render_frame_all_layers), which follows an ALPHA write and no TEST: those keep their packets, and
 * no run reached the RenderFrame call (it needs the cutscene flag set in play). OPENRAC_UI_SKIP naming DrawLetterboxBars,
 * OPENRAC_UI_REPLAY and no window keep the game's own.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "game_protos.h"
#include "openrac/frame.h"
#include "openrac/game_host.h"
#include "openrac/host_env.h"
#include "rac2.h"

enum {
    REGISTER_PACKET = 48u,
    BARS_PACKET = 0x80u,
    TEST_REGISTER = 0x47u,
    TEST_NO_COLOUR_DEPTH = 0x33001u,
    BAR_PRIM = 0x104u,
    BAR_RGBAQ = 0x80000000u,
    BAR_Z = 0xFFFFF3u,
    RECORDER_ROWS = 448,
    RECORDER_COLUMNS = 512
};

static uint64_t vertex(gaddr at, int i) {
    return GREF(uint64_t, at + 64u + (gaddr)i * 8u);
}

static int vx(uint64_t v) {
    return (int)(v & 0xFFFFu);
}

static int vy(uint64_t v) {
    return (int)((v >> 16) & 0xFFFFu);
}

static int vz(uint64_t v) {
    return (int)((v >> 32) & 0xFFFFFFu);
}

void func_0027D900__game(void);

void func_0027D900(void) {
    const gaddr cursor = RAC2_LIST_CURSOR;
    const gaddr before = GREF(gaddr, cursor);
    func_0027D900__game();
    const char* list = openrac_env_string("OPENRAC_UI_SKIP");
    if (!openrac_frame_native()
        || (list != NULL && strstr(list, RAC2_NAME(func_0027D900)) != NULL)) {
        return;
    }
    const gaddr after = GREF(gaddr, cursor);
    const gaddr anchor = openrac_frame_ram_cursor(before);
    if (anchor == UINT32_MAX || after != before + BARS_PACKET || before < REGISTER_PACKET) {
        return; /* no bars this frame, or not the packet read from the C */
    }
    /* The TEST write RenderFrame makes right before it. */
    const gaddr test = before - REGISTER_PACKET;
    if (GREF(uint32_t, test) != 0x10000002u || GREF(uint32_t, test + 12u) != 0x50000002u
        || GREF(uint32_t, test + 16u) != 0x8001u || GREF(uint32_t, test + 24u) != 0x0Eu
        || GREF(uint64_t, test + 32u) != TEST_NO_COLOUR_DEPTH
        || GREF(uint32_t, test + 40u) != TEST_REGISTER) {
        /* The cutscene's own call (the level's frame loop, game mode 2, after the frame's lists)
         * follows an ALPHA write, not this TEST: its state is not proved here. */
        if (openrac_env_string("OPENRAC_UI_LOG") != NULL) {
            static int kept;
            if (kept++ < 2) {
                fprintf(stderr, "letterbox: bars kept (no TEST 0x33001 before them)\n");
            }
        }
        return;
    }
    if (GREF(uint32_t, before) != 0x10000007u || GREF(uint32_t, before + 12u) != 0x50000007u
        || GREF(uint64_t, before + 32u) != BAR_PRIM || GREF(uint64_t, before + 40u) != BAR_RGBAQ) {
        return;
    }
    const uint64_t v0 = vertex(before, 0), v1 = vertex(before, 1), v2 = vertex(before, 2),
                   v3 = vertex(before, 3), v4 = vertex(before, 4), v5 = vertex(before, 5),
                   v6 = vertex(before, 6), v7 = vertex(before, 7);
    const int bar = vy(v1) - vy(v0); /* the bars' height, 12.4 */
    const int height = vy(v6) - vy(v0);
    if (bar <= 0 || height <= 0 || (bar & 15) != 0 || (height & 15) != 0 || bar * 2 > height
        || vx(v2) <= vx(v0) || vx(v1) != vx(v0) || vx(v3) != vx(v2) || vx(v4) != vx(v2)
        || vx(v5) != vx(v2) || vx(v6) != vx(v0) || vx(v7) != vx(v0) || vy(v2) != vy(v0)
        || vy(v3) != vy(v1) || vy(v4) != vy(v6) || vy(v5) != vy(v4) - bar || vy(v7) != vy(v6) - bar
        || vz(v0) != (int)BAR_Z || vz(v7) != (int)BAR_Z) {
        return;
    }
    const float scale = (float)RECORDER_ROWS / (float)(height >> 4);
    const float h = (float)(bar >> 4) * scale;
    const float top[4] = {0.0f, 0.0f, (float)RECORDER_COLUMNS, h};
    const float bottom[4] =
        {0.0f, (float)RECORDER_ROWS - h, (float)RECORDER_COLUMNS, (float)RECORDER_ROWS};
    const unsigned writes = OPENRAC_WRITE_RED | OPENRAC_WRITE_GREEN | OPENRAC_WRITE_BLUE;
    openrac_frame_anchor(anchor);
    if (!openrac_frame_fill_depth(
            top,
            BAR_RGBAQ,
            OPENRAC_BLEND_OPAQUE | OPENRAC_PICTURE_WHOLE,
            0,
            24,
            OPENRAC_DEPTH_ALWAYS,
            writes
        )) {
        return;
    }
    openrac_frame_fill_depth(
        bottom,
        BAR_RGBAQ,
        OPENRAC_BLEND_OPAQUE | OPENRAC_PICTURE_WHOLE,
        0,
        24,
        OPENRAC_DEPTH_ALWAYS,
        writes
    );
    GREF(gaddr, cursor) = before;
    if (openrac_env_string("OPENRAC_UI_LOG") != NULL) {
        static int logged;
        if (logged++ < 4) {
            fprintf(stderr, "letterbox: bars %d of %d rows native\n", bar >> 4, height >> 4);
        }
    }
}
