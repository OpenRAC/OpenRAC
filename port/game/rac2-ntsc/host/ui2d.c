/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Going Commando: the 2D layer's draw routines, wrapped (libraries.tsv, port = wrap) as Ratchet &
 * Clank's are (rac1/host/ui2d.c): each routine's quad goes to the window as a record, marked as a
 * glyph of text inside the font's prints, so text is drawn from the font's distance field, sharp at
 * any size (OPENRAC_UI_REPLAY replays the packets, for comparing). The routines were paired with
 * Ratchet & Clank's instruction by instruction; the differences are the addresses, the HUD's depth
 * being a word the HUD's update sets, and a second glyph routine with float corners and a colour
 * for each (func_0027E1A0), which the HUD's scaled counters and the GUI's text elements use.
 *
 * What still goes through the game's packets draws at the frame's resolution all the same: the
 * chip's coordinates are scaled to the frame. OPENRAC_UI_RESIDUAL lists what that is. Left there:
 * the gradient sprite (func_0028EA58: a colour a corner, which a record has not got), the rotated
 * sprite with its own texture window (func_0027E4A8), the ring segments (func_0027E920, in colours)
 * and the pause pages' boxes, which are strips of flat colour. None of them is text.
 */
#include <stdint.h>
#include <string.h>

#include "../../common/lib/ui_quad.h"
#include "game_protos.h"
#include "openrac/game_host.h"

/* The offset the game adds to every 2D position (g_gsPixelOffsetX, Y), shared by every program. */
#define SCREEN_OFX GREF(int, 0x001A7350u)
#define SCREEN_OFY GREF(int, 0x001A7354u)
/* The display list's cursor, the HUD's depth, and its texture tables: by a HUD texture's number
 * (g_hudIconMap) its palette's and its pixels' entries, each 8 bytes (g_hudTextureSlots: +6 and +7
 * the log2 of the size). The loaded program's copies. */
#define LIST_CURSOR OPENRAC_DATA(0x001B22A8u)
#define HUD_DEPTH GREF(int, OPENRAC_DATA(0x001B187Cu))
#define HUD_ICON_MAP GREF(gaddr, OPENRAC_DATA(0x001B1890u))
#define HUD_TEXTURES GREF(gaddr, OPENRAC_DATA(0x001B1894u))

/* Inside a font print: the quads drawn are glyphs of text. */
static int g_glyphs = 0;

static int to_screen_x(int sixteenths) {
    return sixteenths + SCREEN_OFX - 8;
}

static int to_screen_y(int sixteenths) {
    return sixteenths + SCREEN_OFY - 8;
}

/* A HUD texture's size: 1 << its entry's +6/+7 log2 sizes. */
static void hud_size(int tex, int* tw, int* th) {
    const gaddr pair = HUD_ICON_MAP + (gaddr)tex * 4;
    const gaddr e = HUD_TEXTURES + (gaddr)GREF(int16_t, pair + 2) * 8;
    *tw = 1 << GREF(uint8_t, e + 6);
    *th = 1 << GREF(uint8_t, e + 7);
}

static void hud_common(openrac_game_ui_rect* q, unsigned long long tex0, int alpha) {
    memset(q, 0, sizeof(*q));
    q->flags = 2; /* these routines send sprites */
    q->z = (uint32_t)HUD_DEPTH;
    q->rgba = ((uint32_t)alpha << 24) | 0x7F7F7Fu;
    q->tex0 = tex0;
}

static void hud_rect(openrac_game_ui_rect* q, int x0, int y0, int x1, int y1, int u0, int v0, int u1, int v1) {
    openrac_ui_rect(q, to_screen_x(x0), to_screen_y(y0), to_screen_x(x1), to_screen_y(y1), u0, v0, u1, v1);
}

/* The font's two prints: a string, glyph by glyph (func_0027E018; func_0027E1A0 for the scaled). */
void func_0027F2A8(int x, int y, unsigned long long colour, gaddr text, int limit, unsigned long long texture,
                   gaddr glyphs) {
    ++g_glyphs;
    func_0027F2A8__game(x, y, colour, text, limit, texture, glyphs);
    --g_glyphs;
}

