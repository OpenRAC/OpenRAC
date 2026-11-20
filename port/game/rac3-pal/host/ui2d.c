/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Up Your Arsenal: the 2D layer's draw routines, wrapped (libraries.tsv, port = wrap) as Going
 * Commando's are (port/game/rac2/host/ui2d.c) and Ratchet & Clank's (rac1/host/ui2d.c): each
 * routine's quad goes to the window as a record, marked as a glyph of text inside the font's
 * prints, so text is drawn from the font's distance field, sharp at any size (OPENRAC_UI_REPLAY
 * replays the game's packets, for comparing).
 *
 * The routines were paired with Going Commando's instruction by instruction (their opcode
 * sequences against the boot program's functions): the flat box (func_0038B310, rac2's
 * func_0027DE50), the glyph quad (func_0038B8B0, func_0027E018), the turned sprite
 * (func_0038C0B8, func_0027E4A8), the ring segment (func_0038C518, func_0027E920) and the shaded
 * box (func_0038C628, func_0027EA28) are the same routines at other addresses. Up Your Arsenal's
 * font builds its glyphs with routines of its own, float corners and the glyph's cell as a region
 * clamp: one colour (func_0038BA50), a colour a corner (func_0038BC30), and a glyph with its drop
 * shadow (func_0038BE10); func_0038B500 is a turned glyph quad. Its print (func_0038EBC8, which
 * the others call) marks what it draws as glyphs; func_0038F458's, a colour a corner, is not
 * wrapped yet (its prototype is unknown), and its glyphs stay the game's packets.
 *
 * The menus' frames and art are Widget3D sprites: a small animated mesh of quads that
 * func_003AA780 poses (its bones through func_003AA168, its vertices to the scratchpad through
 * func_003A08E8 or func_003A0A28) and sends one quad at a time, a GIF tag, TEX0, PRIM and four
 * corners (RGBAQ, ST, XYZ2) each. Its wrapper takes those quads, which it alone builds, as records
 * in their place.
 */
#include <stdint.h>
#include <string.h>

#include "../../common/lib/ui_quad.h"
#include "game_protos.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"

/* The offset the game adds to every 2D position (12.4), and the display list's cursor. */
#define SCREEN_OFX GREF(int32_t, 0x001D4BD0u)
#define SCREEN_OFY GREF(int32_t, 0x001D4BD4u)
#define LIST_CURSOR ((gaddr)0x001DA050u)
/* The far depth the 2D layer's boxes and glyph quads are drawn at (0xFFFFF0 in XYZ2's z). */
#define FAR_Z 0xFFFFF0u

/* Inside a font print: the quads drawn are glyphs of text. */
static int g_glyphs = 0;

static int to_screen_x(int sixteenths) {
    return sixteenths + SCREEN_OFX - 8;
}

static int to_screen_y(int sixteenths) {
    return sixteenths + SCREEN_OFY - 8;
}

/* A glyph's screen corners as the font's builders work them out: the left and top edges at
 * x * 16 + offset - 8, the right and bottom ones a sixteenth less; with snap, on pixel centres.
 * False when the glyph is off the chip's used 4096 x 4096 window, which the game leaves out. */
static int glyph_corners(float x, float y, float w, float h, int snap, int out[4]) {
    int x0 = (int)(x * 16.0f) + SCREEN_OFX - 8;
    int x1 = (int)((x + w) * 16.0f) + SCREEN_OFX - 9;
    int y0 = (int)(y * 16.0f) + SCREEN_OFY - 8;
    int y1 = (int)((y + h) * 16.0f) + SCREEN_OFY - 9;
    if (x0 > 0x9000 || x1 < 0x7000 || y0 > 0x9000 || y1 < 0x7000) {
        return 0;
    }
    if (snap) {
        x0 = (x0 & 0xFFF0) + 8;
        x1 = (x1 & 0xFFF0) + 8;
        y0 = (y0 & 0xFFF0) + 8;
        y1 = (y1 & 0xFFF0) + 8;
    }
    out[0] = x0, out[1] = y0, out[2] = x1, out[3] = y1;
    return 1;
}

/* The glyphs of the print being drawn: their records' indices, for their markers. The print sends
 * its glyphs as one GIF packet, a loop of six quadwords each, behind a header it writes last; a
 * glyph's marker cannot go where its loop would have, so the print's wrapper puts them all in the
 * packet's place when it returns. */
#define MOST_GLYPHS 1024
static int g_pending[MOST_GLYPHS];
static int g_npending = 0;

