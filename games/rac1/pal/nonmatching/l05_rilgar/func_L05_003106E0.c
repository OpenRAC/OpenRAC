/* NON_MATCHING func_L05_003106E0 -- src/overlays/l05_rilgar/vendor_0030EB68.c
 * Best so far: BYTES 10/492 (98.0% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Scatter points loop. Best p3.c/p5.c: only 3 insns differ (mov.s f12,f20 scheduled after daddu a2/a3 instead of
 */
extern void func_001F9BC0(void *);
extern float func_001FA888(int);
extern float func_002140F8(float, float);
extern float func_00214158(void);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern void func_001F9BD8(void *, void *, void *);
extern int func_001F9850(int);
extern float func_L00_00258C80(float lo, float hi);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern void func_L05_0029CA28(void *, void *, int, int, float, int);
extern float D_0015EE6C MACRO_ADDR;
extern short D_L05_00161E74;
extern short D_L05_00161E78;
extern short D_L05_00161E7C;
extern short D_L05_00161E80;
extern short D_L05_00161E84;
extern short D_L05_00161E88;
extern short D_L05_00161E8C;
extern short D_L05_00161E90;

/* Scatters points along a range, placing each one with a heading and radius. */
void func_L05_003106E0(char *obj, float a, float b) {
    float v[4];
    float w[4];
    float ground, x, y, t, u;
    int i, j, k, m, n;
    func_001F9BC0(v);
    v[2] = *(float *)&D_L05_00161E88 * D_0015EE6C;
    ground = v[2] * 0.75f * func_001FA888(*(int *)&D_L05_00161E78);
    j = 0;
    for (i = j; (float)i < (b - a) / *(float *)&D_L05_00161E74; i++) {
        float fi = (float)j;
        x = func_002140F8(fi, *(float *)&D_L05_00161E90);
        y = func_00214158();
        w[0] = func_001F9F90(y) * x;
        w[1] = func_001F9FA8(y) * x;
        w[2] = fi;
        func_001F9BD8(w, obj + 0x10, w);
        w[2] = func_002140F8(a, b) - ground;
        t = func_002140F8((a + b) * 0.5f, b) - ground;
        if (t < w[2]) w[2] = t;
        u = func_002140F8(*(float *)&D_L05_00161E7C, *(float *)&D_L05_00161E80);
        k = func_001F9850(*(int *)&D_L05_00161E78);
        m = func_001FA898_r(func_L00_00258C80(fi, (float)*(int *)&D_L05_00161E84));
        n = *(int *)&D_L05_00161E8C;
        func_L05_0029CA28(w, v, k, m, u, n);
    }
}
