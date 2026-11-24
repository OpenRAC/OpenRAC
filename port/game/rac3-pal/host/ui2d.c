/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Up Your Arsenal: the 2D layer's draw routines, wrapped (libraries.tsv, port = wrap): what each
 * would draw goes to the window through the frame recorder (openrac/frame.h), placed by the
 * display list's cursor with nothing written into the list. Text goes through the shared 2D layer
 * (openrac/ui.h, ui_adapter.c): a glyph of a font's print is its cell of the font page as the game
 * keeps it in its font header, no word of the console's graphics chip, drawn from the page's
 * distance field, sharp at any size. OPENRAC_UI_REPLAY sends the game's own packets, for
 * comparing.
 *
 * The routines were paired with Going Commando's instruction by instruction (their opcode
 * sequences against the boot program's functions): the flat box (func_0038B310, rac2's
 * func_0027DE50), the glyph quad (func_0038B8B0, func_0027E018), the turned sprite
 * (func_0038C0B8, func_0027E4A8), the ring segment (func_0038C518, func_0027E920) and the shaded
 * box (func_0038C628, func_0027EA28) are the same routines at other addresses. Up Your Arsenal's
 * font builds its glyphs with routines of its own, float corners and the glyph's cell as a region
 * clamp: one colour (func_0038BA50), a colour a corner (func_0038BC30), and a glyph with its drop
 * shadow (func_0038BE10); func_0038B500 is a turned glyph quad. Its prints mark what they draw as
 * glyphs: func_0038EBC8, which the menus call, and func_0038F458, the same print with a colour a
 * corner (four RGBAQ doublewords in place of the colour), which no code of the boot program or of
 * frontbin calls (frontbin's twin is func_0038F078) but which is wrapped as its twin is, so that
 * nothing of the font is drawn from the game's packets.
 *
 * The darkening over the main menu's background picture and the fade between screens
 * (func_0038B200, a colour over the picture as sixteen 32-pixel sprites of a fixed packet,
 * D_00141850) is a fill of the whole window.
 *
 * The menus' frames and art (the help boxes' frames too, func_003AC9F0) are Widget3D sprites: a
 * small animated mesh of quads that func_003AA780 poses (its bones through func_003AA168, its
 * vertices to the scratchpad through func_003A08E8 or func_003A0A28) and sends one quad at a time.
 * Its wrapper draws those quads from the posed vertices and the mesh's own quad list and textures
 * (widget_sprite), and takes what the routine sent back out of the list unread.
 *
 * The other textured routines (glyph quads outside a print, turned glyph quads, turned sprites,
 * ring segments, the turned HUD quad) are given their texture as the handle a texture getter
 * returned; the getters are wrapped (textures.c), so the handle names the image the game's tables
 * hold (rac3_texture_image), drawn through the shared layer. A handle no getter named stays the
 * older record.
 *
 * Every level's program carries its own copy of all of these (rac3-decomp's catalogue folds them
 * with frontbin's, and hostgen names them after Veldin's places, func_L01_...): the same
 * instructions as the boot program's, writing to the level's display list (Veldin's cursor
 * 0x001E1068 for the boot program's 0x001DA050) and reading the level's HUD texture tables. The
 * level's copies are wrapped the same way, through the same code (a Ui2d says whose copy), so the
 * HUD, the pause menu and every page drawn in a level are records too. The HUD's own sprites, which
 * the boot program has no use for, are Going Commando's (func_0028E868, func_0028ECD8,
 * func_0028F078 and func_0028EA58, paired with Veldin's copies instruction by instruction: hud.c).
 */
#include "ui2d.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../../common/lib/ui_quad.h"
#include "exec.h"
#include "game_protos.h"
#include "mp/game_protos.h"
#include "openrac/frame.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "openrac/host_env.h"
#include "openrac/ui.h"
#include "rac3_version.h"
#include "textures.h"
#include "ui_adapter.h"

/* The offset the game adds to every 2D position (12.4): the core's, which every program shares
 * (the multiplayer executable's core has it at 0x00240310). */
static gaddr screen_offset(void) {
    if (rac3_exec_program() == RAC3_EXEC_MULTIPLAYER) {
        return RAC3_MADDR(0x00240310u);
    }
    return RAC3_ADDR(0x001D4BD0u);
}

#define SCREEN_OFX GREF(int32_t, screen_offset())
#define SCREEN_OFY GREF(int32_t, screen_offset() + 4u)
/* The far depth the 2D layer's boxes and glyph quads are drawn at (0xFFFFF0 in XYZ2's z). */
#define FAR_Z 0xFFFFF0u

/* The boot program's copy: its display list's cursor, and its HUD textures: by a texture's number,
 * a 4-byte entry at *D_001D9EA8 whose halfword at +2 indexes the 8-byte entries at *D_001D9EAC, +6
 * and +7 the log2 width and height. */
static const Rac3Ui2d kBoot =
    {RAC3_ADDR(0x001DA050u), RAC3_ADDR(0x001D9EA8u), RAC3_ADDR(0x001D9EACu), 0};

/* The loaded level's copy, at Veldin's addresses (OPENRAC_LDATA): the cursor, the HUD texture
 * tables and the depth the HUD's sprites are drawn at, which its update (func_L01_0041FE80, Going
 * Commando's func_0028E458) sets each frame. */
Rac3Ui2d rac3_ui2d_level(void) {
    const Rac3Ui2d u = {
        OPENRAC_LDATA(1, RAC3_LADDR(1, 0x001E1068u)),
        OPENRAC_LDATA(1, RAC3_LADDR(1, 0x001DFD20u)),
        OPENRAC_LDATA(1, RAC3_LADDR(1, 0x001DFD24u)),
        OPENRAC_LDATA(1, RAC3_LADDR(1, 0x001DFD0Cu)),
    };
    return u;
}

/* Inside a font print: the quads drawn are glyphs of text. */
static int g_glyphs = 0;

static int to_screen_x(int sixteenths) {
    return sixteenths + SCREEN_OFX - 8;
}

static int to_screen_y(int sixteenths) {
    return sixteenths + SCREEN_OFY - 8;
}

int rac3_ui2d_screen_x(int sixteenths) {
    return to_screen_x(sixteenths);
}

int rac3_ui2d_screen_y(int sixteenths) {
    return to_screen_y(sixteenths);
}

/* A HUD texture's size: 1 << its entry's +6/+7 log2 sizes. */
void rac3_ui2d_hud_size(const Rac3Ui2d* u, int tex, int* tw, int* th) {
    const gaddr entry = GREF(gaddr, u->hud_entries) + 4u * (gaddr)tex;
    const gaddr image = GREF(gaddr, u->hud_images) + 8u * (gaddr)GREF(int16_t, entry + 2u);
    *tw = 1 << GREF(uint8_t, image + 6u);
    *th = 1 << GREF(uint8_t, image + 7u);
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

/* A program's copy of the font's handle tables: the words holding its pointers to the current
 * font's table of page handles and of palette handles (what FontSelect sets: the boot program's
 * and frontbin's D_001D5B38 and D_001D5B3C, a level's own). A glyph's builder is given its page as
 * the handles the font's print looked up there; the page's image itself is the font header's
 * (rac3_ui_font_page). */
typedef struct FontTables {
    gaddr pages;
    gaddr palettes;
} FontTables;

static const FontTables kBootFonts = {RAC3_ADDR(0x001D5B38u), RAC3_ADDR(0x001D5B3Cu)};
/* The multiplayer core's, and level 39's (which the maps' copies share). */
static const FontTables kMpCoreFonts = {RAC3_MADDR(0x00241CE8u), RAC3_MADDR(0x00241CECu)};
static const FontTables kMpMenuFonts = {RAC3_MADDR(0x00242820u), RAC3_MADDR(0x00242824u)};

static FontTables level_fonts(void) {
    const FontTables f = {
        OPENRAC_LDATA(1, RAC3_LADDR(1, 0x001D64F0u)),
        OPENRAC_LDATA(1, RAC3_LADDR(1, 0x001D64F4u)),
    };
    return f;
}

/* The print being drawn: where the list had got to when it started (its glyphs go there, before
 * what it sends itself), whose font tables it reads (none: a copy whose tables are not known yet,
 * whose glyphs go to the window as the older records), and how many of its glyphs were drawn
 * natively and how many the game sent itself. */
static struct {
    gaddr anchor;
    FontTables fonts;
    int fonts_known;
    int native;
    int sent;
} g_print;

/* Where the window places what is recorded now: inside a print, at the print's start; else at the
 * list's cursor (the word at `cursor`). Nothing is written into the list. */
static void anchor_at(gaddr cursor) {
    const gaddr at = g_glyphs > 0 ? g_print.anchor : GREF(gaddr, cursor);
    openrac_frame_anchor(at < 0x02000000u ? at : 0);
}

/* A quad the game draws in flat colour (boxes, the darkening, the letterbox's bars): through the
 * shared 2D layer as colour alone, its corners (the chip's 12.4 with the game's offset, as the
 * game computes them) in the game's 2D pixels, a colour each corner, shaded or not as the record
 * says. */
static int send_flat(const openrac_game_ui_rect* q, gaddr cursor) {
    openrac_ui_quad quad;
    memset(&quad, 0, sizeof(quad));
    const float ofx = (float)(rac3_ui2d_screen_x(0) + 8);
    const float ofy = (float)(rac3_ui2d_screen_y(0) + 8);
    for (int i = 0; i < 4; ++i) {
        quad.x[i] = ((float)(int)q->x[i] - ofx) / 16.0f;
        quad.y[i] = ((float)(int)q->y[i] - ofy) / 16.0f;
        quad.rgba[i] = (q->flags & 4u) != 0 ? q->corners[i] : q->rgba;
    }
    quad.flags = OPENRAC_UI_FLAT | ((q->flags & 32u) != 0 ? OPENRAC_UI_SHADED : 0u);
    rac3_ui_adapter_install();
    anchor_at(cursor);
    return openrac_ui_draw_quad(&quad);
}

/* Whether a box drawn on the list at `cursor` is drawn natively: in the single-player programs, and in
 * the multiplayer executable from its core and level 39, whose scissor is followed (SetScissor). */
static int flat_native(gaddr cursor) {
    if (rac3_exec_program() != RAC3_EXEC_MULTIPLAYER) {
        return 1;
    }
    return cursor == RAC3_MADDR(0x002446A8u) || GREF(int32_t, RAC3_MADDR(0x002412DCu)) == 39;
}

/* The HUD bank whose handles a quad drawn on the list at `cursor` may hold: the boot program's and
 * frontbin's, or the loaded level's. */
static gaddr hud_tables_of(gaddr cursor) {
    return cursor == kBoot.cursor ? kBoot.hud_entries : rac3_ui2d_level().hud_entries;
}

/* A textured quad through the shared 2D layer, its image the one the game's texture getters named
 * for its handle (rac3_texture_image: HUD, effect and picture textures by their tables): its
 * corners (12.4 with the game's offset) and texels (12.4) in the game's pixels and the image's, a
 * colour each corner, shaded or not, the image repeated when the record's wrap says so. False when
 * no image is known for the handle, or the window does not draw it. */
static int send_textured(const openrac_game_ui_rect* q, gaddr cursor) {
    openrac_ui_quad quad;
    memset(&quad, 0, sizeof(quad));
    if (rac3_exec_program() == RAC3_EXEC_MULTIPLAYER
        || !rac3_texture_image(q->tex0, hud_tables_of(cursor), &quad.image)) {
        return 0;
    }
    const float ofx = (float)(rac3_ui2d_screen_x(0) + 8);
    const float ofy = (float)(rac3_ui2d_screen_y(0) + 8);
    for (int i = 0; i < 4; ++i) {
        quad.x[i] = ((float)(int)q->x[i] - ofx) / 16.0f;
        quad.y[i] = ((float)(int)q->y[i] - ofy) / 16.0f;
        quad.u[i] = (float)(int32_t)q->u[i] / 16.0f;
        quad.v[i] = (float)(int32_t)q->v[i] / 16.0f;
        quad.rgba[i] = (q->flags & 4u) != 0 ? q->corners[i] : q->rgba;
    }
    quad.flags = ((q->flags & 32u) != 0 ? OPENRAC_UI_SHADED : 0u)
                 | ((q->flags & 16u) != 0 && (q->clamp & 3u) == 0 ? OPENRAC_UI_REPEAT : 0u);
    rac3_ui_adapter_install();
    anchor_at(cursor);
    return openrac_ui_draw_quad(&quad);
}

/* A record to the window through the frame recorder, placed by the list's cursor: through the
 * shared 2D layer in flat colour, or textured when its image is known by the game's tables; else
 * of the older kind (with the chip's TEX0: a texture the tables do not name, the maps' boxes). False
 * when the 2D layer is replayed (the caller sends the game's). */
static int send_quad(const openrac_game_ui_rect* q, gaddr cursor) {
    /* The maps' scissor is not followed yet: their boxes stay the older records. */
    if ((q->flags & 8u) != 0 && flat_native(cursor)) {
        return send_flat(q, cursor);
    }
    if ((q->flags & 8u) == 0 && q->tex0 != 0 && send_textured(q, cursor)) {
        return 1;
    }
    anchor_at(cursor);
    return openrac_frame_ui_quad(q);
}

/* The image of the glyph page whose handles are tex0's (its TBP and CBP fields, the values the
 * print read from the copy's tables), from the current font's header. */
static int glyph_page(unsigned long long tex0, openrac_game_ui_image* out) {
    if (!g_print.fonts_known) {
        return 0;
    }
    const gaddr pages = GREF(gaddr, g_print.fonts.pages);
    const gaddr palettes = GREF(gaddr, g_print.fonts.palettes);
    if (pages == 0 || palettes == 0) {
        return 0;
    }
    const int font = rac3_ui_font_index();
    const int page = rac3_ui_font_page_of(
        font, (uint32_t)(tex0 & 0x3FFFu), (uint32_t)((tex0 >> 37) & 0x3FFFu), pages, palettes
    );
    return page >= 0 && rac3_ui_font_page(font, page, out);
}

/* The 2D layer's scissor (SetScissor: the boot program's func_003A98B8, frontbin's func_003A9500,
 * the levels' copy func_L01_004C5C50): what the game cuts its 2D to, x0 to x1 and y0 to y1
 * inclusive, kept within its screen (the size at +0x150 and +0x152 of 0x001CFEC0). Natively drawn
 * quads are cut to the same, placed where the game's own scissor goes in its list; the game's is
 * still sent, for what the list draws itself. */
static void record_scissor_of(gaddr screen, gaddr cursor, int x0, int x1, int y0, int y1) {
    const int w = GREF(int16_t, screen + 0x150u) - 1;
    const int h = GREF(int16_t, screen + 0x152u) - 1;
    x0 = x0 < 0 ? 0 : x0;
    y0 = y0 < 0 ? 0 : y0;
    x1 = w < x1 ? w : x1;
    y1 = h < y1 ? h : y1;
    const float rect[4] = {(float)x0, (float)y0, (float)(x1 + 1), (float)(y1 + 1)};
    rac3_ui_adapter_install();
    anchor_at(cursor);
    openrac_ui_scissor(rect);
}

static void record_scissor(gaddr cursor, int x0, int x1, int y0, int y1) {
    record_scissor_of(RAC3_ADDR(0x001CFEC0u), cursor, x0, x1, y0, y1);
}

void func_003A98B8(int x0, int x1, int y0, int y1) {
    record_scissor(kBoot.cursor, x0, x1, y0, y1);
    func_003A98B8__game(x0, x1, y0, y1);
}

void func_003A9500__frontbin(int x0, int x1, int y0, int y1) {
    record_scissor(kBoot.cursor, x0, x1, y0, y1);
    func_003A9500__frontbin__game(x0, x1, y0, y1);
}

int func_L01_004C5C50(int x0, int x1, int y0, int y1) {
    record_scissor(rac3_ui2d_level().cursor, x0, x1, y0, y1);
    return func_L01_004C5C50__game(x0, x1, y0, y1);
}

/* OPENRAC_UI_LOG: how the prints' glyphs went to the window, now and then in the log. */
static void note_glyph(int from_page) {
    static int log = -1;
    static unsigned pages = 0, older = 0;
    if (log < 0) {
        log = openrac_env_string("OPENRAC_UI_LOG") != NULL;
    }
    if (!log) {
        return;
    }
    from_page ? ++pages : ++older;
    if ((pages + older) % 2000 == 1) {
        fprintf(
            stderr, "ui2d: glyphs from their font page %u, as older records %u\n", pages, older
        );
    }
}

/* A glyph: the cell (u, v) to (u + w, v + h) texels of tex0's page at the corners c (12.4, with the
 * game's offset), in rgba, or with a colour a corner (corners, else NULL; each triangle in its last
 * corner's, as the packet's PRIM 0x154 is unshaded). From the font's own page image when the
 * print's copy is known, else as the older record. False when the window does not draw it, and the
 * caller sends the game's packet. */
static int record_glyph_corners(
    const int c[4],
    int u,
    int v,
    int w,
    int h,
    uint32_t rgba,
    unsigned long long tex0,
    const uint32_t* corners
) {
    openrac_ui_glyph glyph;
    const int from_page = glyph_page(tex0, &glyph.page);
    note_glyph(from_page);
    if (from_page) {
        const float ofx = (float)(rac3_ui2d_screen_x(0) + 8);
        const float ofy = (float)(rac3_ui2d_screen_y(0) + 8);
        glyph.cell[0] = (float)u;
        glyph.cell[1] = (float)v;
        glyph.cell[2] = (float)(u + w);
        glyph.cell[3] = (float)(v + h);
        glyph.rect[0] = ((float)c[0] - ofx) / 16.0f;
        glyph.rect[1] = ((float)c[1] - ofy) / 16.0f;
        glyph.rect[2] = ((float)c[2] - ofx) / 16.0f;
        glyph.rect[3] = ((float)c[3] - ofy) / 16.0f;
        for (int i = 0; i < 4; ++i) {
            glyph.rgba[i] = corners != NULL ? corners[i] : rgba;
        }
        anchor_at(0);
        if (!openrac_ui_draw_glyph(&glyph)) {
            ++g_print.sent;
            return 0;
        }
        ++g_print.native;
        return 1;
    }
    openrac_game_ui_rect q;
    memset(&q, 0, sizeof(q));
    openrac_ui_rect(&q, c[0], c[1], c[2], c[3], u * 16, v * 16, (u + w) * 16, (v + h) * 16);
    q.rgba = rgba;
    q.tex0 = tex0;
    q.flags = 1u;
    if (corners != NULL) {
        memcpy(q.corners, corners, sizeof(q.corners));
        q.flags |= 4u;
    }
    anchor_at(0);
    if (!openrac_frame_ui_quad(&q)) {
        ++g_print.sent;
        return 0;
    }
    ++g_print.native;
    return 1;
}

static int record_glyph(
    const int c[4], int u, int v, int w, int h, uint32_t rgba, unsigned long long tex0
) {
    return record_glyph_corners(c, u, v, w, h, rgba, tex0, NULL);
}

/* A marker at the list's cursor (the word at `cursor`): an A+D write of `value` to the NOP
 * register. Only for what has no recorder call yet (picture.c's frame copy, P6). */
void rac3_ui2d_marker(gaddr cursor, uint64_t value) {
    const gaddr at = GREF(gaddr, cursor);
    GREF(uint32_t, at + 0) = 0x10000002u;
    GREF(uint32_t, at + 4) = 0;
    GREF(uint32_t, at + 8) = 0;
    GREF(uint32_t, at + 12) = 0x50000002u;
    GREF(uint64_t, at + 16) = 1ull | (1ull << 15) | (1ull << 60);
    GREF(uint64_t, at + 24) = 0xEull;
    GREF(uint64_t, at + 32) = value;
    GREF(uint64_t, at + 40) = 0x7Full;
    GREF(gaddr, cursor) = at + 48;
}

/* After a print whose glyphs were all drawn natively: its packet's header (a cnt tag and VIF
 * DIRECT of 6n + 1 quadwords, then the GIF tag), which the print writes last where the glyphs'
 * loops, none of them written, would have followed it, is taken out; what followed it (the
 * register the print restores) moves up in its place. */
static void drop_print_header(gaddr cursor, gaddr start) {
    const gaddr end = GREF(gaddr, cursor);
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
        return; /* not the packet the print writes */
    }
    const uint32_t rest = end - (h + 32);
    memmove(G(h), G(h + 32), rest);
    GREF(gaddr, cursor) = h + rest;
}

/* A print starts: its glyphs are recorded from here on, read with `fonts` (NULL: as older
 * records). */
typedef struct Print {
    gaddr start;
} Print;

static Print print_begin(gaddr cursor, const FontTables* fonts) {
    const Print p = {GREF(gaddr, cursor)};
    g_print.anchor = p.start;
    g_print.fonts_known = fonts != NULL;
    if (fonts != NULL) {
        g_print.fonts = *fonts;
    }
    g_print.native = 0;
    g_print.sent = 0;
    ++g_glyphs;
    rac3_ui_adapter_install();
    return p;
}

static void print_end(gaddr cursor, Print p) {
    --g_glyphs;
    if (g_print.native > 0 && g_print.sent == 0) {
        drop_print_header(cursor, p.start);
    }
}

void rac3_menu_log_text(float x, float y, gaddr text, int length); /* menu_log.c */

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
    rac3_menu_log_text(a0, a1, (gaddr)a5, a6); /* x, y, the text, its length (-1: to its end) */
    const Print p = print_begin(kBoot.cursor, &kBootFonts);
    func_0038EBC8__game(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
    print_end(kBoot.cursor, p);
}

/* The shaded print: func_0038EBC8's with a colour for each glyph's corner (colours: four RGBAQ
 * doublewords, the corners in the strip's order), its glyphs through func_0038BC30, the same
 * packet. Its prototype is the assembly's: it saves and uses the registers as func_0038EBC8 does,
 * a4 aside, which it reads as a pointer (four words at +0, +8, +0x10, +0x18). */
void func_0038F458(
    float a0,
    float a1,
    float a2,
    float a3,
    int colours, /* the four colours' address */
    int a5,
    int a6,
    int a7,
    int a8,
    unsigned long long a9,
    float a10,
    float a11
) {
    rac3_menu_log_text(a0, a1, (gaddr)a5, a6);
    const Print p = print_begin(kBoot.cursor, &kBootFonts);
    func_0038F458__game(a0, a1, a2, a3, colours, a5, a6, a7, a8, a9, a10, a11);
    print_end(kBoot.cursor, p);
}

/* The level's copies of the two prints (Veldin's 0x00410208 and 0x00410A98). */
void func_L01_00410208(
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
    const gaddr cursor = rac3_ui2d_level().cursor;
    rac3_menu_log_text(a0, a1, (gaddr)a5, a6);
    const FontTables fonts = level_fonts();
    const Print p = print_begin(cursor, &fonts);
    func_L01_00410208__game(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
    print_end(cursor, p);
}

void func_L01_00410A98(
    float a0,
    float a1,
    float a2,
    float a3,
    int colours,
    int a5,
    int a6,
    int a7,
    int a8,
    unsigned long long a9,
    float a10,
    float a11
) {
    const gaddr cursor = rac3_ui2d_level().cursor;
    rac3_menu_log_text(a0, a1, (gaddr)a5, a6);
    const FontTables fonts = level_fonts();
    const Print p = print_begin(cursor, &fonts);
    func_L01_00410A98__game(a0, a1, a2, a3, colours, a5, a6, a7, a8, a9, a10, a11);
    print_end(cursor, p);
}

/* A glyph in one colour: the cell (u, v), w x h texels, at (x, y), fw x fh pixels. Only a glyph of
 * a print comes here natively: elsewhere the game's packet keeps the texels to the cell (a region
 * CLAMP), which a record does not. -1: not drawn natively (the caller runs the game's). */
static int glyph(
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
        return -1;
    }
    if (!glyph_corners(x, y, fw, fh, flag, c)) {
        return 0;
    }
    return record_glyph(c, u, v, w, h, (uint32_t)t0, (unsigned long long)t1) ? 1 : -1;
}

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
    const int r = glyph(u, v, w, h, t0, t1, flag, x, y, fw, fh);
    return r >= 0 ? r : func_0038BA50__game(u, v, w, h, t0, t1, flag, x, y, fw, fh);
}