void func_0027F640(float x, float y, unsigned long long colour, gaddr text, int limit, float scale,
                   unsigned long long texture, gaddr glyphs) {
    ++g_glyphs;
    func_0027F640__game(x, y, colour, text, limit, scale, texture, glyphs);
    --g_glyphs;
}

/* DrawGlyphQuad: a w x h screen rectangle at (x, y) textured from (u, v) .. (u + uw, v + vh) of the
 * texture tex (TEX0) in colour rgba. */
void func_0027E018(int x, int y, int w, int h, int u, int v, int uw, int vh, unsigned long long rgba,
                   unsigned long long tex) {
    openrac_game_ui_rect q;
    memset(&q, 0, sizeof(q));
    hud_rect(&q, x * 16, y * 16, (x + w) * 16, (y + h) * 16, u * 16, v * 16, (u + uw) * 16, (v + vh) * 16);
    q.z = 0xFFFFF0u;
    q.rgba = (uint32_t)rgba;
    q.tex0 = tex;
    q.flags = g_glyphs > 0 ? 1u : 0u;
    if (!openrac_ui_send(&q, LIST_CURSOR)) {
        func_0027E018__game(x, y, w, h, u, v, uw, vh, rgba, tex);
    }
}

/* DrawTexturedQuad2d: func_0027E018 with float corners and a colour for each corner, which leaves
 * out a quad off the chip's 4096 x 4096 coordinate window's used part as the game does. Only a
 * glyph comes here natively: elsewhere it may be a gradient, and the game's packet holds the
 * texels to the quad (a region CLAMP), which a record does not. */
void func_0027E1A0(float x, float y, float w, float h, int u, int v, int uw, int vh, gaddr colours,
                   unsigned long long tex) {
    const int same = g_glyphs > 0 && GREF(uint32_t, colours + 4) == GREF(uint32_t, colours)
                     && GREF(uint32_t, colours + 8) == GREF(uint32_t, colours)
                     && GREF(uint32_t, colours + 12) == GREF(uint32_t, colours);
    if (!same) {
        func_0027E1A0__game(x, y, w, h, u, v, uw, vh, colours, tex);
        return;
    }
    const int x0 = to_screen_x(func_00283CF0(x * 16.0f));
    const int x1 = to_screen_x(func_00283CF0((x + w) * 16.0f));
    const int y0 = to_screen_y(func_00283CF0(y * 16.0f));
    const int y1 = to_screen_y(func_00283CF0((y + h) * 16.0f));
    if (x0 > 0x9000 || x1 < 0x7000 || y0 > 0x9000 || y1 < 0x7000) {
        return;
    }
    openrac_game_ui_rect q;
    memset(&q, 0, sizeof(q));
    openrac_ui_rect(&q, x0, y0, x1, y1, u * 16, v * 16, (u + uw) * 16, (v + vh) * 16);
    q.z = 0xFFFFF0u;
    q.rgba = GREF(uint32_t, colours);
    q.tex0 = tex;
    q.flags = 1u;
    if (!openrac_ui_send(&q, LIST_CURSOR)) {
        func_0027E1A0__game(x, y, w, h, u, v, uw, vh, colours, tex);
    }
}

/* A HUD texture as a sprite at (x, y), w x h pixels, its whole texture, with alpha. */
void func_0028E868(int tex, int x, int y, int w, int h, int alpha) {
    int tw, th;
    openrac_game_ui_rect q;
    hud_size(tex, &tw, &th);
    hud_common(&q, func_0028E640(tex), alpha);
    hud_rect(&q, x * 16, y * 16, (x + w) * 16, (y + h) * 16, 0, 0, tw * 16, th * 16);
    if (!openrac_ui_send(&q, LIST_CURSOR)) {
        func_0028E868__game(tex, x, y, w, h, alpha);
    }
}

/* func_0028E868 with one texel a pixel, whatever the texture's size: a bar cut to a length. */
void func_0028ECD8(int tex, int x, int y, int w, int h, int alpha) {
    openrac_game_ui_rect q;
    hud_common(&q, func_0028E640(tex), alpha);
    hud_rect(&q, x * 16, y * 16, (x + w) * 16, (y + h) * 16, 0, 0, w * 16, h * 16);
    if (!openrac_ui_send(&q, LIST_CURSOR)) {
        func_0028ECD8__game(tex, x, y, w, h, alpha);
    }
}

