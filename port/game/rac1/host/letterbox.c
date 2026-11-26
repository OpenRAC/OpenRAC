/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Ratchet & Clank PAL v2.00: the letterbox bars of Blarg's (level 6) frame draw as the frame
 * recorder's depth-tested rectangles (docs/port/RAC1_COVERS.md). Other versions leave this out
 * (RAC1_OMIT_func_001F5148).
 *
 * draw_letterbox_bars (func_001F5148) runs as the game's C, once, every time: it steps the height
 * and appends the bars' packet. Only where the state the packet draws in is proved is that packet
 * taken back and the two bars recorded at the cursor it began at, with its own depth (0xFFFFF3 of
 * the 24-bit buffer, tested greater-or-equal and written) and colour (unblended black, alpha 128).
 * Proved: while the bars are enabled, in every level and in play and the freeze dialog alike (the
 * frame renderer's vu1_gs_regs_alt packet, byte for byte the same in all 19 level images, sets
 * TEST and ALPHA right before the HUD; update_hud returns at once while enabled; this is checked
 * on the list per frame). Measured only: the shrinking frames in play on Blarg (level 6). The
 * shrinking frames elsewhere (their HUD callbacks are level code), no window, OPENRAC_UI_REPLAY
 * and OPENRAC_UI_SKIP naming this function keep the packet.
 */
#include <stdint.h>
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

/* The packet vu1_gs_regs_alt (func_00234F40) sends: 48 bytes in every level's data (read from
 * each of the 19 level images of the PAL disc: byte for byte the same, at a different offset). A
 * GIF tag (one packed A+D list of two writes, EOP) and the writes TEST_1 = 0x5380B (alpha test
 * GEQUAL with reference 0x80, failing pixels write RGB only, DATE off, depth test GEQUAL) and
 * ALPHA_1 = 0x44 (source over). */
static const uint32_t ALT_PACKET[12] =
    {0x00008001u,
     0x20000000u,
     0x000000EEu,
     0x00000000u,
     0x0005380Bu,
     0x00000000u,
     0x00000047u,
     0x00000000u,
     0x00000044u,
     0x00000000u,
     0x00000042u,
     0x00000000u};

/* True when the DMA ref tag vu1_gs_regs_alt appended (0x30000003 .. 0x50000003) sits before the
 * HUD's output and names that packet, and nothing between it and the bars is a register write
 * the HUD could have used: the HUD's own packets are drawn through DrawRectOverlay,
 * DrawTexturedQuad and FontPrintWindow only (docs/port/RAC1_COVERS.md). */
static int alt_state_proved(gaddr before) {
    for (gaddr a = before; a >= 16u && before - a < 0x10000u; a -= 16u) {
        if (GREF(uint32_t, a) == 0x30000003u && GREF(uint32_t, a + 12u) == 0x50000003u) {
            const gaddr packet = GREF(gaddr, a + 4u);
            if (packet < 16u || packet > GUEST_RAM_SIZE - sizeof ALT_PACKET) {
                return 0; /* an unset or wild pointer names no packet */
            }
            for (int i = 0; i < 12; ++i) {
                if (GREF(uint32_t, packet + (gaddr)i * 4u) != ALT_PACKET[i]) {
                    return 0;
                }
            }
            return 1;
        }
    }
    return 0;
}

void func_001F5148(void) {
    const gaddr cursor = OPENRAC_DATA(LIST_CURSOR);
    const gaddr before = GREF(gaddr, cursor);
    /* Read before the game's own step, which steps the height and never writes the enable word. */
    const int enabled = GREF(int, OPENRAC_DATA(BARS_ENABLED)) != 0;
    const int shrinking = !enabled && GREF(int, OPENRAC_DATA(BARS_HEIGHT)) > 0;
    func_001F5148__game();

    if (letterbox_skipped()) {
        return;
    }

    /* The known states. Enabled, in any level and either game mode (play, the freeze dialog):
     * the frame renderer resets TEST and ALPHA with vu1_gs_regs_alt right before the HUD, and
     * update_hud returns at once while the bars are enabled; the packet that reset sends is
     * checked below, per frame. Shrinking: the HUD's callbacks run first through pointers, a
     * level's own code, and were measured on Blarg in play only; elsewhere the packet stays. */
    if (enabled) {
        const gaddr start = openrac_frame_ram_cursor(before);
        if (start == UINT32_MAX || !alt_state_proved(start)) {
            return;
        }
    } else if (!shrinking || GREF(int, OPENRAC_DATA(CURRENT_LEVEL)) != BLARG
               || GREF(int, LEVEL_MODE) != PLAY) {
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