int func_L01_004064A8(
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
    const int r = glyph(u, v, w, h, t0, t1, flag, x, y, fw, fh);
    return r >= 0 ? r : func_L01_004064A8__game(u, v, w, h, t0, t1, flag, x, y, fw, fh);
}

/* func_0038BA50 with a colour a corner (colours: four RGBAQ doublewords): natively for a glyph,
 * its corners each in their colour (the shaded print's). */
static int glyph_shaded(
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
    int c[4];
    if (g_glyphs <= 0) {
        return -1;
    }
    const uint32_t corners[4] = {
        GREF(uint32_t, colours),
        GREF(uint32_t, colours + 8),
        GREF(uint32_t, colours + 16),
        GREF(uint32_t, colours + 24),
    };
    const int same =
        corners[1] == corners[0] && corners[2] == corners[0] && corners[3] == corners[0];
    if (!glyph_corners(x, y, fw, fh, flag, c)) {
        return 0;
    }
    return record_glyph_corners(
               c, u, v, w, h, corners[0], (unsigned long long)t1, same ? NULL : corners
           )
               ? 1
               : -1;
}

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
    const int r = glyph_shaded(u, v, w, h, colours, t1, flag, x, y, fw, fh);
    return r >= 0 ? r : func_0038BC30__game(u, v, w, h, colours, t1, flag, x, y, fw, fh);
}

