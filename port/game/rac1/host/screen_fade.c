/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Ratchet & Clank PAL v2.00: the screen-wide tint and the anti-aliasing pass without GS packets
 * (docs/port/SCREEN_EFFECTS.md, section 4).
 *
 * emit_rgba_draw_packet (func_001F55C0) is the one routine behind every screen-wide tint: the
 * death and white fades, the underwater tint, FadeToBlack's black, the dimmed pause and freeze
 * dialogs. Its packet sets RGBAQ and draws the 16 strips of a screen-wide sprite, source-over
 * with the draw environment's alpha (the default mix), no texture. Here the routine runs as the
 * game's C, then the same fill is recorded natively at the cursor the packet began at and the
 * packet is taken back, so the fade keeps its place among the list's other draws.
 *
 * AA_BlurPass (func_001FB848) appends the packet that blurs the frame into the console's small
 * buffer. The window draws at its own resolution and keeps no such buffer: with a window the
 * packet is not sent.
 *
 * OPENRAC_UI_SKIP naming either keeps its packet, for comparing in one build.
 */
#include <stdio.h>
#include <string.h>

#include "openrac/frame.h"
#include "openrac/host_env.h"
#include "rac1.h"
#include "screen_cover.h"

static int skipped(const char* name) {
    const char* list = openrac_env_string("OPENRAC_UI_SKIP");
    return list != NULL && strstr(list, name) != NULL;
}

#if !RAC1_OMIT(func_001F55C0)
#define LIST_CURSOR RAC1_ADDR(0x00161000u)
#define SCREEN_EXTENT RAC1_ADDR(0x0013E600u)
/* The most one tint appends: a register write and the sprite's DMA call. */
#define TINT_PACKET_MAX 0x80u

void func_001F55C0(int r, int g, int b, int a) {
    const gaddr cursor = OPENRAC_DATA(LIST_CURSOR);
    const gaddr before = GREF(gaddr, cursor);
    func_001F55C0__game(r, g, b, a);
    if (!openrac_frame_native() || skipped(RAC1_NAME(func_001F55C0))) {
        return;
    }
    const gaddr anchor = openrac_frame_ram_cursor(before);
    const gaddr after = GREF(gaddr, cursor);
    if (anchor == UINT32_MAX || after <= before || after - before > TINT_PACKET_MAX) {
        return;
    }
    uint32_t rgba;
    if (!rac1_cover_colour(r, g, b, a, &rgba)) {
        return;
    }
    const gaddr extent = OPENRAC_DATA(SCREEN_EXTENT);
    const rac1_cover_view view = {GREF(int, extent), GREF(int, extent + 4u), NULL, anchor, 1};
    /* Source-over; the console's depth write of the sprite (Z 0, test always) is not kept: the
     * 2D layer after a tint tests with GEQUAL at depths of its own. */
    const int taken = rac1_cover_fill(&view, NULL, rgba, OPENRAC_BLEND_MIX);
    if (taken) {
        GREF(gaddr, cursor) = before;
    }
    /* OPENRAC_UI_LOG: each tint, once per colour, in the log. */
    if (openrac_env_string("OPENRAC_UI_LOG") != NULL) {
        static uint32_t seen[16];
        static int count;
        for (int i = 0; i < count; ++i) {
            if (seen[i] == (rgba & 0xFFFFFF00u) + (uint32_t)(a >> 4)) {
                return;
            }
        }
        if (count < 16) {
            seen[count++] = (rgba & 0xFFFFFF00u) + (uint32_t)(a >> 4);
            fprintf(stderr, "screen_fade: tint %d %d %d %d (%u bytes) %s\n", r, g, b, a,
                    (unsigned)(after - before), taken ? "native" : "kept");
        }
    }
}
#endif

#if !RAC1_OMIT(func_001FB848)
void func_001FB848(void) {
    if (openrac_frame_native() && !skipped(RAC1_NAME(func_001FB848))) {
        return;
    }
    func_001FB848__game();
}
#endif
