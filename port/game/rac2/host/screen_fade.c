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
 * in the colour is recorded at the cursor they began at. Every other caller (the pause menu's
 * dimming, the wipes, the splash, the letterbox of the hud, the levels' own) draws in a state this
 * file does not prove and keeps its packets.
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
    /* Nothing but the ALPHA write of this tail's between the frame's last paging and the fade. */
    if (anchor == UINT32_MAX || paged == UINT32_MAX || before != paged + REGISTER_PACKET
        || after != before + REGISTER_PACKET + CALL_PACKET) {
        return;
    }
    uint64_t value;
    uint32_t reg;
    if (!register_packet(paged, &value, &reg) || reg != ALPHA_REGISTER
        || value != ALPHA_SOURCE_OVER) {
        return;
    }
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
    /* The colour is the four values the game packed, each a byte. */
    if (((unsigned)r | (unsigned)g | (unsigned)b | (unsigned)a) > 0xFFu) {
        return;
    }
    const uint32_t rgba = (uint32_t)r | (uint32_t)g << 8 | (uint32_t)b << 16 | (uint32_t)a << 24;
    openrac_frame_anchor(anchor);
    openrac_frame_fill(NULL, rgba, OPENRAC_BLEND_MIX | OPENRAC_PICTURE_WHOLE);
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
