/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Ratchet & Clank: the 2D layer's draw routines, wrapped (libraries.tsv, port = wrap) so the
 * HUD and menus are drawn natively instead of from their GS packets (OPENRAC_UI_REPLAY replays the
 * packets, for comparing). Each routine's quad goes to the window as a record through the frame
 * recorder (openrac_ui_record) with the corners and texel coordinates its packet would have held,
 * placed where the display list has got to, so the window draws it where it came; nothing is
 * written into the list.
 * OPENRAC_UI_LOG: each distinct caller and texture of DrawTexturedQuad, once, in the log.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../common/lib/ui_quad.h"
#include "../../common/lib/vendor_title.h"
#include "openrac/host_env.h"
#include "prompt_spots.h"
#include "rac1.h"

/* The screen offset the game adds to every 2D position (D_0013E600 +0x10/+0x14), and the HUD
 * arena (D_0019A4E8): +0xC the depth its sprites are drawn at, +0x20/+0x24 its texture tables. */
#define SCREEN_OFX GREF(int, RAC1_ADDR(0x0013E610u))
#define SCREEN_OFY GREF(int, RAC1_ADDR(0x0013E614u))
#define HUD_ARENA OPENRAC_DATA(RAC1_ADDR(0x0019A4E8u))

static void log_caller(const char* what, unsigned long long tex, int w, int h) {
    static const char* seen[256];
    static int count = 0;
    /* Who called the routine that called this, as the host names it (guest.h: any host). */
    char caller[160];
    openrac_host_caller(1, caller, sizeof(caller));
    char key[256];
    snprintf(key, sizeof(key), "%s%s tbp %#x %dx%d", what, caller, (unsigned)tex & 0x3FFF, w, h);
    for (int i = 0; i < count; ++i) {
        if (strcmp(seen[i], key) == 0) {
            return;
        }
    }
    if (count < 256) {
        seen[count++] = strdup(key);
        fprintf(stderr, "ui2d: %s\n", key);
    }
}

/* OPENRAC_UI_SKIP=NAME,...: those routines send their own packets (bisecting). */
static int skipped(const char* name) {
    const char* list = openrac_env_string("OPENRAC_UI_SKIP");
    return list != NULL && strstr(list, name) != NULL;
}

static int emit(openrac_game_ui_rect* q) {
    static int log = -1;
    if (log < 0) {
        log = openrac_env_string("OPENRAC_UI_LOG") != NULL;
    }
    if (log) {
        log_caller("emit ", q->tex0, (int)(q->x[3] - q->x[0]) / 16, (int)(q->y[3] - q->y[0]) / 16);
    }
    return openrac_ui_record(q, OPENRAC_DATA(RAC1_ADDR(0x00161000u)));
}

/* A HUD texture's size: 1 << its entry's +6/+7 log2 sizes (the entry found through the arena's
 * +0x20 index table into its +0x24 texture table). */
static void hud_size(int tex, int* tw, int* th) {
    const gaddr arena = HUD_ARENA;
    const gaddr index = GREF(gaddr, arena + 0x20);
    const gaddr e = GREF(gaddr, arena + 0x24) + (gaddr)GREF(int16_t, index + (gaddr)tex * 4 + 2) * 8;
    *tw = 1 << GREF(uint8_t, e + 6);
    *th = 1 << GREF(uint8_t, e + 7);
}

static void hud_common(openrac_game_ui_rect* q, unsigned long long tex0, int alpha) {
    memset(q, 0, sizeof(*q));
    q->flags = 2; /* the HUD's routines send sprites (the turned and rotated ones strips) */
    q->z = (uint32_t)GREF(int, HUD_ARENA + 0xC);
    q->rgba = ((uint32_t)alpha << 24) | 0x7F7F7Fu;
    q->tex0 = tex0;
}

/* Inside FontPrint: the quads drawn are glyphs of text. */
static int g_glyphs = 0;
static float g_vendor_title_scale = 1.0f;

