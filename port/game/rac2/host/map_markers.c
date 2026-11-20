/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/*
 * The Mapper's crystal and moonstone markers on USA v1.01 and PAL (difference "mapper-markers", T-44).
 *
 * USA v2.00 added them: its map (func_US2_00296810) draws the hero's arrow and then, while the
 * Mapper is owned, func_US2_00298AA8, which pulses a marker on the map over every moby of the
 * classes the level lists: Tabora's crystals (a diamond of screen lines), Grelbin's moonstones
 * (HUD icon 0xE99E). v1.01's map, DrawMapOverlay (func_00295F60), has neither, so players hunt
 * them by hand. This draws v2.00's markers after v1.01's map, as v2.00 does after
 * its arrow: inside the map's clip, with the same rectangle, scale, classes, colours and pulse.
 * The pulse is kept here, not in game memory.
 *
 * PAL's map is v1.01's (src/pal/game/00296870-map-g8.c: the same rectangle and state), so it gets
 * them too. USA v2.00 draws them itself: its RAC2_K(MAP_MARKERS) is 0 (release.json), and the wrap
 * only calls its map.
 *
 * The wrap also notes where the map was drawn, for the mouse to drag and zoom it (ui_quads.h).
 */
#include <math.h>
#include <stdint.h>

#include "game_protos.h"
#include "openrac/game_host.h"
#include "rac2.h"
#include "ui_quads.h"

void func_00295F60__game(int paging, int set_clip);

/* Does this release's map lack the markers (1: v1.01, PAL), or draw them itself (0: v2.00)? */
#define MARKERS RAC2_K(MAP_MARKERS, 1, func_00295F60)

/* The globals (executable.h names). */
#define MAP_INDEX RAC2_ADDR(0x001A79F0u)  /* g_current_map_index */
#define ITEM_OWNED RAC2_ADDR(0x001A7AF8u) /* g_item_owned_flags */
#define ITEM_MAPPER 5
#define SCREEN RAC2_ADDR(0x001A7340u)        /* g_screen_width: width, height */
#define MAP_HIDDEN RAC2_ADDR(0x001A95F0u)    /* g_map_icons_hidden */
#define MAP_SCISSOR_A RAC2_ADDR(0x001A960Cu) /* g_map_scissor_a, b, c, d */
#define MAP_SCISSOR_B RAC2_ADDR(0x001A9610u)
#define MAP_SCISSOR_C RAC2_ADDR(0x001A9614u)
#define MAP_SCISSOR_D RAC2_ADDR(0x001A9618u)
#define MAP_STATE RAC2_ADDR(0x001C4FA0u)  /* g_map_overlay_state */
#define MOBY_FIRST RAC2_ADDR(0x001B18D8u) /* g_moby_context_first, second */
#define MOBY_LAST RAC2_ADDR(0x001B18DCu)

/* g_map_overlay_state's fields (00295F60-map-g8.c). */
#define MAP_ENABLED 0x24u
#define MAP_ZOOM 0xB4u
#define MAP_OFFSET_X 0x108u
#define MAP_OFFSET_Y 0x15Cu
#define MAP_SELECTED 0x234u
#define MAPS 21

/* A moby, as func_US2_00298AA8 reads it. */
#define MOBY_SIZE 0x100u
#define MOBY_X 0x10u
#define MOBY_Y 0x14u
#define MOBY_STATE 0x20u
#define MOBY_CLASS 0xAAu

/* v2.00's tables (D_US2_001A9768 ..): the classes marked on each map and their colour. */
#define TABORA 8
#define GRELBIN 19
static const int k_crystals[] = {2410, 4098, 2244, 0};
static const int k_moonstones[] = {4551, 4710, 3927, 0};
#define CRYSTAL_COLOUR 0x80F060F0u
#define MOONSTONE_COLOUR 0x80F0F060u
#define PULSE_START 0x64
#define PULSE_UP 1
#define PULSE_DOWN 1
#define SPREAD 4
#define SHADOW 1
#define MOONSTONE_ICON 0xE99E

static unsigned char g_alpha = PULSE_START;
static int g_rising;

static void line(int x0, int y0, int x1, int y1, uint32_t colour) {
    func_0028FAB8(x0, y0, x1, y1, (int)colour, 0);
}

