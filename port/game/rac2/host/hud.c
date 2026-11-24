/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Going Commando: the level HUD in a picture wider than the game's. The same 13 slots as Ratchet &
 * Clank's (0x00255330), whose update func_0028E458 calls once a frame; func_0028B740 turns a slot's
 * place into a corner by the same flags. game/common/lib/hud_edges.h says how "HUD: Edges" moves them.
 *
 * The weapon, ammunition, health and bolts are not slots here: func_0034E4F8 draws them from a tree of
 * widgets with places of their own. With "HUD: Edges" two markers around it have the window move what
 * it draws left or right of the middle out to the picture's sides (renderer::GifInterpreter::
 * kHudSideMarker). What overlaps moves as one element; one across the middle stays, and so does the
 * top middle panel (func_0034AD10).
 */
#include <stdint.h>
#include <string.h>

#include "../../common/lib/hud_edges.h"
#include "game_protos.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "rac2.h"

unsigned long long func_0027C878(int index);
int func_0027F178(gaddr text, int length);
int func_0027F198(gaddr text, int length);
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

/* The mods' lines of text (openrac_game_hud_texts), after the HUD's widgets, in the game's own fonts
 * on its 512 x 416 screen (font.c: the large font is texture 1, the small one texture 2). */
#define FONT_LARGE RAC2_ADDR(0x00263B50u)
#define FONT_SMALL RAC2_ADDR(0x00263EF0u)
#define MOD_LINES 16
#define MOD_LINE_BYTES 64

static gaddr mod_text;

/* The GS colour (0xAABBGGRR): the colour as it comes, the opacity's 0 to 255 to the GS's 0 to 128. */
static unsigned long long gs_colour(uint32_t rgba) {
    const uint32_t alpha = (((rgba >> 24) & 0xFFu) * 128u + 127u) / 255u;
    return (unsigned long long)(((rgba << 8) >> 8) | (alpha << 24));
}

static void draw_line(
    const openrac_game_hud_text* line, gaddr text, float dx, float dy, uint32_t rgba
) {
    const int small = (line->flags & 1u) != 0;
    const float width =
        (float)(small ? func_0027F198(text, -1) : func_0027F178(text, -1)) * line->scale;
    func_0027F640(
        line->x * 512.0f - width / 2.0f + dx,
        line->y * 416.0f - 8.0f * line->scale + dy,
        gs_colour(rgba),
        text,
        -1,
        line->scale,
        func_0027C878(small ? 2 : 1),
        OPENRAC_DATA(small ? FONT_SMALL : FONT_LARGE)
    );
}

static void draw_mod_texts(void) {
    openrac_game_hud_text lines[MOD_LINES];
    int n = openrac_game_hud_texts(lines, MOD_LINES);
    n = n < MOD_LINES ? n : MOD_LINES;
    if (n <= 0) {
        return;
    }
    if (mod_text == 0) {
        mod_text = openrac_guest_static(MOD_LINES * MOD_LINE_BYTES);
    }
    for (int i = 0; i < n; i++) {
        /* The printable characters only: the font has no others. */
        char* out = (char*)G(mod_text + (gaddr)i * MOD_LINE_BYTES);
        int k = 0;
        for (const char* c = lines[i].text; *c != '\0' && k < MOD_LINE_BYTES - 1; ++c) {
            if (*c >= 32 && *c <= 126) {
                out[k++] = *c;
            }
        }
        out[k] = '\0';
        const gaddr text = mod_text + (gaddr)i * MOD_LINE_BYTES;
        if ((lines[i].flags & 2u) != 0) {
            draw_line(&lines[i], text, 1.0f, 1.0f, lines[i].rgba & 0xFF000000u);
        }
        draw_line(&lines[i], text, 0.0f, 0.0f, lines[i].rgba);
    }
}

static void draw_hud(void) {
    func_0028E458__game();
}

void func_0028E458(void) {
    openrac_hud_edges(OPENRAC_DATA(RAC2_ADDR(0x00255330u)), draw_hud);
}

enum {
    REG_NOP = 0x7F
};

static void side_marker(int side) {
    if (side != 0) {
        func_002FC560(REG_NOP, 0x4853ull << 48 | (uint32_t)side);
    }
}

static int widgets_side;

void func_0034E4F8(gaddr object) {
    const int side = openrac_game_hud_side();
    widgets_side = side;
    side_marker(side);
    func_0034E4F8__game(object);
    if (side != 0) {
        func_002FC560(REG_NOP, 0x4853ull << 48);
    }
    widgets_side = 0;
    /* After the widgets, whose textures the fonts' would replace. */
    draw_mod_texts();
}

