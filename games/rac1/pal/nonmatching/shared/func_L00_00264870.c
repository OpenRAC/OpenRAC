/* NON_MATCHING func_L00_00264870 -- src/overlays/shared/mobyutil_00261B00.c
 * Best so far: SIZE ours 716 / retail 720, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spark-emitter update for moby class 0x2EE: two passes of loops calling func_L00_0026DEA0 and emitting particle
 *   Remaining differences: retail saves $30 (the sp+0x30 pointer kept in a register across the loops); ours remate
 *   Unblock: a way to keep sp+0x30 live across the calls without an extra use, or the loop-1 branch form.
 */
extern void func_0020DAF8(char *, int, char *);
extern float func_001F9CB8(void *a);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern int func_002140B0(int);
extern void *func_L00_0026DEA0_c(void *, int, void *, float, float, float, float, int) __asm__("func_L00_0026DEA0");
extern int func_001F9850(int);
extern float D_L00_0015F660[] MACRO_ADDR;

// Per-frame update of a moby's spark emitters: scales them by distance and spawns particles
void func_L00_00264870(char *a) {
    char b0[0x20];
    char b20[0x10];
    char b30[0x10];
    char b40[0x10];
    char b50[0x10];
    char *p20;
    char *p30;
    char *p40;
    char *p50;
    float s;
    float v21, v22, f12;
    int i, x, y, d;
    int k2, ct;
    unsigned char *r;
    unsigned char *q;

    if (*(short *)(a + 0xA6) == 0x2EE) {
        if (a != 0) {
            func_0020DAF8(a, 0, b0);
            s = func_001F9CB8(b0) / 1.026f;
            if (1.0f < s) {
                s = 1.0f;
            }
            p50 = b50;
            p20 = b20;
            f12 = s * -0.025f + -0.025f;
            v22 = s * 44000.0f + 6000.0f;
            v21 = s * 0.02f + 0.01f;
            func_L00_001FF4B0(p50, p20, f12);
            k2 = 2;
            p40 = b40;
            p30 = b30;
            ct = 0x50;
            func_001F9BD8(p40, p30, p50);
            for (i = 1; i >= 0; i--) {
                x = func_002140B0(0x10);
                y = func_002140B0(2);
                d = -x;
                if (y == 0) {
                    d = x;
                }
                r = func_L00_0026DEA0_c(p40, d, D_L00_0015F660, v21, 1.0f, 0.75f, v22, 0x503030FF);
                if (r != 0) {
                    *(short *)(r + 0xA) = func_001F9850(6);
                    q = r + 0x20;
                    *(int *)(q + 4) = k2;
                    q[0xA] = ct;
                    q[0xB] = r[0xA];
                }
            }
            f12 = s * -0.015f + -0.01f;
            v22 = s * 25000.0f + 5000.0f;
            v21 = s * 0.015f + 0.01f;
            func_L00_001FF4B0(p50, p20, f12);
            s = 1.0f;
            k2 = 2;
            ct = 0x7F;
            func_001F9BD8(p40, p30, p50);
            for (i = 4; i >= 0; i--) {
                r = func_L00_0026DEA0_c(p40, 0x10, D_L00_0015F660, v21, 1.0f, 1.0f, v22, 0x7FFFFFFF);
                if (r != 0) {
                    *(short *)(r + 0xA) = func_001F9850(2);
                    r[8] = func_002140B0(0xFF);
                    q = r + 0x20;
                    *(int *)(q + 4) = k2;
                    q[0xA] = ct;
                    q[0xB] = r[0xA];
                }
            }
        }
    }
}
