/* NON_MATCHING func_L14_002B4828 -- src/overlays/shared/vendor_002B2A28.c
 * Best so far: SIZE ours 476 / retail 472, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Pitch/yaw steering of a moby (float f12 arg x; returns func_001FA850 result): p2.c/p5.c are closest (476 vs 47
 *   Remaining: in the second func_L00_0025CE58 call ours CSEs d+0x280/d+0x284 into extra saved regs (s0/s1) where 
 */
#include "common.h"
extern char D_0013E633[];
extern float func_001FA790(float, float);
extern float func_L00_0025CE58(float *, float *, float, float, float, float);
extern void func_L00_001FFED8(void *, int, float);
extern void func_001F9BF0(void *dst, void *a, void *b);
extern float func_001F9CE8(void *);
extern float func_L00_001FF860(float, float);
extern void func_001FA588(void *, void *, void *);
extern float func_001FA850(float, float);
extern float D_0015EE70 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;

// steers a moby's pitch toward a target, and its yaw toward its owner when asked
float func_L14_002B4828(char *moby, int flag, float x) {
    char *d = *(char **)(moby + 0x78);
    char *owner = *(char **)(d + 0x110);
    float s = func_001FA790(x, *(float *)(moby + 0x48));
    if (s > 1.2217304706573486f) s = 1.2217304706573486f;
    else if (s < -1.2217304706573486f) s = -1.2217304706573486f;
    func_L00_0025CE58((float *)(d + 0x268), (float *)(d + 0x26C), s, D_0015EE70 * 12.566370964050293f, D_0015EE70 * 12.566370964050293f, D_0015EE6C * 6.2831854820251465f);
    func_L00_001FFED8(d + 0x170, 2, *(float *)(d + 0x268));
    if (flag) {
        float a[4];
        float b[4];
        float c[4];
        float *bp;
        float *cp;
        char *g;
        qcopy(a, owner + 0x10);
        g = D_0013E633 + 0xE1D;
        if (*(char **)(g + 0x2080) == owner) a[2] = a[2] - *(float *)(g + 0x2DC);
        bp = b;
        func_001F9BF0(bp, a, moby + 0x10);
        b[2] = b[2] + 1.5f;
        func_L00_0025CE58((float *)(d + 0x280), (float *)(d + 0x284), -func_L00_001FF860(func_001F9CE8(bp), b[2]), D_0015EE70 * 12.566370964050293f, D_0015EE70 * 12.566370964050293f, D_0015EE6C * 6.2831854820251465f);
        cp = c;
        func_L00_001FFED8(cp, 1, *(float *)(d + 0x280));
        func_001FA588(d + 0x170, d + 0x170, cp);
    }
    return func_001FA850(s, *(float *)(d + 0x268));
}
