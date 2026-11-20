/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal: the game's own renderers, which the native port does not run. As Going
 * Commando's (rac2/host/render.c) and Ratchet & Clank's (rac1/host/render_native.c): on the console
 * they are hand-written assembly that turns level data into VU1 and GS packets; the port draws the
 * same things with its own renderers from the extracted level (editor/extract.py port, --levels).
 * The frame's draw pass (DrawFrame, Veldin's 0x00403440, Going Commando's func_0027BD70) still runs:
 * its passes end here, where the window is told which layers the game drew, with what camera, and
 * where the world goes in the display list.
 *
 * Every level's program carries its own copy of the engine. These are written with Veldin's copy
 * (the names rac3-decomp's catalogue gives the levels' places, hostgen.json "places"), and its
 * globals go through OPENRAC_LDATA(1, ...) to the loaded level's. The multiplayer maps (levels 39 to
 * 53) share most of them, but DrawTfrag, the tie and shrub passes and MobyProc have copies of their
 * own there, named after level 39's places, whose globals go through OPENRAC_LDATA(39, ...). Each
 * was paired with Going Commando's function instruction by instruction
 * (port/game/rac3-pal/IDENTIFIED.tsv): DrawFrame's calls line up one for one with Going Commando's.
 */
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "game/common/lib/effect_texture.h"
#include "game_protos.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"

/* A global of Veldin's program, in the program loaded now. */
#define VELDIN(a) OPENRAC_LDATA(1, (gaddr)(a))

/* The globals a copy of the renderers reads, at the addresses of the level it is written with
 * (`from`), relocated to the loaded level's (OPENRAC_LDATA). */
typedef struct Copy {
    int from;
    /* The display list's write pointer (DrawTfrag reads it first; VU1_addGSregister appends
     * through it). */
    gaddr list_pointer;
    /* BuildViewMatrices's rotation rows (the profile's camera_rows): the view block 0x160 before
     * them, the eye 0x20 before them. */
    gaddr camera_rows;
    /* The view context: +0xB0 and +0xB4 the tangents of half the field of view. */
    gaddr view_context;
    /* MobyProc's inputs: the cap on a moby's draw distance ($s7 from $gp) and the group masks it
     * copies to the scratchpad (Going Commando's 0x001B1B74 and 0x001BADC0). */
    gaddr moby_max_distance;
    gaddr moby_groups;
} Copy;

/* Veldin's (the single-player levels'): MobyProc's cap is $gp + 0x37A0. */
static const Copy kVeldin = {1, 0x001E1068u, 0x001E29E0u, 0x001E7580u, 0x001E0050u, 0x001EA840u};
/* The multiplayer menu's (the multiplayer maps'): MobyProc's cap is $gp + 0x3FD0. */
static const Copy kMultiplayer =
    {39, 0x0024CAD0u, 0x0024DE20u, 0x0025D6C0u, 0x001E0880u, 0x0025F580u};

static gaddr at_copy(const Copy* c, gaddr address) {
    return OPENRAC_LDATA(c->from, address);
}

/* The sky: its header (+0x06 the shell count, +0x20 the shells) and the matrix the shell being
 * drawn takes (0x004A7E10 builds it from the shell's angles before each 0x004A8158). */
#define SKY_MATRIX 0x001E0E10u

enum {
    NOP_REGISTER = 0x7F
};

/* A marker in the display list: the A+D write of `value` to the chip's NOP register (the packet
 * 0x004C5B90, Going Commando's func_002FC560, appends: a DMA tag of two quadwords, DIRECT, a GIF
 * tag of one A+D register). The chip ignores it; the window reads its top 16 bits. Written here
 * rather than through the translated routine, whose second argument is a word: the value is a
 * doubleword. */
static void put_marker(const Copy* c, uint64_t value) {
    const gaddr pointer = at_copy(c, c->list_pointer);
    const gaddr at = GREF(gaddr, pointer);
    if (at == 0 || at >= 0x02000000u - 0x30u) {
        return;
    }
    uint32_t* w = (uint32_t*)G(at);
    w[0] = 0x10000002u;
    w[1] = 0;
    w[2] = 0;
    w[3] = 0x50000002u;
    w[4] = 0x8001u;
    w[5] = 0x10000000u;
    w[6] = 0xEu;
    w[7] = 0;
    memcpy(G(at + 32u), &value, 8);
    w[10] = NOP_REGISTER;
    w[11] = 0;
    GREF(gaddr, pointer) = at + 0x30u;
}

