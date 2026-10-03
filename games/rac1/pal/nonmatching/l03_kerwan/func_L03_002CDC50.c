/* NON_MATCHING func_L03_002CDC50 -- src/overlays/l03_kerwan/vendor_002CB280.c
 * Best so far: SIZE ours 364 / retail 372, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawns a class 0x273 moby (func_0020D348) copying fields from src, pos, sets data floats, calls func_L00_0025D
 *   Best p6.c (same size 372, 95 diff words): `long` for the ld/sd at 0x38, `if (m == 0) return 0;` early return g
 */
#include "common.h"
extern struct Moby *func_0020D348_m(int) __asm__("func_0020D348");
extern float func_001F9CE8(void *);
extern float func_L00_001FF860(float, float);
extern void func_L00_0025D5B0(float ang, char *o, float *s, int a, int b, int c);
extern float D_0015EE64 MACRO_ADDR;

/* spawn a class 0x273 moby at pos, aimed along dir */
char *func_L03_002CDC50(char *src, float *pos, float *dir) {
    char *m = (char *)func_0020D348_m(0x273);
    if (m != 0) {
        char *d;
        float g;
        *(unsigned short *)(m + 0x32) = *(unsigned short *)(src + 0x32);
        m[0x31] = 1;
        m[0x30] = src[0x32];
        *(long *)(m + 0x38) = *(long *)(src + 0x38);
        qcopy(m + 0x10, pos);
        g = D_0015EE64;
        d = *(char **)(m + 0x78);
        *(float *)(d + 0x10) = g * 0.008f;
        *(float *)(d + 0x14) = g * 0.0005f;
        *(float *)(d + 0x18) = func_001F9CE8(dir);
        *(int *)(d + 0x20) = 0x400;
        *(float *)(d + 0x28) = 1.0f;
        *(float *)(d + 0x30) = 0.75f;
        *(float *)(d + 0x34) = 0.5f;
        *(float *)(d + 0x38) = 0.65f;
        *(float *)(d + 0x48) = 0.2f;
        *(float *)(d + 0x4C) = 0.01f;
        *(float *)(d + 0x1C) = dir[2];
        d[0x3D] = 0;
        *(int *)(d + 0x24) = 1;
        func_L00_0025D5B0(func_L00_001FF860(dir[0], dir[1]), m, (float *)d, 0, 1, 0);
        *(char **)(d + 0x68) = src;
        *(int *)(d + 0x6C) = 0;
        m[0x20] = 1;
    }
    return m;
}
