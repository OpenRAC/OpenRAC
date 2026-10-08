/* NON_MATCHING func_L11_00313148 -- src/overlays/l11_pokitaru/vendor_00312BD8.c
 * Best so far: BYTES 8/208 (96.2% of the bytes match), checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Per-frame target tracking for a moby (+0x88 target, +0x8C timer): clears a dead target, calls func_L11_00312E1
 *   Best is p6.c (8 bytes differ, 2 instrs): retail schedules `daddu $a3,$sp,$zero` before `mov.s $f13,$f12`, ours
 *   Unblock: some source form that moves the &v argument setup ahead of the float duplicate (scheduler tie).
 *   q27 s01: re-tried with char* result local, and an ang float local (with an __asm__ alias for the callee since 
 */
#include "common.h"
extern char D_0013E633[];
extern float D_L11_00167850[];
extern short D_L11_001621F0;
extern short D_L11_001621F4;
extern void func_001F9908(int *arg0);
extern char *f12e10(float, float, float, int, char *, float *, float *, char *, int) __asm__("func_L11_00312E10");

/* Per-frame target tracking: drops a dead target, re-picks one and restarts the timer on change. */
void func_L11_00313148(int a, char *moby) {
    float v[4];
    float ang = 0.19634955f;
    char *r;
    if (*(unsigned char *)(D_0013E633 + 0x2413) == 0) {
        *(int *)(moby + 0x88) = 0;
    } else {
        char *cur = *(char **)(moby + 0x88);
        if (cur != 0) {
            unsigned char st = cur[0x20];
            if (st == 0xFE) {
                *(int *)(moby + 0x88) = 0;
            } else if (st == 0xFD) {
                *(int *)(moby + 0x88) = 0;
            }
        }
        qcopy(v, D_L11_00167850);
        r = f12e10(ang, ang, 255.0f, a, moby, D_L11_00167850 - 4, v, *(char **)(moby + 0x88), 0);
        if (r != *(char **)(moby + 0x88)) {
            int t = *(int *)&D_L11_001621F0 + *(int *)&D_L11_001621F4;
            *(char **)(moby + 0x88) = r;
            *(int *)(moby + 0x8C) = t;
        } else if (r != 0) {
            func_001F9908((int *)(moby + 0x8C));
        }
    }
}