/* The button icons FontPrint drew (prompt_spots.h). A menu panel's text goes to a texture first,
 * which DrawTexturedQuad then puts on the screen (the capture texture, CAPTURE_TEX0): the icons
 * drawn since are moved to where it puts them (g_pending to g_spots). What no capture puts on the
 * screen was drawn on it. g_open_spot is the icon whose line the glyphs after it join (-1 none).
 * Not at the vendor, whose text goes to its monitors (mode 5). */
#define CAPTURE_TEX0 GREF(unsigned long long, RAC1_ADDR(0x0015EFD0u))
static openrac_rac1_prompt_spot g_spots[OPENRAC_RAC1_PROMPT_SPOTS_MAX];
static int g_spot_count = 0;
static openrac_rac1_prompt_spot g_pending[OPENRAC_RAC1_PROMPT_SPOTS_MAX];
static int g_pending_count = 0;
static int g_open_spot = -1;
static int g_spots_off = 0;

static void keep_spot(const openrac_rac1_prompt_spot* s) {
    if (g_spot_count < OPENRAC_RAC1_PROMPT_SPOTS_MAX) {
        g_spots[g_spot_count++] = *s;
    }
}

int openrac_rac1_take_prompt_spots(openrac_rac1_prompt_spot* out) {
    for (int i = 0; i < g_pending_count; ++i) {
        keep_spot(&g_pending[i]);
    }
    g_pending_count = 0;
    g_open_spot = -1;
    const int n = g_spot_count;
    memcpy(out, g_spots, sizeof(g_spots[0]) * (size_t)n);
    g_spot_count = 0;
    return n;
}

/* The capture texture's (u, v) .. (u + uw, v + vh) put on the screen at (x, y), w x h: the icons
 * drawn into that part of it go there. */
static void place_spots(int x, int y, int w, int h, int u, int v, int uw, int vh) {
    if (uw <= 0 || vh <= 0) {
        return;
    }
    for (int i = 0; i < g_pending_count; ++i) {
        openrac_rac1_prompt_spot s = g_pending[i];
        const int cx = (s.x0 + s.x1) / 2, cy = (s.y0 + s.y1) / 2;
        if (cx < u || cx > u + uw || cy < v || cy > v + vh) {
            continue;
        }
        s.x0 = x + (s.x0 - u) * w / uw;
        s.x1 = x + (s.x1 - u) * w / uw;
        s.y0 = y + (s.y0 - v) * h / vh;
        s.y1 = y + (s.y1 - v) * h / vh;
        keep_spot(&s);
    }
    g_pending_count = 0;
    g_open_spot = -1;
}

/* A glyph FontPrint drew at (x, y), w x h, from (u, v) of its font: an icon (the font's row 96,
 * key_prompts.h's cells) starts a spot; a glyph after it on its line makes the spot longer. */
static void prompt_glyph(int x, int y, int w, int h, int u, int v) {
    static const int cell_u[10] = {0, 24, 48, 72, 96, 120, 144, 168, 208, 232};
    int cell = -1;
    if (v == 96) {
        for (int i = 0; i < 10; ++i) {
            cell = u == cell_u[i] ? i : cell;
        }
    }
    if (cell >= 0) {
        g_open_spot = -1;
        if (g_pending_count < OPENRAC_RAC1_PROMPT_SPOTS_MAX) {
            openrac_rac1_prompt_spot* s = &g_pending[g_pending_count];
            s->cell = cell;
            s->x0 = x;
            s->y0 = y;
            s->x1 = x + w;
            s->y1 = y + h;
            g_open_spot = g_pending_count++;
        }
        return;
    }
    if (g_open_spot >= 0) {
        openrac_rac1_prompt_spot* s = &g_pending[g_open_spot];
        /* Only on the icon's line: a glyph below it starts no part of it. */
        if (y < s->y1 && y + h > s->y0) {
            s->x1 = x + w > s->x1 ? x + w : s->x1;
        } else {
            g_open_spot = -1;
        }
    }
}

