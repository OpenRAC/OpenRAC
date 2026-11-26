/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Going Commando: the screen-wide fades and tints RenderFrame (func_0027BD70) draws at the end of
 * every frame, as fills of the frame recorder instead of GS packets (docs/port/SCREEN_EFFECTS.md,
 * section 5.2).
 *
 * gfx_set_clear_color_packet (func_0027DDC0) appends two things to the display list: a register
 * write of RGBAQ (VU1_addGSregister(1, r | g << 8 | b << 16 | a << 24), 48 bytes) and a DMA call
 * of the 16-byte tag { 0x30000014, g_gfx_clear_packet_a, 0, 0x50000014 }. That packet (the same
 * words on the three discs, read from each program's data) writes TEST = 0x30000 (no alpha test,
 * depth test ALWAYS, depth written) and PRIM = 0x146 (a sprite, alpha blended, no texture, no fog),
 * then 16 sprites a pair of corners each, 32 pixels wide (x 0x6FF8 + i * 0x200), from
 * y 0x7FF8 - h * 8 to y 0x7FF8 + h * 8 for the game's picture height h (gfx_..., func_00284268):
 * the whole 512 x h picture in the colour set before it. Nothing in the list reads the sprites'
 * depth (0) after them: the frame ends with the stripe effect, which sets its own state.
 *
 * What RenderFrame's tail proves of the blend: after the frame's last DoGifPaging, with the layer
 * flag 0x40, it writes ALPHA (VU1_addGSregister(0x42, 0x8000000044): A = Cs, B = Cd, C = As,
 * D = Cd, source over) and then, in this order, the clear colour (when the level has one), the
 * black fade g_screen_fade * 128 and the white flash g_screen_flash_alpha * 128, each through
 * gfx_set_clear_color_packet. Here the game's own C runs, and when the list ends in exactly
 * { ALPHA = 0x44 register packet, RGBAQ register packet, the DMA call } with the ALPHA packet at
 * the end of that last DoGifPaging, the three are taken back and a whole-picture source-over fill
 * in the colour is recorded at the cursor they began at.
 *
 * The second proved shape is local to the call: gfx_write_clear_color_packets (func_0027B7E0)
 * right before the RGBAQ write, its list { call of g_gfx_clear_color_packet, call of
 * g_gs_draw_env_regs, register 0x3D } read back from the list and the two packets' own words
 * (ALPHA_1 = 0x44 source over, FBMSK 0, ZBUF 24-bit written; the same bytes on NTSC-U v1.00, v2.00
 * and PAL). That is the pause menu's dimming, the splash's fade and the other callers that put the
 * clear packets down first. Here the sprites' own depth (0, test ALWAYS, written) is recorded too,
 * as a depth fill, since later 2D of the same list is tested against it. Every other caller (the
 * wipes, the hud's letterbox, the levels' own) draws in a state this file does not prove and keeps
 * its packets (OPENRAC_UI_LOG: `tint ... kept`).
 *
 * OPENRAC_UI_SKIP naming gfx_set_clear_color_packet, OPENRAC_UI_REPLAY and no window keep the
 * game's own. OPENRAC_UI_LOG logs each distinct fade once.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "game_protos.h"
#include "openrac/frame.h"
#include "openrac/game_host.h"
#include "openrac/host_env.h"
#include "rac2.h"

/* OPENRAC_UI_SKIP naming a function keeps its packets, for comparing in one build. */
static int skipped(const char* name) {
    const char* list = openrac_env_string("OPENRAC_UI_SKIP");
    return list != NULL && strstr(list, name) != NULL;
}

enum {
    REGISTER_PACKET = 48u, /* VU1_addGSregister's entry in the list */
    CALL_PACKET = 16u,     /* the DMA call of the clear packet */
    CALL_TAG = 0x30000014u,
    CALL_VIF = 0x50000014u,
    ALPHA_REGISTER = 0x42u,
    RGBAQ_REGISTER = 0x01u,
    ALPHA_SOURCE_OVER = 0x8000000044ull,
    PACKET_TEST = 0x30000u,
    PACKET_PRIM = 0x146u,
    REG_TEST_1 = 0x47u,
    REG_PRIM = 0x00u
};