int func_L01_00406688(
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
    const int r = glyph_shaded(u, v, w, h, colours, t1, flag, x, y, fw, fh);
    return r >= 0 ? r : func_L01_00406688__game(u, v, w, h, colours, t1, flag, x, y, fw, fh);
}

/* A glyph with its drop shadow: the shadow first, in t1, moved by (dx, dy) pixels, then the glyph
 * in t0, both of the cell (x, y), w x h texels, of the texture t2. */
static int glyph_shadowed(
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
        return -1;
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
    if (!record_glyph(shadow, x, y, w, h, (uint32_t)t1, (unsigned long long)t2)) {
        return -1;
    }
    record_glyph(c, x, y, w, h, (uint32_t)t0, (unsigned long long)t2);
    return 1;
}

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
    const int r = glyph_shadowed(x, y, w, h, t0, t1, t2, snap, sx, sy, sw, sh, dx, dy);
    return r >= 0 ? r : func_0038BE10__game(x, y, w, h, t0, t1, t2, snap, sx, sy, sw, sh, dx, dy);
}

int func_L01_00406868(
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
    const int r = glyph_shadowed(x, y, w, h, t0, t1, t2, snap, sx, sy, sw, sh, dx, dy);
    return r >= 0 ? r
                  : func_L01_00406868__game(x, y, w, h, t0, t1, t2, snap, sx, sy, sw, sh, dx, dy);
}

/* The glyph quad: a w x h screen rectangle at (x, y) textured from (u, v) .. (u + uw, v + vh) of
 * the texture a9 in colour a8, at the far depth. */
static int glyph_quad(
    gaddr cursor,
    int x,
    int y,
    int w,
    int h,
    int u,
    int v,
    int uw,
    int vh,
    long long a8,
    long long a9
) {
    /* Inside a print, a glyph of the font's page (the button icons in a text): from the page. */
    openrac_ui_glyph glyph;
    if (g_glyphs > 0 && glyph_page((unsigned long long)a9, &glyph.page)) {
        glyph.cell[0] = (float)u;
        glyph.cell[1] = (float)v;
        glyph.cell[2] = (float)(u + uw);
        glyph.cell[3] = (float)(v + vh);
        glyph.rect[0] = (float)x - 0.5f;
        glyph.rect[1] = (float)y - 0.5f;
        glyph.rect[2] = (float)(x + w) - 0.5f;
        glyph.rect[3] = (float)(y + h) - 0.5f;
        for (int i = 0; i < 4; ++i) {
            glyph.rgba[i] = (uint32_t)a8;
        }
        anchor_at(cursor);
        return openrac_ui_draw_glyph(&glyph);
    }
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
    return send_quad(&q, cursor);
}

void func_0038B8B0(
    int x, int y, int w, int h, int u, int v, int uw, int vh, long long a8, long long a9
) {
    if (!glyph_quad(kBoot.cursor, x, y, w, h, u, v, uw, vh, a8, a9)) {
        func_0038B8B0__game(x, y, w, h, u, v, uw, vh, a8, a9);
    }
}

void func_L01_00406078(
    int x, int y, int w, int h, int u, int v, int uw, int vh, long long a8, long long a9
) {
    if (!glyph_quad(rac3_ui2d_level().cursor, x, y, w, h, u, v, uw, vh, a8, a9)) {
        func_L01_00406078__game(x, y, w, h, u, v, uw, vh, a8, a9);
    }
}

/* func_0038B8B0 turned by ang (its sine s, cosine c, the program's own) about its centre: the
 * corners centre +- a +- b, a = h/2 (sin, cos), b = w/2 (-cos, sin), as the game sums them. */
static int turned_glyph_quad(
    gaddr cursor,
    int x,
    int y,
    int w,
    int h,
    int u,
    int v,
    int du,
    int dv,
    float s,
    float c,
    long long col,
    long long tex
) {
    const int hw = w >> 1, hh = h >> 1;
    const float cx = (float)(x + hw), cy = (float)(y + hh);
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
    return send_quad(&q, cursor);
}

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
    const float s = func_0038D490(ang), c = func_0038D478(ang);
    if (!turned_glyph_quad(kBoot.cursor, x, y, w, h, u, v, du, dv, s, c, col, tex)) {
        func_0038B500__game(x, y, w, h, u, v, du, dv, ang, col, tex);
    }
}

void func_L01_00405CC8(
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
    const float s = func_L01_0040E910(ang), c = func_L01_0040E8F8(ang);
    if (!turned_glyph_quad(rac3_ui2d_level().cursor, x, y, w, h, u, v, du, dv, s, c, col, tex)) {
        func_L01_00405CC8__game(x, y, w, h, u, v, du, dv, ang, col, tex);
    }
}

