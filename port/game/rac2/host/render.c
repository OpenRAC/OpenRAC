/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Going Commando: the game's own renderers, which the native port does not run. On the console they
 * are hand-written assembly that turns level data into VU1 and GS packets; the port draws the same
 * things with its own renderers from the extracted level, as for Ratchet & Clank
 * (port/game/rac1/host/render_native.c, whose reading of each routine holds here: the routines were
 * paired instruction by instruction, ~/RACSwarm's RAC1_MAP.tsv). DrawWorld (func_0027BD70, matched C)
 * still runs: each pass it calls ends here, where a native renderer takes over.
 */
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "draw_owner.h"
#include "game_protos.h"
#include "rac2.h"
#include "game/common/lib/effect_texture.h"
#include "openrac/game_host.h"

/* BuildViewMatrices's rotation rows; the camera block is 0x370 before them and the eye 0x230. */
#define CAMERA_ROWS RAC2_ADDR(0x001B5570u)
#define CAMERA_BLOCK (CAMERA_ROWS - 0x370u)
#define CAMERA_EYE (CAMERA_ROWS - 0x230u)
/* The view context: +0xB0 and +0xB4 the tangents of half the field of view. */
#define VIEW_CONTEXT RAC2_ADDR(0x001B9040u)
/* MobyProc's inputs: the global draw distance it takes the minimum with (s7), the group masks it
 * copies to the scratchpad. */
#define MOBY_MAX_DISTANCE RAC2_ADDR(0x001B1B74u)
#define MOBY_GROUPS RAC2_ADDR(0x001BADC0u)
/* The particle pool, the highest slot in use (-1 none: FreeParticle, func_002B9180, walks it down),
 * and the texture table PartProc sends to the scratchpad. */
#define PARTICLE_POOL RAC2_ADDR(0x001B1D90u)
#define PARTICLE_HIGHEST RAC2_ADDR(0x001B1D98u)
#define PARTICLE_TEXTURES RAC2_ADDR(0x001F1F40u)
/* The effect textures' uploads this frame: their count and the records GetEffectTex (func_0027C878)
 * writes. */
#define TEXTURE_UPLOADS RAC2_ADDR(0x001B15FCu)
#define TEXTURE_RECORDS RAC2_ADDR(0x001B9340u)
/* The matrix a sky shell is drawn with (func_002E3548 builds it). */
#define SKY_MATRIX RAC2_ADDR(0x001B20F0u)

enum {
    NOP_REGISTER = 0x7F
};

/* A marker in the display list: the A+D write of `value` to the chip's NOP register. The chip ignores
 * it; the window reads its top 16 bits (renderer::GifInterpreter's kMobyMarker and kWorldMarker). */
static void put_marker(uint64_t value) {
    func_002FC560(NOP_REGISTER, value);
}

/* DrawTfrag: the world's first native pass after the sky, so the world's marker goes here (Ratchet
 * & Clank's TfragProc says why). */
void func_002EF3B0(void) {
    openrac_game_draw(OPENRAC_DRAW_TERRAIN);
    put_marker(0x574Cull << 48);
}

/* The tie pass of DrawWorld. */
void func_002F0EF8(void) {
    openrac_game_draw(OPENRAC_DRAW_TIES);
}

/* The shrub pass of DrawWorld. */
void func_002E11C0(void) {
    openrac_game_draw(OPENRAC_DRAW_SHRUBS);
}

/* SkyDrawShell(shell): the window draws the shell with the matrix the sky code built for it. The sky
 * is the world's first pass, so its camera is the world's: a flight, and a scene played in one (level
 * 0's end), draws no terrain, and the camera the game holds at the frame's end is another view's (the
 * cockpit screen's). */
void func_002E3940(int shell) {
    openrac_game_world_view();
    float m[16];
    memcpy(m, G(OPENRAC_DATA(SKY_MATRIX)), sizeof(m));
    openrac_game_sky_shell(shell, m);
    openrac_game_draw(OPENRAC_DRAW_SKY);
}

/*
 * What MobyProc decides for each moby before drawing it, which the game reads back: +0x31, drawn this
 * frame. As Ratchet & Clank's (render_native.c, mark_drawn_mobys), which func_002A4E68 repeats: the
 * group mask at +0x36 against the table indexed by +0x37, the hidden bits of the mode at +0x34, the
 * bounding sphere against the draw distance (+0x32, at most the global one) and the view.
 */
/* The Electrolyzer boards' classes (game/rac2/profile.cpp kBoardMobys). */
static int board_moby(int class_id) {
    static const int kBoard[] = {2163, 2198, 2596, 2645, 2809, 2810, 2811, 2812, 2813, 2819, 2894};
    for (size_t i = 0; i < sizeof(kBoard) / sizeof(kBoard[0]); ++i) {
        if (kBoard[i] == class_id) {
            return 1;
        }
    }
    return 0;
}

static void mark_drawn_mobys(gaddr first, int count) {
    float eye[3], block[12], rows[12], tx, ty;
    memcpy(eye, G(OPENRAC_DATA(CAMERA_EYE)), sizeof(eye));
    memcpy(block, G(OPENRAC_DATA(CAMERA_BLOCK)), sizeof(block));
    for (int c = 0; c < 3; ++c) {
        rows[c] = block[c * 4 + 2];
        rows[4 + c] = -block[c * 4 + 0];
        rows[8 + c] = -block[c * 4 + 1];
    }
    memcpy(&tx, G(OPENRAC_DATA(VIEW_CONTEXT) + 0xB0), 4);
    memcpy(&ty, G(OPENRAC_DATA(VIEW_CONTEXT) + 0xB4), 4);
    if (!(tx > 0.05f && tx < 10.0f)) {
        tx = 0.63f;
    }
    if (!(ty > 0.05f && ty < 10.0f)) {
        ty = tx * 0.756f;
    }
    /* The game fix (differences.json): off, the console's 4:3 view. */
    static const unsigned char* fix;
    if (!fix) {
        fix = openrac_game_fix("fix_wide_moby_effects");
    }
    /* Both tangents opened as the player's Field of View opens them too. */
    const float opened = *fix ? openrac_game_view_opened() : 1.0f;
    tx *= *fix ? openrac_game_picture_wider() * opened : 1.0f;
    ty *= opened;
    const float kx = sqrtf(1.0f + tx * tx), ky = sqrtf(1.0f + ty * ty);
    const gaddr end = first + (gaddr)count * 0x100u;
    const int max_distance = GREF(int, OPENRAC_DATA(MOBY_MAX_DISTANCE));
    const float scale = openrac_game_draw_distance();
    const uint8_t* groups = G(OPENRAC_DATA(MOBY_GROUPS));
    for (gaddr m = first; (count < 0 || m != end) && m - first < 0x100u * 4096u; m += 0x100) {
        uint8_t* b = G(m);
        const int8_t state = (int8_t)b[0x20];
        if (state == -1) {
            break;
        }
        if (state < 0) {
            continue;
        }
        int drawn = (b[0x36] & groups[b[0x37]]) != 0 && (*(uint16_t*)(b + 0x34) & 0x81) == 0;
        if (drawn) {
            float sphere[4];
            memcpy(sphere, b, sizeof(sphere));
            const float r = sphere[3] / 1024.0f;
            const float d[3] =
                {sphere[0] / 1024.0f - eye[0],
                 sphere[1] / 1024.0f - eye[1],
                 sphere[2] / 1024.0f - eye[2]};
            int distance = *(int16_t*)(b + 0x32);
            if (distance > max_distance) {
                distance = max_distance;
            }
            const float z = d[0] * rows[0] + d[1] * rows[1] + d[2] * rows[2];
            const float x = d[0] * rows[4] + d[1] * rows[5] + d[2] * rows[6];
            const float y = d[0] * rows[8] + d[1] * rows[9] + d[2] * rows[10];
            /* As far as the window draws it (openrac_game_draw_distance): the active list
             * (func_002A08A0) takes a drawn moby whatever its update distance (+0x30), so what is
             * seen farther away moves too. A moby with no draw distance stays undrawn. */
            /* An Electrolyzer board's piece keeps the game's distance (game/rac2/profile.cpp
             * kBoardMobys): the game parks the boards out of sight within reach of a longer one. */
            /* Above Original, a quarter farther than the window draws it, so that one barely in
             * view at the far end is already active. */
            const float own = board_moby(*(int16_t*)(b + 0xAA)) ? 1.0f
                              : scale > 1.0f                    ? scale * 1.25f
                                                                : scale;
            const int near_only = own <= 0.0f && distance > 0;
            drawn = (near_only || z <= (float)distance * (own > 0.0f ? own : 1.0f))
                    && z + r > 32.0f / 1024.0f && fabsf(x) - r * kx <= tx * z
                    && fabsf(y) - r * ky <= ty * z;
        }
        b[0x31] = (uint8_t)(drawn ? 1 : 0);
    }
}

/* MobyProc: the moby list `a0`, `a2` mobys of it (-1 the whole list, the world's draw). */
int func_002A4E68(gaddr a0, gaddr a1, int a2, int a3) {
    (void)a1;
    (void)a3;
    mark_drawn_mobys(a0, a2);
    /* The world's mobys come after the draw callback lists DrawFrame runs after the ties and the
     * shrubs (func_0027CEE0, func_0027CF58): what those drew goes before them (a level's water). */
    if (a2 < 0) {
        put_marker((uint64_t)OPENRAC_WORLD_MOBYS_MARKER << 48);
    }
    openrac_game_draw(OPENRAC_DRAW_MOBYS);
    if (openrac_game_mobys_drawn(a0, a2)) {
        put_marker((0x4D42ull << 48) | ((uint64_t)(a2 & 0xFFFF) << 32) | (uint32_t)a0);
    }
    return 0;
}

/* The moby textures' DMA and MobyAnimProc's draw side. */
void func_002A4AD8(int a0) {
    (void)a0;
}

void func_002A4D88(gaddr a0, int a1) {
    (void)a0;
    (void)a1;
}

/* moby_anim_eval_chain: the pose of the joints marked in a1, into the scratchpad. */
int func_002A4078(gaddr a0, gaddr a1) {
    openrac_game_moby_chain(a0, a1);
    return 0;
}

/*
 * PartProc: each live record of the pool (0x40 bytes, byte 1 bit 7 a free one) goes to the window, as
 * Ratchet & Clank's. Its texture is entry +0x02 of the table: the low half the palette's address << 4
 * with the CLUT offset, the high half the pixels' address << 4 with the log2 of the side: unlike
 * Ratchet & Clank's 32 x 32, this game's are 32 x 32 or 64 x 64 (the table at 0x001F1F40). The
 * records are slots 0 to the highest in use, that one included: PartProc counts the word plus one
 * (0x002B9490). Short of it, the newest particle was left out whenever it held the highest slot: the
 * Lancer's chamber halo (type 89) went out on 42 of 666 ticks of a level 3 run, a flicker.
 */
/*
 * What a particle is from one tick to the next, for the window to move it between ticks: its slot and
 * type; for an attached halo (type 89, ParticleUpdateAttachedHalo, func_002C6B80), which the game makes
 * in a new slot each tick, its parent moby (+0x20), joint (+0x24) and offset (+0x30) instead. The
 * Lancer's chamber halo is one: by its slot it was drawn at the latest tick between ticks, ahead of
 * the gun.
 */
static uint32_t particle_id(const uint8_t* r, gaddr at) {
    if (r[0] == 89) {
        /* FNV-1a of +0x20..+0x27 and +0x30..+0x3B (not +0x28, its scale). */
        static const uint8_t ranges[2][2] = {{0x20, 0x28}, {0x30, 0x3C}};
        uint32_t h = 2166136261u;
        for (int n = 0; n < 2; ++n) {
            for (int k = ranges[n][0]; k < ranges[n][1]; ++k) {
                h = (h ^ r[k]) * 16777619u;
            }
        }
        return 0x40000000u | (h & 0x003FFFFFu) << 8 | r[0];
    }
    return 0x80000000u | ((at >> 6) & 0x7FFFFu) << 8 | r[0];
}

void func_002B9420(void) {
    if (openrac_guest_overlay() < 0) {
        return;
    }
    const gaddr pool = GREF(gaddr, OPENRAC_DATA(PARTICLE_POOL));
    const int highest = GREF(int, OPENRAC_DATA(PARTICLE_HIGHEST));
    const gaddr table = OPENRAC_DATA(PARTICLE_TEXTURES);
    if (pool == 0 || pool >= 0x01FE0000u) {
        return;
    }
    for (int i = 0; i <= highest && i < 2048; ++i) {
        const uint8_t* r = G(pool + (gaddr)i * 0x40);
        if ((r[1] & 0x80) != 0) {
            continue;
        }
        uint64_t entry;
        memcpy(&entry, G(table + (gaddr)r[2] * 8), 8);
        openrac_game_particle(
            r,
            (uint32_t)(entry >> 32) >> 4,
            (uint32_t)entry >> 4,
            (int)((entry >> 32) & 0xF),
            particle_id(r, pool + (gaddr)i * 0x40)
        );
    }
}

/* What the console's renderers needed each frame: the texture packets, and the baked lights of
 * terrain, ties and shrubs, which the extracted level carries. */
void func_002F0468(gaddr a0) {
    (void)a0;
}

void func_002E2C38(gaddr a0) {
    (void)a0;
}

void func_002F3E78(void) {}

void func_002EF310(void) {}

void func_002F0E38(void) {}

void func_002E10B8(void) {}

void func_0029FEE0(void) {}

/* draw_owner.h: the running draw callback's argument (wide_view.c sets it). */
gaddr rac2_draw_owner;

static uint32_t effect_texture_source(uint64_t tex0, uint32_t* pixels, uint32_t* clut) {
    return openrac_effect_texture(
        tex0, GREF(int, OPENRAC_DATA(TEXTURE_UPLOADS)), OPENRAC_DATA(TEXTURE_RECORDS), pixels, clut
    );
}

/*
 * The world's quad routine (hand-written, it runs a loop from the scratchpad): a0 a quad record, the
 * same as Ratchet & Clank's (host/effect_quad.c): four corners (+0x00), four RGBA words (+0x40), four
 * ST pairs (+0x50), then CLAMP_1, TEX0_1, TEX1_1 and ALPHA_1 (+0x70..+0x88); a1, when set, a matrix
 * the corners go through first. The window draws it in the world.
 */
static void world_quad(gaddr a0, gaddr a1) {
    const uint8_t* r = G(a0);
    openrac_game_quad q;
    memset(&q, 0, sizeof(q));
    memcpy(q.corner, r, sizeof(q.corner));
    if (a1 != 0) {
        float m[16];
        memcpy(m, G(a1), sizeof(m));
        for (int k = 0; k < 4; ++k) {
            const float x = q.corner[k][0], y = q.corner[k][1], z = q.corner[k][2],
                        w = q.corner[k][3];
            for (int i = 0; i < 4; ++i) {
                q.corner[k][i] = x * m[i] + y * m[4 + i] + z * m[8 + i] + w * m[12 + i];
            }
        }
    }
    memcpy(q.rgba, r + 0x40, sizeof(q.rgba));
    memcpy(q.st, r + 0x50, sizeof(q.st));
    memcpy(&q.clamp, r + 0x70, 8);
    memcpy(&q.tex0, r + 0x78, 8);
    memcpy(&q.tex1, r + 0x80, 8);
    memcpy(&q.alpha, r + 0x88, 8);
    q.source = effect_texture_source(q.tex0, &q.pixels, &q.clut);
    q.parent = rac2_draw_owner;
    openrac_game_effect_quad(&q);
    /* Where the console's packet would have gone in the list, so that the window draws the quad
     * with the TEST and ZBUF in effect there (the game sets TEST_1 around its quads with
     * func_002FC560: 0x5360B, alpha GEQUAL 0x60 with failing pixels writing colour only, for the
     * flight's sparks and flames; 0x31801 for some). */
    const uint32_t marker = openrac_game_effect_quad_marker();
    if (marker != 0) {
        put_marker((uint64_t)OPENRAC_EFFECT_QUAD_MARKER << 48 | marker);
    }
}

void func_00280B90(gaddr a0, int a1, int a2) {
    (void)a2;
    world_quad(a0, (gaddr)a1);
}

void func_00280B90_1(gaddr a0, gaddr a1, int a2) {
    (void)a2;
    world_quad(a0, a1);
}

void func_00280B90_7(gaddr a0, gaddr a1, int a2) {
    (void)a2;
    world_quad(a0, a1);
}