/*
 * The panel in the top middle, drawn among those widgets: its tip overlaps the health widget's, which
 * would make them one element, left of the middle. It is kept out of what the markers move.
 */
void func_0034AD10(gaddr panel) {
    const int side = widgets_side;
    if (side != 0) {
        func_002FC560(REG_NOP, 0x4853ull << 48);
    }
    func_0034AD10__game(panel);
    side_marker(side);
}

/*
 * The ship levels draw a HUD of their own from a callback (func_0029B758), which is not the widgets
 * above: Feltzin, Hrugis and Gorn. The same markers move it.
 */
/* The name is expanded first, so that a release's table (rac2_version.h) renames it before ## pastes. */
#define SHIP_HUD(name) SHIP_HUD_(name)
#define SHIP_HUD_(name)                                                                            \
    int name(void) {                                                                               \
        const int side = openrac_game_hud_side();                                                  \
        side_marker(side);                                                                         \
        const int result = name##__game();                                                         \
        if (side != 0) {                                                                           \
            func_002FC560(REG_NOP, 0x4853ull << 48);                                               \
        }                                                                                          \
        return result;                                                                             \
    }

SHIP_HUD(func_L05_0041DF58)
SHIP_HUD(func_L10_00421790)
SHIP_HUD(func_L15_0042E1B0)
/* Giant Clank's fights draw theirs the same way: his panel and the enemy's go to the sides. */
SHIP_HUD(func_L22_003A5018)
SHIP_HUD(func_L23_0041DA48)
/*
 * The hoverbike races (Barlow, Joba): before the start, during the race and at the end; and the
 * crane on Endako, a callback of func_0029B7C0, whose camera's frame goes round the screen. With
 * the stretch bit (renderer::GifInterpreter::kHudStretch) what crosses the middle grows out to the
 * sides with the rest.
 */
#define STRETCH (1ull << 32)

/* Takes the game function's own result type: the decomp's matched C may give one (rac2-decomp's
 * func_L04_00411298 is s32) where the assembly's is void. The cleanup restores the plain HUD after
 * the call, whichever it is. */
static inline void openrac_race_hud_end(const int* side) {
    if (*side != 0) {
        func_002FC560(REG_NOP, 0x4853ull << 48);
    }
}

#define RACE_HUD(name)                                                                             \
    __typeof__(name##__game()) name(void) {                                                        \
        const int side __attribute__((cleanup(openrac_race_hud_end))) = openrac_game_hud_side();   \
        if (side != 0) {                                                                           \
            func_002FC560(REG_NOP, 0x4853ull << 48 | STRETCH | (uint32_t)side);                     \
        }                                                                                          \
        return name##__game();                                                                     \
    }

RACE_HUD(func_L04_0040AD28)
RACE_HUD(func_L04_00411298)
RACE_HUD(func_L04_00417308)
RACE_HUD(func_L03_003C7018)

/* Tabora's own HUD while driving the Mining Vehicle, from a callback: its radar, and at the bottom
 * its health bar beside the distance to the raritanium. The two bars are drawn side by side and
 * meet right of the middle (283 of 512): with the split bit (renderer::GifInterpreter::kHudSplit)
 * what lies left of that line goes left as one block and the rest right, each out to its edge,
 * whole (stretched, or each piece by the middle, the bars grew or came apart). */
#define SPLIT_AT(x) (1ull << 33 | (uint64_t)(x) << 34)
#define TABORA_BARS_MEET 283

static inline void openrac_tabora_hud_end(const int* side) {
    if (*side != 0) {
        func_002FC560(REG_NOP, 0x4853ull << 48);
    }
}

int func_L08_0041D808(void) {
    const int side __attribute__((cleanup(openrac_tabora_hud_end))) = openrac_game_hud_side();
    if (side != 0) {
        func_002FC560(REG_NOP, 0x4853ull << 48 | SPLIT_AT(TABORA_BARS_MEET) | (uint32_t)side);
    }
    return func_L08_0041D808__game();
}

/* The meter Tabora and Grelbin draw at the bottom, from a callback: its parts go out to their side. */
SHIP_HUD(func_L08_003DB518)

/* The meter func_00290010 shows for a while (the Thermanator's, on what it freezes or thaws), a
 * callback of func_0029B7C0: drawn right of the middle, it goes out to that side. */
SHIP_HUD(func_002900B8)