/* A glyph's record: the cell (u, v) to (u + w, v + h) texels of tex0 at the corners c, in rgba.
 * False when the window replays the 2D layer (or the print has too many glyphs), and the caller
 * sends the game's packet. */
static int record_glyph(
    const int c[4], int u, int v, int w, int h, uint32_t rgba, unsigned long long tex0
) {
    if (g_npending >= MOST_GLYPHS) {
        return 0;
    }
    openrac_game_ui_rect q;
    memset(&q, 0, sizeof(q));
    openrac_ui_rect(&q, c[0], c[1], c[2], c[3], u * 16, v * 16, (u + w) * 16, (v + h) * 16);
    q.rgba = rgba;
    q.tex0 = tex0;
    q.flags = 1u;
    const int index = openrac_game_ui_quad(&q);
    if (index < 0) {
        return 0;
    }
    g_pending[g_npending++] = index;
    return 1;
}

/* A record's marker at the list's cursor, as openrac_ui_send writes it. */
static void write_marker(int index) {
    const gaddr at = GREF(gaddr, LIST_CURSOR);
    GREF(uint32_t, at + 0) = 0x10000002u;
    GREF(uint32_t, at + 4) = 0;
    GREF(uint32_t, at + 8) = 0;
    GREF(uint32_t, at + 12) = 0x50000002u;
    GREF(uint64_t, at + 16) = 1ull | (1ull << 15) | (1ull << 60);
    GREF(uint64_t, at + 24) = 0xEull;
    GREF(uint64_t, at + 32) = (0x4E55ull << 48) | (uint32_t)index;
    GREF(uint64_t, at + 40) = 0x7Full;
    GREF(gaddr, LIST_CURSOR) = at + 48;
}

/* After a print whose glyphs were recorded: its packet's header (a cnt tag and VIF DIRECT of
 * 6n + 1 quadwords, then the GIF tag) stands where the glyphs' loops, none of them written, would
 * have followed it, then the scissor the print restores. The glyphs' markers go in the header's
 * place, what followed it after them. */
static void place_glyph_markers(gaddr start, int first) {
    const gaddr end = GREF(gaddr, LIST_CURSOR);
    gaddr h = start;
    while (h + 32 <= end) {
        const uint32_t tag = GREF(uint32_t, h), direct = GREF(uint32_t, h + 12);
        const uint32_t qwc = tag & 0xFFFFu;
        if ((tag & 0xFFFF0000u) == 0x10000000u && direct == (0x50000000u | qwc) && qwc % 6 == 1) {
            break;
        }
        h += 16;
    }
    if (h + 32 > end || end - (h + 32) > 256) {
        return; /* not the packet the print writes: the glyphs' records go unused */
    }
    uint8_t tail[256];
    const uint32_t rest = end - (h + 32);
    memcpy(tail, G(h + 32), rest);
    GREF(gaddr, LIST_CURSOR) = h;
    for (int i = first; i < g_npending; ++i) {
        write_marker(g_pending[i]);
    }
    const gaddr at = GREF(gaddr, LIST_CURSOR);
    memcpy(G(at), tail, rest);
    GREF(gaddr, LIST_CURSOR) = at + rest;
}

