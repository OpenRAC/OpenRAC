/* NON_MATCHING func_L08_002DB768 -- src/overlays/l08_batalia/vendor_002B9438.c
 * Best so far: SIZE ours 964 / retail 972, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Particle streak from a1 toward a2 (n steps via func_L00_0026DEA0, extra sprites on the moby), then two ring bu
 *   Best: p3.c (964 vs 972 bytes). Left: retail keeps &t spilled at 0x34($sp) (lw before calls) and a2 homed at 0x
 */
typedef int u128_2DB768 __attribute__((mode(TI)));
extern void func_001F9BF0(void *dst, void *a, void *b);
extern void func_001F9C30(void *, void *, float);
extern float func_001FA888(int);
extern void func_001F9BD8(void *, void *, void *);
extern float func_L00_00258C80(float lo, float hi);
extern float func_002140F8(float, float);
extern int func_L00_00258BC8(int, int);
extern char *func_L00_0026DEA0(void *, int, void *, int, float, float, float, float);
extern int func_002140B0(int);
extern float func_00214158(void);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern int func_001F9850(int);
extern char *func_L00_0026DA50(void *pos, void *dir, int c, int d, int n, int k, float f);
extern short D_L08_00161A28, D_L08_00161A30, D_L08_00161A38;

// Emits a streak of particles from a1 toward a2, then two ring bursts.
void func_L08_002DB768(int unused, char *a1, char *a2, int n, float x0, float x1) {
    float tmp0[4];
    float v10[4];
    float t[4];
    int i;
    float step;
    float *pv, *pt;
    pt = t;
    func_001F9BF0(tmp0, a2, a1);
    func_001F9C30(tmp0, tmp0, 1.0f / (float)n);
    x1 = x1 - x0;
    step = x1 / (float)n;
    *(u128_2DB768 *)t = 0;
    t[2] = 0.01f;
    t[3] = 1.0f;
    pv = v10;
    for (i = 0; i < n; i++) {
        int c1, c2, c3, spd;
        float rnd;
        char *m;
        func_001F9C30(pv, tmp0, func_001FA888(i));
        func_001F9BD8(pv, a1, pv);
        t[0] = func_L00_00258C80(0.0f, 0.005f);
        t[1] = func_L00_00258C80(0.0f, 0.005f);
        t[2] = func_002140F8(*(float *)&D_L08_00161A28 * 0.1f, *(float *)&D_L08_00161A28);
        c1 = func_L00_00258BC8(0x30, 0x70);
        c2 = func_L00_00258BC8(0x30, 0x7F);
        c2 = c2 | (c2 << 8) | (c2 << 16);
        rnd = func_002140F8(1.0f, 1.01f);
        spd = func_L00_00258BC8(-2, 2);
        m = func_L00_0026DEA0(pv, spd, pt, (c1 << 24) | c2,
                              *(float *)&D_L08_00161A30, 1.0f, rnd,
                              step + x0 * 210000.0f);
        if (m != 0) {
            char *p = m + 0x20;
            if (func_002140B0(2) != 0) {
                m[3] = 0x44;
                c3 = func_L00_00258BC8(0x60, 0xE0);
                c3 = c3 | (c3 << 8) | (c3 << 16);
                *(int *)(m + 4) = (c1 << 24) | c3;
            }
            *(short *)(m + 0xA) = D_L08_00161A38;
            *(int *)(p + 4) = 2;
            p[0xA] = c1;
            p[0xB] = D_L08_00161A38;
        }
    }
    t[0] = func_001F9F90(func_00214158()) * 0.05f;
    t[1] = func_001F9FA8(func_00214158()) * 0.05f;
    *(int *)&t[2] = 0;
    t[2] = func_002140F8(0.01f, 0.03f);
    func_L00_0026DA50(a1, pt, 0x4F007FFF, 0x1FFFFFFF,
                      func_L00_00258BC8(func_001F9850(10), func_001F9850(20)), 1, 10000.0f);
    t[0] = func_001F9F90(func_00214158()) * 0.05f;
    t[1] = func_001F9FA8(func_00214158()) * 0.05f;
    *(int *)&t[2] = 0;
    t[2] = func_002140F8(0.01f, 0.03f);
    func_L00_0026DA50(a2, pt, 0x4F007FFF, 0x1FFFFFFF,
                      func_L00_00258BC8(func_001F9850(10), func_001F9850(20)), 1, 20000.0f);
}