/* FontPrint: a string at (x, y) in colour, glyph by glyph through DrawTexturedQuad. */
void func_001F6668(int x, int y, unsigned long long colour, gaddr text, int limit, long long texture,
                   gaddr glyphs) {
    const float previous_scale = g_vendor_title_scale;
    uint32_t mode = 0;
    /* Only the title in the vendor's 128-pixel left monitor. */
    g_vendor_title_scale = 1.0f;
    if (x == 6 && y == 8 && glyphs == OPENRAC_DATA(RAC1_ADDR(0x001DF770u)) && openrac_game_mode
        && openrac_game_mode(&mode) && mode == 5) {
        g_vendor_title_scale =
            openrac_vendor_title_scale((const uint8_t*)G(text), limit, (const int8_t*)G(glyphs));
    }
    const int spots_off = g_spots_off;
    g_spots_off = openrac_game_mode && openrac_game_mode(&mode) && mode == 5;
    g_open_spot = -1;
    ++g_glyphs;
    func_001F6668__game(x, y, colour, text, limit, texture, glyphs);
    --g_glyphs;
    g_open_spot = -1;
    g_spots_off = spots_off;
    g_vendor_title_scale = previous_scale;
}

/* DrawTexturedQuad: a w x h screen rectangle at (x, y) textured from (u, v) .. (u + uw, v + vh) of
 * the texture tex (TEX0) in colour rgba. */
void func_001F5800(int x, int y, int w, int h, int u, int v, int uw, int vh,
                   RAC1_TYPE_func_001F5800_u64 rgba_, RAC1_TYPE_func_001F5800_u64 tex_) {
    const unsigned long long rgba = (unsigned long long)rgba_, tex = (unsigned long long)tex_;
    const float scale = g_glyphs > 0 ? g_vendor_title_scale : 1.0f;
    const float left = 6 + (x - 6) * scale, top = 8 + (y - 8) * scale;
    if (g_glyphs > 0 && !g_spots_off) {
        prompt_glyph(x, y, w, h, u, v);
    } else if (g_glyphs == 0 && g_pending_count > 0 && tex == CAPTURE_TEX0) {
        place_spots(x, y, w, h, u, v, uw, vh);
    }
    if (skipped(RAC1_NAME(func_001F5800))) {
        func_001F5800__game(x, y, w, h, u, v, uw, vh, rgba, tex);
        return;
    }
    static int log = -1;
    if (log < 0) {
        log = openrac_env_string("OPENRAC_UI_LOG") != NULL;
    }
    if (log) {
        log_caller("quad ", tex, w, h);
    }
    openrac_game_ui_rect q;
    memset(&q, 0, sizeof(q));
    const int ofx = SCREEN_OFX, ofy = SCREEN_OFY;
    openrac_ui_rect(
        &q,
        (int)(left * 16) + ofx - 8,
        (int)(top * 16) + ofy - 8,
        (int)((left + w * scale) * 16) + ofx - 8,
        (int)((top + h * scale) * 16) + ofy - 8,
        u * 16,
        v * 16,
        (u + uw) * 16,
        (v + vh) * 16
    );
    q.z = 0xFFFFF0u;
    q.rgba = (uint32_t)rgba;
    q.tex0 = tex;
    q.flags = g_glyphs > 0 ? 1u : 0u;
    if (!emit(&q)) {
        func_001F5800__game(x, y, w, h, u, v, uw, vh, rgba, tex);
    }
}