/* The font's prints: a string, glyph by glyph. */
void func_0038EBC8(
    float a0,
    float a1,
    float a2,
    float a3,
    int a4,
    int a5,
    int a6,
    int a7,
    int a8,
    unsigned long long a9,
    float a10,
    float a11
) {
    const gaddr start = GREF(gaddr, LIST_CURSOR);
    const int first = g_npending;
    ++g_glyphs;
    func_0038EBC8__game(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
    --g_glyphs;
    if (g_npending > first) {
        place_glyph_markers(start, first);
    }
    g_npending = first;
}

/* A glyph in one colour: the cell (u, v), w x h texels, at (x, y), fw x fh pixels. Only a glyph of
 * a print comes here natively: elsewhere the game's packet keeps the texels to the cell (a region
 * CLAMP), which a record does not. */
int func_0038BA50(
    int u,
    int v,
    int w,
    int h,
    long long t0,
    long long t1,
    int flag,
    float x,
    float y,
    float fw,
    float fh
) {
    int c[4];
    if (g_glyphs <= 0) {
        return func_0038BA50__game(u, v, w, h, t0, t1, flag, x, y, fw, fh);
    }
    if (!glyph_corners(x, y, fw, fh, flag, c)) {
        return 0;
    }
    if (!record_glyph(c, u, v, w, h, (uint32_t)t0, (unsigned long long)t1)) {
        return func_0038BA50__game(u, v, w, h, t0, t1, flag, x, y, fw, fh);
    }
    return 1;
}

/* func_0038BA50 with a colour a corner (colours: four RGBAQ doublewords): natively for a glyph
 * whose corners share their colour. */
int func_0038BC30(
    int u,
    int v,
    int w,
    int h,
    gaddr colours,
    long long t1,
    int flag,
    float x,
    float y,
    float fw,
    float fh
) {
    const uint32_t first = GREF(uint32_t, colours);
    const int same = g_glyphs > 0 && GREF(uint32_t, colours + 8) == first
                     && GREF(uint32_t, colours + 16) == first
                     && GREF(uint32_t, colours + 24) == first;
    int c[4];
    if (!same) {
        return func_0038BC30__game(u, v, w, h, colours, t1, flag, x, y, fw, fh);
    }
    if (!glyph_corners(x, y, fw, fh, flag, c)) {
        return 0;
    }
    if (!record_glyph(c, u, v, w, h, first, (unsigned long long)t1)) {
        return func_0038BC30__game(u, v, w, h, colours, t1, flag, x, y, fw, fh);
    }
    return 1;
}

/* A glyph with its drop shadow: the shadow first, in t1, moved by (dx, dy) pixels, then the glyph
 * in t0, both of the cell (x, y), w x h texels, of the texture t2. */
int func_0038BE10(
    int x,
    int y,
    int w,
    int h,
    long long t0,
    long long t1,
    long long t2,
    int snap,
    float sx,
    float sy,
    float sw,
    float sh,
    float dx,
    float dy
) {
    int c[4];
    if (g_glyphs <= 0) {
        return func_0038BE10__game(x, y, w, h, t0, t1, t2, snap, sx, sy, sw, sh, dx, dy);
    }
    /* The game tests the glyph's own corners and moves the shadow's by whole sixteenths. */
    int x0 = (int)(sx * 16.0f) + SCREEN_OFX - 8;
    int x1 = (int)((sx + sw) * 16.0f) + SCREEN_OFX - 8;
    int y0 = (int)(sy * 16.0f) + SCREEN_OFY - 8;
    int y1 = (int)((sy + sh) * 16.0f) + SCREEN_OFY - 8;
    if (x0 > 0x9000 || x1 < 0x7000 || y0 > 0x9000 || y1 < 0x7000) {
        return 0;
    }
    const int ddx = (int)(dx * 16.0f), ddy = (int)(dy * 16.0f);
    int shadow[4] = {x0 + ddx, y0 + ddy, x1 + ddx, y1 + ddy};
    if (snap) {
        for (int i = 0; i < 4; ++i) {
            shadow[i] = (shadow[i] & 0xFFF0) + 8;
        }
        x0 = (x0 & 0xFFF0) + 8, x1 = (x1 & 0xFFF0) + 8;
        y0 = (y0 & 0xFFF0) + 8, y1 = (y1 & 0xFFF0) + 8;
    }
    c[0] = x0, c[1] = y0, c[2] = x1, c[3] = y1;
    if (g_npending + 2 > MOST_GLYPHS
        || !record_glyph(shadow, x, y, w, h, (uint32_t)t1, (unsigned long long)t2)) {
        return func_0038BE10__game(x, y, w, h, t0, t1, t2, snap, sx, sy, sw, sh, dx, dy);
    }
    record_glyph(c, x, y, w, h, (uint32_t)t0, (unsigned long long)t2);
    return 1;
}

/* The glyph quad: a w x h screen rectangle at (x, y) textured from (u, v) .. (u + uw, v + vh) of
 * the texture a9 in colour a8, at the far depth. */
void func_0038B8B0(
    int x, int y, int w, int h, int u, int v, int uw, int vh, long long a8, long long a9
) {
    openrac_game_ui_rect q;
    memset(&q, 0, sizeof(q));
    openrac_ui_rect(
        &q,
        to_screen_x(x * 16),
        to_screen_y(y * 16),
        to_screen_x((x + w) * 16),
        to_screen_y((y + h) * 16),
        u * 16,
        v * 16,
        (u + uw) * 16,
        (v + vh) * 16
    );
    q.z = FAR_Z;
    q.rgba = (uint32_t)a8;
    q.tex0 = (unsigned long long)a9;
    q.flags = g_glyphs > 0 ? 1u : 0u;
    if (!openrac_ui_send(&q, LIST_CURSOR)) {
        func_0038B8B0__game(x, y, w, h, u, v, uw, vh, a8, a9);
    }
}

/* func_0038B8B0 turned by ang about its centre: the corners centre +- a +- b, a = h/2 (sin, cos),
 * b = w/2 (-cos, sin), as the game sums them. */
void func_0038B500(
    int x,
    int y,
    int w,
    int h,
    int u,
    int v,
    int du,
    int dv,
    float ang,
    long long col,
    long long tex
) {
    const int hw = w >> 1, hh = h >> 1;
    const float cx = (float)(x + hw), cy = (float)(y + hh);
    const float s = func_0038D490(ang), c = func_0038D478(ang);
    const float ax = (float)hh * s, ay = (float)hh * c;
    const float bx = (float)-hw * c, by = (float)hw * s;
    const float px[4] = {(cx - ax) + bx, (cx - ax) - bx, (cx + ax) + bx, (cx + ax) - bx};
    const float py[4] = {(cy - ay) + by, (cy - ay) - by, (cy + ay) + by, (cy + ay) - by};
    const int us[4] = {u << 4, (u + du) << 4, u << 4, (u + du) << 4};
    const int vs[4] = {v << 4, v << 4, (v + dv) << 4, (v + dv) << 4};
    openrac_game_ui_rect q;
    memset(&q, 0, sizeof(q));
    for (int i = 0; i < 4; ++i) {
        q.x[i] = (uint32_t)to_screen_x((int)px[i] << 4) & 0xFFFFu;
        q.y[i] = (uint32_t)to_screen_y((int)py[i] << 4) & 0xFFFFu;
        q.u[i] = (uint32_t)us[i];
        q.v[i] = (uint32_t)vs[i];
    }
    q.z = FAR_Z;
    q.rgba = (uint32_t)col;
    q.tex0 = (unsigned long long)tex;
    q.flags = g_glyphs > 0 ? 1u : 0u;
    if (!openrac_ui_send(&q, LIST_CURSOR)) {
        func_0038B500__game(x, y, w, h, u, v, du, dv, ang, col, tex);
    }
}

/* A quad of tex centred on (px, py), sx by sy, turned by ang about the pivot (ax, ay: 0 to 1 of
 * the quad), its w x h texels mirrored by fx, fy, in one colour at depth z: Going Commando's
 * func_0027E4A8, the corners as the game works them out. */
void func_0038C0B8(
    float px,
    float py,
    float sx,
    float sy,
    float ang,
    int w,
    int h,
    long long tex,
    int z,
    int rgba,
    int fx,
    int fy,
    float ax,
    float ay
) {
    const float vx = sy * func_0038D490(ang), vy = sy * func_0038D478(ang);
    const float hx = sx * func_0038D478(ang), hy = -sx * func_0038D490(ang);
    const float iu = 1.0f - ax, iv = 1.0f - ay;
    const float cx[4] =
        {(px + vx * iv) - hx * iu,
         (px + vx * iv) + hx * ax,
         (px - vx * ay) - hx * iu,
         (px - vx * ay) + hx * ax};
    const float cy[4] =
        {(py + vy * iv) - hy * iu,
         (py + vy * iv) + hy * ax,
         (py - vy * ay) - hy * iu,
         (py - vy * ay) + hy * ax};
    const int left = (uint8_t)fx ? w << 4 : 0x10, right = (uint8_t)fx ? 0x10 : w << 4;
    const int top = (uint8_t)fy ? h << 4 : 0x10, bottom = (uint8_t)fy ? 0x10 : h << 4;
    const int us[4] = {left, right, left, right}, vs[4] = {top, top, bottom, bottom};
    openrac_game_ui_rect q;
    memset(&q, 0, sizeof(q));
    for (int i = 0; i < 4; ++i) {
        q.x[i] = (uint32_t)to_screen_x((int)(cx[i] * 16.0f)) & 0xFFFFu;
        q.y[i] = (uint32_t)to_screen_y((int)(cy[i] * 16.0f)) & 0xFFFFu;
        q.u[i] = (uint32_t)us[i];
        q.v[i] = (uint32_t)vs[i];
    }
    q.z = (uint32_t)z;
    q.rgba = (uint32_t)rgba;
    q.tex0 = (unsigned long long)tex;
    if (!openrac_ui_send(&q, LIST_CURSOR)) {
        func_0038C0B8__game(px, py, sx, sy, ang, w, h, tex, z, rgba, fx, fy, ax, ay);
    }
}

/* A ring's segment: four XYZ2 doublewords, UV words and colours (each triangle in its last
 * corner's colour: the packet's PRIM is unshaded), the texture, and its wrap, clamped when
 * `clamped` (CLAMP_1 5) else repeated: Going Commando's func_0027E920. */
void func_0038C518(gaddr xyz, gaddr uv, gaddr colours, long long tex0, int clamped) {
    openrac_game_ui_rect q;
    memset(&q, 0, sizeof(q));
    for (int i = 0; i < 4; ++i) {
        const uint64_t w = GREF(uint64_t, xyz + 8u * (gaddr)i);
        const uint32_t t = GREF(uint32_t, uv + 4u * (gaddr)i);
        q.x[i] = (uint32_t)(w & 0xFFFFu);
        q.y[i] = (uint32_t)((w >> 16) & 0xFFFFu);
        q.u[i] = t & 0x3FFFu;
        q.v[i] = (t >> 16) & 0x3FFFu;
        q.corners[i] = GREF(uint32_t, colours + 4u * (gaddr)i);
    }
    q.z = (uint32_t)(GREF(uint64_t, xyz) >> 32);
    q.tex0 = (unsigned long long)tex0;
    q.flags = 4u | 16u;
    q.clamp = clamped ? 5u : 0u;
    if (!openrac_ui_send(&q, LIST_CURSOR)) {
        func_0038C518__game(xyz, uv, colours, tex0, clamped);
    }
}

/* A box shaded from one colour on its left to another on its right (func_0038C6F0's: the menus'
 * separators and the highlighted row): four XYZ2 doublewords, sent in the strip order 0, 2, 1, 3,
 * and two colours, the first for corners 0 and 2, untextured: Going Commando's func_0027EA28. */
void func_0038C628(gaddr xyz, gaddr colours) {
    static const int order[4] = {0, 2, 1, 3};
    openrac_game_ui_rect q;
    memset(&q, 0, sizeof(q));
    for (int i = 0; i < 4; ++i) {
        const uint64_t w = GREF(uint64_t, xyz + 8u * (gaddr)order[i]);
        q.x[i] = (uint32_t)(w & 0xFFFFu);
        q.y[i] = (uint32_t)((w >> 16) & 0xFFFFu);
        q.corners[i] = GREF(uint32_t, colours + 4u * (i < 2 ? 0u : 1u));
    }
    q.z = (uint32_t)(GREF(uint64_t, xyz) >> 32);
    q.flags = 4u | 8u | 32u;
    if (!openrac_ui_send(&q, LIST_CURSOR)) {
        func_0038C628__game(xyz, colours);
    }
}

/* A box of flat colour, rows y0 to y1 and columns x0 to x1 in screen pixels, at the far depth:
 * Going Commando's func_0027DE50. */
void func_0038B310(int y0, int y1, int x0, int x1, unsigned long long c) {
    openrac_game_ui_rect q;
    memset(&q, 0, sizeof(q));
    openrac_ui_rect(
        &q,
        to_screen_x(x0 * 16),
        to_screen_y(y0 * 16),
        to_screen_x(x1 * 16),
        to_screen_y(y1 * 16),
        0,
        0,
        0,
        0
    );
    q.z = FAR_Z;
    q.rgba = (uint32_t)c;
    q.flags = 8u;
    if (!openrac_ui_send(&q, LIST_CURSOR)) {
        func_0038B310__game(y0, y1, x0, x1, c);
    }
}

/* One of func_003AA780's quads, at `at` in the display list: a cnt tag of 8 quadwords with VIF
 * DIRECT 8, its GIF tag (D_001D8020: one loop of TEX0, PRIM and four RGBAQ, ST, XYZ2), then the
 * registers. */
#define SPRITE_QUAD_BYTES 0x90u

static int sprite_quad_at(gaddr at) {
    /* PRIM's FST (UV, not ST): the quad's texels are the UV register's, which the routine does not
     * send; such a sprite stays the game's packets. */
    return (GREF(uint64_t, at + 40) & 0x100u) == 0 && GREF(uint32_t, at) == 0x10000008u
           && GREF(uint32_t, at + 12) == 0x50000008u
           && GREF(uint64_t, at + 16) == 0xE400000000008001ull
           && GREF(uint64_t, at + 24) == 0x0052152152152106ull;
}

/* The quad's record: its corners in the order sent (a strip), each corner's colour, shaded when
 * its PRIM is (IIP), textured when it is (TME, with ST over Q into texels of TEX0's size), the
 * texture repeated as the routine's setup (D_001D8030: CLAMP_1 0) leaves it. */
static void sprite_record(gaddr at, openrac_game_ui_rect* q) {
    const gaddr r = at + 32;
    const uint64_t tex0 = GREF(uint64_t, r);
    const uint64_t prim = GREF(uint64_t, r + 8);
    const float tw = (float)(1u << ((tex0 >> 26) & 15u)), th = (float)(1u << ((tex0 >> 30) & 15u));
    float us[4], vs[4];
    float umin = 0.0f, vmin = 0.0f;
    memset(q, 0, sizeof(*q));
    for (int i = 0; i < 4; ++i) {
        const gaddr v = r + 16 + 24u * (gaddr)i;
        const uint64_t rgbaq = GREF(uint64_t, v);
        const uint64_t st = GREF(uint64_t, v + 8);
        const uint64_t xyz = GREF(uint64_t, v + 16);
        float s, t, w;
        uint32_t bits = (uint32_t)st, tbits = (uint32_t)(st >> 32), qbits = (uint32_t)(rgbaq >> 32);
        memcpy(&s, &bits, 4);
        memcpy(&t, &tbits, 4);
        memcpy(&w, &qbits, 4);
        if (w == 0.0f) {
            w = 1.0f;
        }
        q->x[i] = (uint32_t)(xyz & 0xFFFFu);
        q->y[i] = (uint32_t)((xyz >> 16) & 0xFFFFu);
        us[i] = s / w * tw;
        vs[i] = t / w * th;
        umin = i == 0 || us[i] < umin ? us[i] : umin;
        vmin = i == 0 || vs[i] < vmin ? vs[i] : vmin;
        q->corners[i] = (uint32_t)rgbaq;
    }
    /* The texture repeats (CLAMP_1 0), and the quads reach above its top (t below 0): moved by
     * whole repeats to texel coordinates of 0 on, which the record's UV holds. */
    const float du = tw * (float)(int)(umin < 0.0f ? (umin / tw) - 1.0f : umin / tw);
    const float dv = th * (float)(int)(vmin < 0.0f ? (vmin / th) - 1.0f : vmin / th);
    for (int i = 0; i < 4; ++i) {
        q->u[i] = (uint32_t)(int32_t)((us[i] - du) * 16.0f);
        q->v[i] = (uint32_t)(int32_t)((vs[i] - dv) * 16.0f);
    }
    q->z = (uint32_t)(GREF(uint64_t, r + 16 + 16) >> 32);
    q->rgba = q->corners[3];
    q->tex0 = tex0;
    q->flags = 4u | 16u | ((prim & 8u) ? 32u : 0u) | ((prim & 16u) ? 0u : 8u);
    q->clamp = 0u;
}

/* A Widget3D sprite: the game poses it and sends its quads, which become records in their place
 * (markers), the routine's other packets left as they are. Should the window replay the 2D layer,
 * the packets stay the game's. */
void func_003AA780(gaddr a0, float a1, float a2, int a3, float a4, gaddr a5) {
    const gaddr start = GREF(gaddr, LIST_CURSOR);
    func_003AA780__game(a0, a1, a2, a3, a4, a5);
    const gaddr end = GREF(gaddr, LIST_CURSOR);
    gaddr at = start;
    while (at + 16 <= end && !sprite_quad_at(at)) {
        at += 16; /* the setup before the quads (func_003AA328's reference to D_001D8030) */
    }
    if (at >= end) {
        return;
    }
    /* Records first, all of them, before the list is written over: markers are shorter than the
     * quads, so each lands before the quads not read yet. */
    openrac_game_ui_rect quads[64];
    int n = 0;
    gaddr q = at;
    while (q + SPRITE_QUAD_BYTES <= end && sprite_quad_at(q) && n < 64) {
        sprite_record(q, &quads[n++]);
        q += SPRITE_QUAD_BYTES;
    }
    if (q != end) {
        return; /* something else among the quads, or more than 64: the game's packets stay */
    }
    GREF(gaddr, LIST_CURSOR) = at;
    for (int i = 0; i < n; ++i) {
        if (!openrac_ui_send(&quads[i], LIST_CURSOR)) {
            GREF(gaddr, LIST_CURSOR) = end; /* replaying: the packets are still there, untouched */
            return;
        }
    }
}