/* DrawTfrag (Going Commando's func_002EF3B0, the same 208 bytes): the world's first native pass
 * after the sky, so the world's marker goes here. */
static void draw_tfrag(const Copy* c) {
    openrac_game_draw(OPENRAC_DRAW_TERRAIN);
    put_marker(c, 0x574Cull << 48);
}

int func_L01_004B1728(void) {
    draw_tfrag(&kVeldin);
    return 0;
}

int func_L39_005F4690(void) {
    draw_tfrag(&kMultiplayer);
    return 0;
}

/* The tie pass of DrawFrame (func_002F0EF8). */
int func_L01_004B3238(void) {
    openrac_game_draw(OPENRAC_DRAW_TIES);
    return 0;
}

int func_L39_005F61A0(void) {
    openrac_game_draw(OPENRAC_DRAW_TIES);
    return 0;
}

/* The shrub pass of DrawFrame (func_002E11C0). */
int func_L01_004A59F8(void) {
    openrac_game_draw(OPENRAC_DRAW_SHRUBS);
    return 0;
}

int func_L39_005EDE28(void) {
    openrac_game_draw(OPENRAC_DRAW_SHRUBS);
    return 0;
}

/* SkyDrawShell(shell) (func_002E3940): the window draws the shell with the matrix the sky code
 * built for it. The sky is the world's first pass, so its camera is the world's. */
int func_L01_004A8158(int shell) {
    openrac_game_world_view();
    float m[16];
    memcpy(m, G(VELDIN(SKY_MATRIX)), sizeof(m));
    openrac_game_sky_shell(shell, m);
    openrac_game_draw(OPENRAC_DRAW_SKY);
    return 0;
}

/*
 * MobyProc's drawn flag (+0x31), as the console sets it (Going Commando's mark_drawn_mobys,
 * rac2/host/render.c): group mask, mode bits 0x81, alpha below 8, draw distance (+0x32, capped)
 * and the game's 4:3 view. It is game logic, not only drawing (the moby loop updates a drawn moby,
 * the lock-on drops one not drawn), so it is the console's test whatever the window draws.
 */
static void mark_drawn_mobys(const Copy* c, gaddr first, int count) {
    float eye[3], block[12], rows[12], tx, ty;
    memcpy(eye, G(at_copy(c, c->camera_rows - 0x20u)), sizeof(eye));
    memcpy(block, G(at_copy(c, c->camera_rows - 0x160u)), sizeof(block));
    for (int c = 0; c < 3; ++c) {
        rows[c] = block[c * 4 + 2];
        rows[4 + c] = -block[c * 4 + 0];
        rows[8 + c] = -block[c * 4 + 1];
    }
    const gaddr view_context = at_copy(c, c->view_context);
    memcpy(&tx, G(view_context + 0xB0), 4);
    memcpy(&ty, G(view_context + 0xB4), 4);
    if (!(tx > 0.05f && tx < 10.0f)) {
        tx = 0.63f;
    }
    if (!(ty > 0.05f && ty < 10.0f)) {
        ty = tx * 0.756f;
    }
    const float kx = sqrtf(1.0f + tx * tx), ky = sqrtf(1.0f + ty * ty);
    const gaddr end = first + (gaddr)count * 0x100u;
    const int max_distance = GREF(int, at_copy(c, c->moby_max_distance));
    const uint8_t* groups = G(at_copy(c, c->moby_groups));
    if (first == 0 || first >= 0x02000000u) {
        return;
    }
    for (gaddr m = first; (count < 0 || m != end) && m - first < 0x100u * 4096u; m += 0x100) {
        uint8_t* b = G(m);
        const int8_t state = (int8_t)b[0x20];
        if (state == -1) {
            break;
        }
        if (state < 0) {
            continue;
        }
        int drawn = (b[0x36] & groups[b[0x37]]) != 0 && (*(uint16_t*)(b + 0x34) & 0x81) == 0
                    && b[0x23] >= 8;
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
            drawn = z <= (float)distance && z + r > 32.0f / 1024.0f && fabsf(x) - r * kx <= tx * z
                    && fabsf(y) - r * ky <= ty * z;
        }
        b[0x31] = (uint8_t)(drawn ? 1 : 0);
    }
}

