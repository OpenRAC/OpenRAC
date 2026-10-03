#include "common.h"
#include "structs.h"

/*
 * effects.cpp in the original source; text 0x1EDFF8-0x1EE9F8.
 * Name and boundary from the NTSC split of this game, mapped to PAL by matching function
 * sizes -- see docs/DECOMP_PROGRESS.md. Compiled as C for now.
 */

/* Declarations in scope here before the split. */
extern char D_0013E650[];
extern int D_0015F694;
extern void func_001F9A98(void *, void *, int);
extern char D_00189310[];
extern char D_001899D0[];
extern void *D_001871C0 NOT_SDA;
typedef struct {
    char unk_00[8];
    void (*fn_08)(void *);
    char unk_0C[4];
    void (*fn_10)(void *);
} DispatchRec;
extern DispatchRec D_001E8F80[];

INCLUDE_ASM("asm/nonmatchings/text", func_001EDFF8);

extern int func_001F4868(int);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern float func_001FA748(float, float);
extern void func_001F5E60(float, float, float, float, float, int, int, int, int, int, int, int,
                           float, float);

/*
 * Spawns a burst of particle effects around (arg1, arg2) using the source
 * object's unk10 (a size, scaled by 40.0f) and unk18 (looked up through
 * func_001F4868 to get a handle passed on to func_001F5E60). unk2C picks
 * the pattern: 0 -- a fan of unk26 particles, stepping the angle (unk1C)
 * by unk28 each time (func_001FA748 adds and wraps the angle); 1 -- four
 * particles offset from the point along and across the current angle;
 * 2 -- a single particle. Matching the case order (1, then 0 vs
 * negative, then 2 vs default) needed a real switch so gcc's own
 * binary-search lowering picked the same compare order as retail.
 *
 * The 40.0f constant is materialized once, before anything else (retail
 * loads it into $f24 before even the first call), because it is live
 * across every switch arm. The four offsets in case 1 live in a local
 * array, not scalars: retail spills them to fixed stack slots (0x0, 0x4,
 * 0x10, 0x14 with an 8-byte gap) instead of extra saved float registers.
 */
void func_001EE3B0(void *arg0, float arg1, float arg2)
{
    float forty = 40.0f;
    char *p = (char *)arg0;
    int handle = GetEffectTex(*(int *)(p + 0x18));
    int mode = *(int *)(p + 0x2C);
    float angle = *(float *)(p + 0x1C);
    float k;
    int i;

    switch (mode) {
    case 0:
        for (i = 0; i < *(short *)(p + 0x26); i++) {
            func_001F5E60(arg1, arg2, forty * *(float *)(p + 0x10),
                          forty * *(float *)(p + 0x10), angle,
                          0x3F, 0x3F, handle, 0xFFFFF3, *(int *)(p + 0x14), 0, 0,
                          0.0f, 0.0f);
            angle = FastAddRots(angle, *(float *)(p + 0x28));
        }
        break;
    case 1: {
        float tmp[6];

        tmp[0] = FastSin(angle) * forty * *(float *)(p + 0x10);
        tmp[1] = FastCos(angle) * forty * *(float *)(p + 0x10);
        tmp[4] = FastCos(angle) * forty * *(float *)(p + 0x10);
        tmp[5] = FastSin(angle) * -forty * *(float *)(p + 0x10);

        k = *(float *)(p + 0x10) * forty;
        func_001F5E60(arg1, arg2, k, k, angle,
                      0x3F, 0x3F, handle, 0xFFFFF3, *(int *)(p + 0x14), 0, 0,
                      0.0f, 0.0f);

        k = *(float *)(p + 0x10);
        k *= forty;
        func_001F5E60(arg1 + tmp[4], arg2 + tmp[5], k, k, angle,
                      0x3F, 0x3F, handle, 0xFFFFF3, *(int *)(p + 0x14), 1, 0,
                      0.0f, 0.0f);

        k = *(float *)(p + 0x10);
        k *= forty;
        func_001F5E60(arg1 - tmp[0], arg2 - tmp[1], k, k, angle,
                      0x3F, 0x3F, handle, 0xFFFFF3, *(int *)(p + 0x14), 0, 1,
                      0.0f, 0.0f);

        k = *(float *)(p + 0x10);
        k *= forty;
        func_001F5E60(arg1 + tmp[4] - tmp[0], arg2 + tmp[5] - tmp[1], k, k, angle,
                      0x3F, 0x3F, handle, 0xFFFFF3, *(int *)(p + 0x14), 1, 1,
                      0.0f, 0.0f);
        break;
    }
    case 2:
        k = *(float *)(p + 0x10);
        k *= forty;
        func_001F5E60(arg1, arg2, k, k, angle,
                      0x3F, 0x3F, handle, 0xFFFFF3, *(int *)(p + 0x14), 0, 0,
                      0.5f, 0.5f);
        break;
    }
}

LINKER_REMNANT("asm/remnants/text", func_001EE6D0);

/* The queue of glows to draw this frame: 0x30-byte records, the count
   at +0xC0. */
typedef struct {
    char pad00[0x20];
    unsigned char *moby;    /* 0x20 */
    short onScreen;         /* 0x24 */
    char pad26[0xA];
} GlowSlot;

typedef struct {
    GlowSlot slot[4];
    int count;              /* 0xC0 */
} GlowQueue;

extern GlowQueue D_00189400;
extern int D_001414D4;
extern int D_0013E600[];
extern float func_001FA888(int);
extern void func_001F2418(float *, void *);
extern void func_001EE3B0(void *, float, float);

/* Draws every queued glow whose moby is live (not in state 0xFE/0xFD):
   at its projected screen position (func_001F2418, then relative to the
   D_0013E600 offset in 1/16 units) or, for off-screen ones, at the
   screen centre; then empties the queue. Nothing in level 0x72. */
void func_001EE6E0(void) {
    float centre[4];
    float pos[4];
    int i;

    if (D_001414D4 == 0x72) {
        D_00189400.count = 0;
    }
    if (D_00189400.count != 0) {
        centre[0] = func_001FA888(D_0013E600[2]);
        centre[1] = func_001FA888(D_0013E600[3]);
        for (i = 0; i < D_00189400.count; i++) {
            GlowSlot *s = &D_00189400.slot[i];

            if (s->moby == 0 || s->moby[0x20] == 0xFE || s->moby[0x20] == 0xFD) {
                continue;
            }
            if (s->onScreen != 0) {
                projectWorldPoint(pos, s);
                pos[0] = (pos[0] - func_001FA888(D_0013E600[4])) * 0.0625f;
                pos[1] = (pos[1] - func_001FA888(D_0013E600[5])) * 0.0625f;
            } else {
                qcopy(pos, centre);
            }
            func_001EE3B0(s, pos[0], pos[1]);
        }
        D_00189400.count = 0;
    }
}

LINKER_REMNANT("asm/remnants/text", func_001EE850);

INCLUDE_ASM("asm/nonmatchings/text", func_001EE858);

INCLUDE_ASM("asm/nonmatchings/text", func_001EE9E8);
