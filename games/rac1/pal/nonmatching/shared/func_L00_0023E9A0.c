/* NON_MATCHING func_L00_0023E9A0 -- src/overlays/shared/initonce_0023E4C8.c
 * Best so far: BYTES 18/608 (97.0% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_0023E9A0: four iterations, each emitting two particles (func_L00_00272F00) at a moby's position (func
 *   Lombyte's port (p0) matches except 5 instructions in the FIRST func_L00_00272F00 call: retail has `mov.s $f13,
 *   `0.1f * s`, a local for the mul and the fr() result (p1, p3) give identical bytes. Looks like a sched1/reorg o
 */
extern float D_0015EE60 MACRO_ADDR;
extern void func_L00_00250800(void *, int, void *);
extern float func_002140F8(float, float);
extern int func_002140B0(int);
extern float func_00214158(void);
extern void func_00215C00(void *, float, float, float);
extern float func_L00_00258C80(float, float);
extern int func_001F9850(int);
extern unsigned char *func_L00_00272F00(float *pos, int a, int col, int mode, int b, float *vel, float f0, float f1, float f2);
extern int func_001FA898_r(float) __asm__("func_001FA898");

/* Emits four bursts of two sparks each at a moby's position, with random speed and direction. Adapted from Lombyte (MIT) for PAL: overlays/shared/unclassified_0023db30.c, FUN_L00_0023e008. */
void func_L00_0023E9A0(void *p) {
    char dir[16] __attribute__((aligned(16)));
    char old[16] __attribute__((aligned(16)));
    float pos[4] __attribute__((aligned(16)));
    int i, c;
    float f0;
    unsigned char *r;
    func_L00_00250800(p, 0, pos);
    for (i = 3; i >= 0; i--) {
        float s = func_002140F8(0.15f, 0.5f);
        int k = func_002140B0(2);
        float a, b;
        if (k == 0) k--;
        a = func_00214158();
        b = func_002140F8(0.34906584f, 1.53588974f);
        func_00215C00(dir, func_002140F8(0.05f, 0.2f) * D_0015EE60, a, b);
        qcopy(old, pos);
        pos[0] += func_L00_00258C80(0.0f, 0.15f);
        pos[1] += func_L00_00258C80(0.0f, 0.15f);
        pos[2] += func_L00_00258C80(0.0f, 0.15f);
        c = func_001F9850(0x3C);
        f0 = s * 0.1f;
        r = func_L00_00272F00(pos, c, 0x7F7F2020, 0, k, (float *)dir, f0, s, 0.009f);
        if (r)
            r[9] = func_001FA898_r(4.0f) + 0x70;
        k = -k;
        r = func_L00_00272F00(pos, func_001F9850(0x3C), 0x7F7F7F7F, 1, k, (float *)dir, s * 0.07f, s * 0.7f, 0.009f);
        if (r)
            r[9] = func_001FA898_r(4.0f) + 0x70;
    }
}
