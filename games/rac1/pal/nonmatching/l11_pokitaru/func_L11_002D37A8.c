/* NON_MATCHING func_L11_002D37A8 -- src/overlays/l11_pokitaru/vendor_002CC828.c
 * Best so far: SIZE ours 448 / retail 452, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Probes whether a moby can drop to ground at a target vector, writing the landing point on success. Best is p3.
 *   Only the first loop differs: retail jumps to the condition and increments i at the top, with beqz (not beql) a
 *   Would need the loop shape that keeps a jump into the condition.
 */
extern int func_00215570(void *arg0, int arg1);
extern void func_001F9BF0(void *, void *, void *);
extern float func_001F9CB8(void *a);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern int func_L00_0025A778(void *, void *, int);
extern float func_L00_0025A748_f(void *) __asm__("func_L00_0025A748");
extern float func_001F9B88(float);
extern int func_L00_001EFFF0(void *, void *, int, void *, int);
extern int *D_L11_001B11B0[];
typedef int u128 __attribute__((mode(TI)));

/* Tests whether a moby can drop onto the ground, writing the landing point. */
int func_L11_002D37A8(char *m, float *out) {
    char *s = *(char **)(m + 0x78);
    int *p = (int *)(s + 0xF8);
    int i;
    int found = 0;
    float a[4];
    float b[4];
    float g;
    i = 0;
    while (i < 2) {
        if (func_00215570(m + 0x10, *p++)) {
            found = 1;
            break;
        }
        i++;
    }
    if (found) {
        float *pos = (float *)(m + 0x10);
        func_001F9BF0(a, out, pos);
        if (func_001F9CB8(a) > 4.0f) func_L00_001FF4B0(a, a, 4.0f);
        func_001F9BD8(a, a, pos);
        {
            int *q = D_L11_001B11B0[*(int *)(s + 0x60 + *(int *)(s + 0x158) * 4)];
            if (func_L00_0025A778(a, q + 4, *q)) {
                a[2] += 5.0f;
                g = func_L00_0025A748_f(a);
                if (func_001F9B88(g - *(float *)(m + 0x18)) > 0.5f) {
                    qcopy(b, pos);
                    b[2] += 0.15f;
                    a[2] = g + 0.15f;
                    if (func_L00_001EFFF0(b, a, 2, m, 0)) {
                        a[2] = g;
                        qcopy(out, a);
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}
