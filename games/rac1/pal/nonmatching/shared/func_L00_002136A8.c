/* NON_MATCHING func_L00_002136A8 -- src/overlays/shared/help_0020CDF0.c
 * Best so far: BYTES 6/708 (99.2% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Hero step-up snap (HeroStepUpSnap): if flag at g+0x20B3 is set, pulls the hero position g+0x80 toward g+0x2A0 
 *   Best: p4.c, BYTES 6/708 (2 words). Only difference: in the g+0x88 / g+0x304 call setup, retail emits `addiu $a
 *   The store `*(int *)(h + 0x304) = 0` needs `char *h = D_0013E633 + 0xE1D;` declared inside the store's own bloc
 *   Would unblock: a wording that changes the scheduler's tie between the two address computations (unknown).
 */
extern float func_001F9C78(void *, void *);
extern float func_001F9CB8(void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern float func_001F9D48(void *, void *);
extern float func_L00_0025C918(float *p, float *v, float t, float u1, float u2, float eps);
extern float D_0015EE60 MACRO_ADDR;

/* Snaps the hero's follow position toward its target, stepping up by a bounded amount. */
void func_L00_002136A8(void) {
    char *g = D_0013E633 + 0xE1D;
    float v[4];
    if (*(unsigned char *)(g + 0x20B3) != 0) {
        float f2 = func_001F9C78(g + 0xE0, g + 0x270);
        float f1;
        float len;
        if (*(short *)(g + 0x30E) != 0) return;
        f1 = *(float *)(g + 0x2DC);
        if (!(f1 < 0.0f)) return;
        if (f1 < f2 + 0.0001f || *(int *)(g + 0x2084) == 0x3E) {
            qcopy(g + 0x80, g + 0x2A0);
            return;
        }
        func_001F9BF0(v, g + 0x2A0, g + 0x80);
        len = func_001F9CB8(v);
        if (len < D_0015EE60 * 0.21f) {
            if (D_0015EE60 * 0.05f < len) len = D_0015EE60 * 0.05f;
        } else {
            len = len * 0.5f;
        }
        func_L00_001FF4B0(v, v, len);
        func_001F9BD8(D_0013E633 + 0xE9D, D_0013E633 + 0xE9D, v);
    } else {
        if (*(short *)(g + 0x30E) == 0 && *(float *)(g + 0x88) < *(float *)(g + 0x2D8)) {
            float x = func_001F9B88(*(float *)(g + 0x88) - *(float *)(g + 0x2D8));
            float y = func_001F9B88(*(float *)(g + 0xE8)) + 0.01f;
            if (x < y) {
                if (*(unsigned char *)(g + 0x257) != 0) {
                    float d = func_001F9D48(g + 0x210, g + 0x80);
                    if (*(float *)(g + 0x234) * 0.5f < d) {
                        *(float *)(g + 0x88) = *(float *)(g + 0x2D8);
                        return;
                    }
                } else {
                    *(float *)(g + 0x88) = *(float *)(g + 0x2D8);
                    return;
                }
                func_L00_0025C918((float *)(g + 0x88), (float *)(g + 0x304), *(float *)(g + 0x2D8),
                                  D_0015EE64 * 0.057f, D_0015EE64 * 0.3f, D_0015EE6C * 2.0f);
            } else {
                float f2 = *(float *)(g + 0x108);
                char *q;
                if (f2 < 0.0f) {
                    float n = *(float *)(g + 0x88) - f2;
                    float lim = *(float *)(g + 0x2D8);
                    *(float *)(g + 0x88) = n;
                    if (lim < n) *(float *)(g + 0x88) = lim;
                }
                q = D_0013E633 + 0xEA5;
                func_L00_0025C918((float *)q, (float *)(q + 0x27C), *(float *)(q + 0x250),
                                  D_0015EE64 * 0.057f, D_0015EE64 * 0.3f, D_0015EE6C + D_0015EE6C);
            }
        } else {
            char *h = D_0013E633 + 0xE1D;
            *(int *)(h + 0x304) = 0;
        }
    }
}
