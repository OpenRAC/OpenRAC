/* NON_MATCHING func_L00_0020BC30 -- src/overlays/shared/help_00203E98.c
 * Best so far: BYTES 5/888 (99.4% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Hero ledge-grab check: predicts a position with gravity, tests via func_L00_001EFFF0 and angle/dot thresholds,
 *   Left: $v0/$v1 swapped around `q = p - 16; lq; lwc1 0x44(q)` (retail q in $v0, the lq temp in $v1; ours reverse
 */
typedef int u128 __attribute__((mode(TI)));
extern float func_L00_002342F8(float *v);
extern int func_001F9850(int);
extern int func_L00_0020CDF0(void *, void *);
extern int func_L00_0020A8B8(int, float, float);
extern void func_L00_00233E48(float *out, float r, float angle, float z);
extern int func_L00_001EFFF0(void *, void *, int, int, int);
extern int func_L00_001F3958(void);
extern float func_L00_002345B0(float *v);
extern void func_001F9EE8(void *, void *, void *);
extern float func_L00_001FF860(float, float);
extern float func_001FA850(float, float);
extern float func_001F9D48(void *, void *);
extern float D_0015EE6C MACRO_ADDR;
extern float D_L00_00173F80[];

// Checks whether the hero can grab a ledge in front of it and picks the grab state.
void func_L00_0020BC30(void) {
    u128 s[3];
    char *g = D_0013E633 + 0xE1D;
    char *c, *h;
    float *p, *q;
    float f20, f21, th;
    int n, t;
    if (*(float *)(g + 0x2DC) < 0.4f) return;
    if (func_L00_002342F8((float *)(g + 0xE0)) < D_0015EE6C * -20.0f) return;
    if (*(int *)(g + 0x4E8) != 0) {
        *(int *)(g + 0x1B4) = 0;
        return;
    }
    qcopy((char *)s, g + 0x80);
    n = func_001F9850(0x14);
    ((float *)s)[0] = ((float *)s)[0] + *(float *)(g + 0x100) * n;
    ((float *)s)[1] = ((float *)s)[1] + *(float *)(g + 0x104) * n;
    ((float *)s)[2] = ((float *)s)[2] + *(float *)(g + 0xE8) * n - *(float *)(g + 0x4A0) * n * n * 0.5f;
    if (func_L00_0020CDF0(s, g + 0x98)) {
        *(int *)(g + 0x1B4) = 0;
        *(short *)(g + 0x1F0) = n + 2;
        return;
    }
    if (func_L00_0020A8B8(0, 0.3f, 1.0f) == 0) return;
    f21 = 1.7f;
    if (*(int *)(g + 0x2084) != 0x11) {
        if (*(unsigned char *)(g + 0x255) == 0 || *(unsigned char *)(g + 0x254) != 0) f21 = 3.0f;
    }
    f20 = 0.0f;
    c = D_0013E633 + 0xE1D;
    func_L00_00233E48((float *)s, f20, f20, f21);
    func_L00_00233E48((float *)s + 4, *(float *)(c + 0x234) + 1.4f, f20, f21);
    if (func_L00_001EFFF0(s, (float *)s + 4, 2, 0, 0) == 0) return;
    t = func_L00_001F3958();
    if (t == 0xA || t == 0xC) return;
    p = D_L00_00173F80;
    if (func_L00_002345B0(p) < 1.3089969f) return;
    func_001F9EE8((float *)s + 8, p, c + 0x40);
    if (func_001FA850(func_L00_001FF860(((float *)s)[8], ((float *)s)[9]), 3.1415927f) > 0.87266463f) return;
    th = 0.72f;
    if (*(int *)(c + 0x2084) == 0x11) th = 0.95f;
    if (func_001F9D48((char *)p - 0x20, s) > th) return;
    q = p - 16;
    *(u128 *)(c + 0x460) = *(u128 *)p;
    *(float *)(c + 0x46C) = func_L00_001FF860(q[16], q[17]);
    if (*(int *)(c + 0x208C) != 4 && *(int *)(c + 0x2094) != 4 && *(int *)(c + 0x20A0) != 4) return;
    h = D_0013E633 + 0xE1D;
    if (*(int *)(h + 0x22B4) != 0) {
        if (func_001FA850(*(float *)(h + 0x47C), *(float *)(h + 0x46C)) < 2.9670596f) return;
    }
    if (*(int *)(h + 0x2084) == 0x11) *(int *)(h + 0x1B4) = func_001F9850(7);
    else *(int *)(h + 0x1B4) = func_001F9850(6);
}
