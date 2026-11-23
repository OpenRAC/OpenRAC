/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal's screen effects (Going Commando has none), drawn by the window as native post
 * passes through the frame recorder (openrac_frame_screen_effect, docs/port/SCREEN_EFFECTS.md).
 *
 * The screen glow: DrawFrame (Veldin's 0x00403440), while the bits at 0x001DBCA0 are set (it clears
 * them as the frame starts; the sky's untextured shells and some mobys' draws set them), calls
 * 0x004DE670 and 0x004DE6F0 after its passes. On the console they append two GS packets the level
 * built once at its start (0x004DDE98 and 0x004DE5B0, the catalogue's rows func_003D7DE8 and
 * func_003D8500): the frame copied into a 256 x 256 buffer through an alpha test (the frame's
 * alpha times the tint's above 0x40), each channel times the tint (the four words after the glow
 * word, 0x80 = 1.0), shrunk to 128 and 64 and blurred at each size (the 256 one twice more when
 * the byte at the glow word + 0x24 is 1), made large again, blurred, and added back over the frame
 * by the word at + 0x14 (ALPHA Cs * FIX + Cd). Here the window does the same with its own passes
 * from those values; no packet is sent, and the glow word (0x6000 for the frame, the room the glow
 * takes in the chip's memory, which GetEffectTex's allocator reads) is set and cleared as before.
 *
 * The second effect (0x004DF310, while 0x001DBCBC is set, which DrawFrame clears after; 0x004D94C0
 * sets it): two more packets (0x004DF070 and 0x004DF1B0, rows func_003D8FC0 and func_003D9100):
 * the frame copied at 256 x 256, blurred twice, and mixed back over the frame by an alpha the
 * game makes from the depth buffer (the depth read as a texture, against the depth 0xFF82FF). The
 * window mixes the blurred copy over the whole frame: it has no depth in the chip's units at that
 * point of the frame.
 *
 * Each copy reads its own globals: the single-player levels' here, level 39's and the maps' (mp.c,
 * mp_maps.c) through the same calls. */
#include <stdint.h>

#include "openrac/frame.h"
#include "openrac/guest.h"
#include "rac3_screen.h"
#include "rac3_version.h"

/* The glow's packet tests the copy's alpha against this (TEST 0x3040D: GREATER, AREF 0x40). */
#define GLOW_THRESHOLD 0x40u
/* The game's 1.0. */
#define FULL 0x80u
/* After the glow word: the tint's red, green, blue and alpha words, then the add-back weight. */
#define TINT_OFFSET 0x04u
#define ADD_OFFSET 0x14u
/* The value the glow word holds for the frame: the chip's memory the glow's buffers take. */
#define GLOW_ROOM 0x6000

/* The display list's write pointer, as the place the recorded effect goes among what the list
 * still draws (openrac/frame.h, docs/port/FRAME_RECORDER.md section 4). */
static void anchor_at(gaddr pointer) {
    const gaddr at = GREF(gaddr, pointer);
    openrac_frame_anchor(at < 0x02000000u ? at : 0);
}

/* A byte of the game's 0..255 from a word it keeps the value in. */
static uint32_t byte_of(gaddr at) {
    const uint32_t v = GREF(uint32_t, at);
    return v > 0xFFu ? 0xFFu : v;
}

int rac3_screen_glow(const Rac3Glow* glow) {
    GREF(int32_t, glow->word) = GLOW_ROOM;
    const gaddr tint = glow->word + TINT_OFFSET;
    openrac_frame_screen_effect effect = {0};
    effect.kind = OPENRAC_SCREEN_GLOW;
    effect.tint = byte_of(tint) | byte_of(tint + 4u) << 8 | byte_of(tint + 8u) << 16
                  | byte_of(tint + 12u) << 24;
    effect.threshold = GLOW_THRESHOLD;
    effect.strength = byte_of(glow->word + ADD_OFFSET);
    effect.blur = GREF(uint8_t, glow->twice) == 1 ? 1u : 0u;
    anchor_at(glow->list);
    openrac_frame_screen_effect(&effect);
    return 0;
}

int rac3_screen_glow_end(const Rac3Glow* glow) {
    GREF(int32_t, glow->word) = 0;
    return 0;
}

int rac3_screen_soften(gaddr list) {
    openrac_frame_screen_effect effect = {0};
    effect.kind = OPENRAC_SCREEN_SOFTEN;
    effect.tint = FULL | FULL << 8 | FULL << 16 | FULL << 24;
    effect.strength = FULL;
    anchor_at(list);
    openrac_frame_screen_effect(&effect);
    return 0;
}

/* Veldin's (the single-player levels') globals: the glow word at $gp - 0xC0C, the blur byte after
 * the second effect's word, the list pointer DrawTfrag reads. */
static const Rac3Glow kLevels = {
    RAC3_LADDR(1, 0x001DBCA4u), RAC3_LADDR(1, 0x001DBCC8u), RAC3_LADDR(1, 0x001E1068u)
};

static Rac3Glow levels_glow(void) {
    const Rac3Glow g = {
        OPENRAC_LDATA(1, kLevels.word), OPENRAC_LDATA(1, kLevels.twice),
        OPENRAC_LDATA(1, kLevels.list)
    };
    return g;
}

int func_L01_004DE670(void) {
    const Rac3Glow g = levels_glow();
    return rac3_screen_glow(&g);
}

int func_L01_004DE6F0(void) {
    const Rac3Glow g = levels_glow();
    return rac3_screen_glow_end(&g);
}

int func_L01_004DF310(void) {
    return rac3_screen_soften(OPENRAC_LDATA(1, kLevels.list));
}

/* The boot program's and frontbin's copies (the catalogue's func_003D85C0, func_003D8640 and
 * func_003D9260 are frontbin's; the boot program's own are 0x003D8978, 0x003D89F8, 0x003D9618),
 * run by their DrawFrames (boot 0x003BCEB0 and the loading screen's 0x00388ED8, frontbin 0x003BCAF8
 * and 0x00388AF8) while 0x001D9348 is set. Both programs keep the glow's globals at the same
 * addresses, not relocated: the glow word 0x001D934C, the blur byte 0x20 after it, the list pointer
 * 0x001DA050. */
static const Rac3Glow kBoot = {
    RAC3_ADDR(0x001D934Cu), RAC3_ADDR(0x001D936Cu), RAC3_ADDR(0x001DA050u)
};

int func_003D8978(void) {
    return rac3_screen_glow(&kBoot);
}

int func_003D89F8(void) {
    return rac3_screen_glow_end(&kBoot);
}

int func_003D9618(void) {
    return rac3_screen_soften(kBoot.list);
}

int func_003D85C0__frontbin(void) {
    return rac3_screen_glow(&kBoot);
}

int func_003D8640__frontbin(void) {
    return rac3_screen_glow_end(&kBoot);
}

int func_003D9260__frontbin(void) {
    return rac3_screen_soften(kBoot.list);
}
