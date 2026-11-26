/* SPDX-License-Identifier: GPL-3.0-or-later
 * Going Commando's font prints, passing the font's TEX0 whole.
 *
 * 0x0027C878 (rac1's GetEffectTex, func_001F4868) returns a texture's whole
 * TEX0: TBP0, width and format in the low word, CBP and the palette's load
 * control in the high one (hostgen.json "returns_64"). The decompilation's
 * matching C keeps it in an int before handing it to the glyph builders,
 * which is right on the console (the register holds all 64 bits) but cuts
 * it to 32 here: the labels came out with no palette. These are the same
 * wrappers with the TEX0 as an unsigned long long, as rac1's port does for
 * FontPrintWindow (rac1/host/hand.c, func_001F7560 and func_001F75D0). The
 * builders take it so too (asm.signatures).
 */
#include <stdint.h>
#include <string.h>

#include "gui_texts.h"
#include "openrac/guest.h"
#include "rac2.h"
#include "ui_quads.h"

unsigned long long func_0027C878(int index);
int func_0027F178(gaddr text, int length);
int func_0027F198(gaddr text, int length);
int func_0027F1B8(gaddr text, int length);
void func_0027F2A8(
    int x, int y, long long style, int text, int length, unsigned long long tex0, int table
);
void func_0027FC70(int x, long long y, int text, int length, unsigned long long tex0, int table);
void func_0027F640(
    float x,
    float y,
    unsigned long long colour,
    gaddr text,
    int limit,
    float scale,
    unsigned long long tex0,
    gaddr glyphs
);
int func_00336A40(gaddr object);

/* The fonts' glyph tables: large, small and the third. */
#define FONT_LARGE RAC2_ADDR(0x00263B50u)
#define FONT_SMALL RAC2_ADDR(0x00263EF0u)
#define FONT_THIRD RAC2_ADDR(0x00264290u)

static void record_print(
    int x, int y, uint32_t colour, gaddr text, int length, int width, gaddr table
);

/* Print in the large font (texture 1) and the small one (texture 2). */
void func_0027F538(int x, int y, int style, gaddr text, int length) {
    record_print(
        x, y, (uint32_t)style, text, length, func_0027F178(text, length), OPENRAC_DATA(FONT_LARGE)
    );
    func_0027F2A8(x, y, style, (int)text, length, func_0027C878(1), (int)OPENRAC_DATA(FONT_LARGE));
}

gaddr rac2_save_score_text(gaddr text); /* save_marks.c */

void func_0027F5B8(int x, int y, int style, gaddr text, int length) {
    text = rac2_save_score_text(text);
    record_print(
        x, y, (uint32_t)style, text, length, func_0027F198(text, length), OPENRAC_DATA(FONT_SMALL)
    );
    func_0027F2A8(x, y, style, (int)text, length, func_0027C878(2), (int)OPENRAC_DATA(FONT_SMALL));
}

/* Right-aligned (x is the right edge). */
void func_0027F980(int x, int y, int style, gaddr text, int length) {
    x -= func_0027F178(text, length);
    record_print(
        x, y, (uint32_t)style, text, length, func_0027F178(text, length), OPENRAC_DATA(FONT_LARGE)
    );
    func_0027F2A8(x, y, style, (int)text, length, func_0027C878(1), (int)OPENRAC_DATA(FONT_LARGE));
}

void func_0027FA10(int x, int y, int style, gaddr text, int length) {
    x -= func_0027F198(text, length);
    record_print(
        x, y, (uint32_t)style, text, length, func_0027F198(text, length), OPENRAC_DATA(FONT_SMALL)
    );
    func_0027F2A8(x, y, style, (int)text, length, func_0027C878(2), (int)OPENRAC_DATA(FONT_SMALL));
}

/* Centred on x; the left edge is returned. */
int func_0027FAA8(int x, int y, int style, gaddr text, int length) {
    x -= func_0027F178(text, length) >> 1;
    record_print(
        x, y, (uint32_t)style, text, length, func_0027F178(text, length), OPENRAC_DATA(FONT_LARGE)
    );
    func_0027F2A8(x, y, style, (int)text, length, func_0027C878(1), (int)OPENRAC_DATA(FONT_LARGE));
    return x;
}

int func_0027FB40(int x, int y, int style, gaddr text, int length) {
    x -= func_0027F198(text, length) >> 1;
    record_print(
        x, y, (uint32_t)style, text, length, func_0027F198(text, length), OPENRAC_DATA(FONT_SMALL)
    );
    func_0027F2A8(x, y, style, (int)text, length, func_0027C878(2), (int)OPENRAC_DATA(FONT_SMALL));
    return x;
}

int func_0027FBD8(int x, int y, int style, gaddr text, int length) {
    x -= func_0027F1B8(text, length) >> 1;
    record_print(
        x, y, (uint32_t)style, text, length, func_0027F1B8(text, length), OPENRAC_DATA(FONT_THIRD)
    );
    func_0027F2A8(x, y, style, (int)text, length, func_0027C878(3), (int)OPENRAC_DATA(FONT_THIRD));
    return x;
}

/* The same through 0x0027FC70: text wrapped in a box (x is the box, y the colour). Their button
 * icons are found by their glyphs (ui2d.c). */