/* Where the list ended when the last DoGifPaging returned: the tail's ALPHA register begins there. */
static gaddr s_paged_end = UINT32_MAX;

void func_0027C660__game(void);

void func_0027C660(void) {
    func_0027C660__game();
    s_paged_end = GREF(gaddr, RAC2_LIST_CURSOR);
}

/* The A+D register write the game's VU1_addGSregister appended at `at`: its value and register, or
 * 0 when the 48 bytes are not that. */
static int register_packet(gaddr at, uint64_t* value, uint32_t* reg) {
    if (GREF(uint32_t, at) != 0x10000002u || GREF(uint32_t, at + 12u) != 0x50000002u
        || GREF(uint32_t, at + 16u) != 0x8001u || GREF(uint32_t, at + 20u) != 0x10000000u
        || GREF(uint32_t, at + 24u) != 0x0Eu) {
        return 0;
    }
    *value = GREF(uint64_t, at + 32u);
    *reg = GREF(uint32_t, at + 40u);
    return 1;
}

/* The state the game's clear packets leave. gfx_write_clear_color_packets (func_0027B7E0) appends
 * { a call of g_gfx_clear_color_packet (19 quadwords: ALPHA_1 and ALPHA_2 = 0x8000000044, TEST,
 * ...), a call of g_gs_draw_env_regs (FRAME, ZBUF, XYOFFSET, SCISSOR as the frame's) and the
 * register write 0x3D } and nothing else; a tint whose RGBAQ write follows at once draws with
 * source-over alpha, the draw environment's masks (FRAME's FBMSK 0, ZBUF 24-bit and written) and
 * the clear packet's own TEST, PRIM. Read from each program's data, not assumed. `at` is where
 * the RGBAQ write starts. */
static int clear_packets_state(gaddr at) {
    const gaddr reg = at - REGISTER_PACKET;
    const gaddr env_tag = reg - CALL_PACKET;
    const gaddr clear_tag = env_tag - CALL_PACKET;
    uint64_t value;
    uint32_t id;
    if (at < 3u * 16u + REGISTER_PACKET || !register_packet(reg, &value, &id) || id != 0x3Du
        || GREF(uint32_t, env_tag) != 0x3000000Bu || GREF(uint32_t, env_tag + 12u) != 0x5000000Bu
        || GREF(uint32_t, clear_tag) != 0x30000013u
        || GREF(uint32_t, clear_tag + 12u) != 0x50000013u) {
        return 0;
    }
    const gaddr clear = GREF(gaddr, clear_tag + 4u);
    const gaddr env = GREF(gaddr, env_tag + 4u);
    /* ALPHA_1 (0x42) = source over: the second quadword of the packet. */
    if (GREF(uint64_t, clear + 16u) != ALPHA_SOURCE_OVER || GREF(uint64_t, clear + 24u) != 0x42u) {
        return 0;
    }
    /* FRAME_1 with FBMSK 0 (qword 1), ZBUF_1 24-bit and written (qword 3). */
    if (GREF(uint64_t, env + 24u) != 0x4Cu || (GREF(uint64_t, env + 16u) >> 32) != 0u
        || GREF(uint64_t, env + 56u) != 0x4Eu) {
        return 0;
    }
    const uint64_t zbuf = GREF(uint64_t, env + 48u);
    return ((zbuf >> 24) & 0xFu) == 1u && (zbuf >> 32) == 0u;
}

static void log_kept(const char* why, int r, int g, int b, int a) {
    if (openrac_env_string("OPENRAC_UI_LOG") == NULL) {
        return;
    }
    static uint32_t seen[16];
    static int count;
    const uint32_t key =
        ((uint32_t)r << 24) ^ ((uint32_t)g << 16) ^ ((uint32_t)b << 8) ^ (uint32_t)a;
    for (int i = 0; i < count; ++i) {
        if (seen[i] == key) {
            return;
        }
    }
    if (count < 16) {
        seen[count++] = key;
        fprintf(stderr, "screen_fade: tint %d %d %d %d kept (%s)\n", r, g, b, a, why);
    }
}

