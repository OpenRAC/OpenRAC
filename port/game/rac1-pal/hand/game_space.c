/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */

/*
 * Ratchet & Clank (PAL): a draw callback of the space scenes, in C.
 *
 * The decompilation keeps func_0022F4C0 as assembly: its first two instructions are another
 * function's leftover epilogue, and the callback's real entry is eight bytes in, where the game
 * points its draw callbacks (space.c: resident_effect_entry + 8). Written from the instructions.
 */
#include "common.h"

extern char D_0013E130[];     /* the space scene: +0x00 the camera, +0x26 its mode */
extern int D_001605C0[];      /* per mode, how many glows */
extern char D_001D9C40[];     /* per mode, the glows: position, w their size */
extern char D_001D9CC0[];
extern char D_001D9CE0[];
extern char D_001D9C20[];     /* the quad's four texture coordinates */
extern char D_001D9D00[];     /* the quad's four corners */
extern long func_001F4868_s(int) __asm__("func_001F4868");
extern void func_001F7EF8(void *, int, int);
extern void func_001F9C30(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_001F9EC0(void *, void *, void *);
extern float func_001FA888(int);
extern int func_002140B0(int);

/* What func_001F7EF8 draws: four corners, their colours and texture coordinates, then the
 * texture's GS registers. */
struct glow_quad {
    float corner[4][4];
    unsigned int colour[4];
    float uv[4][2];
    unsigned long reg[4];
    float origin[4];
    float *at;
};

/*
 * Draws the scene's glows around a moby: for each glow of the camera mode, a quad facing the
 * camera at the glow's place, sized by the glow and by the moby's alpha byte (+0xBC, plus a
 * flicker when +0xB2 is set), orange (green-blue for class 0x215).
 */
void func_0022F4C8(unsigned char *moby) {
    struct glow_quad q;
    int mode = *(short *)(D_0013E130 + 0x26);
    char *glows = mode == 1 ? D_001D9CC0 : mode == 2 ? D_001D9CE0 : D_001D9C40;
    int i;
    int j;

    q.reg[1] = (unsigned long)func_001F4868_s(5);
    q.at = q.origin;
    q.reg[2] = (0xFF90ul << 32) | 0x260;
    q.reg[3] = (0x8000ul << 24) | 0x48;
    q.reg[0] = 0;
    for (j = 0; j < 4; j++) {
        q.uv[j][0] = ((float *)D_001D9C20)[j * 2];
        q.uv[j][1] = ((float *)D_001D9C20)[j * 2 + 1];
    }
    func_001F9C30(q.at, moby, 1.0f / 1024.0f);
    for (i = 0; i < D_001605C0[*(short *)(D_0013E130 + 0x26)]; i++) {
        int alpha = moby[0xBC];
        unsigned int colour;
        float size;

        if (*(short *)(moby + 0xB2) != 0) {
            alpha += func_002140B0(*(short *)(moby + 0xB2));
        }
        size = func_001FA888(alpha) * (*(float *)(glows + i * 16 + 0xC) / 40.0f);
        colour = ((unsigned int)alpha << 24) | 0x2058B0;
        if (*(short *)(moby + 0xA6) == 0x215) {
            colour = ((unsigned int)alpha << 24) | 0x308000;
        }
        for (j = 0; j < 4; j++) {
            q.colour[j] = colour;
            func_001F9C30(q.corner[j], D_001D9D00 + j * 16, size);
            func_001F9BD8(q.corner[j], q.corner[j], glows + i * 16);
            func_001F9EC0(q.corner[j], q.corner[j], (char *)*(int *)D_0013E130 + 0xC0);
            func_001F9BD8(q.corner[j], q.corner[j], q.at);
        }
        func_001F7EF8(&q, 0, 0);
    }
}

/* The leftover entry: the same callback (its epilogue's stack step aside). */
void func_0022F4C0(unsigned char *moby) {
    func_0022F4C8(moby);
}
