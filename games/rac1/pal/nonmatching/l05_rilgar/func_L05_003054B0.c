/* NON_MATCHING func_L05_003054B0 -- src/overlays/l05_rilgar/vendor_002D28D0.c
 * Best so far: BYTES 465/712 (34.7% of the bytes match), checked 2026-10-06.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9CA0(void *, void *, void *);
extern float func_001F9B88(float);
extern float func_001F9CB8(void *);
extern float func_001F9C78(void *, void *);

/* Pushes pt out of the walls of fence path idx (segments whose points both have w == 0 are open) to at
 * least radius away in the XY plane; writes the pushed point to out if any wall was hit and returns the
 * smallest distance to a wall line. */
float func_L05_003054B0(int idx, float *pt, float *out, float radius) {
    float a[4];
    float res[4];
    float p[4];
    float e[4];
    float n[4];
    float c[4];
    float q[4];
    int found = 0;
    float best = 512.0f;
    int i;
    qcopy(p, pt);
    for (i = 0; i < *(int *)D_L05_001B0CB0_x[idx] - 1; i++) {
        char **lp = &D_L05_001B0CB0_x[idx];
        float d, len, t;
        if (*(float *)(*lp + (i << 4) + 0x1C) == 0.0f && *(float *)(*lp + ((i + 1) << 4) + 0x1C) == 0.0f) continue;
        func_001F9BF0(a, p, *lp + ((i << 4) + 0x10));
        a[2] = 0.0f;
        func_001F9BF0(e, *lp + ((i << 4) + 0x20), *lp + ((i << 4) + 0x10));
        e[2] = 0.0f;
        func_L00_001FF4B0(n, e, 1.0f);
        func_001F9CA0(c, a, n);
        d = func_001F9B88(c[2]);
        if (d < best) best = d;
        if (radius < d) continue;
        len = func_001F9CB8(e);
        t = func_001F9C78(a, n);
        if (len < t || t < 0.0f) {
            if (!(func_001F9CB8(a) < radius)) continue;
            found = 1;
            func_L00_001FF4B0(res, a, radius);
        } else {
            found = 1;
            func_L00_001FF4B0(q, n, t);
            func_001F9BF0(res, a, q);
            func_L00_001FF4B0(res, res, radius);
            func_001F9BD8(res, res, q);
        }
        func_001F9BD8(p, res, *lp + ((i << 4) + 0x10));
        p[2] = pt[2];
    }
    if (found) qcopy(out, p);
    return best;
}
