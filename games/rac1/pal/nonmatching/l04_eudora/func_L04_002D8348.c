/* NON_MATCHING func_L04_002D8348 -- src/overlays/l04_eudora/vendor_002CB800.c
 * Best so far: SIZE ours 328 / retail 324, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   UpdateMoby_617: reads a path record via moby[0x78], blends two keyframes of a table (f = func_00200210(...), i
 *   Difference: retail holds the record pointer in $a0 and the D_L04_001B0930 address in $a2 (ours: swapped, and t
 *   Unblock: a source form that changes the allocation priority of the record pointer vs the table address; not fo
 */
#include "common.h"
extern int *D_L04_001B0930[];
extern char *D_L04_00160058;
extern float func_001FA888(int);
extern float func_L00_00200210(float, float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern void func_001F9C30(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);

/* update: blend two keyframes of a path and write the result to a position */
void func_L04_002D8348(char *moby) {
    int *p = *(int **)(moby + 0x78);
    int *t = D_L04_001B0930[p[2]];
    char *base = D_L04_00160058;
    float *src = *(float **)(base + p[0] * 256 + 0x78);
    char *dst = base + p[1] * 256;
    float a[4];
    float b[4];
    float f, g;
    int idx;
    int n = 1;
    float one = 1.0f;

    f = func_001FA888(t[0] - 1);
    g = (one - src[0]) * f;
    f = func_L00_00200210(g, one);
    idx = func_001FA898_r(g);
    if (f == 0.0f) n = 0;
    func_001F9C30(a, (char *)t + idx * 16 + 0x10, one - f);
    n += idx;
    func_001F9C30(b, (char *)t + n * 16 + 0x10, f);
    func_001F9BD8(moby + 0x10, a, b);
    qcopy(dst + 0x10, moby + 0x10);
}
