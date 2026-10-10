/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Ratchet & Clank (PAL): the game's own renderers and the loaders that prepare their data, which the
 * native port does not run. In the retail program they are hand-written assembly (or not decompiled
 * yet) that turns level data into VU1 and GS packets; the port draws the same things with its own
 * renderers (port/renderer, game/common/frontend.cpp) from the extracted level, so here they do
 * nothing. Each is where a native renderer takes over.
 */
#include "game_protos.h"
#include "openrac/game_host.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

/*
 * Where an effect texture's pixels and CLUT are, for the TEX0 GetEffectTex (func_001F4868) gave it:
 * its upload records this frame (D_0018D140, D_0015F558 of them; +0 the CLUT's address, +6 its
 * block, +8 the pixels' address, +0xE their block), which the game's texture paging turns into GS
 * uploads; a TEX0 seen before keeps the sources it had then (the game caches the word and records
 * the upload only when it pages the texture in).
 */
static void effect_texture_source(uint64_t tex0, uint32_t* pixels, uint32_t* clut) {
    static struct {
        uint64_t tex0;
        uint32_t pixels, clut;
    } known[256];
    static int next = 0;
    const uint32_t tbp = (uint32_t)(tex0 & 0x3FFF);
    const uint32_t cbp = (uint32_t)((tex0 >> 37) & 0x3FFF);
    const int count = GREF(int, OPENRAC_DATA(0x0015F558u));
    const gaddr records = OPENRAC_DATA(0x0018D140u);
    for (int i = 0; i < count && i < 64; ++i) {
        const uint8_t* r = G(records + (gaddr)i * 16);
        if (*(const uint16_t*)(r + 0xE) == tbp && *(const uint16_t*)(r + 6) == cbp) {
            *clut = *(const uint32_t*)(r + 0);
            *pixels = *(const uint32_t*)(r + 8);
            for (int k = 0; k < 256; ++k) {
                if (known[k].tex0 == tex0) {
                    known[k].pixels = *pixels;
                    known[k].clut = *clut;
                    return;
                }
            }
            known[next].tex0 = tex0;
            known[next].pixels = *pixels;
            known[next].clut = *clut;
            next = (next + 1) & 255;
            return;
        }
    }
    for (int k = 0; k < 256; ++k) {
        if (known[k].tex0 == tex0 && tex0 != 0) {
            *pixels = known[k].pixels;
            *clut = known[k].clut;
            return;
        }
    }
}

/*
 * The draw callbacks' quad routine (ReRAC's FastDrawQuadReal): a0 a quad record, four corners
 * (+0x00, x y z w each), four RGBA words (+0x40), four ST pairs (+0x50), then CLAMP_1, TEX0_1,
 * TEX1_1 and ALPHA_1 (+0x70, +0x78, +0x80, +0x88; the GIF packet's register list); a1, when set,
 * a matrix the corners go through first (rows at +0x00..+0x30: x * row0 + y * row1 + z * row2 +
 * w * row3). The game projects the quad with its camera and sends it to the GS as a strip; the
 * window draws it in the world (openrac_game_effect_quad).
 */
