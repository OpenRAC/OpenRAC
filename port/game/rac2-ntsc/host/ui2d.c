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
 * The rest of the 2D layer's routines go the same way: the HUD's gradient sprite (func_0028EA58, a
 * colour a corner, shaded), the turned sprite (func_0027E4A8), the ring segments (func_0027E920, a
 * colour a corner unshaded, and the texture's wrap) and the pages' boxes, of flat colour
 * (func_0027DE50) or shaded across (func_0027EA28). What is still sent as the game's packets is drawn at the frame's resolution all
 * the same (the chip's coordinates are scaled to the frame); OPENRAC_UI_RESIDUAL lists it.
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

static void hud_rect(
    openrac_game_ui_rect* q, int x0, int y0, int x1, int y1, int u0, int v0, int u1, int v1
) {
    openrac_ui_rect(
        q, to_screen_x(x0), to_screen_y(y0), to_screen_x(x1), to_screen_y(y1), u0, v0, u1, v1
    );
}

/* The font's two prints: a string, glyph by glyph (func_0027E018; func_0027E1A0 for the scaled). */
void func_0027F2A8(
    int x,
    int y,
    unsigned long long colour,
    gaddr text,
    int limit,
    unsigned long long texture,
    gaddr glyphs
) {
    ++g_glyphs;
    func_0027F2A8__game(x, y, colour, text, limit, texture, glyphs);
    --g_glyphs;
}

void func_0027F640(
    float x,
    float y,
    unsigned long long colour,
    gaddr text,
    int limit,
    float scale,
    unsigned long long texture,
    gaddr glyphs
) {
    ++g_glyphs;
    func_0027F640__game(x, y, colour, text, limit, scale, texture, glyphs);
    --g_glyphs;
}

/* DrawGlyphQuad: a w x h screen rectangle at (x, y) textured from (u, v) .. (u + uw, v + vh) of the
 * texture tex (TEX0) in colour rgba. */
void func_0027E018(
    int x,
    int y,
    int w,
    int h,
    int u,
    int v,
    int uw,
    int vh,
    unsigned long long rgba,
    unsigned long long tex
) {
    openrac_game_ui_rect q;
    memset(&q, 0, sizeof(q));
    hud_rect(
        &q, x * 16, y * 16, (x + w) * 16, (y + h) * 16, u * 16, v * 16, (u + uw) * 16, (v + vh) * 16
    );
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
void func_0027E1A0(
    float x,
    float y,
    float w,
    float h,
    int u,
    int v,
    int uw,
    int vh,
    gaddr colours,
    unsigned long long tex
) {
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
void func_0028F278(
    long long tex0, int x, int y, int lw, int lh, int w, int h, int u, int v, int alpha
) {
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
void func_0028F788(
    int tex, int x0, int y0, int x1, int y1, int u0, int v0, int u1, int v1, int alpha
) {
    openrac_game_ui_rect q;
    hud_common(&q, func_0028E640(tex), alpha);
    hud_rect(&q, x0, y0, x1, y1, u0, v0, u1, v1);
    if (!openrac_ui_send(&q, LIST_CURSOR)) {
        func_0028F788__game(tex, x0, y0, x1, y1, u0, v0, u1, v1, alpha);
    }
}

/* Corners from the chip's XYZ2 words (12.4 x, y; z in the high word) into a record. */
static void corners_from_xyz(openrac_game_ui_rect* q, const gaddr xyz) {
    for (int i = 0; i < 4; ++i) {
        const uint64_t w = GREF(uint64_t, xyz + 8u * (gaddr)i);
        q->x[i] = (uint32_t)(w & 0xFFFFu);
        q->y[i] = (uint32_t)((w >> 16) & 0xFFFFu);
    }
    q->z = (uint32_t)(GREF(uint64_t, xyz) >> 32);
}

/* A HUD texture as a sprite with a colour for each corner (colours: four RGBA words), shaded
 * between them: the HUD's gradient bars. Returns the display list's cursor, as the game's does. */
int func_0028EA58(int tex, int x, int y, int w, int h, gaddr colours) {
    int tw, th;
    openrac_game_ui_rect q;
    hud_size(tex, &tw, &th);
    hud_common(&q, func_0028E640(tex), 0x80);
    q.flags = 4u | 32u;
    hud_rect(&q, x * 16, y * 16, (x + w) * 16, (y + h) * 16, 0, 0, tw * 16, th * 16);
    for (int i = 0; i < 4; ++i) {
        q.corners[i] = GREF(uint32_t, colours + 4u * (gaddr)i);
    }
    if (!openrac_ui_send(&q, LIST_CURSOR)) {
        return func_0028EA58__game(tex, x, y, w, h, colours);
    }
    return (int)GREF(gaddr, LIST_CURSOR);
}

/* A quad of tex0 centred on (cx, cy), w by h, turned by angle about the pivot (pu, pv: 0 to 1 of
 * the quad), its texture's tw x th texels mirrored by flip_u, flip_v, in one colour at depth z: the
 * corners as the game works them out (centre +- the turned half edges, in its order of sums). */
void func_0027E4A8(float cx, float cy, float w, float h, float angle, int tw, int th, long long tex0,
                   long long z, int colour, unsigned char flip_u, unsigned char flip_v, float pu, float pv) {
    const float vx = h * func_00283198(angle), vy = h * func_00283180(angle);
    const float hx = w * func_00283180(angle), hy = -w * func_00283198(angle);
    const float iu = 1.0f - pu, iv = 1.0f - pv;
    const float px[4] = {(cx + vx * iv) - hx * iu, (cx + vx * iv) + hx * pu, (cx - vx * pv) - hx * iu,
                         (cx - vx * pv) + hx * pu};
    const float py[4] = {(cy + vy * iv) - hy * iu, (cy + vy * iv) + hy * pu, (cy - vy * pv) - hy * iu,
                         (cy - vy * pv) + hy * pu};
    const int left = flip_u ? tw << 4 : 0x10, right = flip_u ? 0x10 : tw << 4;
    const int top = flip_v ? th << 4 : 0x10, bottom = flip_v ? 0x10 : th << 4;
    const int us[4] = {left, right, left, right}, vs[4] = {top, top, bottom, bottom};
    openrac_game_ui_rect q;
    memset(&q, 0, sizeof(q));
    for (int i = 0; i < 4; ++i) {
        q.x[i] = (uint32_t)to_screen_x(func_00283CF0(px[i] * 16.0f)) & 0xFFFFu;
        q.y[i] = (uint32_t)to_screen_y(func_00283CF0(py[i] * 16.0f)) & 0xFFFFu;
        q.u[i] = (uint32_t)us[i];
        q.v[i] = (uint32_t)vs[i];
    }
    q.z = (uint32_t)z;
    q.rgba = (uint32_t)colour;
    q.tex0 = (unsigned long long)tex0;
    if (!openrac_ui_send(&q, LIST_CURSOR)) {
        func_0027E4A8__game(cx, cy, w, h, angle, tw, th, tex0, z, colour, flip_u, flip_v, pu, pv);
    }
}

/* A segment of a ring (func_0027EE58's): four XYZ2 words, UV words and colours (each triangle in
 * its last corner's colour: the packet's PRIM is unshaded), the texture, and its wrap, clamped
 * when `clamped` (CLAMP_1 5) else repeated (0), which stays set after it as the packet leaves it. */
void func_0027E920(gaddr xyz, gaddr uv, gaddr colours, unsigned long long tex0, int clamped) {
    openrac_game_ui_rect q;
    memset(&q, 0, sizeof(q));
    corners_from_xyz(&q, xyz);
    for (int i = 0; i < 4; ++i) {
        const uint32_t t = GREF(uint32_t, uv + 4u * (gaddr)i);
        q.u[i] = t & 0x3FFFu;
        q.v[i] = (t >> 16) & 0x3FFFu;
        q.corners[i] = GREF(uint32_t, colours + 4u * (gaddr)i);
    }
    q.tex0 = tex0;
    q.flags = 4u | 16u;
    q.clamp = clamped ? 5u : 0u;
    if (!openrac_ui_send(&q, LIST_CURSOR)) {
        func_0027E920__game(xyz, uv, colours, tex0, clamped);
    }
}

/* A box of flat colour, rows top to bottom and columns left to right in screen pixels (the
 * pages' frames and boxes), at the far depth. */
void func_0027DE50(int top, int bottom, int left, int right, unsigned long long colour) {
    openrac_game_ui_rect q;
    memset(&q, 0, sizeof(q));
    openrac_ui_rect(&q, to_screen_x(left * 16), to_screen_y(top * 16), to_screen_x(right * 16),
                    to_screen_y(bottom * 16), 0, 0, 0, 0);
    q.z = 0xFFFFF0u;
    q.rgba = (uint32_t)colour;
    q.flags = 8u;
    if (!openrac_ui_send(&q, LIST_CURSOR)) {
        func_0027DE50__game(top, bottom, left, right, colour);
    }
}

/* A box shaded from one colour on its left to another on its right (func_0027EAE8's, the save and
 * menu pages' bars): four XYZ2 words, sent in the strip order 0, 2, 1, 3, and two colours, the
 * first for corners 0 and 2, untextured. */
void func_0027EA28(gaddr xyz, gaddr colours) {
    openrac_game_ui_rect q;
    memset(&q, 0, sizeof(q));
    static const int order[4] = {0, 2, 1, 3};
    for (int i = 0; i < 4; ++i) {
        const uint64_t w = GREF(uint64_t, xyz + 8u * (gaddr)order[i]);
        q.x[i] = (uint32_t)(w & 0xFFFFu);
        q.y[i] = (uint32_t)((w >> 16) & 0xFFFFu);
        q.corners[i] = GREF(uint32_t, colours + 4u * (i < 2 ? 0u : 1u));
    }
    q.z = (uint32_t)(GREF(uint64_t, xyz) >> 32);
    q.flags = 4u | 8u | 32u;
    if (!openrac_ui_send(&q, LIST_CURSOR)) {
        func_0027EA28__game(xyz, colours);
    }
}
