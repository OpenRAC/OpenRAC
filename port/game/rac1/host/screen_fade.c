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

#define LIST_CURSOR RAC1_ADDR(0x00161000u)
#define SCREEN_EXTENT RAC1_ADDR(0x0013E600u)
/* The game's vertical-sync counter (game_frame_counter): one step a displayed frame. */
#define FRAME_COUNTER RAC1_ADDR(0x0015F538u)

/* The list cursor the last screen-wide tint left (taken back or kept) and the frame it was in:
 * draw_screen_effect tells from them whether a tint ran right before it (screen_effect below). */
static gaddr s_tint_end __attribute__((unused));
static int s_tint_frame __attribute__((unused)) = -1;

#if !RAC1_OMIT(func_001F55C0)
/* The most one tint appends: a register write and the sprite's DMA call. */
#define TINT_PACKET_MAX 0x80u

void func_001F55C0(int r, int g, int b, int a) {
    const gaddr cursor = OPENRAC_DATA(LIST_CURSOR);
    const gaddr before = GREF(gaddr, cursor);
    func_001F55C0__game(r, g, b, a);
    s_tint_end = GREF(gaddr, cursor);
    s_tint_frame = GREF(int, OPENRAC_DATA(FRAME_COUNTER));
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
        s_tint_end = before;
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

#if !RAC1_OMIT(func_001F5368)
/* draw_screen_effect (func_001F5368): the tail of the frame fills the screen in vertical stripes
 * from the table the pointer at 0x0015F450 names (a colour for the whole screen, then stripes of
 * two widths and colours, each with an optional ALPHA word). Its own C runs, then the rectangles
 * it appended are recorded as fills at the cursor it began at and taken back.
 *
 * The state a stripe is drawn in, from the C: DrawRectOverlay's triangle strip at depth 0xFFFFF0
 * of the 24-bit buffer, no texture, FRAME and ZBUF masks as the frame's draw environment (colour
 * and depth written), the blend the last ALPHA word names (0x44, source over, until a stripe table
 * word sets another). The depth test is the frame's own, GEQUAL (the level's TEST packet, as the
 * letterbox proof in docs/port/RAC1_COVERS.md reads it), unless a screen-wide tint ran right
 * before: its packet leaves the test at ALWAYS (resident packet 0x0013CD90). Only the two blends
 * the cover helper has (source over, 0x44, and the colour taken as it is, 0x64) are native: a
 * table with any other word keeps the packets. */
#define STRIPES_POINTER RAC1_ADDR(0x0015F450u)
enum { STRIPE_DEPTH = 0xFFFFF0, STRIPE_DEPTH_BITS = 24, ALPHA_MIX = 0x44, ALPHA_COPY = 0x64 };

/* The blend of an ALPHA word (low byte and FIX byte masked as the game does), or -1. */
static int stripe_blend(uint64_t word) {
    const unsigned mode = (unsigned)(word & 0xFFu);
    const unsigned fix = (unsigned)(word >> 32) & 0xFFu;
    if (mode == ALPHA_MIX) {
        return OPENRAC_BLEND_MIX;
    }
    if (mode == ALPHA_COPY && fix == 0x80u) {
        return OPENRAC_BLEND_OPAQUE;
    }
    return -1;
}

/* One fill of the stripe table; 0 when the helper would not take it. */
static int stripe_fill(const rac1_cover_view* view, const int* rect, uint32_t colour, int blend,
                       int test) {
    return rac1_cover_fill_depth(
        view, rect, colour, blend, STRIPE_DEPTH, STRIPE_DEPTH_BITS, test,
        OPENRAC_WRITE_COLOUR | OPENRAC_WRITE_DEPTH
    );
}

void func_001F5368(void) {
    const gaddr cursor = OPENRAC_DATA(LIST_CURSOR);
    const gaddr before = GREF(gaddr, cursor);
    const int tinted = s_tint_frame == GREF(int, OPENRAC_DATA(FRAME_COUNTER)) && s_tint_end == before;
    func_001F5368__game();
    if (!openrac_frame_native() || skipped(RAC1_NAME(func_001F5368))) {
        return;
    }
    const gaddr anchor = openrac_frame_ram_cursor(before);
    const gaddr after = GREF(gaddr, cursor);
    const gaddr table = GREF(gaddr, OPENRAC_DATA(STRIPES_POINTER));
    if (anchor == UINT32_MAX || after < before || table == 0) {
        return;
    }
    if (after == before) {
        return; /* it drew nothing */
    }
    const gaddr extent = OPENRAC_DATA(SCREEN_EXTENT);
    const int w = GREF(int, extent), h = GREF(int, extent + 4u);
    const int test = tinted ? OPENRAC_DEPTH_ALWAYS : OPENRAC_DEPTH_GEQUAL;
    const rac1_cover_view whole = {w, h, NULL, anchor, 1};
    const rac1_cover_view picture = {w, h, NULL, anchor, 0};

    /* The table, as draw_screen_effect reads it. */
    const uint32_t colour = GREF(uint32_t, table + 4u);
    const uint64_t enable = GREF(uint64_t, table + 8u);
    const int step0 = GREF(int, table + 0x10u), step1 = GREF(int, table + 0x20u);
    const uint32_t colour0 = GREF(uint32_t, table + 0x14u), colour1 = GREF(uint32_t, table + 0x24u);
    const uint64_t enable0 = GREF(uint64_t, table + 0x18u), enable1 = GREF(uint64_t, table + 0x28u);
    if (w <= 0 || h <= 0 || step0 < 0 || step1 < 0 || step0 + step1 <= 0) {
        return;
    }

    /* First every word, so that nothing is recorded for a table the helper cannot draw. */
    int blend = OPENRAC_BLEND_MIX;
    const uint64_t words[3] = {enable, enable0, enable1};
    for (int i = 0; i < 3; ++i) {
        if (words[i] != 0 && stripe_blend(words[i]) < 0) {
            return;
        }
    }

    /* The walk, with the blend as the stripes leave it. */
    if (enable != 0) {
        blend = stripe_blend(enable);
    }
    if ((colour & 0xFF000000u) != 0 && !stripe_fill(&whole, NULL, colour, blend, test)) {
        return;
    }
    for (int x = 0; x < w;) {
        const int widths[2] = {step0, step1};
        const uint32_t colours[2] = {colour0, colour1};
        const uint64_t enables[2] = {enable0, enable1};
        for (int k = 0; k < 2; ++k) {
            if (enables[k] != 0) {
                blend = stripe_blend(enables[k]);
            }
            const int x1 = x + widths[k] < w - 1 ? x + widths[k] : w - 1;
            const int rect[4] = {x, 0, x1, h};
            if ((colours[k] & 0xFF000000u) != 0 && !stripe_fill(&picture, rect, colours[k], blend, test)) {
                return;
            }
            x += widths[k];
        }
    }
    GREF(gaddr, cursor) = before;
}
#endif

#if !RAC1_OMIT(func_001F4E08)
/* FadeToBlack (func_001F4E08), the `frames`-step fade of the picture the console is showing. On
 * the console each step darkens the small buffer the display reads (the blend of the black sprite
 * second_clear_packet draws with the alpha 0x80 - (n << 7) / (n + 1), n counting down from
 * frames - 1) while the black the large buffer gets goes unseen; the steps multiply, so after the
 * step that has n left the picture is n / frames of the one that was showing, black at n = 0.
 * Here the same loop of syncs, clock steps and sent lists runs, and each list holds one recorded
 * black fill of the whole target with that total alpha, (frames - n) / frames, over the picture
 * the window shows, with none of the buffer packets (they only move the console's two buffers). The
 * tint packet of emit_rgba_draw_packet is the unseen large buffer's: it is not drawn. */
void func_001F4E08(int frames) {
    if (!openrac_frame_native() || skipped(RAC1_NAME(func_001F4E08))) {
        func_001F4E08__game(frames);
        return;
    }
    const gaddr cursor = OPENRAC_DATA(LIST_CURSOR);
    const gaddr counter = OPENRAC_DATA(FRAME_COUNTER);
    const gaddr extent = OPENRAC_DATA(SCREEN_EXTENT);

    func_00234AC8(1);
    func_00122598(0);
    GREF(int, counter) += 1;
    func_002348E8();
    for (int n = frames - 1; n >= 0; --n) {
        const gaddr anchor = openrac_frame_ram_cursor(GREF(gaddr, cursor));
        const rac1_cover_view view = {GREF(int, extent), GREF(int, extent + 4u), NULL, anchor, 1};
        const int alpha = (frames - n) * 0x80 / frames;
        uint32_t rgba;
        if (anchor != UINT32_MAX && rac1_cover_colour(0, 0, 0, alpha, &rgba)) {
            rac1_cover_fill(&view, NULL, rgba, OPENRAC_BLEND_MIX);
        }
        func_00234AC8(1);
        func_00122598(0);
        GREF(int, counter) += 1;
        func_002349B8();
        func_00234948();
    }
    func_00234AC8(1);
    func_00122598(0);
    GREF(int, counter) += 1;
    func_002348E8();
    func_001FB498();
    func_001FB530();
}
#endif