/* func_0028E868 with the position and size in 16ths of a pixel. */
void func_0028EE98(int tex, int x, int y, int w, int h, int alpha) {
    int tw, th;
    openrac_game_ui_rect q;
    hud_size(tex, &tw, &th);
    hud_common(&q, func_0028E640(tex), alpha);
    hud_rect(&q, x, y, x + w, y + h, 0, 0, tw * 16, th * 16);
    if (!openrac_ui_send(&q, LIST_CURSOR)) {
        func_0028EE98__game(tex, x, y, w, h, alpha);
    }
}

/* func_0028EE98 in a colour. */
void func_0028F078(int tex, int x, int y, int w, int h, int alpha, unsigned int colour) {
    int tw, th;
    openrac_game_ui_rect q;
    hud_size(tex, &tw, &th);
    hud_common(&q, func_0028E640(tex), alpha);
    q.rgba = ((uint32_t)alpha << 24) | (colour & 0xFFFFFFu);
    hud_rect(&q, x, y, x + w, y + h, 0, 0, tw * 16, th * 16);
    if (!openrac_ui_send(&q, LIST_CURSOR)) {
        func_0028F078__game(tex, x, y, w, h, alpha, colour);
    }
}

/* A sprite of the texture tex0 at (x, y), w x h in 16ths of a pixel, texels from (u, v) on by
 * (1 << (lw + 4), 1 << (lh + 4)), with alpha. */
void func_0028F278(long long tex0, int x, int y, int lw, int lh, int w, int h, int u, int v, int alpha) {
    openrac_game_ui_rect q;
    hud_common(&q, (unsigned long long)tex0, alpha);
    hud_rect(&q, x, y, x + w, y + h, u, v, u + (1 << (lw + 4)), v + (1 << (lh + 4)));
    if (!openrac_ui_send(&q, LIST_CURSOR)) {
        func_0028F278__game(tex0, x, y, lw, lh, w, h, u, v, alpha);
    }
}

/* A w x h sprite of tex at (x, y) (16ths of a pixel) turned by rot: corners pos +- a +- b with
 * a = h (sin, cos), b = w (-cos, sin), texels from (0, 0) to (u, v) texels. */
void func_0028F410(int u, int v, long long tex, float x, float y, float w, float h, float rot) {
    openrac_game_ui_rect q;
    hud_common(&q, (unsigned long long)tex, 0x80);
    q.flags = 0;
    q.rgba = 0x807F7F7Fu;
    const float s = func_00283198(rot), c = func_00283180(rot);
    const float ax = h * s, ay = h * c, bx = -w * c, by = w * s;
    const float cx[4] = {x + ax - bx, x + ax + bx, x - ax - bx, x - ax + bx};
    const float cy[4] = {y + ay - by, y + ay + by, y - ay - by, y - ay + by};
    const int us[4] = {u * 16, u * 16, 0, 0}, vs[4] = {0, v * 16, 0, v * 16};
    for (int i = 0; i < 4; ++i) {
        q.x[i] = (uint32_t)to_screen_x((int)cx[i]);
        q.y[i] = (uint32_t)to_screen_y((int)cy[i]);
        q.u[i] = (uint32_t)us[i];
        q.v[i] = (uint32_t)vs[i];
    }
    if (!openrac_ui_send(&q, LIST_CURSOR)) {
        func_0028F410__game(u, v, tex, x, y, w, h, rot);
    }
}

/* A HUD texture as a sprite from (x0, y0) to (x1, y1) (16ths of a pixel), texels (u0, v0) to
 * (u1, v1) (12.4), with alpha. */
void func_0028F788(int tex, int x0, int y0, int x1, int y1, int u0, int v0, int u1, int v1, int alpha) {
    openrac_game_ui_rect q;
    hud_common(&q, func_0028E640(tex), alpha);
    hud_rect(&q, x0, y0, x1, y1, u0, v0, u1, v1);
    if (!openrac_ui_send(&q, LIST_CURSOR)) {
        func_0028F788__game(tex, x0, y0, x1, y1, u0, v0, u1, v1, alpha);
    }
}
