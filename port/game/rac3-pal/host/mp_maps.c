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
 * (0x001DF97C / 0x001E7880), and the screen glow's word 0x00245BB4 (0x001DBCA4). */
#include <stdint.h>

#include "game/common/lib/wad_routine.h"
#include "mp/game_protos.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "rac3_coll_line.h"
#include "rac3_screen.h"
#include "rac3_version.h"

/* host/mp_fixes.c: the flags a map's collision line runs with (wall sniping's fix). */
int rac3_mp_fix_coll_flags(int map, int from, int flags, int moby);

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

/* The screen glow and the second effect, drawn natively as the single-player levels' are
 * (screen.c), from the map's copy of level 39's globals: the glow word, the blur byte 0x24 after
 * it, the list pointer. */
static Rac3Glow map_glow(void) {
    const Rac3Glow g =
        {L39(MAP_GLOW_WORD), L39(RAC3_LADDR(39, 0x00245BD8u)), L39(MAP_LIST_POINTER)};
    return g;
}

static inline int glow_copy(void) {
    const Rac3Glow g = map_glow();
    return rac3_screen_glow(&g);
}

static inline int glow_add(void) {
    const Rac3Glow g = map_glow();
    return rac3_screen_glow_end(&g);
}

static inline int soften(void) {
    return rac3_screen_soften(L39(MAP_LIST_POINTER));
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
    coll                                                                                           \
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
        return soften();                                                                           \
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
    int mp_func_L##m##_##coll##__game(int a0, int a1, int a2, int a3, int a4);                     \
    int mp_func_L##m##_##coll(int a0, int a1, int a2, int a3, int a4) {                            \
        const int flags = rac3_mp_fix_coll_flags(m, a0, a2, a3);                                   \
        return rac3_coll_line(mp_func_L##m##_##coll##__game, a0, a1, flags, a3, a4);               \
    }

#define MP_MAP_UI(...)
#define MP_MAP_HUD(...)
#define MP_MAP_VU0(...)
#include "mp_maps.h"
