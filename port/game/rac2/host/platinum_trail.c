/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/*
 * A platinum bolt coin's trail is silver (openrac_api.moby_material,
 * OPENRAC_MOBY_MATERIAL_PLATINUM), an obsidian one's in its glow's colour
 * (OPENRAC_MOBY_MATERIAL_OBSIDIAN, interface 1.24).
 *
 * The coin's update (func_00300F58) calls part_spawn_trail_puff (func_00302848) on each tick it is
 * seen: a fading puff on the coin, now and then a sparkle on the ground, and every other tick a
 * streak rising from it. Their colours are the game's words at 0x1AE900 (ABGR, alpha 0x80 = 1): the
 * puff's 0x60102830 (gold), the sparkle's 0x60004040 (yellow) and the streak's, between 0x60004040
 * and 0x60202040 (yellow to orange). They are particles of their own, not the coin's mesh, so
 * mesh.frag's platinum never reaches them: a platinum coin kept a gold glow around it. For a coin a
 * mod draws as platinum, the four words are made silver as mesh.frag makes a glow packet (grey at
 * the colour's luminance, times kPlatinumTint and kPlatinumColourGain) while the game makes the
 * trail, then put back: the particles keep the colour they were made with, and a gold coin's trail
 * is the game's. Nothing reads the words but this function. For an obsidian coin they are made the
 * glow's colour at the colour's luminance, as mesh.frag's obsidian_colour makes its glow packets
 * (times kObsidianGlowPacket): the mod's rgb, or kObsidianGlow's violet for 0xFFFFFF.
 *
 * USA v1.01 alone until the carried releases' tables are written again (rac2.h, RAC2_PRIMARY):
 * PAL's coin calls its own copy (func_PAL_00303950), v2.00 has it as func_00303640.
 */
#include <stdint.h>
#include <string.h>

#include "game_protos.h"
#include "openrac/game_host.h"
#include "rac2.h"

#if defined(RAC2_PRIMARY)

/* v1.01's address, as this block is built for v1.01 alone (RAC2_PRIMARY). */
#define TRAIL_COLOURS 0x001AE900u /* the puff's, the sparkle's, the streak's two */
#define TRAIL_COLOUR_WORDS 4

/* mesh.frag's kPlatinumTint times kPlatinumColourGain (1.35), in thousandths. */
#define PLATINUM_RED 1161u   /* 0.86 */
#define PLATINUM_GREEN 1283u /* 0.95 */
#define PLATINUM_BLUE 1566u  /* 1.16 */

/* mesh.frag's kObsidianGlowPacket (1.6) and kObsidianGlow (0.55, 0.15, 0.95), in thousandths. */
#define OBSIDIAN_GAIN 1600u
#define OBSIDIAN_RED 550u
#define OBSIDIAN_GREEN 150u
#define OBSIDIAN_BLUE 950u
/* The rgb that asks for the default glow (0xRRGGBB). */
#define OBSIDIAN_DEFAULT_RGB 0xFFFFFFu

/* Returns `channel` x `scale` / 1000, at most 0xFF. */
static uint32_t scaled(uint32_t channel, uint32_t scale) {
    const uint32_t value = (channel * scale + 500u) / 1000u;
    return value > 0xFFu ? 0xFFu : value;
}

/* Returns the ABGR word `abgr` in platinum: grey at its luminance, turned blue and brightened as
 * mesh.frag's platinum_colour; its alpha as it is. */
static uint32_t platinum_word(uint32_t abgr) {
    const uint32_t red = abgr & 0xFFu;
    const uint32_t green = (abgr >> 8) & 0xFFu;
    const uint32_t blue = (abgr >> 16) & 0xFFu;
    const uint32_t grey = (299u * red + 587u * green + 114u * blue + 500u) / 1000u;
    return (abgr & 0xFF000000u) | scaled(grey, PLATINUM_BLUE) << 16
           | scaled(grey, PLATINUM_GREEN) << 8 | scaled(grey, PLATINUM_RED);
}

/* Returns the ABGR word `abgr` in obsidian's glow `rgb` (0xRRGGBB): the glow's colour at its
 * luminance, brightened as mesh.frag's obsidian_colour makes a glow packet; its alpha as it is. */
static uint32_t obsidian_word(uint32_t abgr, uint32_t rgb) {
    const uint32_t red = abgr & 0xFFu;
    const uint32_t green = (abgr >> 8) & 0xFFu;
    const uint32_t blue = (abgr >> 16) & 0xFFu;
    const uint32_t grey = (299u * red + 587u * green + 114u * blue + 500u) / 1000u;
    /* The glow's channels in thousandths: the default's, or the mod's (0xFF = 1000). */
    uint32_t glow_red = OBSIDIAN_RED;
    uint32_t glow_green = OBSIDIAN_GREEN;
    uint32_t glow_blue = OBSIDIAN_BLUE;
    if (rgb != OBSIDIAN_DEFAULT_RGB) {
        glow_red = ((rgb >> 16) & 0xFFu) * 1000u / 0xFFu;
        glow_green = ((rgb >> 8) & 0xFFu) * 1000u / 0xFFu;
        glow_blue = (rgb & 0xFFu) * 1000u / 0xFFu;
    }
    const uint32_t lit = scaled(grey, OBSIDIAN_GAIN);
    return (abgr & 0xFF000000u) | scaled(lit, glow_blue) << 16 | scaled(lit, glow_green) << 8
           | scaled(lit, glow_red);
}

/* part_spawn_trail_puff */
void func_00302848(gaddr object, int quiet) {
    uint32_t rgb = OBSIDIAN_DEFAULT_RGB;
    const int obsidian = openrac_moby_obsidian(object, &rgb);

    /* A gold coin, or any other: the game's colours. */
    if (!obsidian && !openrac_moby_platinum(object)) {
        func_00302848__game(object, quiet);
        return;
    }

    uint32_t game[TRAIL_COLOUR_WORDS];
    uint32_t drawn[TRAIL_COLOUR_WORDS];
    memcpy(game, G(TRAIL_COLOURS), sizeof game);

    /* Each of the four colours, in obsidian's glow or in platinum. */
    for (int i = 0; i < TRAIL_COLOUR_WORDS; ++i) {
        drawn[i] = obsidian ? obsidian_word(game[i], rgb) : platinum_word(game[i]);
    }

    memcpy(G(TRAIL_COLOURS), drawn, sizeof drawn);
    func_00302848__game(object, quiet);
    memcpy(G(TRAIL_COLOURS), game, sizeof game);
}

#endif
