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

#include "game_protos.h"
#include "openrac/game_host.h"

/* BuildViewMatrices's rotation rows; the camera block is 0x370 before them and the eye 0x230. */
#define CAMERA_ROWS 0x001B5570u
#define CAMERA_BLOCK (CAMERA_ROWS - 0x370u)
#define CAMERA_EYE (CAMERA_ROWS - 0x230u)
/* The view context: +0xB0 and +0xB4 the tangents of half the field of view. */
#define VIEW_CONTEXT 0x001B9040u
/* MobyProc's inputs: the global draw distance it takes the minimum with (s7), the group masks it
 * copies to the scratchpad. */
#define MOBY_MAX_DISTANCE 0x001B1B74u
#define MOBY_GROUPS 0x001BADC0u
/* The particle pool, the records in use, and the texture table PartProc sends to the scratchpad. */
#define PARTICLE_POOL 0x001B1D90u
#define PARTICLE_COUNT 0x001B1D98u
#define PARTICLE_TEXTURES 0x001F1F40u
/* The matrix a sky shell is drawn with (func_002E3548 builds it). */
#define SKY_MATRIX 0x001B20F0u

enum { NOP_REGISTER = 0x7F };

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

/* SkyDrawShell(shell): the window draws the shell with the matrix the sky code built for it. */
void func_002E3940(int shell) {
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
    tx *= openrac_game_picture_wider();
    const float kx = sqrtf(1.0f + tx * tx), ky = sqrtf(1.0f + ty * ty);
    const gaddr end = first + (gaddr)count * 0x100u;
    const int max_distance = GREF(int, OPENRAC_DATA(MOBY_MAX_DISTANCE));
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
            const float d[3] = {sphere[0] / 1024.0f - eye[0], sphere[1] / 1024.0f - eye[1],
                                sphere[2] / 1024.0f - eye[2]};
            int distance = *(int16_t*)(b + 0x32);
            if (distance > max_distance) {
                distance = max_distance;
            }
            const float z = d[0] * rows[0] + d[1] * rows[1] + d[2] * rows[2];
            const float x = d[0] * rows[4] + d[1] * rows[5] + d[2] * rows[6];
            const float y = d[0] * rows[8] + d[1] * rows[9] + d[2] * rows[10];
            drawn = z <= (float)distance && z + r > 32.0f / 1024.0f && fabsf(x) - r * kx <= tx * z
                    && fabsf(y) - r * ky <= ty * z;
        }
        b[0x31] = (uint8_t)(drawn ? 1 : 0);
    }
}

/* MobyProc: the moby list `a0`, `a2` mobys of it (-1 the whole list, the world's draw). */
int func_002A4E68(int a0, int a1, int a2, int a3) {
    (void)a1;
    (void)a3;
    mark_drawn_mobys((gaddr)a0, a2);
    openrac_game_draw(OPENRAC_DRAW_MOBYS);
    if (openrac_game_mobys_drawn((gaddr)a0, a2)) {
        put_marker((0x4D42ull << 48) | ((uint64_t)(a2 & 0xFFFF) << 32) | (uint32_t)a0);
    }
    return 0;
}

/* The moby textures' DMA and MobyAnimProc's draw side. */
void func_002A4AD8(int a0) {
    (void)a0;
}

void func_002A4D88(int a0, int a1) {
    (void)a0;
    (void)a1;
}

/* moby_anim_eval_chain: the pose of the joints marked in a1, into the scratchpad. */
void func_002A4078(gaddr a0, gaddr a1) {
    openrac_game_moby_chain(a0, a1);
}

/*
 * PartProc: each live record of the pool (0x40 bytes, byte 1 bit 7 a free one) goes to the window, as
 * Ratchet & Clank's. Its texture is entry +0x02 of the table: the low half the palette's address << 4
 * with the CLUT offset, the high half the pixels' address << 4.
 */
void func_002B9420(void) {
    if (openrac_guest_overlay() < 0) {
        return;
    }
    const gaddr pool = GREF(gaddr, OPENRAC_DATA(PARTICLE_POOL));
    const int count = GREF(int, OPENRAC_DATA(PARTICLE_COUNT));
    const gaddr table = OPENRAC_DATA(PARTICLE_TEXTURES);
    if (pool == 0 || pool >= 0x01FE0000u) {
        return;
    }
    for (int i = 0; i < count && i < 2048; ++i) {
        const uint8_t* r = G(pool + (gaddr)i * 0x40);
        if ((r[1] & 0x80) != 0) {
            continue;
        }
        uint64_t entry;
        memcpy(&entry, G(table + (gaddr)r[2] * 8), 8);
        openrac_game_particle(r, (uint32_t)(entry >> 32) >> 4, (uint32_t)entry >> 4, 5);
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

void func_002F3E78(void) {
}

void func_002EF310(void) {
}

void func_002F0E38(void) {
}

void func_002E10B8(void) {
}

void func_0029FEE0(void) {
}