/* MobyProc (func_002A4E68, hand-written): the moby list `a0`, `a2` mobys of it (-1 the whole
 * list, the world's draw). */
static void moby_proc(const Copy* c, gaddr list, int count) {
    mark_drawn_mobys(c, list, count);
    if (count < 0) {
        put_marker(c, (uint64_t)OPENRAC_WORLD_MOBYS_MARKER << 48);
    }
    openrac_game_draw(OPENRAC_DRAW_MOBYS);
    if (openrac_game_mobys_drawn(list, count)) {
        put_marker(c, (0x4D42ull << 48) | ((uint64_t)(count & 0xFFFF) << 32) | (uint32_t)list);
    }
}

int func_L01_00441B78(int a0, int a1, int a2, int a3) {
    (void)a1;
    (void)a3;
    moby_proc(&kVeldin, (gaddr)a0, a2);
    return 0;
}

int func_L39_005B25C8(int a0, int a1, int a2, int a3) {
    (void)a1;
    (void)a3;
    moby_proc(&kMultiplayer, (gaddr)a0, a2);
    return 0;
}

/* The moby textures' DMA (func_002A4AD8) and MobyAnimProc's draw side (func_002A4D88; the boot
 * program's copy is 0x003CB6B8 below). */
int func_L01_004417E8(int a0) {
    (void)a0;
    return 0;
}

int func_L01_00441A98(int a0, int a1) {
    (void)a0;
    (void)a1;
    return 0;
}

/* moby_anim_eval_chain (func_002A4078): the pose of the joints marked in a1, into the scratchpad. */
int func_L01_00440090(int a0, int a1) {
    openrac_game_moby_chain((gaddr)a0, (gaddr)a1);
    return 0;
}

/*
 * PartProc (func_002B9420, the same instructions): each live record of the pool (0x40 bytes, byte
 * 1 bit 7 a free one), slots 0 to the highest in use, to the window, as Going Commando's. Its
 * texture is entry +0x02 of the table PartProc sends to the scratchpad (Veldin's 0x00220A40, Going
 * Commando's 0x001F1F40): the low half the palette's address << 4, the high half the pixels'
 * address << 4 with the log2 of the side.
 */
#define PARTICLE_POOL 0x001E0278u
#define PARTICLE_HIGHEST 0x001E0280u
#define PARTICLE_TEXTURES 0x00220A40u

int func_L01_00461CB8(void) {
    const gaddr pool = GREF(gaddr, VELDIN(PARTICLE_POOL));
    const int highest = GREF(int, VELDIN(PARTICLE_HIGHEST));
    const gaddr table = VELDIN(PARTICLE_TEXTURES);
    if (pool == 0 || pool >= 0x01FE0000u) {
        return 0;
    }
    for (int i = 0; i <= highest && i < 2048; ++i) {
        const gaddr at = pool + (gaddr)i * 0x40;
        const uint8_t* r = G(at);
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
            0x80000000u | ((at >> 6) & 0x7FFFFu) << 8 | r[0]
        );
    }
    return 0;
}

/*
 * The screen glow (Up Your Arsenal's own; Going Commando has none): DrawFrame, while the word
 * 0x001DBCA0 is set, appends two prebuilt GS packets by DMA reference. 0x004DE670 the first
 * (0x001D0A50, built by 0x004DDE98: the frame at block 0x1E00 copied into a 256 x 256 buffer at
 * FBP 0x198 and shrunk into FBP 0 and 0x1FD), 0x004DE6F0 the second (0x001D2A50, built by
 * 0x004DE5B0: blurred and added back over the frame, ALPHA 0x221, FIX 0x80). Replayed over the
 * native world they drew a ghost of the picture and a copy of it in its top left corner. The port
 * leaves them out: the enhanced renderer's bloom (enhanced/docs/bloom.md) is its glow. The word
 * each sets for the frame (0x6000 at $gp - 0xC0C, 0 again after) is kept.
 */
