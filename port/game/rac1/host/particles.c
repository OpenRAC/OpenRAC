/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 */
/* The game's own particle effects for a mod (openrac_api.particle_spawn_ex, interface 1.33): each
 * kind calls the game's routine that makes its particle record, and the game's per-tick update and
 * renderer take the record from there. The game's routines take their pointers as game addresses
 * (gaddr), so the position and velocity are copied into guest statics first. The argument orders follow the shared overlay's definitions
 * (games/rac1 effect_particle_updates.c, alpha_fade_particle_updates.c) and the stack slots the PAL
 * build reads. The addresses are checked for PAL only (rac1_version.h); other versions answer -1. */
#include <stddef.h>
#include <string.h>

#include "openrac/game_host.h"
#include "rac1.h"

#ifndef RAC1_PARTICLE_SPAWN
#define RAC1_PARTICLE_SPAWN 0
#endif

/* The kinds of openrac_api.particle_spawn (OPENRAC_PARTICLE_SMOKE, _BURST, _SPARK in the SDK's
 * openrac_mod.h; the port hands them over as they are). */
#define PARTICLE_KIND_SMOKE 0u
#define PARTICLE_KIND_BURST 1u
#define PARTICLE_KIND_SPARK 2u
/* OPENRAC_PARTICLE_CHILDREN (the SDK's bit 0 of the options' flags). */
#define PARTICLE_FLAG_CHILDREN 0x1u
/* The game's record: a texture byte at 2 and a blend byte at 3 (sentrybot.c's Part). */
#define PARTICLE_RECORD_TEXTURE 2
#define PARTICLE_RECORD_BLEND 3
/* The 1.33 smoke's spin rate: the game's own C2 value. */
#define PARTICLE_SMOKE_ROT_25 (-0.0002f)

#if RAC1_PARTICLE_SPAWN

/* The game's routines (games/rac1 names, shared_levels.h), in the level's shared code. Each returns
 * the particle record's game address, or 0 when the pool has no room. */
typedef gaddr (*PartType44SpawnFn)(
    gaddr pos,
    gaddr vel,
    int life,
    int alpha,
    int rgb,
    int spin,
    float size,
    float size_rate,
    float z,
    float rot,
    float w
);
typedef gaddr (*SpawnBurstFn)(gaddr pos, gaddr dir, int c, int d, int n, int k, float f);
/* spawn_shrinking_effect_particle_with_gravity_and_secondary_spawns: the flag is the secondary
 * spawns (0 none); texture and blend -1 take the game's defaults. */
typedef gaddr (*SpawnShrinkFn)(
    gaddr pos, gaddr vel, int c, int d, int life, int flag, int texture, int blend, float size
);

#define PARTICLE_PART_TYPE44 OPENRAC_LCODE(0, RAC1_LADDR(00, 0x00272158u))
#define PARTICLE_SPAWN_BURST OPENRAC_LCODE(0, RAC1_LADDR(00, 0x0026DA50u))
#define PARTICLE_SPAWN_SHRINK OPENRAC_LCODE(0, RAC1_LADDR(00, 0x0026CA10u))

/* Two 16-byte guest statics per call site, kept for the process (openrac_guest_static_site). */
#define PARTICLE_SITE_POS 0x4F50454E52414350ULL
#define PARTICLE_SITE_VEL 0x4F50454E52414351ULL

/* scale_ticks (func_001F9850): n times the factor at 0x15EE60, plus a half, truncated. */
static int particle_scale_ticks(int n) {
    return openrac_f2i(0.5f + (float)n * GREF(float, RAC1_ADDR(0x0015EE60u)));
}

/* Copies three floats into a guest static of four words (the fourth is w). */
static gaddr particle_stage(uint64_t site, const float v[3], float w) {
    int fresh = 0;
    const gaddr at = openrac_guest_static_site(16, site, &fresh);
    const float words[4] = {v[0], v[1], v[2], w};
    memcpy(G(at), words, sizeof words);
    return at;
}

