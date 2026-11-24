/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Ratchet & Clank PAL v2.00: the letterbox bars of Blarg's (level 6) frame draw as the frame
 * recorder's depth-tested rectangles (docs/port/RAC1_COVERS.md). The proof of the caller state is
 * the level-6 one there; other versions leave this out (RAC1_OMIT_func_001F5148).
 *
 * draw_letterbox_bars (func_001F5148) runs as the game's C, once, every time: it steps the height
 * and appends the bars' packet. Only where the state the packet draws in is known on level 6 is
 * that packet taken back and the two bars recorded at the cursor it began at, with its own depth
 * (0xFFFFF3 of the 24-bit buffer, tested greater-or-equal and written) and colour (unblended
 * black, alpha 128): while the bars are enabled (update_hud returns before drawing: proved from the
 * C), and while they shrink in play (game mode 0: the HUD's callbacks run first and left the same
 * state in the in-game check). The shrinking frames of the freeze dialog (game mode 4), other
 * levels, no window, OPENRAC_UI_REPLAY and OPENRAC_UI_SKIP naming this function keep the packet.
 */
#include <string.h>

#include "openrac/frame.h"
#include "openrac/host_env.h"
#include "rac1.h"
#include "screen_cover.h"

#if !RAC1_OMIT(func_001F5148)
enum {
    BLARG = 6,                /* the level whose frame draw's state is proved */
    PLAY = 0,                 /* the game mode whose shrinking frames were measured */
    BAR_PACKET = 0x80,        /* the bars' packet: its DMA tag, two GIF tags and eight vertices */
    BAR_DEPTH = 0xFFFFF3,     /* the depth of its vertices (draw_letterbox_bars' XYZF2 words) */
    DEPTH_BITS = 24,          /* the depth buffer SetPalMode sets up (ZBUF's 24-bit format) */
    SCREEN_HEIGHT_OFFSET = 4u /* g_screen_extent[1], the picture's height, after its width */
};

/* The bars' enable word, their height, the 2D list's write pointer, the current level and the
 * screen extent (g_screen_extent: width, height, then the viewport in 12.4). */
#define BARS_ENABLED RAC1_ADDR(0x0015F544u)
#define BARS_HEIGHT RAC1_ADDR(0x0015F548u)
#define LIST_CURSOR RAC1_ADDR(0x00161000u)
#define CURRENT_LEVEL RAC1_ADDR(0x0015EE84u)
#define SCREEN_EXTENT RAC1_ADDR(0x0013E600u)
/* The game mode (D_L00_0015F6A8, level data: the loaded level's copy of level 0's word). */
#define LEVEL_MODE OPENRAC_LDATA(0, RAC1_LADDR(00, 0x0015F6A8u))

/* OPENRAC_UI_SKIP naming draw_letterbox_bars keeps its packet, for comparing in one build. */
static int letterbox_skipped(void) {
    const char* list = openrac_env_string("OPENRAC_UI_SKIP");
    return list != NULL && strstr(list, RAC1_NAME(func_001F5148)) != NULL;
}

void func_001F5148(void) {
    const gaddr cursor = OPENRAC_DATA(LIST_CURSOR);
    const gaddr before = GREF(gaddr, cursor);
    /* Read before the game's own step, which steps the height and never writes the enable word. */
    const int enabled = GREF(int, OPENRAC_DATA(BARS_ENABLED)) != 0;
    const int shrinking = !enabled && GREF(int, OPENRAC_DATA(BARS_HEIGHT)) > 0;
    func_001F5148__game();

    /* Only on Blarg, and not when OPENRAC_UI_SKIP asks for the packet. */
    if (GREF(int, OPENRAC_DATA(CURRENT_LEVEL)) != BLARG || letterbox_skipped()) {
        return;
    }

    /* The known states: enabled (no HUD before the bars), or shrinking in play (the HUD's
     * callbacks first, then the same state at the bars); the freeze dialog keeps its packet. */
    if (!enabled && !(shrinking && GREF(int, LEVEL_MODE) == PLAY)) {
        return;
    }

    /* Where the packet began, and where the game's step left the cursor. */
    const gaddr anchor = openrac_frame_ram_cursor(before);
    const gaddr after = openrac_frame_ram_cursor(GREF(gaddr, cursor));

    /* The game appended exactly its bars' packet at a reachable list position, or it keeps it. */
    if (anchor == UINT32_MAX || after != anchor + BAR_PACKET) {
        return;
    }

    /* The bars span the whole target, as the residual packet's full-width triangles did. */
    const gaddr extent = OPENRAC_DATA(SCREEN_EXTENT);
    const rac1_cover_view view =
        {GREF(int, extent), GREF(int, extent + SCREEN_HEIGHT_OFFSET), NULL, anchor, 1};
    const int height = GREF(int, OPENRAC_DATA(BARS_HEIGHT));

    /* Recorded natively: the packet is taken back, so DoGifPaging's tail tag takes its place in the
     * walk, after the HUD and help range. Without a window the packet stays. */
    if (rac1_cover_letterbox_depth(&view, height, BAR_DEPTH, DEPTH_BITS, OPENRAC_DEPTH_GEQUAL)) {
        GREF(gaddr, cursor) = before;
    }
}
#endif