/* A quad of tex centred on (px, py), sx by sy, turned by ang (its sine s and cosine c) about the
 * pivot (ax, ay: 0 to 1 of the quad), its w x h texels mirrored by fx, fy, in one colour at depth
 * z: Going Commando's func_0027E4A8, the corners as the game works them out. */
static int rotated_sprite(
    gaddr cursor,
    float px,
    float py,
    float sx,
    float sy,
    float s,
    float c,
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
    const float vx = sy * s, vy = sy * c;
    const float hx = sx * c, hy = -sx * s;
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
    return send_quad(&q, cursor);
}

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
    const float s = func_0038D490(ang), c = func_0038D478(ang);
    if (!rotated_sprite(kBoot.cursor, px, py, sx, sy, s, c, w, h, tex, z, rgba, fx, fy, ax, ay)) {
        func_0038C0B8__game(px, py, sx, sy, ang, w, h, tex, z, rgba, fx, fy, ax, ay);
    }
}

void func_L01_00406B10(
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
    const float s = func_L01_0040E910(ang), c = func_L01_0040E8F8(ang);
    if (!rotated_sprite(
            rac3_ui2d_level().cursor, px, py, sx, sy, s, c, w, h, tex, z, rgba, fx, fy, ax, ay
        )) {
        func_L01_00406B10__game(px, py, sx, sy, ang, w, h, tex, z, rgba, fx, fy, ax, ay);
    }
}

/* A ring's segment: four XYZ2 doublewords, UV words and colours (each triangle in its last
 * corner's colour: the packet's PRIM is unshaded), the texture, and its wrap, clamped when
 * `clamped` (CLAMP_1 5) else repeated: Going Commando's func_0027E920. */
static int ring_segment(
    gaddr cursor, gaddr xyz, gaddr uv, gaddr colours, long long tex0, int clamped
) {
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
    return send_quad(&q, cursor);
}

void func_0038C518(gaddr xyz, gaddr uv, gaddr colours, long long tex0, int clamped) {
    if (!ring_segment(kBoot.cursor, xyz, uv, colours, tex0, clamped)) {
        func_0038C518__game(xyz, uv, colours, tex0, clamped);
    }
}

void func_L01_00406F70(gaddr xyz, gaddr uv, gaddr colours, long long tex0, int clamped) {
    if (!ring_segment(rac3_ui2d_level().cursor, xyz, uv, colours, tex0, clamped)) {
        func_L01_00406F70__game(xyz, uv, colours, tex0, clamped);
    }
}

/* A box shaded from one colour on its left to another on its right (func_0038C6F0's: the menus'
 * separators and the highlighted row): four XYZ2 doublewords, sent in the strip order 0, 2, 1, 3,
 * and two colours, the first for corners 0 and 2, untextured: Going Commando's func_0027EA28. */
static int shaded_box(gaddr cursor, gaddr xyz, gaddr colours) {
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
    return send_quad(&q, cursor);
}

void func_0038C628(gaddr xyz, gaddr colours) {
    if (!shaded_box(kBoot.cursor, xyz, colours)) {
        func_0038C628__game(xyz, colours);
    }
}

void func_L01_00407080(gaddr xyz, gaddr colours) {
    if (!shaded_box(rac3_ui2d_level().cursor, xyz, colours)) {
        func_L01_00407080__game(xyz, colours);
    }
}

/* A box of flat colour, rows y0 to y1 and columns x0 to x1 in screen pixels, at the far depth:
 * Going Commando's func_0027DE50. */
static int flat_box(gaddr cursor, int y0, int y1, int x0, int x1, unsigned long long c) {
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
    return send_quad(&q, cursor);
}

void func_0038B310(int y0, int y1, int x0, int x1, unsigned long long c) {
    if (!flat_box(kBoot.cursor, y0, y1, x0, x1, c)) {
        func_0038B310__game(y0, y1, x0, x1, c);
    }
}

void func_L01_00405AD8(int y0, int y1, int x0, int x1, unsigned long long c) {
    if (!flat_box(rac3_ui2d_level().cursor, y0, y1, x0, x1, c)) {
        func_L01_00405AD8__game(y0, y1, x0, x1, c);
    }
}

/* The fixed packet func_0038B200 sends (a ref tag of 0x14 quadwords): an A+D tag, TEST_1 and PRIM
 * (an untextured, blended sprite), then a tag of sixteen sprites (two XYZ2 each, from +0x40), the
 * picture in 32-pixel columns, their corners as the front end's start-up set them. The core's: the
 * level's copy (Veldin's 0x004059C8) sends the same. */
#define DARKEN_PACKET RAC3_ADDR(0x00141850u)

/* The picture darkened: (r, g, b, a) over the whole of it (the routine's sixteen sprites span the
 * game's screen), as a fill of the whole window, so a wider window darkens to its sides: the main
 * menu's 0x60 of black over its background, the pause menu's over the world, and the fade between
 * the boot program's screens (func_003BCC10's, black at the fade's level 0x001D8CF0 times 128). No
 * packet of the game's is read. `packet` is the routine's (its sprites' place), unused natively.
 * False when the window does not draw it (the caller runs the game's). */
static int darken(gaddr cursor, gaddr packet, int r, int g, int b, int a) {
    (void)packet;
    if (!openrac_frame_native()) {
        return 0;
    }
    const uint32_t rgba = ((uint32_t)r & 0xFFu) | (((uint32_t)g & 0xFFu) << 8)
                          | (((uint32_t)b & 0xFFu) << 16) | (((uint32_t)a & 0xFFu) << 24);
    anchor_at(cursor);
    openrac_frame_fill(NULL, rgba, OPENRAC_BLEND_MIX | OPENRAC_PICTURE_WHOLE);
    return 1;
}

void func_0038B200(int r, int g, int b, int a) {
    if (!darken(kBoot.cursor, DARKEN_PACKET, r, g, b, a)) {
        func_0038B200__game(r, g, b, a);
    }
}

void func_L01_004059C8(int r, int g, int b, int a) {
    if (!darken(rac3_ui2d_level().cursor, DARKEN_PACKET, r, g, b, a)) {
        func_L01_004059C8__game(r, g, b, a);
    }
}

/* The letterbox (func_0038AD28, the levels' func_L01_004054F0): while its word is set its height
 * climbs a line a frame to 52 (down again when cleared), and it sends two black bars that tall
 * over the screen's top and bottom. The game's routine runs (it keeps the height); what it sent is
 * taken back out of the list, and the bars go to the window through the shared 2D layer as colour
 * alone, from the game's own data: the height (the boot program's 0x001D9BE0, a level's own) and
 * the screen's edges the routine draws them to (0x001D4BD0 left and top, 0x001D4BD8 right and
 * bottom, 12.4, with the game's 2D offset). */
#define LETTERBOX_BYTES 0x80u
#define SCREEN_EDGES RAC3_ADDR(0x001D4BD0u)

static void letterbox_bars(gaddr cursor, gaddr start, gaddr height) {
    const gaddr end = GREF(gaddr, cursor);
    const int n = GREF(int32_t, height);
    if (end - start != LETTERBOX_BYTES || n <= 0) {
        return; /* nothing sent (no bars, or the cutscene setting's off) */
    }
    const float ofx = (float)(rac3_ui2d_screen_x(0) + 8);
    const float ofy = (float)(rac3_ui2d_screen_y(0) + 8);
    const float left = ((float)GREF(int32_t, SCREEN_EDGES) - ofx) / 16.0f;
    const float top = ((float)GREF(int32_t, SCREEN_EDGES + 4u) - ofy) / 16.0f;
    const float right = ((float)GREF(int32_t, SCREEN_EDGES + 8u) - ofx) / 16.0f;
    const float bottom = ((float)GREF(int32_t, SCREEN_EDGES + 12u) - ofy) / 16.0f;
    const float bars[2][4] =
        {{left, top, right, top + (float)n}, {left, bottom - (float)n, right, bottom}};
    const uint32_t black[4] = {0x80000000u, 0x80000000u, 0x80000000u, 0x80000000u};
    GREF(gaddr, cursor) = start;
    rac3_ui_adapter_install();
    for (int i = 0; i < 2; ++i) {
        anchor_at(cursor);
        if (!openrac_ui_draw_rect(NULL, bars[i], NULL, black, OPENRAC_UI_FLAT)) {
            GREF(gaddr, cursor) = end; /* replaying: the game's packet is still there */
            return;
        }
    }
}

void func_0038AD28(void) {
    const gaddr start = GREF(gaddr, kBoot.cursor);
    func_0038AD28__game();
    letterbox_bars(kBoot.cursor, start, RAC3_ADDR(0x001D9BE0u));
}

int func_L01_004054F0(void) {
    const gaddr cursor = rac3_ui2d_level().cursor;
    const gaddr start = GREF(gaddr, cursor);
    const int result = func_L01_004054F0__game();
    letterbox_bars(cursor, start, OPENRAC_LDATA(1, RAC3_LADDR(1, 0x001DF960u)));
    return result;
}

/*
 * A Widget3D sprite from its posed mesh (func_003AA780, frontbin's func_003AA3C8, the levels'
 * func_L01_004CBC60). The game's routine runs: it sets its textures' places, poses the mesh's bones
 * (func_003AA168) and its vertices onto the screen (func_003A08E8, or func_003A0A28 for a packed
 * mesh), each to the scratchpad's 32 bytes from 0x70000000: +0 the colour (the vertex's times the
 * sprite's), +8 and +0xC its S and T (of the texture's size, Q 1), +0x10 its X and Y (12.4, with the
 * game's 2D offset). Its quads are then the mesh's own list: the mesh (+0x2 its quads, +0x6 its
 * textures, +0x14 the list, +0x18 the texture table, +0x1C the textures' data) lists u32 entries, a
 * texture's choice (the top 24 bits all set, the low byte its index) or a quad's four vertices (a
 * byte each, drawn in the order 0, 1, 3, 2); a mesh of no textures starts one entry later and is
 * drawn in its colours alone. A texture (16 bytes: +8 its pixels' and +0xA its palette's offsets in
 * quadwords from the data, +0xC and +0xE its log2 width and height) is 8-bit with a 256-colour
 * palette in the stored order, repeated past its edges. What the routine sent into the list is
 * taken back out unread, and each quad goes to the window through the shared 2D layer, shaded
 * between its corners, from that data alone.
 */