/* The game's own effect with its options (the checks are the port's, mod_play.cpp). */
int rac1_particle_spawn_ex(const openrac_game_particle_spawn* spawn) {
    if (!rac1_level_running()) {
        return -1;
    }
    /* The game copies its positions as four words: w is 0 here (the smoke's pw is 0 too). */
    const gaddr at = particle_stage(PARTICLE_SITE_POS, spawn->position, 0.0f);
    const gaddr vel = particle_stage(PARTICLE_SITE_VEL, spawn->velocity, 0.0f);
    const int life = particle_scale_ticks((int)spawn->life_ticks);
    const int children = (spawn->flags & PARTICLE_FLAG_CHILDREN) != 0 ? 1 : 0;
    gaddr made = 0;
    switch (spawn->kind) {
        case PARTICLE_KIND_SMOKE: {
            /* The smoke call of the game's own (alpha_fade_particle_updates.c, PartType44Spawn):
             * spin 3, size rate 1000, z 1, pw 0; alpha is the top byte of the colour. The game's
             * caller then sets the record's texture and blend bytes (the -1 values keep the
             * defaults). */
            const PartType44SpawnFn spawn44 = GFN(PartType44SpawnFn, PARTICLE_PART_TYPE44);
            made = spawn44(
                at,
                vel,
                life,
                (int)(spawn->color >> 24),
                (int)(spawn->color & 0xFFFFFFu),
                3,
                spawn->size,
                1000.0f,
                1.0f,
                spawn->rot,
                0.0f
            );
            if (made != 0) {
                uint8_t* record = (uint8_t*)G(made);
                if (spawn->texture >= 0) {
                    record[PARTICLE_RECORD_TEXTURE] = (uint8_t)spawn->texture;
                }
                if (spawn->blend >= 0) {
                    record[PARTICLE_RECORD_BLEND] = (uint8_t)spawn->blend;
                }
            }
            break;
        }
        case PARTICLE_KIND_BURST: {
            /* spawn_burst(pos, dir, c, d, n, k, f): k is the secondary-spawn flag. */
            const SpawnBurstFn burst = GFN(SpawnBurstFn, PARTICLE_SPAWN_BURST);
            made = burst(
                at, vel, (int)spawn->color, (int)spawn->color_end, life, children, spawn->size
            );
            break;
        }
        case PARTICLE_KIND_SPARK: {
            /* spawn_shrinking...(pos, vel, colour, colour end, life, flag, texture, blend, size). */
            const SpawnShrinkFn spark = GFN(SpawnShrinkFn, PARTICLE_SPAWN_SHRINK);
            made = spark(
                at,
                vel,
                (int)spawn->color,
                (int)spawn->color_end,
                life,
                children,
                spawn->texture,
                spawn->blend,
                spawn->size
            );
            break;
        }
        default:
            return -1;
    }
    /* Every routine answers 0 when the particle pool has no room (create_part). */
    return made != 0 ? 1 : 0;
}

#else

int rac1_particle_spawn_ex(const openrac_game_particle_spawn* spawn) {
    (void)spawn;
    return -1;
}

#endif

/* Particle textures for a mod (openrac_api.particle_texture_register, 1.33). A texture is staged in
 * the guest's statics: its 1024 indices, then its 1024-byte palette, one block per id, kept for the
 * process. The record's texture byte is then the id, and the renderer (render_native.c) draws it from
 * here: the level's own table holds 128 entries and an id past them is not in it. The same content
 * gets the same id. */
#define PARTICLE_TEXTURE_BYTES 1024u
#define PARTICLE_TEXTURE_SITE 0x4F50454E52545800ULL /* "OPENRTX" + the id */
static gaddr g_particle_texture_at[RAC1_PARTICLE_TEXTURE_COUNT];
static int g_particle_texture_count = 0;

int rac1_particle_texture_register(const uint8_t* indices, const uint8_t* clut) {
    if (!rac1_level_running()) {
        return -1;
    }
    for (int i = 0; i < g_particle_texture_count; ++i) {
        const uint8_t* at = (const uint8_t*)G(g_particle_texture_at[i]);
        if (memcmp(at, indices, PARTICLE_TEXTURE_BYTES) == 0
            && memcmp(at + PARTICLE_TEXTURE_BYTES, clut, PARTICLE_TEXTURE_BYTES) == 0) {
            return RAC1_PARTICLE_TEXTURE_FIRST + i;
        }
    }
    if (g_particle_texture_count >= RAC1_PARTICLE_TEXTURE_COUNT) {
        return 0;
    }
    int fresh = 0;
    const gaddr at = openrac_guest_static_site(
        2 * PARTICLE_TEXTURE_BYTES,
        PARTICLE_TEXTURE_SITE + (uint64_t)g_particle_texture_count,
        &fresh
    );
    uint8_t* block = (uint8_t*)G(at);
    memcpy(block, indices, PARTICLE_TEXTURE_BYTES);
    memcpy(block + PARTICLE_TEXTURE_BYTES, clut, PARTICLE_TEXTURE_BYTES);
    g_particle_texture_at[g_particle_texture_count] = at;
    return RAC1_PARTICLE_TEXTURE_FIRST + g_particle_texture_count++;
}

int rac1_particle_texture_at(uint8_t texture, uint32_t* pixels, uint32_t* clut) {
    if (texture < RAC1_PARTICLE_TEXTURE_FIRST
        || texture - RAC1_PARTICLE_TEXTURE_FIRST >= g_particle_texture_count) {
        return 0;
    }
    const gaddr at = g_particle_texture_at[texture - RAC1_PARTICLE_TEXTURE_FIRST];
    *pixels = at;
    *clut = at + PARTICLE_TEXTURE_BYTES;
    return 1;
}