/* HUD texture tex as a sprite at (x, y), w x h pixels, its whole texture, with alpha. */
void func_00200468(int tex, int x, int y, int w, int h, int alpha) {
    if (skipped(RAC1_NAME(func_00200468))) {
        func_00200468__game(tex, x, y, w, h, alpha);
        return;
    }
    int tw, th;
    openrac_game_ui_rect q;
    hud_size(tex, &tw, &th);
    hud_common(&q, func_00200248(tex), alpha);
    const int ofx = SCREEN_OFX, ofy = SCREEN_OFY;
    openrac_ui_rect(
        &q,
        x * 16 + ofx - 8,
        y * 16 + ofy - 8,
        (x + w) * 16 + ofx - 8,
        (y + h) * 16 + ofy - 8,
        0,
        0,
        tw * 16,
        th * 16
    );
    if (!emit(&q)) {
        func_00200468__game(tex, x, y, w, h, alpha);
    }
}

/* func_00200468 with the texture turned a quarter: corners (x, y+h), (x, y), (x+w, y+h), (x+w, y)
 * take texels (tw, 0), (tw, th), (0, 0), (0, th). */
void func_00200650(int tex, int x, int y, int w, int h, int alpha) {
    if (skipped(RAC1_NAME(func_00200650))) {
        func_00200650__game(tex, x, y, w, h, alpha);
        return;
    }
    int tw, th;
    openrac_game_ui_rect q;
    hud_size(tex, &tw, &th);
    hud_common(&q, func_00200248(tex), alpha);
    q.flags = 0;
    const int ofx = SCREEN_OFX, ofy = SCREEN_OFY;
    const int xl = x * 16 + ofx - 8, xr = (x + w) * 16 + ofx - 8;
    const int yt = y * 16 + ofy - 8, yb = (y + h) * 16 + ofy - 8;
    const int xs[4] = {xl, xl, xr, xr}, ys[4] = {yb, yt, yb, yt};
    const int us[4] = {tw * 16, tw * 16, 0, 0}, vs[4] = {0, th * 16, 0, th * 16};
    for (int i = 0; i < 4; ++i) {
        q.x[i] = (uint32_t)xs[i], q.y[i] = (uint32_t)ys[i], q.u[i] = (uint32_t)us[i], q.v[i] = (uint32_t)vs[i];
    }
    if (!emit(&q)) {
        func_00200650__game(tex, x, y, w, h, alpha);
    }
}

/* func_00200468 with the position and size in 16ths of a pixel. */
void func_002008B8(int tex, int x, int y, int w, int h, int alpha) {
    if (skipped(RAC1_NAME(func_002008B8))) {
        func_002008B8__game(tex, x, y, w, h, alpha);
        return;
    }
    int tw, th;
    openrac_game_ui_rect q;
    hud_size(tex, &tw, &th);
    hud_common(&q, func_00200248(tex), alpha);
    const int ofx = SCREEN_OFX, ofy = SCREEN_OFY;
    openrac_ui_rect(
        &q, x + ofx - 8, y + ofy - 8, x + w + ofx - 8, y + h + ofy - 8, 0, 0, tw * 16, th * 16
    );
    if (!emit(&q)) {
        func_002008B8__game(tex, x, y, w, h, alpha);
    }
}

/* func_002008B8 with a texture window: texels from (u, v) on by the texture's size (12.4). */
void func_00200A90(int tex, int x, int y, int w, int h, int u, int v, int alpha) {
    if (skipped(RAC1_NAME(func_00200A90))) {
        func_00200A90__game(tex, x, y, w, h, u, v, alpha);
        return;
    }
    int tw, th;
    openrac_game_ui_rect q;
    hud_size(tex, &tw, &th);
    hud_common(&q, func_00200248(tex), alpha);
    const int ofx = SCREEN_OFX, ofy = SCREEN_OFY;
    openrac_ui_rect(
        &q,
        x + ofx - 8,
        y + ofy - 8,
        x + w + ofx - 8,
        y + h + ofy - 8,
        u,
        v,
        u + tw * 16,
        v + th * 16
    );
    if (!emit(&q)) {
        func_00200A90__game(tex, x, y, w, h, u, v, alpha);
    }
}

