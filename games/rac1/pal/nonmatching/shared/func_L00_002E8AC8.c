/* NON_MATCHING func_L00_002E8AC8 -- src/overlays/shared/vendor_002E1660.c
 * Best so far: SIZE ours 944 / retail 940, checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Clips a move of pos (param 2) against up to three planes (3x func_L00_0025ED30 / func_L00_002E89E0 passes), re
 *   Best is p3.c..p6.c (same bytes): SIZE 936/940, structure, register allocation and frame (0x160) all match by e
 *   Retail's first `func_L00_001F10E0` call builds the pad pointer with `lui $3,hi; daddu $2,$3,$0; addiu $2,$2,lo
 *   Round q27/s11: the file now declares `int func_L00_002E8AC8(void *, void *, int, float, float)` and D_L00_0017
 */
#include "common.h"

extern void func_001F9BC0(void *);
extern void func_001F9BD8(void *, void *, void *);
extern float func_L00_0025ED30(float, void *, void *, void *, void *);
extern float func_001F9D10(void *, void *);
extern int func_L00_001F10E0(float, void *, int, void *);
extern void func_001F9BF0(float *, float *, float *);
extern char *D_L00_00166F00;
extern char D_L00_00173F70[];
extern int D_L00_0015F05C MACRO_ADDR;
extern short D_L00_00161DA8;
extern char D_0013E633[];
extern void func_L00_002E89E0(char *m, float *out, float *b, float *c, float *d, float s0, float s1);

/* clips a move of pos against up to three planes, returns whether it was clipped */
extern char D_L00_00173F60_a[] __asm__("D_L00_00173F60");
int func_L00_002E8AC8(void *mv, void *posv, int unused, float sa, float sb) {
    char *m = mv;
    float *pos = posv;
    float r[4];
    float t[4];
    float o[4];
    float a[4];
    float b[4];
    float e[4];
    float w[4];
    float u[4];
    char *g = *(char **)(D_L00_00166F00 + 0x70);
    char *d = *(char **)(m + 0x70);
    char *q = d + 0x1D0;
    char *c = d + 0x130;
    char *pad;
    char *hi;
    int res = 0;
    float f21;
    float f20;
    float lim;
    f21 = *(float *)&D_L00_00161DA8 + *(float *)(g + 0x214) * *(float *)(g + 0x20C) * (*(float *)(g + 0x210) - 1.0f);
    func_001F9BC0(r);
    qcopy(o, pos);
    qcopy(a, D_L00_00173F60_a);
    qcopy(b, D_L00_00173F70);
    func_001F9BD8(e, d + 0x90, c);
    f20 = func_L00_0025ED30(0.0f, w, a, d + 0x90, e);
    lim = (*(float *)&D_L00_00161DA8 + (f21 - *(float *)&D_L00_00161DA8) * (func_001F9D10(d + 0x90, w) / (*(float *)(c + 0x2C) - *(float *)(q + 0x30)))) * 0.75f;
    if (f20 < lim) {
        res = 1;
        func_L00_002E89E0(m, r, w, a, o, lim - f20, sa);
        qcopy(pos, b);
        pad = D_0013E633 + 0xE1D;
        if (func_L00_001F10E0(sb, pos, D_L00_0015F05C, *(void **)(pad + 0x2080))) {
            func_001F9BF0(t, o, pos);
            func_001F9BD8(a, D_L00_00173F60_a, t);
            f20 = func_L00_0025ED30(0.0f, w, a, d + 0x90, e);
            lim = (*(float *)&D_L00_00161DA8 + (f21 - *(float *)&D_L00_00161DA8) * (func_001F9D10(d + 0x90, w) / (*(float *)(c + 0x2C) - *(float *)(q + 0x30)))) * 0.75f;
            if (f20 < lim) {
                func_L00_002E89E0(m, u, w, a, o, lim - f20, sa);
                func_001F9BD8(r, r, u);
                qcopy(pos, D_L00_00173F70);
                pad = D_0013E633 + 0xE1D;
                if (func_L00_001F10E0(sb, pos, D_L00_0015F05C, *(void **)(pad + 0x2080))) {
                    func_001F9BF0(t, o, pos);
                    func_001F9BD8(a, D_L00_00173F60_a, t);
                    f20 = func_L00_0025ED30(0.0f, w, a, d + 0x90, e);
                    lim = (*(float *)&D_L00_00161DA8 + (f21 - *(float *)&D_L00_00161DA8) * (func_001F9D10(d + 0x90, w) / (*(float *)(c + 0x2C) - *(float *)(q + 0x30)))) * 0.75f;
                    if (f20 < lim) {
                        func_L00_002E89E0(m, u, w, a, o, lim - f20, sa);
                        func_001F9BD8(r, r, u);
                    }
                }
            }
        }
    }
    func_001F9BD8(pos, o, r);
    return res;
}