/* func_US2_00298AA8, on v1.01's functions (the release's, renamed by its rac2_names.tsv). */
static void draw_markers(float x0, float x1, float y0, float y1, float scale) {
    const int map = GREF(int32_t, MAP_INDEX);
    const int* table;
    uint32_t colour;
    if (map == TABORA) {
        table = k_crystals;
        colour = CRYSTAL_COLOUR;
    } else if (map == GRELBIN) {
        table = k_moonstones;
        colour = MOONSTONE_COLOUR;
    } else {
        return;
    }
    g_alpha = (unsigned char)(g_rising ? g_alpha + PULSE_UP : g_alpha - PULSE_DOWN);
    if (g_alpha < 0x60 || g_alpha > 0xF0) {
        g_rising ^= 1;
        if (g_alpha > 0xF0) {
            g_alpha = 0xF0;
        }
        if (g_alpha < 0x30) {
            g_alpha = 0x30;
        }
    }
    GFRAME(uv, 8);
    const gaddr last = GREF(gaddr, OPENRAC_DATA(MOBY_LAST));
    for (int i = 0; table[i] != 0 && i < 10; i++) {
        for (gaddr m = GREF(gaddr, OPENRAC_DATA(MOBY_FIRST)); m < last; m += MOBY_SIZE) {
            const uint8_t state = GREF(uint8_t, m + MOBY_STATE);
            if (m == 0 || GREF(int16_t, m + MOBY_CLASS) != table[i] || state == 0xFE || state == 0xFD) {
                continue;
            }
            const float mx = GREF(float, m + MOBY_X);
            const float my = GREF(float, m + MOBY_Y);
            if (func_00297218(mx, my, uv, uv + 4u, map) == 0) {
                func_00297FE8(mx, my, uv, uv + 4u, map);
            }
            const float u = x0 + GREF(float, uv) * (x1 - x0);
            const float v = y0 + GREF(float, uv + 4u) * (y1 - y0);
            if (map == TABORA) {
                const int px = (int)u >> 4;
                const uint32_t pulse = (uint32_t)g_alpha << 23;
                int y = ((int)v >> 4) - 8;
                for (int j = 0; j < 3; j++, y++) {
                    line(px - SPREAD - j + SHADOW, y + SHADOW, px + SPREAD + j + SHADOW, y + SHADOW,
                         pulse | 0x80000000u);
                }
                for (int j = 0; j < 5; j++, y++) {
                    line(px - SPREAD + j + SHADOW, y + SHADOW, px + SPREAD - j + SHADOW, y + SHADOW,
                         pulse | 0x80000000u);
                }
                y = ((int)v >> 4) - 8;
                for (int j = 0; j < 3; j++, y++) {
                    line(px - SPREAD - j, y, px + SPREAD + j, y, colour | pulse);
                }
                for (int j = 0; j < 5; j++, y++) {
                    line(px - SPREAD + j, y, px + SPREAD - j, y, colour | pulse);
                }
            } else {
                const float size = scale * 128.0f;
                func_0028F078(func_0028E588(MOONSTONE_ICON, 0), (int)u, (int)v, (int)(size + 24.0f),
                              (int)(size + 24.0f), g_alpha, 0x80000000u);
                func_0028F078(func_0028E588(MOONSTONE_ICON, 0), (int)u, (int)v, (int)size, (int)size,
                              g_alpha, colour);
            }
        }
    }
}

/* The map's state and its selected map, or 0 when none is shown. */
static gaddr shown_map(int* selected) {
    const gaddr state = OPENRAC_DATA(MAP_STATE);
    *selected = GREF(int32_t, state + MAP_SELECTED);
    if (GREF(int32_t, OPENRAC_DATA(MAP_HIDDEN)) != 0 || GREF(int32_t, state + MAP_ENABLED) == 0
        || *selected < 0 || *selected >= MAPS) {
        return 0;
    }
    return state;
}

/* Where the map was last drawn, as fractions of the screen. */
static int g_map_drawn;
static float g_map_box[4];

static void note_map(int set_clip) {
    int selected;
    const float w = (float)GREF(int32_t, SCREEN);
    const float h = (float)GREF(int32_t, SCREEN + 4u);
    if (shown_map(&selected) == 0 || w <= 0.0f || h <= 0.0f) {
        return;
    }
    g_map_box[0] = set_clip ? (float)GREF(int32_t, OPENRAC_DATA(MAP_SCISSOR_A)) / w : 0.0f;
    g_map_box[1] = set_clip ? (float)GREF(int32_t, OPENRAC_DATA(MAP_SCISSOR_B)) / h : 0.0f;
    g_map_box[2] = set_clip ? (float)(GREF(int32_t, OPENRAC_DATA(MAP_SCISSOR_C)) + 1) / w : 1.0f;
    g_map_box[3] = set_clip ? (float)(GREF(int32_t, OPENRAC_DATA(MAP_SCISSOR_D)) + 1) / h : 1.0f;
    g_map_drawn = 1;
}