/* A sprite of the texture tex0 at (x, y), w x h in 16ths of a pixel, texels from (u, v) on by
 * (1 << (lw + 4), 1 << (lh + 4)), with alpha. */
void func_00200CA0(RAC1_TYPE_func_00200CA0_tex0 tex0_, int x, int y, int lw, int lh, int w, int h, int u, int v,
                   int alpha) {
    const long long tex0 = (long long)tex0_;
    if (skipped(RAC1_NAME(func_00200CA0))) {
        func_00200CA0__game(tex0, x, y, lw, lh, w, h, u, v, alpha);
        return;
    }
    openrac_game_ui_rect q;
    hud_common(&q, (unsigned long long)tex0, alpha);
    const int ofx = SCREEN_OFX, ofy = SCREEN_OFY;
    openrac_ui_rect(
        &q,
        x + ofx - 8,
        y + ofy - 8,
        x + w + ofx - 8,
        y + h + ofy - 8,
        u,
        v,
        u + (1 << (lw + 4)),
        v + (1 << (lh + 4))
    );
    if (!emit(&q)) {
        func_00200CA0__game(tex0, x, y, lw, lh, w, h, u, v, alpha);
    }
}

/* A w x h sprite of tex at (x, y) (16ths of a pixel) turned by rot: corners pos +- a +- b with
 * a = h (sin, cos), b = w (-cos, sin), texels from (0, 0) to (u, v) texels. */
void func_00200E38(int u, int v, long long tex, float x, float y, float w, float h, float rot) {
    if (skipped(RAC1_NAME(func_00200E38))) {
        func_00200E38__game(u, v, tex, x, y, w, h, rot);
        return;
    }
    openrac_game_ui_rect q;
    hud_common(&q, (unsigned long long)tex, 0x80);
    q.flags = 0;
    q.rgba = 0x807F7F7Fu;
    const float s = func_001F9FA8(rot), c = func_001F9F90(rot);
    const float ax = h * s, ay = h * c, bx = -w * c, by = w * s;
    const float cx[4] = {x + ax - bx, x + ax + bx, x - ax - bx, x - ax + bx};
    const float cy[4] = {y + ay - by, y + ay + by, y - ay - by, y - ay + by};
    const int us[4] = {u * 16, u * 16, 0, 0}, vs[4] = {0, v * 16, 0, v * 16};
    const int ofx = SCREEN_OFX, ofy = SCREEN_OFY;
    for (int i = 0; i < 4; ++i) {
        q.x[i] = (uint32_t)((int)cx[i] + ofx - 8);
        q.y[i] = (uint32_t)((int)cy[i] + ofy - 8);
        q.u[i] = (uint32_t)us[i];
        q.v[i] = (uint32_t)vs[i];
    }
    if (!emit(&q)) {
        func_00200E38__game(u, v, tex, x, y, w, h, rot);
    }
}

/* HUD texture tex as a sprite from (x0, y0) to (x1, y1) (16ths of a pixel), texels (u0, v0) to
 * (u1, v1) (12.4), with alpha. */
void func_00201190(int tex, int x0, int y0, int x1, int y1, int u0, int v0, int u1, int v1, int alpha) {
    if (skipped(RAC1_NAME(func_00201190))) {
        func_00201190__game(tex, x0, y0, x1, y1, u0, v0, u1, v1, alpha);
        return;
    }
    openrac_game_ui_rect q;
    hud_common(&q, func_00200248(tex), alpha);
    const int ofx = SCREEN_OFX, ofy = SCREEN_OFY;
    openrac_ui_rect(&q, x0 + ofx - 8, y0 + ofy - 8, x1 + ofx - 8, y1 + ofy - 8, u0, v0, u1, v1);
    if (!emit(&q)) {
        func_00201190__game(tex, x0, y0, x1, y1, u0, v0, u1, v1, alpha);
    }
}