void func_001F7EF8(gaddr a0, int a1, int a2) {
    (void)a2;
    const uint8_t* r = G(a0);
    openrac_game_quad q;
    memcpy(q.corner, r, sizeof(q.corner));
    if (a1 != 0) {
        float m[16];
        memcpy(m, G((gaddr)a1), sizeof(m));
        for (int k = 0; k < 4; ++k) {
            const float x = q.corner[k][0], y = q.corner[k][1], z = q.corner[k][2], w = q.corner[k][3];
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
    q.pixels = 0;
    q.clut = 0;
    effect_texture_source(q.tex0, &q.pixels, &q.clut);
    openrac_game_effect_quad(&q);
}

/* a quad renderer */
void func_001F8B6C(void) {
}

/* a quad renderer */
void func_001F91B8(void) {
}

/*
 * shrub_class_init: registers a shrub class (a0) under its class number (a4) and slot. What the level
 * loader reads afterwards is kept: the slot tables (class number to slot, slot to class number and to
 * class), the class's draw distance, its offsets made pointers, and its 16-byte block (a2). What is
 * left out is the GS texture registers it writes into the class's packets (a1, a3), which only the
 * game's shrub renderer reads. In a level, its tables are the level program's copies.
 */
void func_00204340(gaddr a0, gaddr a1, gaddr a2, gaddr a3, int a4) {
    (void)a1;
    (void)a3;
    uint8_t* cls = G(a0);
    const uint32_t slot = GREF(uint32_t, OPENRAC_DATA(0x001604CCu));

    GREF(uint8_t, OPENRAC_DATA(0x001D8440u) + (uint32_t)a4) = (uint8_t)slot;
    GREF(uint16_t, OPENRAC_DATA(0x001D83C0u) + slot * 2) = (uint16_t)a4;
    GREF(gaddr, OPENRAC_DATA(0x001D82C0u) + slot * 4) = a0;
    *(uint16_t*)(cls + 0x26) = (uint16_t)slot;

    float distance;
    memcpy(&distance, cls + 0x10, 4);
    GREF(int, OPENRAC_DATA(0x001D9040u) + slot * 4) = func_001FA898(distance * 1024.0f);
    GREF(uint32_t, OPENRAC_DATA(0x001604CCu)) = slot + 1;

    *(uint32_t*)(cls + 0x18) = 0;
    *(uint16_t*)(cls + 0x16) = 0;
    if (*(uint32_t*)(cls + 0x2C) != 0) {
        *(uint32_t*)(cls + 0x2C) += a0;
    }
    const int16_t packets = *(int16_t*)(cls + 0x28);
    for (int i = 0; i < packets; ++i) {
        *(uint32_t*)(cls + 0x40 + i * 8) += a0;
    }
    if (*(uint32_t*)(cls + 0x1C) != 0) {
        *(uint32_t*)(cls + 0x1C) += a0;
    }

    memcpy(G(OPENRAC_DATA(0x001D9640u) + slot * 16), G(a2), 16);
}

/* a moby texture DMA */
void func_00212258(int a0) {
    (void)a0;
}

/* a moby renderer step */
void func_00212508(void) {
}

/* MobyAnimProc (the draw side) */
void func_00212578(int a0, int a1) {
    (void)a0;
    (void)a1;
}

/*
 * What MobyProc decides for each moby of the list before drawing it, which the game reads back:
 * +0x31, drawn this frame (the moby loop keeps a drawn moby active; vehicles, enemies and effects
 * test it). Its culls, as ReRAC's moby_lod.rs replays them: the bounding sphere (+0x00, x y z r in
 * units of 1/1024) beyond the draw distance (+0x32, s16 units), wholly in front of the near plane,
 * or wholly outside a side plane of the view; a hidden moby (mode bit 0) is not drawn.
 */
static void mark_drawn_mobys(gaddr first, int count) {
    float cam[3], rows[12];
    memcpy(cam, G(OPENRAC_DATA(0x00187180u)), sizeof(cam));
    memcpy(rows, G(OPENRAC_DATA(0x00187390u)), sizeof(rows));
    float tx, ty;
    memcpy(&tx, G(OPENRAC_DATA(0x0018CE00u) + 0xB0), 4);
    memcpy(&ty, G(OPENRAC_DATA(0x0018CE00u) + 0xB4), 4);
    if (!(tx > 0.05f && tx < 10.0f)) {
        tx = 0.63f;
    }
    if (!(ty > 0.05f && ty < 10.0f)) {
        ty = tx * 0.756f;
    }
    const float kx = sqrtf(1.0f + tx * tx), ky = sqrtf(1.0f + ty * ty);
    gaddr end = count < 0 ? GREF(gaddr, OPENRAC_DATA(0x00160020u)) : first + (gaddr)count * 0x100u;
    if (end < first || end - first > 0x100u * 4096u) {
        return;
    }
    for (gaddr m = first; m < end; m += 0x100) {
        uint8_t* b = G(m);
        float sphere[4];
        memcpy(sphere, b, sizeof(sphere));
        const float r = sphere[3] / 1024.0f;
        const float d[3] = {sphere[0] / 1024.0f - cam[0], sphere[1] / 1024.0f - cam[1], sphere[2] / 1024.0f - cam[2]};
        const float z = d[0] * rows[0] + d[1] * rows[1] + d[2] * rows[2];
        const float x = d[0] * rows[4] + d[1] * rows[5] + d[2] * rows[6];
        const float y = d[0] * rows[8] + d[1] * rows[9] + d[2] * rows[10];
        const int distance = *(int16_t*)(b + 0x32);
        const uint16_t mode = *(uint16_t*)(b + 0x34);
        int drawn = (mode & 1) == 0 && b[0x20] != 0xFF;
        drawn = drawn && z <= (float)distance && z + r > 32.0f / 1024.0f;
        drawn = drawn && fabsf(x) - r * kx <= tx * z && fabsf(y) - r * ky <= ty * z;
        b[0x31] = (uint8_t)(drawn ? 1 : 0);
    }
}

/* the moby renderer */
int func_00212658(int a0, int a1, int a2, int a3) {
    mark_drawn_mobys((gaddr)a0, a2);
    openrac_game_draw(OPENRAC_DRAW_MOBYS);
    openrac_game_mobys_drawn((gaddr)a0, a2);
    (void)a0;
    (void)a1;
    (void)a2;
    (void)a3;
    return 0;
}

/*
 * PartProc, the particle renderer: each live record of the pool (2048 records of 0x40 bytes at the
 * pool pointer; byte 1 bit 7 marks a free one; the highest live index is kept with the pool's
 * counters) goes to the window, which draws it as the game's VU1 sprite program does
 * (openrac_game_particle). Its texture is entry +0x02 of the particle texture table, which sits
 * 0x200 bytes past the pool's allocation bitmap: (palette address << 4 | CLUT offset, pixels address
 * << 4 | log2 side).
 */
void func_00218B10(void) {
    if (openrac_guest_overlay() < 0) {
        return;  // the boot program (title, menus, the flight) keeps no particle pool there
    }
    // The pool's globals are the level programs' own (level 0's addresses, relocated to the level
    // loaded); the boot program keeps other things there.
    const gaddr pool = GREF(gaddr, OPENRAC_LDATA(0, 0x0016022Cu));
    const int high = GREF(int, OPENRAC_LDATA(0, 0x00160234u));
    const gaddr table = OPENRAC_LDATA(0, 0x001B1C00u) + 0x200;
    if (pool == 0 || pool >= 0x01FE0000u || table >= 0x01FFF000u) {
        return;
    }
    for (int i = 0; i <= high && i < 2048; ++i) {
        const uint8_t* r = G(pool + (gaddr)i * 0x40);
        if ((r[1] & 0x80) != 0) {
            continue;
        }
        uint64_t lo, hi;
        memcpy(&lo, G(table + (gaddr)r[2] * 16), 8);
        memcpy(&hi, G(table + (gaddr)r[2] * 16 + 8), 8);
        openrac_game_particle(r, (uint32_t)(hi >> 4), (uint32_t)(lo >> 4), (int)(hi & 0xF));
    }
}

/* a shadow renderer */
void func_00228A58(void) {
}

/* a shadow renderer */
void func_00228D20(unsigned int a0, int a1, int a2) {
    (void)a0;
    (void)a1;
    (void)a2;
}

/* a shadow renderer */
int func_00229098(void) {
    return 0;
}

/* a shadow renderer */
int func_002291E8(void) {
    return 0;
}

/* a shadow renderer */
void func_00229838(gaddr a0, gaddr a1) {
    (void)a0;
    (void)a1;
}

/* a shadow renderer */
void func_002298B0(int a0, int a1) {
    (void)a0;
    (void)a1;
}

/* ShrubProc, the shrub renderer */
void func_00229F00(void) {
    openrac_game_draw(OPENRAC_DRAW_SHRUBS);
}

/* BuildShrubTextureDma */
int func_0022B648(int a0) {
    (void)a0;
    return 0;
}

/* LightShrubs */
void func_0022B8F8(gaddr a0, gaddr a1) {
    (void)a0;
    (void)a1;
}

/* SkyDrawShellTextured */
void func_0022CA00(gaddr a0) {
    openrac_game_draw(OPENRAC_DRAW_SKY);
    (void)a0;
}

/* SkyDrawShellGouraud */
void func_0022CC40(gaddr a0) {
    openrac_game_draw(OPENRAC_DRAW_SKY);
    (void)a0;
}

/*
 * SkySpriteProc: the sky's sprites (stars, glows), from the sky header (+0x08 their count, +0x10 the
 * texture table, 16 bytes an entry with the TEX0 first, +0x1C the 0x20-byte records; a negative
 * texture byte +0x02 is not drawn), each to the window (openrac_game_sky_sprite).
 */
void func_0022CEB8(void) {
    openrac_game_draw(OPENRAC_DRAW_SKY);
    const gaddr sky = GREF(gaddr, OPENRAC_DATA(0x0016055Cu));
    if (sky == 0 || sky >= 0x01FFF000u) {
        return;
    }
    const gaddr records = GREF(gaddr, sky + 0x1C);
    const int count = GREF(uint16_t, sky + 0x08);
    const gaddr textures = GREF(gaddr, sky + 0x10);
    if (records == 0 || textures == 0 || records >= 0x01FFF000u) {
        return;
    }
    for (int i = 0; i < count && i < 1024; ++i) {
        const uint8_t* r = G(records + (gaddr)i * 0x20);
        if ((int8_t)r[2] < 0) {
            continue;
        }
        uint64_t tex0;
        memcpy(&tex0, G(textures + (gaddr)r[2] * 16), 8);
        openrac_game_sky_sprite(r, tex0);
    }
}

/* TfragProc, the terrain renderer */
void func_002352C8(void) {
    openrac_game_draw(OPENRAC_DRAW_TERRAIN);
}

/* ComputeTfragTextureUsage */
void func_00235EF0(void) {
}

/* the tfrag texture DMA */
int func_00236060(int a0) {
    (void)a0;
    return 0;
}

/* LightTfrags */
void func_002362B0(gaddr a0) {
    (void)a0;
}

/* TieProc, the tie renderer */
void func_00236F00(void) {
    openrac_game_draw(OPENRAC_DRAW_TIES);
}

/* BuildTieTextureDma */
int func_002383D8(int a0) {
    (void)a0;
    return 0;
}

/* LightTies */
void func_00238688(gaddr a0, gaddr a1) {
    (void)a0;
    (void)a1;
}
/* PatchMobyGifs: texture addresses into the GS packets of the console's renderer */
void func_0020DD48(void) {
}

/* PatchShrubGifs: texture addresses into the GS packets of the console's renderer */
void func_00229D48(void) {
}

/* PatchTfragGifs: texture addresses into the GS packets of the console's renderer */
void func_00234620(void) {
}

/* PatchTieGifs: texture addresses into the GS packets of the console's renderer */
void func_00236A98(void) {
}
/* drawquad.c: a renderer of the console's (the port draws natively) */
int func_001F7C60(void) {
    return 0;
}

/* drawquad.c: a renderer of the console's (the port draws natively) */
int func_001F7DD8(void) {
    return 0;
}

/* drawquad.c: a renderer of the console's (the port draws natively) */
int func_001F7E98(void) {
    return 0;
}

/* drawquad.c: a renderer of the console's (the port draws natively) */
int func_001F84AC(void) {
    return 0;
}

/* drawquad.c: a renderer of the console's (the port draws natively) */
int func_001F852C(void) {
    return 0;
}

/* drawquad.c: a renderer of the console's (the port draws natively) */
int func_001F856C(void) {
    return 0;
}

/* skyproc.c: a renderer of the console's (the port draws natively) */
int func_0022D2AC(int a0, int a1, int a2, int a3) {
    (void)a0;
    (void)a1;
    (void)a2;
    (void)a3;
    return 0;
}

/* skyproc.c: a renderer of the console's (the port draws natively) */
void func_0022D3F8(int a0, int a1, int a2, int a3) {
    (void)a0;
    (void)a1;
    (void)a2;
    (void)a3;
}

/* skyproc.c: a renderer of the console's (the port draws natively) */
void func_0022D520(int a0, int a1, int a2, int a3) {
    (void)a0;
    (void)a1;
    (void)a2;
    (void)a3;
}

/* skyproc.c: a renderer of the console's (the port draws natively) */
void func_0022D7E0(gaddr a0, int a1, gaddr a2) {
    (void)a0;
    (void)a1;
    (void)a2;
}

/* drawquad_001FD1D8.c: a renderer of the console's (the port draws natively) */
void func_L00_001FD1D8(gaddr a0, gaddr a1, int a2) {
    (void)a0;
    (void)a1;
    (void)a2;
}

/* drawquad_001FD1D8.c: a renderer of the console's (the port draws natively) */
int func_L00_001FE688(void) {
    return 0;
}

/* skyproc_0028D958.c: a renderer of the console's (the port draws natively) */
int func_L00_0028D958(void) {
    return 0;
}

/* tfragproc_002963D8.c: a renderer of the console's (the port draws natively) */
int func_L00_002963D8(void) {
    return 0;
}

/* tieproc_00299108.c: a renderer of the console's (the port draws natively) */
void func_L00_00299B68(int a0) {
    (void)a0;
}
/* moby_anim_eval_chain (hand-written VU0, 2640 bytes): the pose matrices of the joints marked in
 * a1 for the moby a0, into the scratchpad (MobyGetBoneMatrix reads them: the menu panels' corners,
 * items in Ratchet's hands). The port's evaluator (viewer/moby_pose, after ReRAC's evaluate_chains;
 * post-scale records are not skipped yet as the chain form does). */
void func_00211808(gaddr a0, gaddr a1) {
    openrac_game_moby_chain(a0, a1);
}
