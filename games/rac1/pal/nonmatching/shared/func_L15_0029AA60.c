/* NON_MATCHING func_L15_0029AA60 -- src/overlays/shared/vendor_00298BB8.c
 * Best so far: BYTES 8/488 (98.4% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Picks a facing state (8/6/7/a) from signed angle to a target, stores it, may start an animation via func_00213
 *   Best p5.c: 8 bytes differ (retail order of stores sw 0x144, swc1 0x174, swc1 0x178 vs ours). Needs asm alias f
 *   Would unblock: another statement order for the 0x144/0x174/0x178 stores (only a scheduling tie left).
 */
extern float func_001FA790(float, float);
extern int func_001F9850(int);
extern void func_00213DE0(void *, int, int, int);

int func_L15_0029AA60_r(unsigned char *moby, char *d, int a, int b, float angle) __asm__("func_L15_0029AA60");

/* Picks a facing state from the signed angle to a target; returns 1 when it turned a lot, else 0. */
int func_L15_0029AA60_r(unsigned char *moby, char *d, int a, int b, float angle) {
    float n = -func_001FA790(*(float *)(moby + 0x48), angle);
    float v;
    int ret = 1;
    *(float *)(d + 0x174) = angle;
    *(float *)(d + 0x170) = *(float *)(moby + 0x48);
    *(float *)(d + 0x178) = n;
    *(int *)(d + 0x144) = 0;
    d[0x15C] = a;
    d[0x15D] = b;
    if (n > 2.0943952f || n < -2.0943952f) {
        moby[0x20] = 8;
        v = *(float *)(d + 0x178);
        if (v > 0.0f) {
            *(float *)(d + 0x178) = v + -6.2831855f;
        }
        if (moby[0x53] != 0xC) {
            func_00213DE0(moby, 0xC, 0, func_001F9850(10));
        }

    } else if (n < -0.7853982f) {
        moby[0x20] = 6;
        if (moby[0x53] != 0xA) {
            func_00213DE0(moby, 0xA, 0, func_001F9850(10));
        }

    } else if (n > 0.7853982f) {
        moby[0x20] = 7;
        if (moby[0x53] != 0xB) {
            func_00213DE0(moby, 0xB, 0, func_001F9850(10));
        }

    } else {
        moby[0x20] = a;
        if (moby[0x53] != b) {
            func_00213DE0(moby, b, 0, func_001F9850(10));
        }
        ret = 0;
    }
    return ret;
}
