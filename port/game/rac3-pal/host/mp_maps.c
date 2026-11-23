/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal's multiplayer maps (levels 40 to 53 of the multiplayer executable, mp.c): the
 * host C of the rows mp/maps.py writes into mp/libraries.tsv. Each map's program carries its own
 * copy of the engine, translated at its own addresses (func_L<map>_...); the routines the port
 * does not run as the console did (the renderers, VIF0's chain, the WAD decompressor) are level
 * 39's copies, at the catalogue's place of each in each map (host/mp_maps.h). What they do is
 * render.c's for level 39 (kMultiplayer) and the single-player levels; their globals are level 39's,
 * relocated to the loaded map's through the two programs as the game read them (OPENRAC_LDATA(39,
 * ...): the next-program hook keeps them, hostgen.json "level_copies" pairs the copies).
 *
 * Level 39's globals named here were found by pairing Veldin's copies of the same routines with
 * level 39's instruction by instruction (each formed by a lui and its low half in both): the sky's
 * shell matrix 0x0024C940 (Veldin's 0x001E0E10), the particle pool and its highest slot
 * 0x0024C188 / 0x0024C190 (0x001E0278 / 0x001E0280), the particles' texture table 0x002961C0
 * (0x00220A40), the effect textures' upload count and records 0x0024B9FC / 0x0025D9C0
 * (0x001DF97C / 0x001E7880), the screen glow's word 0x00245BB4 (0x001DBCA4), and the view context
 * 0x0025D6C0 (Veldin's 0x001E7580, profile.cpp's). */
#include <stdint.h>

#include "game/common/lib/wad_routine.h"
#include "mp/game_protos.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "rac3_version.h"

/* A global of level 39's program, in the map loaded now. */
#define L39(a) OPENRAC_LDATA(39, (gaddr)(a))

#define MAP_GLOW_WORD RAC3_LADDR(39, 0x00245BB4u)
#define MAP_SKY_MATRIX RAC3_LADDR(39, 0x0024C940u)
#define MAP_PARTICLE_POOL RAC3_LADDR(39, 0x0024C188u)
#define MAP_PARTICLE_HIGHEST RAC3_LADDR(39, 0x0024C190u)
#define MAP_PARTICLE_TEXTURES RAC3_LADDR(39, 0x002961C0u)
#define MAP_TEXTURE_UPLOADS RAC3_LADDR(39, 0x0024B9FCu)
#define MAP_TEXTURE_RECORDS RAC3_LADDR(39, 0x0025D9C0u)
#define MAP_LIST_POINTER RAC3_LADDR(39, 0x0024CAD0u)
/* The view context (UpdateViewContext's block, profile.cpp's view_context): the view's scissor in
 * the screen's pixels at +0x240 (left), +0x244 (right, included), +0x248 (top), +0x24C (bottom,
 * included), which the view layout writes from its table. */
#define MAP_VIEW_CONTEXT RAC3_LADDR(39, 0x0025D6C0u)

/* render.c's level 39 copies and shared steps. */
int func_L39_005F4690(void);
int func_L39_005F61A0(void);
int func_L39_005EDE28(void);
int func_L39_005B25C8(int a0, int a1, int a2, int a3);
int rac3_sky_draw_shell(int shell, gaddr matrix);
int rac3_part_proc(gaddr pool_word, gaddr highest_word, gaddr table);
void rac3_world_quad(gaddr record, gaddr matrix, gaddr uploads, gaddr records, gaddr list);
void func_0012D938(gaddr channel, gaddr tag); /* sceDmaSend (dma.c) */

static inline void kick_vif0_chain(unsigned int chain) {
    func_0012D938(0x10008000u, (gaddr)chain & 0x0FFFFFFFu);
}

static inline int wad_decompress(int source, int destination) {
    return openrac_wad_routine(source, destination, "Up Your Arsenal");
}

/* The screen glow's packets are left out as the single-player levels' are (render.c); the word
 * each sets for the frame is kept. */
static inline int glow_copy(void) {
    GREF(int32_t, L39(MAP_GLOW_WORD)) = 0x6000;
    return 0;
}

static inline int glow_add(void) {
    GREF(int32_t, L39(MAP_GLOW_WORD)) = 0;
    return 0;
}

static inline int moby_chain(int a0, int a1) {
    openrac_game_moby_chain((gaddr)a0, (gaddr)a1);
    return 0;
}

static inline int sky_draw_shell(int shell) {
    return rac3_sky_draw_shell(shell, L39(MAP_SKY_MATRIX));
}

static inline int part_proc(void) {
    return rac3_part_proc(
        L39(MAP_PARTICLE_POOL), L39(MAP_PARTICLE_HIGHEST), L39(MAP_PARTICLE_TEXTURES)
    );
}

static inline int world_quad(int a0, int a1) {
    rac3_world_quad(
        (gaddr)a0,
        (gaddr)a1,
        L39(MAP_TEXTURE_UPLOADS),
        L39(MAP_TEXTURE_RECORDS),
        L39(MAP_LIST_POINTER)
    );
    return 0;
}

/* Split screen. Each frame the maps' view loop (level 39's 0x005771D0, map 50's 0x00454AD0) draws
 * every local player's view: RenderView (0x00576E48, map 50's 0x00454748) keeps the view's player,
 * runs its callback, lays the view out (this routine, with the layout: 1 + the player for two
 * players, 3 + for three, 6 + for four, from the table it reads) and runs DrawFrame with that
 * player's camera; then the loop lays out the whole screen (layout 0) and draws the lines between
 * the views over them. So a layout other than 0 starts a view of the frame recorder, and the next
 * layout ends it, while the game still holds the camera the view was drawn with; the one screen's
 * world otherwise (openrac/frame.h, frontend/split_views.cpp). The whole screen's size is the
 * view context's scissor at layout 0. */
static float s_screen_w = 0.0f, s_screen_h = 0.0f;

static void read_scissor(float out[4]) {
    const gaddr context = L39(MAP_VIEW_CONTEXT);
    out[0] = (float)GREF(int32_t, context + 0x240u);
    out[1] = (float)GREF(int32_t, context + 0x248u);
    out[2] = (float)(GREF(int32_t, context + 0x244u) + 1);
    out[3] = (float)(GREF(int32_t, context + 0x24Cu) + 1);
}

static void view_layout(void (*game)(int), int layout) {
    float r[4];
    /* The view before ends while its camera is the game's. */
    openrac_frame_view_end();
    /* The whole screen as the frame was last laid out, before a first view. */
    if (s_screen_w <= 0.0f && layout > 0) {
        read_scissor(r);
        s_screen_w = r[2];
        s_screen_h = r[3];
    }
    game(layout);
    read_scissor(r);
    if (layout <= 0) {
        s_screen_w = r[2];
        s_screen_h = r[3];
        return;
    }
    if (s_screen_w < 1.0f || s_screen_h < 1.0f || r[2] <= r[0] || r[3] <= r[1]) {
        return;
    }
    const int view = layout - (layout >= 6 ? 6 : layout >= 3 ? 3 : 1);
    /* In the recorder's 512 x 448 (openrac_frame_screen is left as it is). */
    const float kx = 512.0f / s_screen_w, ky = 448.0f / s_screen_h;
    const openrac_frame_viewport rect = {r[0] * kx, r[1] * ky, r[2] * kx, r[3] * ky};
    openrac_frame_view_begin(view, &rect);
}

/* One map's rows, in mp/maps.py CARRIED's order. */
#define MP_MAP(                                                                                    \
    m,                                                                                             \
    vif0,                                                                                          \
    wad,                                                                                           \
    glow1,                                                                                         \
    glow2,                                                                                         \
    effect2,                                                                                       \
    tfrag,                                                                                         \
    ties,                                                                                          \
    shrubs,                                                                                        \
    mobys,                                                                                         \
    draw_side,                                                                                     \
    textures,                                                                                      \
    anim_chain,                                                                                    \
    tfrag_light,                                                                                   \
    tfrag_gifs,                                                                                    \
    tie_colours,                                                                                   \
    tie_gifs,                                                                                      \
    shrub_light,                                                                                   \
    shrub_gifs,                                                                                    \
    moby_gifs,                                                                                     \
    sky,                                                                                           \
    particles,                                                                                     \
    quad,                                                                                          \
    layout                                                                                         \
)                                                                                                  \
    void mp_func_L##m##_##vif0(unsigned int chain) {                                               \
        kick_vif0_chain(chain);                                                                    \
    }                                                                                              \
    int mp_func_L##m##_##wad(int source, int destination) {                                        \
        return wad_decompress(source, destination);                                                \
    }                                                                                              \
    int mp_func_L##m##_##glow1(void) {                                                             \
        return glow_copy();                                                                        \
    }                                                                                              \
    int mp_func_L##m##_##glow2(void) {                                                             \
        return glow_add();                                                                         \
    }                                                                                              \
    int mp_func_L##m##_##effect2(void) {                                                           \
        return 0;                                                                                  \
    }                                                                                              \
    int mp_func_L##m##_##tfrag(void) {                                                             \
        return func_L39_005F4690();                                                                \
    }                                                                                              \
    int mp_func_L##m##_##ties(void) {                                                              \
        return func_L39_005F61A0();                                                                \
    }                                                                                              \
    int mp_func_L##m##_##shrubs(void) {                                                            \
        return func_L39_005EDE28();                                                                \
    }                                                                                              \
    int mp_func_L##m##_##mobys(int a0, int a1, int a2, int a3) {                                   \
        return func_L39_005B25C8(a0, a1, a2, a3);                                                  \
    }                                                                                              \
    int mp_func_L##m##_##draw_side(int a0, int a1) {                                               \
        (void)a0;                                                                                  \
        (void)a1;                                                                                  \
        return 0;                                                                                  \
    }                                                                                              \
    int mp_func_L##m##_##textures(int a0) {                                                        \
        (void)a0;                                                                                  \
        return 0;                                                                                  \
    }                                                                                              \
    int mp_func_L##m##_##anim_chain(int a0, int a1) {                                              \
        return moby_chain(a0, a1);                                                                 \
    }                                                                                              \
    int mp_func_L##m##_##tfrag_light(int a0) {                                                     \
        (void)a0;                                                                                  \
        return 0;                                                                                  \
    }                                                                                              \
    int mp_func_L##m##_##tfrag_gifs(void) {                                                        \
        return 0;                                                                                  \
    }                                                                                              \
    int mp_func_L##m##_##tie_colours(void) {                                                       \
        return 0;                                                                                  \
    }                                                                                              \
    int mp_func_L##m##_##tie_gifs(void) {                                                          \
        return 0;                                                                                  \
    }                                                                                              \
    int mp_func_L##m##_##shrub_light(void) {                                                       \
        return 0;                                                                                  \
    }                                                                                              \
    int mp_func_L##m##_##shrub_gifs(void) {                                                        \
        return 0;                                                                                  \
    }                                                                                              \
    int mp_func_L##m##_##moby_gifs(void) {                                                         \
        return 0;                                                                                  \
    }                                                                                              \
    int mp_func_L##m##_##sky(int shell) {                                                          \
        return sky_draw_shell(shell);                                                              \
    }                                                                                              \
    int mp_func_L##m##_##particles(void) {                                                         \
        return part_proc();                                                                        \
    }                                                                                              \
    int mp_func_L##m##_##quad(int a0, int a1) {                                                    \
        return world_quad(a0, a1);                                                                 \
    }                                                                                              \
    void mp_func_L##m##_##layout(int a0) {                                                         \
        view_layout(mp_func_L##m##_##layout##__game, a0);                                          \
    }

#define MP_MAP_UI(...)
#define MP_MAP_HUD(...)
#include "mp_maps.h"