void func_0027DDC0__game(int r, int g, int b, int a);

void func_0027DDC0(int r, int g, int b, int a) {
    const gaddr cursor = RAC2_LIST_CURSOR;
    const gaddr before = GREF(gaddr, cursor);
    const gaddr paged = s_paged_end;
    func_0027DDC0__game(r, g, b, a);
    if (!openrac_frame_native() || skipped(RAC2_NAME(func_0027DDC0))) {
        return;
    }
    const gaddr after = GREF(gaddr, cursor);
    const gaddr anchor = openrac_frame_ram_cursor(before);
    if (anchor == UINT32_MAX || after != before + REGISTER_PACKET + CALL_PACKET) {
        return;
    }
    uint64_t value;
    uint32_t reg;
    if (!register_packet(before, &value, &reg) || reg != RGBAQ_REGISTER) {
        return;
    }
    const gaddr call = before + REGISTER_PACKET;
    if (GREF(uint32_t, call) != CALL_TAG || GREF(uint32_t, call + 12u) != CALL_VIF) {
        return;
    }
    /* The packet the call names is the one proved: TEST (depth ALWAYS) and PRIM (blended sprite). */
    const gaddr packet = GREF(gaddr, call + 4u);
    if (GREF(uint64_t, packet + 16u) != PACKET_TEST || GREF(uint64_t, packet + 24u) != REG_TEST_1
        || GREF(uint64_t, packet + 32u) != PACKET_PRIM
        || GREF(uint64_t, packet + 40u) != REG_PRIM) {
        return;
    }
    /* The blend: either the frame tail's ALPHA write right after its last paging (the sprites'
     * depth is read by nothing after it), or the clear packets right before the RGBAQ write,
     * which put ALPHA_1 = 0x44 and the frame's draw environment back (clear_packets_state). */
    int depth_write = 0;
    if (paged != UINT32_MAX && before == paged + REGISTER_PACKET) {
        if (!register_packet(paged, &value, &reg) || reg != ALPHA_REGISTER
            || value != ALPHA_SOURCE_OVER) {
            return;
        }
    } else if (clear_packets_state(before)) {
        depth_write = 1;
    } else {
        log_kept("no proved blend", r, g, b, a);
        return;
    }
    /* The colour is the four values the game packed, each a byte. */
    if (((unsigned)r | (unsigned)g | (unsigned)b | (unsigned)a) > 0xFFu) {
        return;
    }
    const uint32_t rgba = (uint32_t)r | (uint32_t)g << 8 | (uint32_t)b << 16 | (uint32_t)a << 24;
    openrac_frame_anchor(anchor);
    if (depth_write) {
        /* The sprites' own depth, 0 written over the whole picture whatever was there. */
        const float all[4] = {0.0f, 0.0f, 512.0f, 448.0f};
        if (!openrac_frame_fill_depth(
                all,
                rgba,
                OPENRAC_BLEND_MIX | OPENRAC_PICTURE_WHOLE,
                0,
                24,
                OPENRAC_DEPTH_ALWAYS,
                OPENRAC_WRITE_RED | OPENRAC_WRITE_GREEN | OPENRAC_WRITE_BLUE | OPENRAC_WRITE_DEPTH
            )) {
            return;
        }
    } else {
        openrac_frame_fill(NULL, rgba, OPENRAC_BLEND_MIX | OPENRAC_PICTURE_WHOLE);
    }
    GREF(gaddr, cursor) = before;
    if (openrac_env_string("OPENRAC_UI_LOG") != NULL) {
        static uint32_t seen[16];
        static int count;
        const uint32_t key = (rgba & 0xFFFFFF00u) + (uint32_t)(a >> 4);
        for (int i = 0; i < count; ++i) {
            if (seen[i] == key) {
                return;
            }
        }
        if (count < 16) {
            seen[count++] = key;
            fprintf(stderr, "screen_fade: tint %d %d %d %d native\n", r, g, b, a);
        }
    }
}