#define WIDGET_VERTICES 0x70000000u

static int widget_corner(gaddr mesh_vertex, float tw, float th, float ofx, float ofy,
                         openrac_ui_quad* quad, int i) {
    const uint32_t xy = GREF(uint32_t, mesh_vertex + 0x10u);
    float s, t;
    memcpy(&s, G(mesh_vertex + 8u), 4);
    memcpy(&t, G(mesh_vertex + 0xCu), 4);
    quad->x[i] = ((float)(xy & 0xFFFFu) - ofx) / 16.0f;
    quad->y[i] = ((float)(xy >> 16) - ofy) / 16.0f;
    quad->u[i] = s * tw;
    quad->v[i] = t * th;
    quad->rgba[i] = GREF(uint32_t, mesh_vertex);
    return 1;
}

static void widget_sprite(gaddr cursor, gaddr start, gaddr mesh) {
    const gaddr end = GREF(gaddr, cursor);
    if (end == start || mesh == 0 || !openrac_frame_native()) {
        return; /* nothing drawn (no colour), or the game's packets are replayed */
    }
    const int vertices = GREF(int16_t, mesh + 0x0u);
    const int quads = GREF(int16_t, mesh + 0x2u);
    const int textures = GREF(int16_t, mesh + 0x6u);
    gaddr list = GREF(gaddr, mesh + 0x14u);
    const gaddr table = GREF(gaddr, mesh + 0x18u);
    const gaddr data = GREF(gaddr, mesh + 0x1Cu);
    if (list == 0 || vertices <= 0 || vertices > 256 || quads <= 0 || quads > 4096) {
        return;
    }
    if (textures == 0) {
        list += 4u;
    }
    const float ofx = (float)(rac3_ui2d_screen_x(0) + 8);
    const float ofy = (float)(rac3_ui2d_screen_y(0) + 8);
    GREF(gaddr, cursor) = start;
    rac3_ui_adapter_install();
    openrac_ui_quad quad;
    memset(&quad, 0, sizeof(quad));
    int texture = -1;
    float tw = 1.0f, th = 1.0f;
    static const int order[4] = {0, 1, 3, 2};
    for (int n = 0; n < quads; list += 4u) {
        const uint32_t w = GREF(uint32_t, list);
        if (textures != 0 && ((int32_t)w >> 8) == -1) {
            const int index = (int)(w & 0xFFu);
            texture = index < textures ? index : -1;
            if (texture >= 0) {
                const gaddr e = table + 16u * (gaddr)texture;
                const int log2_width = GREF(int16_t, e + 0xCu);
                const int log2_height = GREF(int16_t, e + 0xEu);
                if (log2_width < 0 || log2_width > 10 || log2_height < 0 || log2_height > 10) {
                    texture = -1;
                    continue;
                }
                quad.image.pixels = data + 16u * (gaddr)GREF(uint16_t, e + 8u);
                quad.image.palette = data + 16u * (gaddr)GREF(uint16_t, e + 0xAu);
                quad.image.width = (uint16_t)(1u << log2_width);
                quad.image.height = (uint16_t)(1u << log2_height);
                quad.image.bits = 8;
                quad.image.swizzled = 1;
                tw = (float)quad.image.width;
                th = (float)quad.image.height;
            }
            continue;
        }
        ++n;
        if (textures != 0 && texture < 0) {
            continue; /* no texture chosen yet: the game's placeholder, nothing of the mesh's */
        }
        for (int i = 0; i < 4; ++i) {
            const unsigned v = (w >> (8 * order[i])) & 0xFFu;
            widget_corner(WIDGET_VERTICES + 32u * v, tw, th, ofx, ofy, &quad, i);
        }
        quad.flags = OPENRAC_UI_SHADED | (textures == 0 ? OPENRAC_UI_FLAT : OPENRAC_UI_REPEAT);
        anchor_at(cursor);
        openrac_ui_draw_quad(&quad);
    }
}

/* A Widget3D sprite: the game poses it, and its quads go to the window from the posed mesh. */
void func_003AA780(gaddr a0, float a1, float a2, int a3, float a4, gaddr a5) {
    const gaddr start = GREF(gaddr, kBoot.cursor);
    func_003AA780__game(a0, a1, a2, a3, a4, a5);
    widget_sprite(kBoot.cursor, start, a0);
}

void func_L01_004CBC60(gaddr a0, float a1, float a2, int a3, float a4, gaddr a5) {
    const gaddr cursor = rac3_ui2d_level().cursor;
    const gaddr start = GREF(gaddr, cursor);
    func_L01_004CBC60__game(a0, a1, a2, a3, a4, a5);
    widget_sprite(cursor, start, a0);
}

/* A HUD texture (by its number, tex0 its TEX0) as a quad centred on (x, y), w and h its half
 * sizes, turned by ang (its sine s and cosine c), a colour a corner (colours: four RGBA words),
 * its whole texture in texels (the packet's PRIM 0x15C: shaded, blended, UV), at the far depth: the
 * menus' corner ornaments. The corners as the game works them out (the VU0's sums: c - a + b,
 * c - a - b, c + a + b, c + a - b with a = h (sin, cos), b = w (-cos, sin)), cut to whole pixels,
 * then 12.4 with the 2D offset. */
static int hud_sprite_turned(
    const Rac3Ui2d* u,
    int tex,
    unsigned long long tex0,
    gaddr colours,
    float x,
    float y,
    float w,
    float h,
    float s,
    float c
) {
    int tw, th;
    rac3_ui2d_hud_size(u, tex, &tw, &th);
    const float ax = h * s, ay = h * c;
    const float bx = -w * c, by = w * s;
    const float px[4] = {(x - ax) + bx, (x - ax) - bx, (x + ax) + bx, (x + ax) - bx};
    const float py[4] = {(y - ay) + by, (y - ay) - by, (y + ay) + by, (y + ay) - by};
    const int us[4] = {0, tw << 4, 0, tw << 4}, vs[4] = {0, 0, th << 4, th << 4};
    /* The image by the HUD bank's own tables (rac3_hud_image), as the game sent it: no TEX0. The
     * multiplayer executable's are not kept at their sending yet. */
    openrac_game_ui_image image;
    if (rac3_exec_program() != RAC3_EXEC_MULTIPLAYER
        && rac3_hud_image(u->hud_entries, tex, &image)) {
        openrac_ui_quad quad;
        memset(&quad, 0, sizeof(quad));
        quad.image = image;
        for (int i = 0; i < 4; ++i) {
            quad.x[i] = (float)(int)px[i] - 0.5f;
            quad.y[i] = (float)(int)py[i] - 0.5f;
            quad.u[i] = (float)(us[i] >> 4);
            quad.v[i] = (float)(vs[i] >> 4);
            quad.rgba[i] = GREF(uint32_t, colours + 4u * (gaddr)i);
        }
        quad.flags = OPENRAC_UI_SHADED;
        rac3_ui_adapter_install();
        anchor_at(u->cursor);
        if (openrac_ui_draw_quad(&quad)) {
            return 1;
        }
    }
    openrac_game_ui_rect q;
    memset(&q, 0, sizeof(q));
    for (int i = 0; i < 4; ++i) {
        q.x[i] = (uint32_t)to_screen_x((int)px[i] << 4) & 0xFFFFu;
        q.y[i] = (uint32_t)to_screen_y((int)py[i] << 4) & 0xFFFFu;
        q.u[i] = (uint32_t)us[i];
        q.v[i] = (uint32_t)vs[i];
        q.corners[i] = GREF(uint32_t, colours + 4u * (gaddr)i);
    }
    q.z = FAR_Z;
    q.rgba = q.corners[3];
    q.tex0 = tex0;
    q.flags = 4u | 32u; /* a colour a corner, shaded */
    return send_quad(&q, u->cursor);
}

void func_00397680(int tex, gaddr colours, float x, float y, float w, float h, float ang) {
    const float s = func_0038D490(ang), c = func_0038D478(ang);
    const unsigned long long tex0 = (unsigned long long)func_00396E18(tex);
    if (!hud_sprite_turned(&kBoot, tex, tex0, colours, x, y, w, h, s, c)) {
        func_00397680__game(tex, colours, x, y, w, h, ang);
    }
}

void func_L01_00420AE8(int tex, gaddr colours, float x, float y, float w, float h, float ang) {
    const Rac3Ui2d u = rac3_ui2d_level();
    const float s = func_L01_0040E910(ang), c = func_L01_0040E8F8(ang);
    const unsigned long long tex0 = (unsigned long long)func_L01_00420080(tex);
    if (!hud_sprite_turned(&u, tex, tex0, colours, x, y, w, h, s, c)) {
        func_L01_00420AE8__game(tex, colours, x, y, w, h, ang);
    }
}

/* A HUD texture's quad turned about its centre (x, y), w and h its half sizes in 12.4 pixels, ang
 * its turn (its sine s and cosine c), tw x th texels of tex0 mirrored left to right, in grey 0x7F
 * at the HUD's depth z: the boot program's 0x00397A18 (frontbin's func_00397638, whose matched C
 * builds the same packet: TEX0, PRIM 0x154, one colour, four corners of UV and XYZ2, a strip).
 * The corners as the VU0 sums them, c + a - b, c + a + b, c - a - b, c - a + b, with a = h (sin,
 * cos) and b = w (-cos, sin), cut to whole sixteenths. The saving prompt's turning icon in a level
 * (func_L01_003EB9C8: HUD texture 0x755D's second frame over its first, the card) is one. */
