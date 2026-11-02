/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Ratchet & Clank: the level HUD in a picture wider than the game's.
 *
 * The HUD is 13 slots of 0x90 bytes (D_00199C60), each an element with a place on the game's
 * 512-pixel screen (+0x50 x, +0x54 y), anchor flags (+0x60: bit 0 top, bit 1 bottom, bit 2 left,
 * bit 3 right; neither of a pair is centred) and a draw callback (+0x18), which func_001FFFB8 calls
 * for each slot once a frame. Every element draws from its +0x50: the bolt counter and the
 * nanotech straight from it, the meters through func_L00_00236400, which turns it into a corner by
 * the flags.
 *
 * With the player's HUD setting on "Edges" (openrac_game_hud_side), the elements anchored to the
 * screen's left or right are drawn further out by the width the picture gained on that side, so
 * the weapon, the bolts and the meters sit at the picture's sides as they sat at the television's;
 * centred ones stay in the middle. The place is moved only around the game's own draw and put back
 * after it (by the same amount, so whatever the draw itself did to it stays): nothing else in the
 * game sees another HUD.
 *
 * The weapon's panel is the exception: the game draws its dark strip from its place to the place
 * again plus the frame's width (draw_weapon_ammo_hud_panel's `wd = x + width`), so moved left it
 * came out shorter by the move and its right end twice as far in. That one is drawn at its own
 * place with the whole screen moved instead (the screen's left, which every 2D draw adds), so it
 * moves as one piece.
 *
 * The idea is ReRAC's, which moves the same anchors out by the same flags (crates/rc-game/src/hud.rs,
 * `anchor`, and crates/rc-engine/src/display.rs, `side_extra`; ISC License, Copyright (c) 2026 ReRAC
 * contributors); no code of it is used.
 */
#include "rac1.h"

enum { SLOTS = 13, SLOT_BYTES = 0x90, SLOT_X = 0x50, SLOT_FLAGS = 0x60, ANCHOR_LEFT = 4, ANCHOR_RIGHT = 8 };

/* The screen's left in 16ths of a pixel (screen_extent.left), added to every 2D draw's x. */
#define SCREEN_LEFT GREF(int, RAC1_ADDR(0x0013E610u))

/* How far each slot is moved while func_001FFFB8 draws them, else all 0. */
static int moved[SLOTS];

/* The mods' lines of text (mod interface 1.12, openrac_game_hud_texts), after the game's elements:
 * glyph by glyph from the game's own font table and texture, as save_marks.c draws its badge, so a
 * line can be any size. A glyph is {u, v, top, advance} in the table; the fonts are 16 pixels. */
#define LARGE_FONT RAC1_ADDR(0x001DF3D0u)   /* normal_font_metrics */
#define SMALL_FONT RAC1_ADDR(0x001DF770u)   /* small_font_metrics */
#define SCREEN_LINES RAC1_ADDR(0x0013E604u) /* the screen's height in lines */
#define FONT_PIXELS 16

static void draw_line(
    const openrac_game_hud_text* line, int dx, int dy, unsigned long long colour
) {
    const int small = (line->flags & 1u) != 0;
    const gaddr table = OPENRAC_DATA(small ? SMALL_FONT : LARGE_FONT);
    const unsigned long long texture = func_001F4868(small ? 2 : 1);
    const float scale = line->scale;
    const int size = (int)(FONT_PIXELS * scale + 0.5f);
    int width = 0;
    for (const char* c = line->text; *c != '\0'; ++c) {
        if (*c >= 32 && *c <= 126) {
            width += (int)(GREF(int8_t, table + 4u * (unsigned char)*c + 3u) * scale + 0.5f);
        }
    }
    const int lines = GREF(int32_t, OPENRAC_DATA(SCREEN_LINES));
    int x = (int)(line->x * 512.0f) - width / 2 + dx;
    const int y = (int)(line->y * (float)(lines > 0 ? lines : 448)) - size / 2 + dy;
    for (const char* c = line->text; *c != '\0'; ++c) {
        if (*c < 32 || *c > 126) {
            continue;
        }
        /* As the game's own print does: no glyph without an advance, and the space only moves on. */
        const gaddr glyph = table + 4u * (unsigned char)*c;
        if (GREF(int8_t, glyph + 3u) == 0) {
            continue;
        }
        if (*c != ' ') {
            func_001F5800(
                x,
                y + (int)(GREF(int8_t, glyph + 2u) * scale),
                size,
                size,
                GREF(uint8_t, glyph),
                GREF(uint8_t, glyph + 1u),
                FONT_PIXELS,
                FONT_PIXELS,
                colour,
                texture
            );
        }
        x += (int)(GREF(int8_t, glyph + 3u) * scale + 0.5f);
    }
}

/* The GS's colour (0xAABBGGRR): the colour channels as they come (128 the font's own shade, 255
 * about twice it, as the game's own HUD colours go), the opacity's 0 to 255 to its 0 to 128. */
static unsigned long long gs_colour(uint32_t rgba) {
    const uint32_t alpha = (((rgba >> 24) & 0xFFu) * 128u + 127u) / 255u;
    return (unsigned long long)(((rgba << 8) >> 8) | (alpha << 24));
}

static void draw_mod_texts(void) {
    openrac_game_hud_text lines[16];
    int n = openrac_game_hud_texts(lines, 16);
    n = n < 16 ? n : 16;
    for (int i = 0; i < n; i++) {
        const uint32_t alpha = lines[i].rgba & 0xFF000000u;
        if ((lines[i].flags & 2u) != 0) {
            draw_line(&lines[i], 1, 1, gs_colour(alpha));
        }
        draw_line(&lines[i], 0, 0, gs_colour(lines[i].rgba));
    }
}

void func_001FFFB8(void) {
    const int side = openrac_game_hud_side();
    const gaddr slots = OPENRAC_DATA(RAC1_ADDR(0x00199C60u));
    for (int i = 0; i < SLOTS; i++) {
        const gaddr slot = slots + (gaddr)i * SLOT_BYTES;
        const int flags = GREF(int, slot + SLOT_FLAGS);
        moved[i] = side == 0 ? 0 : (flags & ANCHOR_LEFT) != 0 ? -side : (flags & ANCHOR_RIGHT) != 0 ? side : 0;
        GREF(int, slot + SLOT_X) += moved[i];
    }
    func_001FFFB8__game();
    for (int i = 0; i < SLOTS; i++) {
        GREF(int, slots + (gaddr)i * SLOT_BYTES + SLOT_X) -= moved[i];
        moved[i] = 0;
    }
    draw_mod_texts();
}

/* draw_weapon_ammo_hud_panel(element): put back at its own place, with the screen moved instead. */
int func_L00_0023B140(gaddr element) {
    const gaddr slots = OPENRAC_DATA(RAC1_ADDR(0x00199C60u));
    const gaddr offset = element - slots;
    const int by =
        offset < SLOTS * SLOT_BYTES && offset % SLOT_BYTES == 0 ? moved[offset / SLOT_BYTES] : 0;
    if (by == 0) {
        return func_L00_0023B140__game(element);
    }
    GREF(int, element + SLOT_X) -= by;
    SCREEN_LEFT += by * 16;
    const int result = func_L00_0023B140__game(element);
    SCREEN_LEFT -= by * 16;
    GREF(int, element + SLOT_X) += by;
    return result;
}
