/* NON_MATCHING func_L00_00208358 -- src/overlays/shared/help_00203E98.c
 * Best so far: BYTES 11/760 (98.5% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * Cannot land as written (#define in a candidate): rewrite that in plain C first.
 * What the last attempts found:
 *   Draws the queued effect particles (DrawQ at D_0013E633+0xE1D) and a per-state glow; p5.c/p7.c/p8.c are 11 byte
 *   Only difference: in the 3-iteration loop (state 3) retail keeps the pointer in $s1 and the counter in $s0; our
 *   Would need a form that changes which pseudo the allocator picks first (not found in 10 runs).
 */
#include "common.h"
#include "include_asm.h"

extern int D_L00_0015F6A8 MACRO_ADDR;
extern unsigned char D_0013E633[] NOT_SDA;
extern unsigned char D_0015EEB0[] MACRO_ADDR;
extern float D_0015EE60 MACRO_ADDR;
typedef struct { float v[4]; } QV;
typedef struct {
    char pad[0x1DD0];
    QV vec[8];
    int val[8];
    float f0[8];
    float f1[8];
    short n;
} DrawQ;
extern void func_L00_00264690(void *, int, float, float);
extern int func_L00_0020DB30(int);
extern float func_00214D28(float *p, float target, float maxstep);

#define QD ((DrawQ *)(D_0013E633 + 0xE1D))
// Draws the queued effect particles and the per-state glow for the current game state.
void func_L00_00208358(void) {
    char *g;
    char *h;
    int i;
    float f;
    float u;
    if (D_L00_0015F6A8 == 2 || D_L00_0015F6A8 == 6) {
        for (i = 0; i < QD->n; i++) {
            func_L00_00264690(&QD->vec[i], QD->val[i], QD->f0[i], QD->f1[i]);
        }
        return;
    }
    g = (char *)D_0013E633 + 0xE1D;
    if (*(unsigned char *)(g + 0x20A4) == 3) {
        int v = *(int *)(g + 0x22E4);
        for (i = 0; i < 3; i++) {
            func_L00_00264690(g + 0x1D50 + i * 16, v, 0.2f, 0.08f);
        }
        return;
    }
    if (*(unsigned char *)(g + 0x20A4) == 0) {
        if ((func_L00_0020DB30(3) == 3 || func_L00_0020DB30(3) == 2) && *(short *)(g + 0x22D8) == 0) {
            u = 0.015f;
            *(float *)(g + 0x1D88) = *(float *)(g + 0x1D88) + u;
            func_L00_00264690(g + 0x1D80, 0x280000C0, 0.057f, 0.08f);
            *(float *)(g + 0x1D88) = *(float *)(g + 0x1D88) - u;
        }
        for (i = 0; i < QD->n; i++) {
            func_L00_00264690(&QD->vec[i], QD->val[i], QD->f0[i], QD->f1[i]);
        }
        return;
    }
    if (*(unsigned char *)(g + 0x20A4) == 2) {
        u = 0.07f;
        *(float *)(g + 0x1D88) = *(float *)(g + 0x1D88) - u;
        func_L00_00264690(g + 0x1D80, 0x300000C0, 0.25f, 0.08f);
        *(float *)(g + 0x1D88) = *(float *)(g + 0x1D88) + u;
        return;
    }
    if (*(unsigned char *)(g + 0x20A4) == 1) {
        f = 0.06f;
        if (*(short *)(g + 0x22FC) != 0) f = 0.087f;
        if (D_0015EEB0[3] != 0) {
            *(float *)(g + 0x1D88) = *(float *)(g + 0x1D88) - 0.01f;
            f = f * 1.7f;
        } else {
            *(float *)(g + 0x1D88) = *(float *)(g + 0x1D88) - 0.02f;
        }
        func_00214D28((float *)(D_0013E633 + 0x2449), f, D_0015EE60 * 0.003f);
        h = (char *)D_0013E633 + 0xE1D;
        func_L00_00264690(h + 0x1D80, *(int *)(h + 0x2300), *(float *)(h + 0x162C), 0.08f);
    }
}
