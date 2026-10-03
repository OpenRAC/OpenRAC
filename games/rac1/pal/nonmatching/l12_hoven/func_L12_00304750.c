/* NON_MATCHING func_L12_00304750 -- src/overlays/l12_hoven/vendor_002EDAA0.c
 * Best so far: SIZE ours 460 / retail 464, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Checks 10 tracked mobys (idx table at data+0x40) against a position (D_L12_00167180+0x140..) and updates data+
 *   Body nearly matches; the loop is strength-reduced by our gcc (pointer increment, bgez) while retail keeps i in
 *   Unblock: some wording that stops loop strength reduction (unknown); 6 runs spent.
 */
extern float func_L00_001FF860(float, float);
extern float func_001F9D48(void *, void *);
extern float func_001FA850(float, float);
extern void func_001F9908(int *arg0);
extern int func_001F9850(int);
extern char D_L12_00167180;
extern char *D_L12_00160058 MACRO_ADDR;

/* Checks whether any of ten tracked mobys is near the vendor and updates its timer. */
void func_L12_00304750(char *moby) {
    char *data = *(char **)(moby + 0x78);
    int found = 0;
    int i;
    for (i = 0; i < 10; i++) {
        int idx = ((int *)(data + 0x40))[i];
        if (idx != -1) {
            char *m = D_L12_00160058 + (idx << 8);
            float vec[4];
            float lim0, lim1;
            char *base = &D_L12_00167180;
            float a, d, b;
            qcopy(vec, m + 0x10);
            lim0 = lim1 = 0.12217305f;
            if (*(short *)(m + 0xA6) == 0x146) {
                vec[2] += 1.5f;
            }
            if (*(short *)(m + 0xA6) == 0x4FE) {
                vec[2] += 1.5f;
                lim1 = 0.034906585f;
                lim0 = 0.05235988f;
            }
            a = func_L00_001FF860(vec[0] - *(float *)(base + 0x140), vec[1] - *(float *)(base + 0x144));
            d = func_001F9D48(base + 0x140, vec);
            b = func_L00_001FF860(d, vec[2] - *(float *)(base + 0x148));
            if (func_001FA850(a, *(float *)(base + 0x158)) < lim1) {
                if (func_001FA850(b, -*(float *)(base + 0x154)) < lim0) {
                    found = 1;
                }
            }
        }

    }
    if (found) {
        func_001F9908((int *)(data + 0x88));
    } else {
        *(int *)(data + 0x88) += 2;
        if (func_001F9850(30) < *(int *)(data + 0x88)) {
            *(int *)(data + 0x88) = func_001F9850(30);
        }
    }
}