static int hud_quad_turned(
    gaddr cursor,
    float x,
    float y,
    float w,
    float h,
    float s,
    float c,
    int tw,
    int th,
    unsigned long long tex0,
    uint32_t z
) {
    const float ax = h * s, ay = h * c;
    const float bx = -w * c, by = w * s;
    const float px[4] = {(x + ax) - bx, (x + ax) + bx, (x - ax) - bx, (x - ax) + bx};
    const float py[4] = {(y + ay) - by, (y + ay) + by, (y - ay) - by, (y - ay) + by};
    const int us[4] = {tw << 4, tw << 4, 0, 0}, vs[4] = {0, th << 4, 0, th << 4};
    openrac_game_ui_rect q;
    memset(&q, 0, sizeof(q));
    for (int i = 0; i < 4; ++i) {
        q.x[i] = (uint32_t)to_screen_x((int)px[i]) & 0xFFFFu;
        q.y[i] = (uint32_t)to_screen_y((int)py[i]) & 0xFFFFu;
        q.u[i] = (uint32_t)us[i];
        q.v[i] = (uint32_t)vs[i];
    }
    q.z = z;
    q.rgba = 0x807F7F7Fu;
    q.tex0 = tex0;
    return send_quad(&q, cursor);
}

void func_00397A18(float x, float y, float w, float h, float ang, int tw, int th, long long tex0) {
    const float s = func_0038D490(ang), c = func_0038D478(ang);
    const uint32_t z = GREF(uint32_t, RAC3_ADDR(0x001D9E94u));
    if (!hud_quad_turned(kBoot.cursor, x, y, w, h, s, c, tw, th, (unsigned long long)tex0, z)) {
        func_00397A18__game(x, y, w, h, ang, tw, th, tex0);
    }
}

/* The levels' copy (frontbin's catalogue row func_00397638). The TEX0 is the caller's whole 64-bit
 * register, as the console passes it on (asm.signatures; the callers' func_L01_00420080 is
 * "returns_64"): read as an int, its upper word (TH's high bits, CBP) was the sign of the lower
 * one, the icon was sampled from a 64 x 16384 texture of no palette, and the saving prompt showed
 * no turning icon. Its float result is the sine it last asked for, which the callers' $f0 holds. */
float func_L01_00421278(
    int tw, int th, long long tex0, float x, float y, float w, float h, float ang
) {
    const Rac3Ui2d u = rac3_ui2d_level();
    const float s = func_L01_0040E910(ang), c = func_L01_0040E8F8(ang);
    const uint32_t z = GREF(uint32_t, u.hud_depth);
    if (!hud_quad_turned(u.cursor, x, y, w, h, s, c, tw, th, (unsigned long long)tex0, z)) {
        return func_L01_00421278__game(tw, th, tex0, x, y, w, h, ang);
    }
    return s;
}

/* The multiplayer executable's 2D layer (I5BOOTN.ELF, mp.c): its own program's (0x0027xxxx, which
 * its loading screen draws with) and level 39's, the multiplayer menus' (frontbin's routines at the
 * catalogue's level 39 places), each paired with the boot program's instruction by instruction
 * (mp/libraries.tsv), drawn as the boot program's are. The core's display list cursor and darkening
 * packet are 0x002446A8 and 0x001A3800, level 39's cursor 0x0024CAD0. */
#define MP_CURSOR RAC3_MADDR(0x002446A8u)
#define MP_MENUS_CURSOR RAC3_LADDR(39, 0x0024CAD0u)
#define MP_DARKEN_PACKET RAC3_MADDR(0x001A3800u)

/* The multiplayer executable's SetScissor, its core's and level 39's (the screen's size at
 * 0x00230150 in both). */
int mp_func_00299218(int x0, int x1, int y0, int y1) {
    record_scissor_of(RAC3_MADDR(0x00230000u), MP_CURSOR, x0, x1, y0, y1);
    return mp_func_00299218__game(x0, x1, y0, y1);
}

int mp_func_L39_005FD128(int x0, int x1, int y0, int y1) {
    record_scissor_of(RAC3_MADDR(0x00230000u), MP_MENUS_CURSOR, x0, x1, y0, y1);
    return mp_func_L39_005FD128__game(x0, x1, y0, y1);
}

