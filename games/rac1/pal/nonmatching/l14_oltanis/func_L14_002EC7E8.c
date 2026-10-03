/* NON_MATCHING func_L14_002EC7E8 -- src/overlays/l14_oltanis/vendor_002E0538.c
 * Best so far: SIZE ours 472 / retail 476, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Fills particle slot idx of a moby: qcopy a template, rotate it about 3 axes by random amounts (func_001F9C30/9
 */
#include "common.h"
extern int D_L14_001601AC;
extern short D_L14_00161D00;
extern short D_L14_00161D04;
extern short D_L14_00161D08;
extern short D_L14_00161D0C;
extern float D_0015EE6C MACRO_ADDR;
extern void func_001F9CA0(void *, void *, void *);
extern float func_L00_00258C80(float lo, float hi);
extern void func_001F9C30(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern float func_002140F8(float, float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern int func_001F9850(int);

// Builds one particle slot idx of a moby: a random offset frame around an axis, a speed and three timers.
void func_L14_002EC7E8(char *moby, int idx, void *pos) {
    char *d = *(char **)(moby + 0x78);
    float v[16];
    float a;
    float r1;
    float r2;
    float y;
    char *p;
    int off;
    float *v1;
    float *v2;
    float *v3;
    qcopy(v, pos);
    v1 = v + 4;
    v2 = v + 8;
    off = idx * 16;
    v[10] = 1.0f;
    v[8] = 0.0f;
    v[9] = 0.0f;
    v[11] = 0.0f;
    func_001F9CA0(v1, v, v2);
    qcopy(d + off + 0x20, (char *)D_L14_001601AC + (*(int *)(d + 0x504) << 7) + 0x30);
    v3 = v + 12;
    a = -*(float *)(d + 0x14);
    r1 = func_L00_00258C80(0.0f, *(float *)(d + 0x18));
    r2 = func_L00_00258C80(0.0f, *(float *)(d + 0x1C));
    func_001F9C30(v3, v, a);
    p = d + (off + 0x20);
    func_001F9BD8(p, p, v3);
    func_001F9C30(v3, v1, r1);
    func_001F9BD8(p, p, v3);
    func_001F9C30(v3, v2, r2);
    func_001F9BD8(p, p, v3);
    *(float *)(d + off + 0x2C) = func_002140F8(*(float *)&D_L14_00161D08, *(float *)&D_L14_00161D0C);
    y = func_002140F8((float)*(int *)&D_L14_00161D00, (float)*(int *)&D_L14_00161D04) * D_0015EE6C;
    *(float *)(d + idx * 4 + 0x440) = y;
    a = *(float *)(d + 0x14);
    a = (a + a) / y;
    {
        int t = func_001F9850(func_001FA898_r(a));
        ((short *)(d + 0x320))[idx] = t;
        ((short *)(d + 0x380))[idx] = t;
        ((short *)(d + 0x3E0))[idx] = 0;
    }
}
