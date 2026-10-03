/* NON_MATCHING func_L14_002FFF08 -- src/overlays/l14_oltanis/vendor_002FF358.c
 * Best so far: SIZE ours 428 / retail 424, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Steering helper: scales a gp float by a global, queries a path table (func_L00_0025E860), smooths 3 position a
 */
#include "common.h"
extern float D_0015EE6C MACRO_ADDR;
extern int D_0015EE84 MACRO_ADDR;
extern char *D_L14_001B0F30[];
extern short D_L14_00162088;
extern short D_L14_0016208C;
extern short D_L14_00162090;
extern int func_L00_0025E860_a(void *, void *, void *, void *, int, float) __asm__("func_L00_0025E860");
extern float func_L00_0025C918(float *p, float *v, float t, float u1, float u2, float eps);
extern void func_001F9BF0(void *dst, void *a, void *b);
extern float func_001F9B88(float);
extern float func_L00_001FF860(float, float);
extern void func_L00_002592B0(char *moby, float *vel, float target, float k, float d, float max);

// Steers a moby toward a target point: updates its velocities and heading from the path lookup.
int func_L14_002FFF08(unsigned char *moby) {
    char *d;
    float f;
    float vec[8];
    int idx;
    int r;
    char *pos;
    f = *(float *)&D_L14_00162088 * D_0015EE6C;
    d = *(char **)(moby + 0x78);
    if (D_0015EE84 == 0x10) f = D_0015EE6C * 20.0f;
    if (*(float *)(d + 0xF8) != 0.0f) f = *(float *)(d + 0xF8) * D_0015EE6C;
    if (moby[0x20] == 5) idx = *(int *)(d + 0xDC);
    else idx = *(int *)(d + 0xD8);
    r = func_L00_0025E860_a(D_L14_001B0F30[idx], vec, d + 0xD0, d + 0xD4, 0, f);
    func_L00_0025C918((float *)(pos = moby + 0x10), (float *)(d + 0xE4), vec[0], *(float *)&D_L14_0016208C, *(float *)&D_L14_00162090, 0.0f);
    func_L00_0025C918((float *)(moby + 0x14), (float *)(d + 0xE8), vec[1], *(float *)&D_L14_0016208C, *(float *)&D_L14_00162090, 0.0f);
    func_L00_0025C918((float *)(moby + 0x18), (float *)(d + 0xEC), vec[2], *(float *)&D_L14_0016208C, *(float *)&D_L14_00162090, 0.0f);
    func_001F9BF0(vec + 4, vec, pos);
    if (func_001F9B88(vec[4]) > 0.01f) {
        if (func_001F9B88(vec[5]) > 0.01f) {
            func_L00_002592B0(moby, (float *)(d + 0xF0), func_L00_001FF860(vec[4], vec[5]), *(float *)&D_L14_0016208C, *(float *)&D_L14_00162090, 0.0f);
        }
    }
    return r;
}
