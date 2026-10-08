/* NON_MATCHING func_L00_002E5238 -- src/overlays/shared/vendor_002E1660.c
 * Best so far: SIZE ours 988 / retail 980, checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_002E5238 (FxGroupUpdate: switch on moby state 0..3 [free / grow / burst / fade], then the common tail
 *   What helped: unsigned state byte (lbu), no hoisted sp+0x30 local, h = d[0x2C]*0.5f local, func_L00_001F10E0 pr
 *   Remaining: in the 25F4A8 call setup retail schedules `sw -1,8(sp)` before `lui a1` and the int arg moves (a2,a
 *   q27/s04: p10 uses the packet's decls (func_L00_0025F4A8 with ints grouped: (void*,void*,void*,float,float,int,
 */
#include "common.h"

extern int func_001F9908_r(int *) __asm__("func_001F9908");
extern float func_001F9878(float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern void func_001F9EC0(void *, void *, void *);
extern void func_001F9BD8(void *, void *, void *);
extern void func_L00_00260460(void *, void *, int, float, float);
extern void func_L00_0025F4A8(void *, void *, void *, float, float, int, float, int, float, float, int, float, int, float, int, int, int, int);
extern void func_0020D678(void *);
extern void func_001FA1F8(void *, void *);
extern void func_001FA4F0(void *, void *, void *);
extern void func_001F9BF0(float *, float *, float *);
extern void func_L00_001FF4B0(void *, void *, float);
extern float func_001F9CB8(void *);
extern float func_001F9C78(void *, void *);
extern void func_L00_001FF610(void *, void *, void *);
extern void func_001F9CA0(void *, void *, void *);
extern void func_00215380(void *arg0, void *axis, float angle);
extern void func_001FA6C0(void *, void *);
extern void func_002153E8(void *, void *);
extern char D_L00_0015F660[] NOT_SDA;
extern float D_L00_00173F80[] NOT_SDA;

/* Update of an effect-group moby: state machine over its life, then it steers a sprite along its path and bounces it off surfaces. */
void func_L00_002E5238(char *m) {
    char *d = *(char **)(m + 0x78);
    float a20[4];
    float a30[12];
    float a60[4];
    float a70[4];
    float a80[4];
    float a90[16];
    float aD0[4];
    float aE0[4];
    char *s23;
    float r;
    float h;
    float *q;

    switch (*(unsigned char *)(m + 0x20)) {
    case 0:
        func_0020D678(m);
        return;
    case 1:
        if (func_001F9908_r((int *)(d + 0x30))) {
            m[0x20] = (*(int *)(d + 0x34) & 1) ? 3 : 2;
            *(int *)(d + 0x30) = func_001FA898_r(func_001F9878(10.0f));
            return;
        }
        break;
    case 2:
        func_001F9EC0(a20, d + 0x20, m + 0xC0);
        func_001F9BD8(a20, a20, m + 0x10);
        if (*(int *)(d + 0x34) & 2) {
            func_L00_00260460(m, a20, -1, *(float *)(d + 0x2C) * 0.5f, 0.0f);
        } else {
            h = *(float *)(d + 0x2C) * 0.5f;
            func_L00_0025F4A8(m, D_L00_0015F660, a20, 0.0f, 0.0f, 5, *(float *)(d + 0x2C), 3, h, 9.0f, 5, h, -1, 0.0f, 0, 0, -1, 0);
        }
        func_0020D678(m);
        return;
    case 3:
        if (func_001F9908_r((int *)(d + 0x30))) {
            func_0020D678(m);
            return;
        }
        m[0x23] = func_001FA898_r((float)*(int *)(d + 0x30) / func_001F9878(10.0f) * 128.0f);
        break;
    }
    s23 = d + 0x10;
    func_001FA1F8(a30, s23);
    func_001F9EC0(a20, d + 0x20, m + 0xC0);
    func_001FA4F0(m + 0xC0, a30, m + 0xC0);
    func_001F9EC0(a60, d + 0x20, m + 0xC0);
    func_001F9BF0(a70, a60, a20);
    func_001F9BF0((float *)(m + 0x10), (float *)(m + 0x10), a70);
    func_001F9BD8(m + 0x10, m + 0x10, d);
    r = *(float *)(d + 0x2C);
    *(float *)(d + 8) = *(float *)(d + 8) - *(float *)(d + 0x38);
    func_001F9EC0(a80, d + 0x20, m + 0xC0);
    func_001F9BD8(a80, a80, m + 0x10);
    if (func_L00_001F10E0(a80, r, 0, m)) {
        float f21, f20;
        q = D_L00_00173F80;
        func_L00_001FF4B0(q, q, 1.0f);
        f21 = func_001F9CB8(d);
        f20 = func_001F9C78(d, q);
        if (f20 < 0.0f) {
            func_L00_001FF610(d, d, q);
            func_L00_001FF4B0(aD0, q, f20 * 0.5f);
            func_001F9BD8(d, d, aD0);
            func_L00_001FF4B0(d, d, f21);
            func_001F9CA0(aE0, q, d);
            func_L00_001FF4B0(aE0, aE0, 1.0f);
            func_00215380(aE0, aE0, func_001F9CB8(s23));
            func_001FA6C0(aE0, a90);
            func_002153E8(a90, s23);
        }
    }
}
