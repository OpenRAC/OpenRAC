/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * OPENRAC_RAC3_HERO=PATH ("-" for stderr): a line for each frame of a level in which the hero's
 * state words changed (and every 60th frame), for scripted checks of his moves, as
 * OPENRAC_RAC3_CHAINS is for the display lists. Development only: nothing reads it otherwise.
 *
 * The words (rac3-decomp include/names.h, include/structs/hero.h; IDENTIFIED.tsv):
 * - hero 0x001A4BE0: +0x80 position, +0x25C4 state (enums/hero_state.h: 7 jump, 14 double jump,
 *   19 wrench combo, 22 hit, 57 death), +0x25CC its type, +0x1B8 frames in the state, +0x36E
 *   frames off the ground, +0x25F7 the area (checkpoint_reached sets it), +0x2850 health;
 * - attack_row 0x001A57BC (0-2 the combo's swing, 3 the throw, 4 the jump attack);
 * - held_item 0x001A5E08 (9 the wrench) and equipped_gun 0x001D4C40, the gun Circle brings out;
 * - item_ammo 0x001427F0, a word per item, and item_owned 0x00142CA0, a byte per item;
 * - hero_max_health 0x00142668, player_bolts 0x00142660;
 * - area_deaths 0x001C5290 (16 areas of a level), the death count 0x001D4CA0 hero_death_done adds
 *   to, and the respawn request 0x001A71D2 it sets;
 * - the last checkpoint reached, Veldin's D_L01_001DCA14 (checkpoint_reached keeps its id there);
 * - the level's progress flags, 16 bytes a level at progress_flags 0x001BBC40.
 *
 * OPENRAC_RAC3_MOBYS=FRAME,... adds, at those frames, a line for each live moby within 500 units of
 * the hero: its class (+0xAA), state (+0x20), update (+0x64) and position (+0x10). Veldin's moby table runs from *0x001E001C to
 * *0x001E0024 (D_L01_001E001C, moby_end), 0x100 bytes a moby.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "openrac/host_env.h"
#include "rac3_version.h"

void rac3_hero_log_install(void);

#define HERO RAC3_ADDR(0x001A4BE0u)
#define ATTACK_ROW RAC3_ADDR(0x001A57BCu)
#define HELD_ITEM RAC3_ADDR(0x001A5E08u)
#define EQUIPPED_GUN RAC3_ADDR(0x001D4C40u)
#define ITEM_AMMO RAC3_ADDR(0x001427F0u)
#define ITEM_OWNED RAC3_ADDR(0x00142CA0u)
#define MAX_HEALTH RAC3_ADDR(0x00142668u)
#define BOLTS RAC3_ADDR(0x00142660u)
#define AREA_DEATHS RAC3_ADDR(0x001C5290u)
#define DEATHS RAC3_ADDR(0x001D4CA0u)
#define RESPAWN RAC3_ADDR(0x001A71D2u)
#define PROGRESS_FLAGS RAC3_ADDR(0x001BBC40u)
#define LEVEL_CURRENT RAC3_ADDR(0x001D545Cu)
#define PADS RAC3_ADDR(0x001D52FCu)
#define LAST_CHECKPOINT OPENRAC_LDATA(1, (gaddr)RAC3_LADDR(1, 0x001DCA14u))
#define MOBY_FIRST OPENRAC_LDATA(1, (gaddr)RAC3_LADDR(1, 0x001E001Cu))
#define MOBY_END OPENRAC_LDATA(1, (gaddr)RAC3_LADDR(1, 0x001E0024u))

static FILE* g_log;
static void (*g_previous)(unsigned frame);
static char g_last[768];

static uint32_t u32_at(gaddr at) {
    uint32_t v;
    memcpy(&v, G(at), sizeof v);
    return v;
}

static int16_t s16_at(gaddr at) {
    int16_t v;
    memcpy(&v, G(at), sizeof v);
    return v;
}

static float f32_at(gaddr at) {
    float v;
    memcpy(&v, G(at), sizeof v);
    return v;
}

static int moby_frame(unsigned frame) {
    const char* list = openrac_env_string("OPENRAC_RAC3_MOBYS");
    for (const char* at = list; at != NULL && *at != '\0';) {
        if ((unsigned)strtoul(at, NULL, 10) == frame) {
            return 1;
        }
        at = strchr(at, ',');
        at = at != NULL ? at + 1 : NULL;
    }
    return 0;
}

static void log_mobys(unsigned frame) {
    const float hx = f32_at(HERO + 0x80u), hy = f32_at(HERO + 0x84u), hz = f32_at(HERO + 0x88u);
    const gaddr first = u32_at(MOBY_FIRST), end = u32_at(MOBY_END);
    for (gaddr m = first; first != 0 && m <= end && m - first < 0x100u * 4096u; m += 0x100u) {
        const int8_t state = *(const int8_t*)G(m + 0x20u);
        const float x = f32_at(m + 0x10u), y = f32_at(m + 0x14u), z = f32_at(m + 0x18u);
        const float dx = x - hx, dy = y - hy, dz = z - hz;
        if (state < 0 || dx * dx + dy * dy + dz * dz > 250000.0f) {
            continue;
        }
        fprintf(
            g_log,
            "%u\tmoby %08x\tclass=%d\tstate=%d\tupdate=%08x\tpos=%.2f,%.2f,%.2f\n",
            frame,
            (unsigned)m,
            (int)s16_at(m + 0xAAu),
            (int)state,
            (unsigned)u32_at(m + 0x64u),
            (double)x,
            (double)y,
            (double)z
        );
    }
}

/* The level's spawn points (D_L01_001E0100, 0x80 bytes each: +0x30 the position), where a respawn
 * puts the hero; checkpoint_reached picks one. Printed with the mobys. */
static void log_spawns(unsigned frame) {
    const gaddr spawns = u32_at(OPENRAC_LDATA(1, (gaddr)RAC3_LADDR(1, 0x001E0100u)));
    for (unsigned i = 0; spawns != 0 && i < 24u; ++i) {
        const gaddr at = spawns + 0x80u * i + 0x30u;
        fprintf(
            g_log,
            "%u\tspawn %u\tpos=%.2f,%.2f,%.2f\n",
            frame,
            i,
            (double)f32_at(at),
            (double)f32_at(at + 4u),
            (double)f32_at(at + 8u)
        );
    }
}

static void on_frame(unsigned frame) {
    if (g_previous != NULL) {
        g_previous(frame);
    }
    const int overlay = openrac_game_loaded_overlay();
    if (overlay <= 0 || overlay >= 100) {
        return;
    }
    const int level = (int)u32_at(LEVEL_CURRENT);
    uint32_t mode = 0;
    if (openrac_game_mode == NULL || !openrac_game_mode(&mode)) {
        mode = 0xFFFFFFFFu;
    }
    const int held = (int)u32_at(HELD_ITEM);
    const int gun = (int)u32_at(EQUIPPED_GUN);
    const unsigned area = *(const uint8_t*)G(HERO + 0x25F7u);
    const gaddr pad = u32_at(PADS);
    const uint8_t* flags = (const uint8_t*)G(PROGRESS_FLAGS + 16u * (unsigned)(level & 0x3F));
    char owned[160];
    size_t used = 0;
    owned[0] = '\0';
    for (unsigned item = 1; item < 0x9Cu && used + 8 < sizeof owned; ++item) {
        if (*(const uint8_t*)G(ITEM_OWNED + item) != 0) {
            used += (size_t)snprintf(owned + used, sizeof owned - used, used ? ",%u" : "%u", item);
        }
    }
    char flag_hex[33];
    for (int i = 0; i < 16; ++i) {
        snprintf(flag_hex + 2 * i, 3, "%02x", flags[i]);
    }
    /* What changes every frame (the position, the frames in the state) is left out of the
     * comparison, so a line is written when something else changed. */
    char key[768];
    snprintf(
        key,
        sizeof key,
        "mode=%d\tst=%d\ttype=%d\thp=%d/%d\tatk=%d\theld=%d\tgun=%d\tammo=%d/%d\tbolts=%d\t"
        "area=%u\tad=%d\tdeaths=%d\trsp=%d\tcp=%d\tflags=%s\towned=%s",
        (int)mode,
        (int)u32_at(HERO + 0x25C4u),
        (int)u32_at(HERO + 0x25CCu),
        (int)u32_at(HERO + 0x2850u),
        (int)u32_at(MAX_HEALTH),
        (int)s16_at(ATTACK_ROW),
        held,
        gun,
        held > 0 && held < 0x9C ? (int)u32_at(ITEM_AMMO + 4u * (unsigned)held) : -1,
        gun > 0 && gun < 0x9C ? (int)u32_at(ITEM_AMMO + 4u * (unsigned)gun) : -1,
        (int)u32_at(BOLTS),
        area,
        (int)u32_at(AREA_DEATHS + 0x40u * (unsigned)(level & 0x3F) + 4u * (area & 15u)),
        (int)u32_at(DEATHS),
        (int)*(const uint8_t*)G(RESPAWN),
        (int)u32_at(LAST_CHECKPOINT),
        flag_hex,
        owned
    );
    if (moby_frame(frame)) {
        log_mobys(frame);
        log_spawns(frame);
    }
    if (strcmp(key, g_last) == 0 && frame % 60u != 0u) {
        return;
    }
    memcpy(g_last, key, sizeof g_last);
    fprintf(
        g_log,
        "%u\tL%d\t%s\tt=%d\tair=%d\tpos=%.2f,%.2f,%.2f\tpad=%04x\n",
        frame,
        level,
        key,
        (int)u32_at(HERO + 0x1B8u),
        (int)s16_at(HERO + 0x36Eu),
        (double)f32_at(HERO + 0x80u),
        (double)f32_at(HERO + 0x84u),
        (double)f32_at(HERO + 0x88u),
        pad != 0 ? u32_at(pad + 0x1A0u) & 0xFFFFu : 0u
    );
    fflush(g_log);
}

void rac3_hero_log_install(void) {
    const char* path = openrac_env_string("OPENRAC_RAC3_HERO");
    if (path == NULL || *path == '\0') {
        return;
    }
    g_log = strcmp(path, "-") == 0 ? stderr : fopen(path, "w");
    if (g_log == NULL) {
        return;
    }
    g_previous = openrac_game_on_frame;
    openrac_game_on_frame = on_frame;
}
