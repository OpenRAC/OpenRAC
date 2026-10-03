/* NON_MATCHING func_L05_00308268 -- src/overlays/l05_rilgar/vendor_002D28D0.c
 * Best so far: SIZE ours 500 / retail 512, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Waypoint follower: loops recomputing angle/distance to a waypoint and advancing the waypoint index (div/mod by
 *   Best candidate p3.c matches the body (496 vs 512 bytes) except the loop entry: retail hoists `move $6,$sp` int
 *   Explicit duplication of the test before the loop (p4/p6) gives 576 bytes (test copied, no cross-jump). Would u
 */
extern short func_L01_0028C2D8(void *, void *, float);
extern int func_002140B0(int);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern float func_L00_001FF860(float, float);
extern float func_001F9D48(float *, void *);
extern float func_001FA850(float, float);
extern char *D_L05_001B0CB0[];

/* Steps a moby along a waypoint table, advancing until the next point is in range and ahead. */
void func_L05_00308268(char *a) {
    char *data = *(char **)(a + 0x78);
    float v[4];
    for (;;) {
        float ang, dist;
        char *p, *t;
        func_L05_00308188(a, *(short *)(data + 0x258), v);
        ang = func_L00_001FF860(v[0] - *(float *)(a + 0x10), v[1] - *(float *)(a + 0x14));
        func_L05_00308188(a, *(short *)(data + 0x258), v);
        dist = func_001F9D48(v, data + 0x200);
        if ((func_001FA850(ang, *(float *)(a + 0x48)) < 1.5707964f || dist > 4.0f) && dist > 2.0f) break;
        p = *(char **)(data + 0x244);
        *(short *)(data + 0x258) = (*(short *)(data + 0x258) + 1) % *(int *)p;
        t = D_L05_001B0CB0[*(int *)(data + 0x220)];
        if (p == t) {
            if (*(float *)(p + *(short *)(data + 0x258) * 16 + 0x1C) > 0.0f) {
                if (func_002140B0(100) > 0) {
                    int r = func_001FA898_r(*(float *)(*(char **)(data + 0x244) + *(short *)(data + 0x258) * 16 + 0x1C));
                    *(short *)(data + 0x258) = 0;
                    *(char **)(data + 0x244) = D_L05_001B0CB0[((int *)(data + 0x220))[r]];
                }
            }
        } else if (*(short *)(data + 0x258) == *(int *)p - 1) {
            *(char **)(data + 0x244) = t;
            *(short *)(data + 0x258) = func_L01_0028C2D8(data + 0x200, t, 0.0f);
            *(short *)(data + 0x258) = func_L00_0025E7F8(*(char **)(data + 0x244), *(short *)(data + 0x258), 5, 1);
        }
    }
}
