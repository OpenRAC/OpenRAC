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

#include <stdint.h>
#include <string.h>

/* a quad renderer */
void func_001F7EF8(gaddr a0, int a1, int a2) {
    (void)a0;
    (void)a1;
    (void)a2;
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

/* the moby renderer */
int func_00212658(int a0, int a1, int a2, int a3) {
    openrac_game_draw(OPENRAC_DRAW_MOBYS);
    openrac_game_mobys_drawn((gaddr)a0, a2);
    (void)a0;
    (void)a1;
    (void)a2;
    (void)a3;
    return 0;
}

/* PartProc, the particle renderer */
void func_00218B10(void) {
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

/* SkySpriteProc */
void func_0022CEB8(void) {
    openrac_game_draw(OPENRAC_DRAW_SKY);
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
/* The space flight's quads (loading between planets), for the console's renderer. */
void func_0022F738(int a0) {
    (void)a0;
}
