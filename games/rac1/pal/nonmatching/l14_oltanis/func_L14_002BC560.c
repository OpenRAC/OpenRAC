/* NON_MATCHING func_L14_002BC560 -- src/overlays/l14_oltanis/vendor_002ACCC0.c
 * Best so far: SIZE ours 452 / retail 456, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawns moby 0x51 at a position, aimed from an owner with random drift/spin params. Best p5.c: body from +0x3c 
 */
#include "common.h"
typedef int u128_2BC560 __attribute__((mode(TI)));
extern char D_0013E633[];
extern float D_0015EE60 MACRO_ADDR;
extern struct Moby *func_0020D348_m(int) __asm__("func_0020D348");
extern void func_001F9BF0(void *dst, void *a, void *b);
extern float func_002140F8(float, float);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9CA0(void *, void *, void *);
extern float func_L00_00258C80(float lo, float hi);
extern void func_002156E0(void *dst, void *vec, void *axis, float angle);
extern int func_001F9850(int);
extern void func_L00_00251E30(void *);

// Spawns moby 0x51 at a position facing away from an owner, with random drift and spin parameters.
unsigned char *func_L14_002BC560(char *owner, float angle, char *posp) {
    float vec[12];
    unsigned char *m;
    char *d;
    char *g;
    float *v0 = vec;
    float *v1;
    qcopy(v0, posp);
    m = (unsigned char *)func_0020D348_m(0x51);
    if (m != 0) {
        d = *(char **)(m + 0x78);
        qcopy(m + 0x10, v0);
        v1 = vec + 4;
        *(float *)(m + 0x48) = angle;
        m[0x31] = 1;
        m[0x20] = 0;
        *(short *)(m + 0x32) = 0xFF;
        m[0x30] = 0xFF;
        func_001F9BF0(v1, v0, owner + 0x10);
        vec[6] = 0.0f;
        func_L00_001FF4B0(v1, v1, func_002140F8(D_0015EE60 * 0.075f, D_0015EE60 * 0.2f));
        g = D_0013E633 + 0x10AD;
        func_001F9CA0(vec + 8, g, v1);
        func_002156E0(d, v1, g, func_L00_00258C80(0.0f, 0.7853982f));
        *(float *)(d + 8) = func_002140F8(D_0015EE60 * 0.05f, D_0015EE60 * 0.32f);
        *(float *)(d + 0x14) = func_L00_00258C80(0.0f, 0.034906585f);
        *(float *)(d + 0x18) = func_L00_00258C80(0.0f, 0.034906585f);
        *(float *)(d + 0x1C) = func_L00_00258C80(0.0f, 0.034906585f);
        *(short *)(d + 0x10) = func_001F9850(200);
        *(char **)(d + 0x20) = owner;
        *(short *)(d + 0x12) = -1;
        func_L00_00251E30(m);
    }
    return m;
}
