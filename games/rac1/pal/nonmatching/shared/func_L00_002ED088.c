/* NON_MATCHING func_L00_002ED088 -- src/overlays/shared/vendor_002EB0D8.c
 * Best so far: SIZE ours 668 / retail 664, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   ActivateCamera_7 init: sets tuning floats in the moby's data (+0x70), copies the current camera pose (D_L00_00
 *   Best candidate p6.c is size-exact (664) but 239 bytes differ: m/p registers swapped (retail m=s18, p=s17), sch
 *   p7 (t pointer + store swap) broke size (668). Budget spent; a fresh run budget with p6 + t-pointer variants in
 */
#include "common.h"
extern int D_0015EE84_m __asm__("D_0015EE84") MACRO_ADDR;
extern char D_0013E633[] NOT_SDA;
extern char D_L00_00166D80_c[] __asm__("D_L00_00166D80");
extern int func_001F9850(int);
extern void func_001F9BC0(void *);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern float func_L00_001FF860(float, float);

/* Sets up the camera activation moby: tuning floats, then the pose copied from the current camera. */
void func_L00_002ED088(void *arg) {
    char *m = arg;
    char *g;
    char *p = *(char **)(m + 0x70) + 0x40;
    float f;
    char *t, *q, *r, *w, *s19, *src;
    char *gg, *gg0;
    *(int *)(p + 0x40) = 0;
    f = 1.5f;
    if (D_0015EE84_m == 0xE && (unsigned)(*(int *)((D_0013E633 + 0xE1D) + 0x2084) - 0x2C) < 2)
        *(float *)(p + 0x4C) = 2.0f;
    else
        *(float *)(p + 0x4C) = 1.5f;
    g = D_0013E633 + 0xE1D;
    *(float *)(p + 0x48) = 0.20943951606750488f;
    *(float *)(p + 0x58) = 0.001745329238474369f;
    *(float *)(p + 0x5C) = -0.004363323096185923f;
    *(int *)(p + 0x44) = *(int *)(g + 0x2080);
    q = *(char **)(m + 0x70);
    *(float *)(q + 0xA0) = 4.639999866485596f;
    q += 0xA0;
    *(float *)(q + 0x14) = 0.05f;
    *(float *)(q + 4) = 0.5f;
    *(float *)(q + 0x10) = 2.0f;
    r = *(char **)(m + 0x70);
    *(int *)(r + 0x18) = 0;
    *(float *)(r + 0x10) = 0.01f;
    *(float *)(r + 0x14) = 0.3f;
    *(short *)(r + 0x1E) = func_001F9850(0x78);
    *(short *)(r + 0x1C) = 0;
    func_001F9BC0(r);
    q = *(char **)(m + 0x70);
    t = q + 0x30;
    *(float *)(t + 4) = 0.02f;
    *(float *)(t + 8) = 0.2f;
    *(int *)(t + 0xC) = 0;
    *(int *)(q + 0x30) = 0;
    s19 = *(char **)(m + 0x70);
    w = s19 + 0xB8;
    *(int *)(w + 4) = 0;
    gg0 = D_L00_00166D80_c;
    src = *(char **)(gg0 + 0x184);
    qcopy(m + 0x30, src + 0x30);
    qcopy(m, src);
    qcopy(m + 0x10, src + 0x10);
    qcopy(m + 0x20, src + 0x20);
    qcopy(m + 0x40, m);
    *(short *)(m + 0x7E) = 0;
    if ((unsigned)(*(int *)(g + 0x2084) - 0x2C) < 2) {
        *(float *)(p + 0x30) = func_001F9F90(*(float *)(g + 0x9AC));
        *(float *)(p + 0x34) = func_001F9FA8(*(float *)(g + 0x9AC));
        *(int *)(p + 0x38) = 0;
        *(float *)(s19 + 0xB8) = *(float *)(g + 0x9AC);
        *(int *)(w + 8) = *(int *)(g + 0x994);
    } else {
        func_001F9BF0(p + 0x30, *(char **)(g + 0x964) + 0x10, m + 0x30);
        *(int *)(p + 0x38) = 0;
        *(int *)(p + 0x3C) = 0;
        func_L00_001FF4B0(p + 0x30, p + 0x30, 1.0f);
        *(float *)(s19 + 0xB8) = func_L00_001FF860(*(float *)(p + 0x30), *(float *)(p + 0x34));
        *(int *)(w + 8) = *(int *)(g + 0x964);
    }
    gg = D_L00_00166D80_c;
    *(char *)(gg + 0x273) = 2;
    *(short *)(gg + 0x270) = 1;
    *(int *)(gg + 0x2F4) = func_001F9850(0x3C);
}
