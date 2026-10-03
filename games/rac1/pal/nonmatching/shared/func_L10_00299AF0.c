/* NON_MATCHING func_L10_00299AF0 -- src/overlays/shared/vendor_00299AF0.c
 * Best so far: BYTES 8/548 (98.5% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L10_00299AF0: animates 4 colour channels of a moby display (drift with wrap at 255, re-roll seed via func
 *   Only difference: scheduling of `daddu $a1,$s0,$zero` vs `andi $a2,$v0,0xFF` (retail puts the col move before a
 */
extern float D_L10_001672C0[];
extern void func_001F9BF0(void *, void *, void *);
extern void func_L00_001FF4B0(void *, void *, float);
extern int func_001F9908(void *);
extern int func_001F9850(int);
extern float func_001FA888(int);
extern float func_001F9B88(float);
extern int func_001FA8A8(int, int, float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern void func_L00_00273E08(float, void *, int, unsigned char, int, int, int, int);

typedef struct {
    char pad[0x10];
    float col[4];   /* 0x10 */
    float rate[4];  /* 0x20 */
    int seed[4];    /* 0x30 */
    float size[4];  /* 0x40 */
} Data;

/* Animates and draws the four colour channels of a moby's display: drifts each value, re-rolls on demand and draws it. */
void func_L10_00299AF0(char *moby) {
    float va[4];
    float vb[4];
    Data *d;
    int i;

    *(float *)(moby + 0x18) = *(float *)(moby + 0x18) + 0.5f;
    d = *(Data **)(moby + 0x78);
    func_001F9BF0(va, D_L10_001672C0, moby + 0x10);
    func_L00_001FF4B0(va, va, -0.3f);
    func_L00_001FF4B0(vb, va, 0.1f);
    func_001F9BD8(va, va, moby + 0x10);
    *(float *)(moby + 0x18) = *(float *)(moby + 0x18) - 0.5f;
    for (i = 0; i < 4; i++) {
        float a = d->col[i] + d->rate[i];
        float t;
        int c;
        int r;
        d->col[i] = a;
        if (a >= 255.0f) {
            d->col[i] = a - 255.0f;
        } else if (a <= 0.0f) {
            d->col[i] = a + 255.0f;
        }
        if (func_001F9908(&d->seed[i]) != 0) {
            d->seed[i] = func_001F9850(0xFF);
        }
        t = func_001FA888(func_001F9850(0xFF) - d->seed[i]);
        t = t / (float)func_001F9850(0xFF);
        c = func_001FA8A8(0x4040FFFF, 0x1040FFFF, func_001F9B88(0.5f - t));
        r = func_001FA898_r(d->col[i]);
        func_L00_00273E08(d->size[i], va, c, r, 0x35, 1, 2, 0);
        func_001F9BD8(va, va, vb);
    }
}