void mp_func_00281240(
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
    rac3_menu_log_text(a0, a1, (gaddr)a5, a6);
    const Print p = print_begin(MP_CURSOR, &kMpCoreFonts);
    mp_func_00281240__game(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
    print_end(MP_CURSOR, p);
}

void mp_func_L39_00592238(
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
    rac3_menu_log_text(a0, a1, (gaddr)a5, a6);
    const Print p = print_begin(MP_MENUS_CURSOR, &kMpMenuFonts);
    mp_func_L39_00592238__game(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
    print_end(MP_MENUS_CURSOR, p);
}

#define MP_GLYPH(name)                                                                             \
    int name(                                                                                      \
        int u,                                                                                     \
        int v,                                                                                     \
        int w,                                                                                     \
        int h,                                                                                     \
        long long t0,                                                                              \
        long long t1,                                                                              \
        int flag,                                                                                  \
        float x,                                                                                   \
        float y,                                                                                   \
        float fw,                                                                                  \
        float fh                                                                                   \
    ) {                                                                                            \
        const int r = glyph(u, v, w, h, t0, t1, flag, x, y, fw, fh);                               \
        return r >= 0 ? r : name##__game(u, v, w, h, t0, t1, flag, x, y, fw, fh);                  \
    }
MP_GLYPH(mp_func_0027F7A0)

int mp_func_0027F980(
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
    const int r = glyph_shaded(u, v, w, h, colours, t1, flag, x, y, fw, fh);
    return r >= 0 ? r : mp_func_0027F980__game(u, v, w, h, colours, t1, flag, x, y, fw, fh);
}

int mp_func_L39_0058BD90(
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
    const int r = glyph_shaded(u, v, w, h, colours, t1, flag, x, y, fw, fh);
    return r >= 0 ? r : mp_func_L39_0058BD90__game(u, v, w, h, colours, t1, flag, x, y, fw, fh);
}

void mp_func_L39_00592AC8(
    float a0,
    float a1,
    float a2,
    float a3,
    int colours,
    int a5,
    int a6,
    int a7,
    int a8,
    unsigned long long a9,
    float a10,
    float a11
) {
    rac3_menu_log_text(a0, a1, (gaddr)a5, a6);
    const Print p = print_begin(MP_MENUS_CURSOR, &kMpMenuFonts);
    mp_func_L39_00592AC8__game(a0, a1, a2, a3, colours, a5, a6, a7, a8, a9, a10, a11);
    print_end(MP_MENUS_CURSOR, p);
}

#define MP_GLYPH_SHADOWED(name)                                                                    \
    int name(                                                                                      \
        int x,                                                                                     \
        int y,                                                                                     \
        int w,                                                                                     \
        int h,                                                                                     \
        long long t0,                                                                              \
        long long t1,                                                                              \
        long long t2,                                                                              \
        int snap,                                                                                  \
        float sx,                                                                                  \
        float sy,                                                                                  \
        float sw,                                                                                  \
        float sh,                                                                                  \
        float dx,                                                                                  \
        float dy                                                                                   \
    ) {                                                                                            \
        const int r = glyph_shadowed(x, y, w, h, t0, t1, t2, snap, sx, sy, sw, sh, dx, dy);        \
        return r >= 0 ? r : name##__game(x, y, w, h, t0, t1, t2, snap, sx, sy, sw, sh, dx, dy);    \
    }
MP_GLYPH_SHADOWED(mp_func_0027FB60)

#define MP_GLYPH_QUAD(name, cursor)                                                                \
    void name(                                                                                     \
        int x, int y, int w, int h, int u, int v, int uw, int vh, long long a8, long long a9       \
    ) {                                                                                            \
        if (!glyph_quad(cursor, x, y, w, h, u, v, uw, vh, a8, a9)) {                               \
            name##__game(x, y, w, h, u, v, uw, vh, a8, a9);                                        \
        }                                                                                          \
    }
MP_GLYPH_QUAD(mp_func_0027F458, MP_CURSOR)
MP_GLYPH_QUAD(mp_func_L39_0058B868, MP_MENUS_CURSOR)

/* The multiplayer executable's own glyph quad with its place in floats (the core's 0x0027F5E8,
 * level 39's 0x0058B9F8, the maps' copies; the boot program has none): the texels (u, v) and their
 * size, the colour and the TEX0, then x, y, w and h in pixels, scaled by 16 and cut to whole
 * sixteenths as glyph_quad's are. It draws the map loading screen's weapon icons; from the game's
 * packets they drew with no texture. */
#define MP_FLOAT_GLYPH_QUAD(name, cursor)                                                          \
    void name(                                                                                     \
        int u,                                                                                     \
        int v,                                                                                     \
        int uw,                                                                                    \
        int vh,                                                                                    \
        long long rgba,                                                                            \
        long long tex0,                                                                            \
        float x,                                                                                   \
        float y,                                                                                   \
        float w,                                                                                   \
        float h                                                                                    \
    ) {                                                                                            \
        openrac_game_ui_rect q;                                                                    \
        memset(&q, 0, sizeof(q));                                                                  \
        openrac_ui_rect(                                                                           \
            &q,                                                                                    \
            to_screen_x((int)(x * 16.0f)),                                                         \
            to_screen_y((int)(y * 16.0f)),                                                         \
            to_screen_x((int)((x + w) * 16.0f)),                                                   \
            to_screen_y((int)((y + h) * 16.0f)),                                                   \
            u * 16,                                                                                \
            v * 16,                                                                                \
            (u + uw) * 16,                                                                         \
            (v + vh) * 16                                                                          \
        );                                                                                         \
        q.z = FAR_Z;                                                                               \
        q.rgba = (uint32_t)rgba;                                                                   \
        q.tex0 = (unsigned long long)tex0;                                                         \
        q.flags = g_glyphs > 0 ? 1u : 0u;                                                          \
        if (!send_quad(&q, cursor)) {                                                              \
            name##__game(u, v, uw, vh, rgba, tex0, x, y, w, h);                                    \
        }                                                                                          \
    }
MP_FLOAT_GLYPH_QUAD(mp_func_0027F5E8, MP_CURSOR)
MP_FLOAT_GLYPH_QUAD(mp_func_L39_0058B9F8, MP_MENUS_CURSOR)

#define MP_TURNED_GLYPH_QUAD(name, cursor, sine, cosine)                                           \
    void name(                                                                                     \
        int x,                                                                                     \
        int y,                                                                                     \
        int w,                                                                                     \
        int h,                                                                                     \
        int u,                                                                                     \
        int v,                                                                                     \
        int du,                                                                                    \
        int dv,                                                                                    \
        float ang,                                                                                 \
        long long col,                                                                             \
        long long tex                                                                              \
    ) {                                                                                            \
        const float s = sine(ang), c = cosine(ang);                                                \
        if (!turned_glyph_quad(cursor, x, y, w, h, u, v, du, dv, s, c, col, tex)) {                \
            name##__game(x, y, w, h, u, v, du, dv, ang, col, tex);                                 \
        }                                                                                          \
    }
MP_TURNED_GLYPH_QUAD(mp_func_0027F0A8, MP_CURSOR, mp_func_002804B0, mp_func_00280498)
MP_TURNED_GLYPH_QUAD(
    mp_func_L39_0058B4B8, MP_MENUS_CURSOR, mp_func_L39_00590A80, mp_func_L39_00590A68
)

void mp_func_0027FE08(gaddr xyz, gaddr colours) {
    if (!shaded_box(MP_CURSOR, xyz, colours)) {
        mp_func_0027FE08__game(xyz, colours);
    }
}

void mp_func_L39_0058C678(gaddr xyz, gaddr colours) {
    if (!shaded_box(MP_MENUS_CURSOR, xyz, colours)) {
        mp_func_L39_0058C678__game(xyz, colours);
    }
}

void mp_func_0027EEB8(int y0, int y1, int x0, int x1, unsigned long long c) {
    if (!flat_box(MP_CURSOR, y0, y1, x0, x1, c)) {
        mp_func_0027EEB8__game(y0, y1, x0, x1, c);
    }
}

void mp_func_L39_0058B2C8(int y0, int y1, int x0, int x1, unsigned long long c) {
    if (!flat_box(MP_MENUS_CURSOR, y0, y1, x0, x1, c)) {
        mp_func_L39_0058B2C8__game(y0, y1, x0, x1, c);
    }
}

void mp_func_0027EDA8(int r, int g, int b, int a) {
    if (!darken(MP_CURSOR, MP_DARKEN_PACKET, r, g, b, a)) {
        mp_func_0027EDA8__game(r, g, b, a);
    }
}

void mp_func_L39_0058B1B8(int r, int g, int b, int a) {
    if (!darken(MP_MENUS_CURSOR, MP_DARKEN_PACKET, r, g, b, a)) {
        mp_func_L39_0058B1B8__game(r, g, b, a);
    }
}

/* The turned HUD sprite with a colour a corner: the core's (0x00284F80) and level 39's
 * (0x0059CFD0) are the boot program's func_00397680 instruction for instruction, their own HUD
 * texture tables aside (the core's at 0x00244428 and 0x0024442C, level 39's at 0x0024BD00 and
 * 0x0024BD04) and, as the boot program's, no depth of their own. */
static const Rac3Ui2d kMpCore = {MP_CURSOR, RAC3_MADDR(0x00244428u), RAC3_MADDR(0x0024442Cu), 0};
static const Rac3Ui2d kMpMenus =
    {MP_MENUS_CURSOR, RAC3_LADDR(39, 0x0024BD00u), RAC3_LADDR(39, 0x0024BD04u), 0};

void mp_func_00284F80(int tex, gaddr colours, float x, float y, float w, float h, float ang) {
    const float s = mp_func_002804B0(ang), c = mp_func_00280498(ang);
    const unsigned long long tex0 = mp_func_00284D30(tex);
    if (!hud_sprite_turned(&kMpCore, tex, tex0, colours, x, y, w, h, s, c)) {
        mp_func_00284F80__game(tex, colours, x, y, w, h, ang);
    }
}

void mp_func_L39_0059CFD0(int tex, gaddr colours, float x, float y, float w, float h, float ang) {
    const float s = mp_func_L39_00590A80(ang), c = mp_func_L39_00590A68(ang);
    const unsigned long long tex0 = mp_func_L39_0059C910(tex);
    if (!hud_sprite_turned(&kMpMenus, tex, tex0, colours, x, y, w, h, s, c)) {
        mp_func_L39_0059CFD0__game(tex, colours, x, y, w, h, ang);
    }
}

/* The core's turned HUD quad (0x00285318, the boot program's func_00397A18 instruction for
 * instruction, its HUD depth word 0x00244414 where the boot program's is 0x001D9E94): the map
 * loading screen's weapon icons, which the game's packets drew with no texture. */
void mp_func_00285318(
    float x, float y, float w, float h, float ang, int tw, int th, long long tex0
) {
    const float s = mp_func_002804B0(ang), c = mp_func_00280498(ang);
    const uint32_t z = GREF(uint32_t, RAC3_MADDR(0x00244414u));
    if (!hud_quad_turned(MP_CURSOR, x, y, w, h, s, c, tw, th, (unsigned long long)tex0, z)) {
        mp_func_00285318__game(x, y, w, h, ang, tw, th, tex0);
    }
}

/* Level 39's one-colour and shadowed glyphs, inside its prints. */
MP_GLYPH(mp_func_L39_0058BBB0)
MP_GLYPH_SHADOWED(mp_func_L39_0058BF70)

/* The multiplayer maps' copies of level 39's 2D layer (mp/maps.py, host/mp_maps.h: each map's place
 * of each), drawn as level 39's: its cursor and HUD tables relocated to the map's (OPENRAC_LDATA(39,
 * ...): the programs the next-program hook keeps), and the HUD's depth word, which the menus leave
 * at 0 and the maps' HUD sets (Veldin's 0x001DFD0C, level 39's 0x0024BCEC). */
#define MP_MAP_CURSOR OPENRAC_LDATA(39, MP_MENUS_CURSOR)

Rac3Ui2d rac3_ui2d_mp_level(void) {
    const Rac3Ui2d u = {
        MP_MAP_CURSOR,
        OPENRAC_LDATA(39, RAC3_LADDR(39, 0x0024BD00u)),
        OPENRAC_LDATA(39, RAC3_LADDR(39, 0x0024BD04u)),
        OPENRAC_LDATA(39, RAC3_LADDR(39, 0x0024BCECu)),
    };
    return u;
}

/* The turned sprite and the turned HUD quad of level 39's copy (the boot program's func_0038C0B8
 * and func_00397A18, as the single-player levels' func_L01_00406B10 and func_L01_00421278). */
#define MP_ROTATED_SPRITE(name, sine, cosine)                                                      \
    void name(                                                                                     \
        float px,                                                                                  \
        float py,                                                                                  \
        float sx,                                                                                  \
        float sy,                                                                                  \
        float ang,                                                                                 \
        int w,                                                                                     \
        int h,                                                                                     \
        long long tex,                                                                             \
        int z,                                                                                     \
        int rgba,                                                                                  \
        int fx,                                                                                    \
        int fy,                                                                                    \
        float ax,                                                                                  \
        float ay                                                                                   \
    ) {                                                                                            \
        const float s = sine(ang), c = cosine(ang);                                                \
        if (!rotated_sprite(                                                                       \
                MP_MAP_CURSOR, px, py, sx, sy, s, c, w, h, tex, z, rgba, fx, fy, ax, ay            \
            )) {                                                                                   \
            name##__game(px, py, sx, sy, ang, w, h, tex, z, rgba, fx, fy, ax, ay);                 \
        }                                                                                          \
    }
#define MP_HUD_QUAD_TURNED(name, sine, cosine)                                                     \
    float name(int tw, int th, long long tex0, float x, float y, float w, float h, float ang) {    \
        const Rac3Ui2d u = rac3_ui2d_mp_level();                                                   \
        const float s = sine(ang), c = cosine(ang);                                                \
        const uint32_t z = GREF(uint32_t, u.hud_depth);                                            \
        if (!hud_quad_turned(u.cursor, x, y, w, h, s, c, tw, th, (unsigned long long)tex0, z)) {   \
            return name##__game(tw, th, tex0, x, y, w, h, ang);                                    \
        }                                                                                          \
        return s;                                                                                  \
    }
MP_ROTATED_SPRITE(mp_func_L39_0058C218, mp_func_L39_00590A80, mp_func_L39_00590A68)
MP_HUD_QUAD_TURNED(mp_func_L39_0059D368, mp_func_L39_00590A80, mp_func_L39_00590A68)

#define MP_MAP(...)
#define MP_MAP_HUD(...)
#define MP_MAP_UI(                                                                                 \
    m,                                                                                             \
    font_print,                                                                                    \
    font_print_shaded,                                                                             \
    glyph_one,                                                                                     \
    glyph_shaded_one,                                                                              \
    glyph_shadowed_one,                                                                            \
    glyph_quad_one,                                                                                \
    turned_glyph_quad_one,                                                                         \
    shaded_box_one,                                                                                \
    darken_one,                                                                                    \
    flat_box_one,                                                                                  \
    hud_sprite_shaded,                                                                             \
    sine,                                                                                          \
    cosine,                                                                                        \
    hud_tex0,                                                                                      \
    rotated_sprite_one,                                                                            \
    hud_quad_turned_one,                                                                           \
    float_glyph_quad_one                                                                           \
)                                                                                                  \
    MP_FLOAT_GLYPH_QUAD(mp_func_L##m##_##float_glyph_quad_one, MP_MAP_CURSOR)                      \
    MP_ROTATED_SPRITE(                                                                             \
        mp_func_L##m##_##rotated_sprite_one, mp_func_L##m##_##sine, mp_func_L##m##_##cosine        \
    )                                                                                              \
    MP_HUD_QUAD_TURNED(                                                                            \
        mp_func_L##m##_##hud_quad_turned_one, mp_func_L##m##_##sine, mp_func_L##m##_##cosine       \
    )                                                                                              \
    void mp_func_L##m##_##font_print(                                                              \
        float a0,                                                                                  \
        float a1,                                                                                  \
        float a2,                                                                                  \
        float a3,                                                                                  \
        int a4,                                                                                    \
        int a5,                                                                                    \
        int a6,                                                                                    \
        int a7,                                                                                    \
        int a8,                                                                                    \
        unsigned long long a9,                                                                     \
        float a10,                                                                                 \
        float a11                                                                                  \
    ) {                                                                                            \
        rac3_menu_log_text(a0, a1, (gaddr)a5, a6);                                                 \
        const gaddr cursor = MP_MAP_CURSOR;                                                        \
        const Print p = print_begin(cursor, &kMpMenuFonts);                                        \
        mp_func_L##m##_##font_print##__game(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);     \
        print_end(cursor, p);                                                                      \
    }                                                                                              \
    void mp_func_L##m##_##font_print_shaded(                                                       \
        float a0,                                                                                  \
        float a1,                                                                                  \
        float a2,                                                                                  \
        float a3,                                                                                  \
        int colours,                                                                               \
        int a5,                                                                                    \
        int a6,                                                                                    \
        int a7,                                                                                    \
        int a8,                                                                                    \
        unsigned long long a9,                                                                     \
        float a10,                                                                                 \
        float a11                                                                                  \
    ) {                                                                                            \
        rac3_menu_log_text(a0, a1, (gaddr)a5, a6);                                                 \
        const gaddr cursor = MP_MAP_CURSOR;                                                        \
        const Print p = print_begin(cursor, &kMpMenuFonts);                                        \
        mp_func_L##m##_##font_print_shaded##__game(                                                \
            a0, a1, a2, a3, colours, a5, a6, a7, a8, a9, a10, a11                                  \
        );                                                                                         \
        print_end(cursor, p);                                                                      \
    }                                                                                              \
    MP_GLYPH(mp_func_L##m##_##glyph_one)                                                           \
    int mp_func_L##m##_##glyph_shaded_one(                                                         \
        int u,                                                                                     \
        int v,                                                                                     \
        int w,                                                                                     \
        int h,                                                                                     \
        gaddr colours,                                                                             \
        long long t1,                                                                              \
        int flag,                                                                                  \
        float x,                                                                                   \
        float y,                                                                                   \
        float fw,                                                                                  \
        float fh                                                                                   \
    ) {                                                                                            \
        const int r = glyph_shaded(u, v, w, h, colours, t1, flag, x, y, fw, fh);                   \
        return r >= 0 ? r                                                                          \
                      : mp_func_L##m##_##glyph_shaded_one##__game(                                 \
                            u, v, w, h, colours, t1, flag, x, y, fw, fh                            \
                        );                                                                         \
    }                                                                                              \
    MP_GLYPH_SHADOWED(mp_func_L##m##_##glyph_shadowed_one)                                         \
    MP_GLYPH_QUAD(mp_func_L##m##_##glyph_quad_one, MP_MAP_CURSOR)                                  \
    MP_TURNED_GLYPH_QUAD(                                                                          \
        mp_func_L##m##_##turned_glyph_quad_one,                                                    \
        MP_MAP_CURSOR,                                                                             \
        mp_func_L##m##_##sine,                                                                     \
        mp_func_L##m##_##cosine                                                                    \
    )                                                                                              \
    void mp_func_L##m##_##shaded_box_one(gaddr xyz, gaddr colours) {                               \
        if (!shaded_box(MP_MAP_CURSOR, xyz, colours)) {                                            \
            mp_func_L##m##_##shaded_box_one##__game(xyz, colours);                                 \
        }                                                                                          \
    }                                                                                              \
    void mp_func_L##m##_##darken_one(int r, int g, int b, int a) {                                 \
        if (!darken(MP_MAP_CURSOR, MP_DARKEN_PACKET, r, g, b, a)) {                                \
            mp_func_L##m##_##darken_one##__game(r, g, b, a);                                       \
        }                                                                                          \
    }                                                                                              \
    void mp_func_L##m##_##flat_box_one(int y0, int y1, int x0, int x1, unsigned long long c) {     \
        if (!flat_box(MP_MAP_CURSOR, y0, y1, x0, x1, c)) {                                         \
            mp_func_L##m##_##flat_box_one##__game(y0, y1, x0, x1, c);                              \
        }                                                                                          \
    }                                                                                              \
    void mp_func_L##m##_##hud_sprite_shaded(                                                       \
        int tex, gaddr colours, float x, float y, float w, float h, float ang                      \
    ) {                                                                                            \
        const float s = mp_func_L##m##_##sine(ang), c = mp_func_L##m##_##cosine(ang);              \
        const unsigned long long tex0 = mp_func_L##m##_##hud_tex0(tex);                            \
        const Rac3Ui2d u = rac3_ui2d_mp_level();                                                   \
        if (!hud_sprite_turned(&u, tex, tex0, colours, x, y, w, h, s, c)) {                        \
            mp_func_L##m##_##hud_sprite_shaded##__game(tex, colours, x, y, w, h, ang);             \
        }                                                                                          \
    }