void openrac_rac2_screen_size(float* width, float* height) {
    const int32_t w = GREF(int32_t, SCREEN);
    const int32_t h = GREF(int32_t, SCREEN + 4u);
    if (w > 0 && h > 0 && w <= 1024 && h <= 1024) {
        *width = (float)w;
        *height = (float)h;
    }
}

int openrac_rac2_take_map(float box[4]) {
    const int drawn = g_map_drawn;
    for (int i = 0; i < 4; ++i) {
        box[i] = g_map_box[i];
    }
    g_map_drawn = 0;
    return drawn;
}

/* A pan moved by `pixels` at `zoom`: a pixel is 16 of the map's units at zoom 1 (DrawMapOverlay
 * draws at zoom * (pan >> 15) sixteenths), kept in the pan's whole range; the game's own update
 * (func_00295D00) clamps it to the map's edges. */
static int32_t panned(int32_t pan, float pixels, float zoom) {
    const double moved = (double)pan - (double)pixels * 16.0 * 32768.0 / (double)zoom;
    return (int32_t)(moved < 0.0 ? 0.0 : moved > 268435456.0 ? 268435456.0 : moved);
}

void openrac_rac2_map_move(float dx, float dy, float notches) {
    int selected;
    const gaddr state = shown_map(&selected);
    if (state == 0) {
        return;
    }
    const gaddr at = 4u * (unsigned)selected;
    float zoom = GREF(float, state + MAP_ZOOM + at);
    if (!(zoom > 0.0f)) {
        return;
    }
    if (notches != 0.0f) {
        zoom *= powf(1.15f, notches);
        zoom = zoom < 0.65f ? 0.65f : zoom > 4.0f ? 4.0f : zoom;
        GREF(float, state + MAP_ZOOM + at) = zoom;
    }
    GREF(int32_t, state + MAP_OFFSET_X + at) =
        panned(GREF(int32_t, state + MAP_OFFSET_X + at), dx * (float)GREF(int32_t, SCREEN), zoom);
    GREF(int32_t, state + MAP_OFFSET_Y + at) = panned(
        GREF(int32_t, state + MAP_OFFSET_Y + at), dy * (float)GREF(int32_t, SCREEN + 4u), zoom
    );
}

/* DrawMapOverlay: v1.01's map, then v2.00's markers where v2.00 draws them. */
void func_00295F60(int paging, int set_clip) {
    func_00295F60__game(paging, set_clip);
    note_map(set_clip);
    if (!MARKERS || GREF(int32_t, OPENRAC_DATA(MAP_HIDDEN)) != 0
        || GREF(uint8_t, ITEM_OWNED + ITEM_MAPPER) == 0) {
        return;
    }
    const gaddr state = OPENRAC_DATA(MAP_STATE);
    const int selected = GREF(int32_t, state + MAP_SELECTED);
    if (GREF(int32_t, state + MAP_ENABLED) == 0 || selected < 0 || selected >= MAPS
        || GREF(int32_t, MAP_INDEX) != selected) {
        return;
    }
    /* The map's rectangle, as DrawMapOverlay works it out. */
    const float zoom = GREF(float, state + MAP_ZOOM + 4u * (unsigned)selected);
    const int ox = GREF(int32_t, state + MAP_OFFSET_X + 4u * (unsigned)selected) >> 15;
    const int oy = GREF(int32_t, state + MAP_OFFSET_Y + 4u * (unsigned)selected) >> 15;
    const int pan_x = (int)(zoom * (float)ox);
    const int pan_y = (int)(zoom * (float)oy);
    const int extent = (int)(zoom * 8192.0f);
    const int rx0 = 0x1000 - pan_x;
    const int ry0 = 0x800 - pan_y;
    const float scale = ((zoom * 4.0f + 10.0f) * 0.75f) / 13.0f;
    if (paging) {
        func_0027C540(0);
    }
    if (set_clip) {
        func_002FC620(GREF(int32_t, OPENRAC_DATA(MAP_SCISSOR_A)), GREF(int32_t, OPENRAC_DATA(MAP_SCISSOR_C)),
                      GREF(int32_t, OPENRAC_DATA(MAP_SCISSOR_B)), GREF(int32_t, OPENRAC_DATA(MAP_SCISSOR_D)));
    }
    draw_markers((float)rx0, (float)(rx0 + extent), (float)ry0, (float)(ry0 + extent), scale);
    func_002FC620(0, GREF(int32_t, SCREEN) - 1, 0, GREF(int32_t, SCREEN + 4u) - 1);
    if (paging) {
        func_0027C660();
    }
}
