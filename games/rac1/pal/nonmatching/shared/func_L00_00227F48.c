/* NON_MATCHING func_L00_00227F48 -- src/overlays/shared/help_00221A98.c
 * Best so far: SIZE ours 552 / retail 564, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Claimed by a11 as the ninth of a pair after N=8 was reached; not attempted.
 *   Help-state timer tick (states 0x11/0x12): decays D at p+0x22A0, signals func_L00_00222B80 with 0x6A/6/0x33/0x3
 *   Left: retail lays out the first `if (gHaveHeliPack-ish byte [4])` as beqz/nop/b (ours bnel), and shares one `f
 */
typedef int u128 __attribute__((mode(TI)));
extern unsigned char D_0013E633[] NOT_SDA;
extern unsigned char D_0013D5CA[] NOT_SDA;
extern int D_0013A5E0[];
extern float D_0015EE6C MACRO_ADDR;
extern int D_L00_00167114 NOT_SDA;
extern int func_001F9850(int);
extern int func_001FFB38(int, int, int, int, int, int, int);
extern void func_L00_00222B80(int, int);
extern int func_L00_001EFFF0(void *, void *, int, int, int);
extern void func_L00_00236BF8(void);
extern void func_L00_00236DE8(void);
extern void func_L00_00236F38(void);

/* per-frame check of the help-state timer; may signal a state change */
int func_L00_00227F48(void) {
    char *p = (char *)D_0013E633 + 0xE1D;
    float va[4];
    float vb[4];
    if (*(int *)(p + 0x208C) == 0x11) {
        int dec;
        char *q;
        if (D_0013D5CA[4] == 0) {
            int s = func_001F9850(0x3C);
            dec = 10000 / (s * 15);
            func_001FFB38(4, 0x753F, (int)func_L00_00236BF8, (int)func_L00_00236DE8, (int)func_L00_00236F38, (int)(p + 0x22A0), 10000);
        } else {
            dec = 0;
        }
        q = (char *)D_0013E633 + 0xE1D;
        *(int *)(q + 0x22A0) -= dec;
        if (*(int *)(q + 0x22A0) < 0) *(int *)(q + 0x22A0) = 0;
        if (*(int *)(q + 0x22A0) == 0) {
            if (!(*(float *)(q + 0x2F0) - 2.0f < *(float *)(q + 0x88))
                || !(*(float *)(q + 0x94) < -0.8742f)
                || !(D_0015EE6C < *(float *)(q + 0x108))) {
                func_L00_00222B80(0x6A, 1);
                return 1;
            }
        }
        {
            char *q2 = (char *)D_0013E633 + 0xE1D;
            if (*(float *)(q2 + 0x2F0) + 0.4f < *(float *)(q2 + 0x88)) {
                D_L00_00167114 = 0;
                func_L00_00222B80(6, 1);
                return 1;
            }
        }
    } else {
        if (*(int *)(p + 0x2084) != 0x6A && *(int *)(p + 0x2084) != 0x76) {
            *(int *)(p + 0x22A0) = 10000;
        }
    }
    {
        char *q3 = (char *)D_0013E633 + 0xE1D;
        if (*(int *)(q3 + 0x208C) == 0x12) {
            qcopy(va, q3 + 0x80);
            va[2] = *(float *)(q3 + 0x2F0) + 0.01f;
            qcopy(vb, va);
            vb[2] = vb[2] + 0.3f;
            if (func_L00_001EFFF0(va, vb, 2, *(int *)(q3 + 0x2080), 0)) {
                if (D_0013D5CA[2] != 0 && (*(int *)((char *)D_0013A5E0 + 0x2600) & 0xA) != 0) {
                    func_L00_00222B80(0x35, 1);
                    return 1;
                }
                func_L00_00222B80(0x33, 1);
                return 1;
            }
            return 0;
        }
    }
    return 0;
}