#define MP_MAP_VU0(...)
#include "mp_maps.h"
#undef MP_MAP
#undef MP_MAP_HUD
#undef MP_MAP_UI
#undef MP_MAP_VU0

/* frontbin's copies of the 2D layer (the program the front end runs after a level: Quit Game, a
 * level's exit to the menus), wrapped as the boot program's and the levels' are. Its globals are the
 * boot program's (the list's cursor 0x001DA050, the HUD tables 0x001D9EA8/0x001D9EAC: kBoot); its
 * routines are at its own addresses, the catalogue's frontbin rows of the levels' copies. Unwrapped,
 * the menus after Quit Game drew their frames and boxes from the game's packets and no text. */
void func_0038AE20__frontbin(int r, int g, int b, int a) {
    if (!darken(kBoot.cursor, DARKEN_PACKET, r, g, b, a)) {
        func_0038AE20__frontbin__game(r, g, b, a);
    }
}

void func_0038AF30__frontbin(int y0, int y1, int x0, int x1, unsigned long long c) {
    if (!flat_box(kBoot.cursor, y0, y1, x0, x1, c)) {
        func_0038AF30__frontbin__game(y0, y1, x0, x1, c);
    }
}

void func_0038B120__frontbin(
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
    const float s = func_0038D0B0__frontbin(ang), c = func_0038D098__frontbin(ang);
    if (!turned_glyph_quad(kBoot.cursor, x, y, w, h, u, v, du, dv, s, c, col, tex)) {
        func_0038B120__frontbin__game(x, y, w, h, u, v, du, dv, ang, col, tex);
    }
}

void func_0038B4D0__frontbin(
    int x, int y, int w, int h, int u, int v, int uw, int vh, long long a8, long long a9
) {
    if (!glyph_quad(kBoot.cursor, x, y, w, h, u, v, uw, vh, a8, a9)) {
        func_0038B4D0__frontbin__game(x, y, w, h, u, v, uw, vh, a8, a9);
    }
}

int func_0038B670__frontbin(
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
    const int r = glyph(u, v, w, h, t0, t1, flag, x, y, fw, fh);
    return r >= 0 ? r : func_0038B670__frontbin__game(u, v, w, h, t0, t1, flag, x, y, fw, fh);
}

int func_0038B850__frontbin(
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
    const int r = glyph_shaded(u, v, w, h, colours, t1, flag, x, y, fw, fh);
    return r >= 0 ? r : func_0038B850__frontbin__game(u, v, w, h, colours, t1, flag, x, y, fw, fh);
}

int func_0038BA30__frontbin(
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
    const int r = glyph_shadowed(x, y, w, h, t0, t1, t2, snap, sx, sy, sw, sh, dx, dy);
    return r >= 0 ? r
                  : func_0038BA30__frontbin__game(
                        x, y, w, h, t0, t1, t2, snap, sx, sy, sw, sh, dx, dy
                    );
}

void func_0038BCD8__frontbin(
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
    const float s = func_0038D0B0__frontbin(ang), c = func_0038D098__frontbin(ang);
    if (!rotated_sprite(kBoot.cursor, px, py, sx, sy, s, c, w, h, tex, z, rgba, fx, fy, ax, ay)) {
        func_0038BCD8__frontbin__game(px, py, sx, sy, ang, w, h, tex, z, rgba, fx, fy, ax, ay);
    }
}

void func_0038C138__frontbin(gaddr xyz, gaddr uv, gaddr colours, long long tex0, int clamped) {
    if (!ring_segment(kBoot.cursor, xyz, uv, colours, tex0, clamped)) {
        func_0038C138__frontbin__game(xyz, uv, colours, tex0, clamped);
    }
}

void func_0038C248__frontbin(gaddr xyz, gaddr colours) {
    if (!shaded_box(kBoot.cursor, xyz, colours)) {
        func_0038C248__frontbin__game(xyz, colours);
    }
}

void func_0038E7E8__frontbin(
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
    const gaddr cursor = kBoot.cursor;
    rac3_menu_log_text(a0, a1, (gaddr)a5, a6);
    const Print p = print_begin(cursor, &kBootFonts);
    func_0038E7E8__frontbin__game(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
    print_end(cursor, p);
}

void func_0038F078__frontbin(
    float a0,
    float a1,
    float a2,
    float a3,
    int colours,
    int a5,
    int a6,
    int a7,
    int a8,
    unsigned long long a9,
    float a10,
    float a11
) {
    const gaddr cursor = kBoot.cursor;
    rac3_menu_log_text(a0, a1, (gaddr)a5, a6);
    const Print p = print_begin(cursor, &kBootFonts);
    func_0038F078__frontbin__game(a0, a1, a2, a3, colours, a5, a6, a7, a8, a9, a10, a11);
    print_end(cursor, p);
}

void func_003972A0__frontbin(
    int tex, gaddr colours, float x, float y, float w, float h, float ang
) {
    const Rac3Ui2d u = kBoot;
    const float s = func_0038D0B0__frontbin(ang), c = func_0038D098__frontbin(ang);
    const unsigned long long tex0 = (unsigned long long)func_00396A38__frontbin(tex);
    if (!hud_sprite_turned(&u, tex, tex0, colours, x, y, w, h, s, c)) {
        func_003972A0__frontbin__game(tex, colours, x, y, w, h, ang);
    }
}

#ifndef RAC3_NTSC
/* NTSC-U leaves this wrap out (rac3-ntsc release.json libraries_left_out): its release view
 * names the frontbin function without the __frontbin suffix. */
void func_003AA3C8__frontbin(gaddr a0, float a1, float a2, int a3, float a4, gaddr a5) {
    const gaddr cursor = kBoot.cursor;
    const gaddr start = GREF(gaddr, cursor);
    func_003AA3C8__frontbin__game(a0, a1, a2, a3, a4, a5);
    widget_sprite(cursor, start, a0);
}
#endif /* RAC3_NTSC */