#define GLOW_WORD 0x001DBCA4u

int func_L01_004DE670(void) {
    GREF(int32_t, VELDIN(GLOW_WORD)) = 0x6000;
    return 0;
}

int func_L01_004DE6F0(void) {
    GREF(int32_t, VELDIN(GLOW_WORD)) = 0;
    return 0;
}

/* The second screen effect (0x004DF310, while 0x001DBCBC is set; DrawFrame clears it after): the
 * same family, two more prebuilt packets, 0x001D3650 (0x004DF070: the frame copied and blurred
 * twice) and 0x001D3EF0 (0x004DF1B0: tinted and added back over the frame). Left out as the glow
 * is. */
int func_L01_004DF310(void) {
    return 0;
}

/* The effect textures' uploads this frame: their count and the records GetEffectTex (Veldin's
 * 0x00404040, Going Commando's func_0027C878) writes, 16 bytes each as Going Commando's. */
#define TEXTURE_UPLOADS 0x001DF97Cu
#define TEXTURE_RECORDS 0x001E7880u

static uint32_t effect_texture_source(uint64_t tex0, uint32_t* pixels, uint32_t* clut) {
    return openrac_effect_texture(
        tex0, GREF(int, VELDIN(TEXTURE_UPLOADS)), VELDIN(TEXTURE_RECORDS), pixels, clut
    );
}

/*
 * DrawWorldQuad (0x00407E28, Going Commando's func_00280B90, the same 608 bytes): the draw
 * callbacks' world quad. a0 a quad record as Going Commando's: four corners (+0x00), four RGBA words
 * (+0x40), four ST pairs (+0x50), then CLAMP_1, TEX0_1, TEX1_1 and ALPHA_1 (+0x70..+0x88); a1, when
 * set, a matrix the corners go through first. The window draws it in the world, with the TEST and
 * ZBUF in effect at its marker (rac2/host/render.c's world_quad).
 */
int func_L01_00407E28(int a0, int a1) {
    const uint8_t* r = G((gaddr)a0);
    openrac_game_quad q;
    memset(&q, 0, sizeof(q));
    memcpy(q.corner, r, sizeof(q.corner));
    if (a1 != 0) {
        float m[16];
        memcpy(m, G((gaddr)a1), sizeof(m));
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
    openrac_game_effect_quad(&q);
    const uint32_t marker = openrac_game_effect_quad_marker();
    if (marker != 0) {
        put_marker(&kVeldin, (uint64_t)OPENRAC_EFFECT_QUAD_MARKER << 48 | marker);
    }
    return 0;
}

/* What the console's renderers needed each frame (Going Commando's func_002F0468, 0x002EF310,
 * 0x002F3E78, 0x002F0E38, 0x002E2C38, 0x002E10B8, 0x0029FEE0): the texture packets, and the baked
 * lights of terrain, ties and shrubs, which the extracted level carries. */
int func_L01_004B27B8(int a0) {
    (void)a0;
    return 0;
}

int func_L01_004B1688(void) {
    return 0;
}

int func_L01_004B6ED0(void) {
    return 0;
}

int func_L01_004B3178(void) {
    return 0;
}

int func_L01_004A75E0(void) {
    return 0;
}

int func_L01_004A58F0(void) {
    return 0;
}

int func_L01_0043AAC0(void) {
    return 0;
}

/* 0x003CB6B8 (boot_elf, hand-written; Going Commando's func_002A4D88, MobyAnimProc's draw side):
 * draws the moby list (D_001D9D30, D_001D9D38) through 0x003C81D8 with VU0 and the scratchpad,
 * called by 0x003C39A8 in the level loading screen's pass (0x003C3BB8). Translated from its
 * instructions it read past the scratchpad (0x700045FC). */
void func_003CB6B8(int list, int count) {
    (void)list;
    (void)count;
}
