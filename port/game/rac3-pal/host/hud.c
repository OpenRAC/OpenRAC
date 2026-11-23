/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Up Your Arsenal: the level's HUD, as Going Commando's (rac2/host/hud.c, ui2d.c). Every level's
 * program carries the same HUD code as Going Commando's core, paired instruction by instruction
 * with Veldin's copies (IDENTIFIED.tsv), which hostgen names after Veldin's places:
 *
 * - its sprites, a HUD texture by its number drawn as a sprite (func_0028E868, func_0028ECD8,
 *   func_0028F078 and the gradient func_0028EA58 there), go to the window as records through the
 *   frame recorder (openrac/frame.h), placed by the list's cursor with nothing written into the
 *   list, so nothing of the HUD is drawn from the game's packets;
 * - its 13 slots (0x90 bytes each, Veldin's at 0x002E60D0), whose update (func_L01_0041FE80, Going
 *   Commando's func_0028E458, from DrawFrame) runs each slot's draw: with the player's "HUD: Edges"
 *   the slots anchored left or right are drawn out at the sides of a picture wider than the game's
 *   (game/common/lib/hud_edges.h), what Ratchet & Clank's and Going Commando's ports do.
 */
#include <stdint.h>
#include <string.h>

#include "../../common/lib/hud_edges.h"
#include "../../common/lib/ui_quad.h"
#include "game_protos.h"
#include "mp/game_protos.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "rac3_version.h"
#include "ui2d.h"

static void draw_slots(void) {
    func_L01_0041FE80__game();
}

/* The HUD's slots: Veldin's at 0x002E60D0, the loaded level's through OPENRAC_LDATA. */
void func_L01_0041FE80(void) {
    openrac_hud_edges(OPENRAC_LDATA(1, RAC3_LADDR(1, 0x002E60D0u)), draw_slots);
}

/* A record for one of the HUD's sprites: a sprite at the HUD's depth, (alpha, grey 0x7F). */
static void hud_common(
    const Rac3Ui2d* u, openrac_game_ui_rect* q, unsigned long long tex0, int alpha
) {
    memset(q, 0, sizeof(*q));
    q->flags = 2; /* these routines send sprites */
    q->z = (uint32_t)GREF(int, u->hud_depth);
    q->rgba = ((uint32_t)alpha << 24) | 0x7F7F7Fu;
    q->tex0 = tex0;
}

static void hud_rect(
    openrac_game_ui_rect* q, int x0, int y0, int x1, int y1, int u0, int v0, int u1, int v1
) {
    openrac_ui_rect(
        q,
        rac3_ui2d_screen_x(x0),
        rac3_ui2d_screen_y(y0),
        rac3_ui2d_screen_x(x1),
        rac3_ui2d_screen_y(y1),
        u0,
        v0,
        u1,
        v1
    );
}

static unsigned long long hud_tex0(int tex) {
    return (unsigned long long)func_L01_00420080(tex);
}

/* A HUD texture as a sprite at (x, y), w x h pixels, its whole texture, with alpha (Going
 * Commando's func_0028E868). */
void func_L01_004202A8(int tex, int x, int y, int w, int h, int alpha) {
    const Rac3Ui2d u = rac3_ui2d_level();
    int tw, th;
    openrac_game_ui_rect q;
    rac3_ui2d_hud_size(&u, tex, &tw, &th);
    hud_common(&u, &q, hud_tex0(tex), alpha);
    hud_rect(&q, x * 16, y * 16, (x + w) * 16, (y + h) * 16, 0, 0, tw * 16, th * 16);
    if (!openrac_ui_record(&q, u.cursor)) {
        func_L01_004202A8__game(tex, x, y, w, h, alpha);
    }
}

/* func_L01_004202A8 with one texel a pixel, whatever the texture's size: a bar cut to a length
 * (func_0028ECD8). */
void func_L01_00420720(int tex, int x, int y, int w, int h, int alpha) {
    const Rac3Ui2d u = rac3_ui2d_level();
    openrac_game_ui_rect q;
    hud_common(&u, &q, hud_tex0(tex), alpha);
    hud_rect(&q, x * 16, y * 16, (x + w) * 16, (y + h) * 16, 0, 0, w * 16, h * 16);
    if (!openrac_ui_record(&q, u.cursor)) {
        func_L01_00420720__game(tex, x, y, w, h, alpha);
    }
}

/* A HUD texture at (x, y), w x h in 16ths of a pixel, in a colour (func_0028F078). */
void func_L01_004208E0(int tex, int x, int y, int w, int h, int alpha, unsigned int colour) {
    const Rac3Ui2d u = rac3_ui2d_level();
    int tw, th;
    openrac_game_ui_rect q;
    rac3_ui2d_hud_size(&u, tex, &tw, &th);
    hud_common(&u, &q, hud_tex0(tex), alpha);
    q.rgba = ((uint32_t)alpha << 24) | (colour & 0xFFFFFFu);
    hud_rect(&q, x, y, x + w, y + h, 0, 0, tw * 16, th * 16);
    if (!openrac_ui_record(&q, u.cursor)) {
        func_L01_004208E0__game(tex, x, y, w, h, alpha, colour);
    }
}

/* A HUD texture as a sprite with a colour for each corner (colours: four RGBA words), shaded
 * between them: the HUD's gradient bars (func_0028EA58). */
void func_L01_00420498(int tex, int x, int y, int w, int h, gaddr colours) {
    const Rac3Ui2d u = rac3_ui2d_level();
    int tw, th;
    openrac_game_ui_rect q;
    rac3_ui2d_hud_size(&u, tex, &tw, &th);
    hud_common(&u, &q, hud_tex0(tex), 0x80);
    q.flags = 4u | 32u;
    hud_rect(&q, x * 16, y * 16, (x + w) * 16, (y + h) * 16, 0, 0, tw * 16, th * 16);
    for (int i = 0; i < 4; ++i) {
        q.corners[i] = GREF(uint32_t, colours + 4u * (gaddr)i);
    }
    if (!openrac_ui_record(&q, u.cursor)) {
        func_L01_00420498__game(tex, x, y, w, h, colours);
    }
}

/*
 * The weapon, ammunition, health and bolts are not slots: as Going Commando's func_0034E4F8, Veldin's
 * func_L01_004D24C0 (the same calls one for one: the bolts counter func_L01_004D1700, Going
 * Commando's func_0034DB98; the messages func_L01_004CF120, func_00338F58) updates a tree of
 * widgets with places of their own, from func_L01_004365A0 (func_0029C108). Unlike Going
 * Commando's, it does not draw them: the GUI's screens are drawn once a frame, all of them, by
 * func_L01_00478B10 (the boot program's func_003B3158, frontbin's row; its root at Veldin's
 * 0x001E15E0), the HUD's panels and texts, the help messages and the pause menu's pages. With
 * "HUD: Edges" two markers around that draw have the window move what it draws left or right of the
 * middle out to the picture's sides (renderer::GifInterpreter::kHudSideMarker), as Going
 * Commando's port does around its widgets: the bolts' panel in the top right goes right; the
 * weapon's panel in the top middle, the messages and the pause menu, across the middle, stay.
 *
 * The widget tree's update draws some widgets itself, the quick select's ring among them (its
 * shaded boxes through func_L01_00407080, from func_L01_004CCE80): the same two markers go around
 * it, so the ring, anchored left, goes out to the left with its slot (3, whose texts
 * func_L01_0041FE80 moves) and does not stay at its 4:3 place.
 */
#define HUD_SIDE_MARKER (0x4853ull << 48)

static void side_begin(gaddr cursor, int side) {
    if (side != 0) {
        rac3_ui2d_marker(cursor, HUD_SIDE_MARKER | (uint32_t)side);
    }
}

static void side_end(gaddr cursor, int side) {
    if (side != 0) {
        rac3_ui2d_marker(cursor, HUD_SIDE_MARKER);
    }
}

int func_L01_00478B10(int a0) {
    const gaddr cursor = rac3_ui2d_level().cursor;
    const int side = openrac_game_hud_side();
    side_begin(cursor, side);
    const int result = func_L01_00478B10__game(a0);
    side_end(cursor, side);
    return result;
}

void func_L01_004D24C0(gaddr a0) {
    const gaddr cursor = rac3_ui2d_level().cursor;
    const int side = openrac_game_hud_side();
    side_begin(cursor, side);
    func_L01_004D24C0__game(a0);
    side_end(cursor, side);
}

/* The multiplayer executable's level 39 (the menus, the map's loading screen: the weapons' icons)
 * and the maps (mp/maps.py, host/mp_maps.h): their copies of DrawHudSprite and DrawHudGradient,
 * drawn as the single-player levels' above with level 39's tables relocated to the program loaded
 * (rac3_ui2d_mp_level). Unwrapped, their packets reached the window with no texture. */
#define MP_HUD(sprite_fn, gradient_fn, tex0_fn)                                                    \
    void sprite_fn(int tex, int x, int y, int w, int h, int alpha) {                               \
        const Rac3Ui2d u = rac3_ui2d_mp_level();                                                   \
        int tw, th;                                                                                \
        openrac_game_ui_rect q;                                                                    \
        rac3_ui2d_hud_size(&u, tex, &tw, &th);                                                     \
        hud_common(&u, &q, (unsigned long long)tex0_fn(tex), alpha);                               \
        hud_rect(&q, x * 16, y * 16, (x + w) * 16, (y + h) * 16, 0, 0, tw * 16, th * 16);          \
        if (!openrac_ui_record(&q, u.cursor)) {                                                    \
            sprite_fn##__game(tex, x, y, w, h, alpha);                                             \
        }                                                                                          \
    }                                                                                              \
    void gradient_fn(int tex, int x, int y, int w, int h, gaddr colours) {                         \
        const Rac3Ui2d u = rac3_ui2d_mp_level();                                                   \
        int tw, th;                                                                                \
        openrac_game_ui_rect q;                                                                    \
        rac3_ui2d_hud_size(&u, tex, &tw, &th);                                                     \
        hud_common(&u, &q, (unsigned long long)tex0_fn(tex), 0x80);                                \
        q.flags = 4u | 32u;                                                                        \
        hud_rect(&q, x * 16, y * 16, (x + w) * 16, (y + h) * 16, 0, 0, tw * 16, th * 16);          \
        for (int i = 0; i < 4; ++i) {                                                              \
            q.corners[i] = GREF(uint32_t, colours + 4u * (gaddr)i);                                \
        }                                                                                          \
        if (!openrac_ui_record(&q, u.cursor)) {                                                    \
            gradient_fn##__game(tex, x, y, w, h, colours);                                         \
        }                                                                                          \
    }
MP_HUD(mp_func_L39_0059CB38, mp_func_L39_0059CD28, mp_func_L39_0059C910)

#define MP_MAP(...)
#define MP_MAP_UI(...)
#define MP_MAP_HUD(m, sprite, gradient, tex0)                                                      \
    MP_HUD(mp_func_L##m##_##sprite, mp_func_L##m##_##gradient, mp_func_L##m##_##tex0)
#include "mp_maps.h"
#undef MP_MAP
#undef MP_MAP_UI
#undef MP_MAP_HUD