void func_00280190(int x, int y, gaddr text, int length) {
    openrac_rac2_box_text_begin(OPENRAC_DATA(FONT_LARGE));
    func_0027FC70(x, y, (int)text, length, func_0027C878(1), (int)OPENRAC_DATA(FONT_LARGE));
    openrac_rac2_box_text_end();
}

void func_00280200(int x, int y, gaddr text, int length) {
    openrac_rac2_box_text_begin(OPENRAC_DATA(FONT_SMALL));
    func_0027FC70(x, y, (int)text, length, func_0027C878(2), (int)OPENRAC_DATA(FONT_SMALL));
    openrac_rac2_box_text_end();
}

void func_00280270(int x, int y, gaddr text, int length) {
    openrac_rac2_box_text_begin(OPENRAC_DATA(FONT_THIRD));
    func_0027FC70(x, y, (int)text, length, func_0027C878(3), (int)OPENRAC_DATA(FONT_THIRD));
    openrac_rac2_box_text_end();
}

static float f32_at(gaddr a) {
    float v;
    memcpy(&v, G(a), sizeof v);
    return v;
}

static int32_t s32_at(gaddr a) {
    int32_t v;
    memcpy(&v, G(a), sizeof v);
    return v;
}

/* The texts drawn since the keyboard and mouse last took them (gui_texts.h). */
static openrac_rac2_gui_text g_texts[OPENRAC_RAC2_GUI_TEXTS_MAX];
static int g_text_count = 0;

int openrac_rac2_take_gui_texts(openrac_rac2_gui_text* out) {
    const int n = g_text_count;
    memcpy(out, g_texts, sizeof(g_texts[0]) * (size_t)n);
    g_text_count = 0;
    return n;
}

/* The cell of a text's first button icon: a character whose glyph (4 bytes in the font's table:
 * u, v, offset, advance) is on row 96 at one of key_prompts.h's cells; -1 for none. */
static int icon_cell(gaddr text, int length, gaddr table) {
    static const int cell_u[10] = {0, 24, 48, 72, 96, 120, 144, 168, 208, 232};
    if (text == 0 || table == 0) {
        return -1;
    }
    const uint8_t* q = (const uint8_t*)G(text);
    for (int i = 0; q[i] != 0 && i < 256 && (length < 0 || i < length); ++i) {
        if (q[i] >= 0x20) {
            continue;
        }
        const uint8_t* g = (const uint8_t*)G(table + (gaddr)q[i] * 4u);
        for (int c = 0; c < 10 && g[1] == 96; ++c) {
            if (g[0] == cell_u[c]) {
                return c;
            }
        }
    }
    return -1;
}

/* A text of `width` at (x, y) in the font's 16-pixel line at `scale`. */
static void record_text(
    gaddr o, float x, float y, int width, float scale, uint32_t colour, int cell
) {
    if (g_text_count < OPENRAC_RAC2_GUI_TEXTS_MAX) {
        openrac_rac2_gui_text* t = &g_texts[g_text_count++];
        t->element = o;
        t->colour = colour;
        t->x0 = x;
        t->y0 = y;
        t->x1 = x + (float)width;
        t->y1 = y + 16.0f * scale;
        t->cell = cell;
    }
}

/* A print, for a click on it: one with a button icon in it as itself (element 0, in no list), a
 * plain one as an item of a list drawn as prints (element 1, as the cheats' and skill points'). */
static void record_print(
    int x, int y, uint32_t colour, gaddr text, int length, int width, gaddr table
) {
    const int cell = icon_cell(text, length, table);
    record_text(cell >= 0 ? 0u : 1u, (float)x, (float)y, width, 1.0f, colour, cell);
}

/* The GUI's text element draw (0x00336A78, the decompilation's C with the
 * texture whole): nothing when its scale (through +16) or its text (+64) is
 * zero; the x position (through +0) moved left by half or all of the text's
 * width for the alignment at +68; then the string drawer with the y (through
 * +0, +4), the scale (through +4), the colour (through +12), the text, the
 * width, the texture of +56 and the word at +52. */
void func_00336A78(gaddr o) {
    const gaddr scale_at = (gaddr)s32_at(o + 16);
    if (f32_at(scale_at) == 0.0f || s32_at(o + 64) == 0) {
        return;
    }
    const int width = func_00336A40(o);
    const gaddr p0 = (gaddr)s32_at(o);
    const gaddr p1 = (gaddr)s32_at(o + 4);
    float x = f32_at(p0);
    switch (s32_at(o + 68)) {
        case 1:
            x = x - (float)(width >> 1);
            break;
        case 2:
            x = x - (float)width;
            break;
        default:
            break;
    }
    const int colour = s32_at((gaddr)s32_at(o + 12));
    const unsigned long long tex0 = func_0027C878(s32_at(o + 56));
    record_text(
        o,
        x,
        f32_at(p0 + 4),
        width,
        f32_at(p1),
        (uint32_t)colour,
        icon_cell((gaddr)s32_at(o + 64), -1, (gaddr)s32_at(o + 52))
    );
    func_0027F640(
        x,
        f32_at(p0 + 4),
        colour,
        (gaddr)s32_at(o + 64),
        width,
        f32_at(p1),
        tex0,
        (gaddr)s32_at(o + 52)
    );
}
