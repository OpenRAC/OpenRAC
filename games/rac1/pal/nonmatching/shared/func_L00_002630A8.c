/* NON_MATCHING func_L00_002630A8 -- src/overlays/shared/mobyutil_00261B00.c
 * Best so far: BYTES 15/816 (98.2% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_002630A8: clamps a point against a polyline (table D_L00_001B0830[idx]: count at +0, 16-byte vertices
 *   p6.c (and p8.c) is BYTES 15/816: everything matches except the order of the argument-register moves at two cal
 *   Key shapes: j = i + 1 computed in the loop and `i = j` as the increment, index locals for the first tests, the
 */
extern float func_001F9C78(void *, void *);
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9CA0(void *, void *, void *);
extern float func_001F9B88(float);
extern float func_001F9CB8(void *);
extern void func_001F9BD8(void *, void *, void *);
extern char D_L00_001B0830[];

/* clamps a point to a polyline's segments and writes the pushed-out point */
int func_L00_002630A8(int idx, void *pos, void *out, float thr) {
    float A[4], F[4], P[4], B[4], D[4], E[4], C[4];
    char *S;
    float base;
    int r = 0;
    int i, j, o;

    float th = thr;
    qcopy(P, pos);
    S = *(char **)(D_L00_001B0830 + idx * 4);
    base = func_001F9C78(P, D_0013E633 + 0x10AD);
    for (i = 0; i < *(int *)S - 1; i = j) {
        char *a;
        float d, len, t;
        j = i + 1;
        o = i * 16;
        if (*(float *)(S + o + 0x1C) == 0.0f && *(float *)(S - (-(j * 16)) + 0x1C) == 0.0f) continue;
        a = S + (o + 0x10);
        func_001F9BF0(A, P, a);
        func_001F9BF0(B, S + (o + 0x20), a);
        t = func_001F9C78(A, D_0013E633 + 0x10AD);
        func_L00_001FF4B0(C, D_0013E633 + 0x10AD, t);
        func_001F9BF0(A, A, C);
        t = func_001F9C78(B, D_0013E633 + 0x10AD);
        func_L00_001FF4B0(C, D_0013E633 + 0x10AD, t);
        func_001F9BF0(B, B, C);
        func_L00_001FF4B0(D, B, 1.0f);
        func_001F9CA0(E, A, D);
        if (thr < func_001F9B88(func_001F9C78(E, D_0013E633 + 0x10AD))) continue;
        len = func_001F9CB8(B);
        d = func_001F9C78(A, D);
        if (len < d || d < 0.0f) {
            if (func_001F9CB8(A) < thr) {
                r = 1;
                func_L00_001FF4B0(F, A, th);
                func_001F9BD8(P, F, a);
            }
        } else {
            r = 1;
            func_L00_001FF4B0(C, D, d);
            func_001F9BF0(F, A, C);
            func_L00_001FF4B0(F, F, th);
            func_001F9BD8(F, F, C);
            func_001F9BD8(P, F, a);
        }
    }
    if (r != 0) {
        float f = base - func_001F9C78(P, D_0013E633 + 0x10AD);
        func_L00_001FF4B0(B, D_0013E633 + 0x10AD, f);
        func_001F9BD8(out, P, B);
    }
    return r;
}
