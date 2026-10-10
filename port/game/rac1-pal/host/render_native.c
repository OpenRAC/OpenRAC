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

/* shrub_class_init (the shrub draw data) */
void func_00204340(gaddr a0, gaddr a1, gaddr a2, gaddr a3, int a4) {
    (void)a0;
    (void)a1;
    (void)a2;
    (void)a3;
    (void)a4;
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
    (void)a0;
}

/* SkyDrawShellGouraud */
void func_0022CC40(gaddr a0) {
    (void)a0;
}

/* SkySpriteProc */
void func_0022CEB8(void) {
}

/* TfragProc, the terrain renderer */
void func_002352C8(void) {
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
