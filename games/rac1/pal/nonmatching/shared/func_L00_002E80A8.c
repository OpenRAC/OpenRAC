/* NON_MATCHING func_L00_002E80A8 -- src/overlays/shared/vendor_002E1660.c
 * Best so far: BYTES 30/360 (91.7% of the bytes match), checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   # func_L00_002E80A8 (360 B)
 *   Best: e1.c BYTES 30/360, size and registers exact. Retail keeps `p = a; b = a + 0x130; a += 0x40`
 *   (and the lw of p) before the first jal func_001F9C30; ours schedules them after it, and loads p after
 *   the %hi of D_0013E633+0x10AD. At lreg the insns are still before the call, so sched2 moves them.
 *   Lever found: retail's `daddu s2,s1 / addiu s1,s1,0x40` = one variable loaded, copied to p, then += 0x40.
 *   Tried: global base local (e2/e3), -fno-schedule-insns (size), -fno-schedule-insns2 (60 B).
 */
#include "common.h"

extern char D_0013E633[];
extern void func_001F9C30(void *, void *, float);
extern float func_001F9C78(void *, void *);
extern void func_001F9BD8(void *, void *, void *);
extern void func_001F9BF0(float *, float *, float *);
extern void func_001F9BC0(void *);

/* Resets the vendor's swing state: anchor from the hero, rope along the down axis, all speeds zero. */
void func_L00_002E80A8(char *m) {
    char *p;
    char *a;
    char *b;
    float v[4];
    float u[4];
    if (*(short *)(m + 0x86) != 0) return;
    a = *(char **)(m + 0x70);
    p = a;
    b = a + 0x130;
    a += 0x40;
    func_001F9C30(v, D_0013E633 + 0x10AD, -1.0f);
    qcopy(a, D_0013E633 + 0xE9D);
    qcopy(p + 0xA0, a);
    func_001F9C30(p + 0x50, v, func_001F9C78(a, v));
    func_001F9BC0(p + 0xB0);
    func_001F9BC0(p + 0xC0);
    func_001F9BC0(p + 0xE0);
    func_001F9C30(u, v, *(float *)(b + 0x30));
    func_001F9BD8(p + 0x90, u, a);
    func_001F9C30(u, v, *(float *)(a + 0xB0));
    func_001F9BD8(p + 0xD0, u, a);
    qcopy(p + 0x80, p + 0xD0);
    func_001F9BF0((float *)b, (float *)(m + 0x30), (float *)(p + 0x90));
    qcopy(p + 0x140, b);
    qcopy(p, m + 0x30);
    func_001F9BD8(p + 0x1F0, p + 0x90, b);
    *(int *)(p + 0x220) = 0;
}
