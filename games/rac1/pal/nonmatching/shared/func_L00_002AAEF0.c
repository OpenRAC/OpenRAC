/* NON_MATCHING func_L00_002AAEF0 -- src/overlays/shared/vendor_002A5138.c
 * Best so far: SIZE ours 636 / retail 640, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Draws two mirrored rings (func_L00_001FD1D8 with a built 0x90-byte descriptor and a rotation matrix) around a 
 *   Best: p6.c/p7.c. Everything matches (movz, sd stores via long, 180.0f) except the three MACRO_ADDR stores D_L0
 *   retail has lui $at/store x3 then `daddu $a0,$s5` in the jal delay slot and `addiu $a1,$gp` early; ours puts `m
 *   in the delay slot (turned gp-relative by check_macro_slots, so size 636 vs 640). Scheduler/delay-slot choice; 
 *   [lb1 p01] p8-p12: float/int store types, source order of the B4/B8/BC stores (3 permutations), and a `rz` poin
 */
#include "common.h"
extern float func_001F9B50(float);
extern float func_L00_001FF860(float, float);
extern float func_001FA748(float, float);
extern int func_001F9850(int);
extern float func_001FA888(int);
extern float func_001F9878(float);
extern float func_001FA7D8(float x);
extern void func_001FA218(void *, void *);
extern void func_001FA540(void *, void *, void *);
extern int func_001F4868(int);
extern void func_L00_001FD1D8(void *, void *, int);
extern int D_L00_0015F6A8 MACRO_ADDR;
extern int D_L00_0015F6B0 MACRO_ADDR;
extern short D_L00_001614B0;
extern float D_L00_001614B4 MACRO_ADDR;
extern float D_L00_001614B8 MACRO_ADDR;
extern float D_L00_001614BC MACRO_ADDR;

/* draw two mirrored rotating rings around a moby */
void func_L00_002AAEF0(char *moby) {
    float w[36];
    float m1[16];
    float m2[16];
    float v[4];
    char *d = *(char **)(moby + 0x78);
    float *rz;
    int i;

    if (D_L00_0015F6A8 == 2) return;
    for (i = 0; i < 2; i++) {
        float f, g, h;
        int r;
        v[0] = func_001FA748(-func_L00_001FF860(func_001F9B50(*(float *)(d + 0x20) * *(float *)(d + 0x20) + *(float *)(d + 0x28) * *(float *)(d + 0x28)), *(float *)(d + 0x24)), 1.5707964f);
        v[1] = func_L00_001FF860(*(float *)(d + 0x28), *(float *)(d + 0x20));
        v[2] = 0;
        v[3] = 0;
        *(float *)&D_L00_001614B0 = 0.0f;
        f = func_001FA888(D_L00_0015F6B0 % func_001F9850(0xB4));
        g = func_001F9878(180.0f);
        f = f * 6.2831855f;
        h = func_001FA7D8(f / g);
        if (i & 1) h = -h;
        D_L00_001614B4 = h;
        rz = (float *)&D_L00_001614B0;
        D_L00_001614B8 = 0.0f;
        D_L00_001614BC = 0.0f;
        func_001FA218(m2, rz);
        func_001FA218(m1, v);
        func_001FA540(m1, m1, m2);
        qcopy(&m1[12], d + 0x10);
        ((int *)w)[0x10] = 0x80808080;
        ((int *)w)[0x11] = 0x80808080;
        ((int *)w)[0x12] = 0x80808080;
        ((int *)w)[0x13] = 0x80808080;
        r = 0x11;
        if (i & 1) r = 0x12;
        r = func_001F4868(r);
        *(long *)((char *)w + 0x80) = 0xFF9000000260;
        *(long *)((char *)w + 0x78) = r;
        *(long *)((char *)w + 0x88) = 0x8000000048;
        *(long *)((char *)w + 0x70) = 0;
        w[0] = -1.0f; w[1] = 0; w[2] = 1.0f; w[3] = 1.0f;
        w[4] = 1.0f; w[5] = 0; w[6] = 1.0f; w[7] = 1.0f;
        w[8] = -1.0f; w[9] = 0; w[10] = -1.0f; w[11] = 1.0f;
        w[12] = 1.0f; w[13] = 0; w[14] = -1.0f; w[15] = 1.0f;
        w[20] = 0; w[21] = 1.0f; w[22] = 1.0f; w[23] = 1.0f;
        w[24] = 0; w[25] = 0; w[26] = 1.0f; w[27] = 0;
        func_L00_001FD1D8(w, m1, 1);
    }
}
