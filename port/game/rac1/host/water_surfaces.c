/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/* Enhanced water on Pokitaru's ocean and Fleet's pools: tags in the level's draw chain that the
 * renderer reads, and nothing else. The game's own draws run unchanged. */
#include "rac1.h"

#include <stdint.h>

/* The level's VIF chain cursor (l11_gif_packet_cursor, D_L11_00161240; the same in level 17). */
#define L11_PACKET OPENRAC_LDATA(11, RAC1_LADDR(11, 0x00161240u))
#define L17_PACKET OPENRAC_LDATA(17, RAC1_LADDR(17, 0x00161240u))

void openrac_skip_ocean_quad(int skip);

/* A harmless GS NOP that names the water surface the following packets draw (0 ends it). It
 * travels with the level's chain, including the one-tick display delay. */
static void water_level_marker(gaddr cursor, unsigned surface) {
    const gaddr at = GREF(gaddr, cursor);
    GREF(uint32_t, at) = 0x10000002u;
    GREF(uint32_t, at + 4) = 0;
    GREF(uint32_t, at + 8) = 0;
    GREF(uint32_t, at + 12) = 0x50000002u;
    GREF(uint64_t, at + 16) = 1ull | (1ull << 15) | (1ull << 60);
    GREF(uint64_t, at + 24) = 0xE;
    GREF(uint64_t, at + 32) = (0x5754ull << 48) | surface;
    GREF(uint64_t, at + 40) = 0x7F;
    GREF(gaddr, cursor) = at + 48;
}

/* Fleet draws each pool twice with different UV scroll. The enhanced material
 * composes transmission itself, so submit each physical strip only once. */
static int fleet_water_strips = -1;

static void fleet_water_draw(void (*draw)(unsigned int), unsigned int moby, unsigned surface, int strips) {
    if (!openrac_game_enhanced_water()) {
        draw(moby);
        return;
    }
    water_level_marker(L17_PACKET, surface);
    fleet_water_strips = strips;
    draw(moby);
    fleet_water_strips = -1;
    water_level_marker(L17_PACKET, 0);
}

/* draw_moby_radial_ripple_mesh_pair: the two landing pool strips. */
void func_L17_002F1FE0(unsigned int moby) {
    fleet_water_draw(func_L17_002F1FE0__game, moby, 1, 2);
}

/* draw_ripple_ring: both ring water callbacks. */
void func_L17_002F2458(unsigned int moby) {
    fleet_water_draw(func_L17_002F2458__game, moby, 2, 1);
}

/* DrawLightRing: the square pools. */
void func_L17_002F2BD8(unsigned int moby) {
    fleet_water_draw(func_L17_002F2BD8__game, moby, 3, 1);
}

/* draw_env_overlay_mesh: inside a Fleet pool's draw, only the first scroll pass. */
void func_L00_001FDE48(int count, unsigned int vertices, unsigned int colours, unsigned int uv, int mode) {
    if (fleet_water_strips == 0) {
        return;
    }
    if (fleet_water_strips > 0) {
        --fleet_water_strips;
    }
    func_L00_001FDE48__game(count, vertices, colours, uv, mode);
}

/* draw_pokitaru_ocean: the original draw packets stay, the tag is local to the callback. */
void func_L11_0030BCD8(unsigned int moby) {
    const int enhanced = openrac_game_enhanced_water();
    if (enhanced) {
        water_level_marker(L11_PACKET, 1);
    }
    if (enhanced == 2) {
        openrac_skip_ocean_quad(1);
    }
    func_L11_0030BCD8__game(moby);
    if (enhanced == 2) {
        openrac_skip_ocean_quad(0);
    }
    if (enhanced) {
        water_level_marker(L11_PACKET, 0);
    }
}
