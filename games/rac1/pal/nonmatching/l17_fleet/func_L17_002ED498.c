/* NON_MATCHING func_L17_002ED498 -- src/overlays/l17_fleet/vendor_002AA068.c
 * Best so far: BYTES 2/1620 (99.9% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   - run13 p12: BYTES 48; hi = old; if (hi < v) form: no change (gcc loads into hi, copies to old; retail the rev
 *   - run14 p13: BYTES 42; lo ternary before hi ternary fixes old/hi; lo-cond reg $3 vs retail $5 left
 *   - run15-16 p14: inline ((int *)(D+0x4A6))[1] folds to %lo(D+0x4AA): worse (run 16 was a re-run for the head of
 *   - run17 p15: BYTES 56; reusing h for the head pointer and if-form lo: worse, reverted
 *   - run18 p16: BYTES; MACRO_ADDR on D_0013E15A makes lui/addiu one la macro (adjacent like retail) but sched hoi
 *   - run19 p17: BYTES 99; tex/s statements with the MACRO_ADDR la: still hoisted; lo = old > v ? v : old: same $3
 *   - permuter (16 workers, ~1h): best scores 35, 45; register allocation tie on loop min/max (v/s3 vs t0/a3)
 *   BEST p13.c BYTES 42/1620. Left: (1) first func_001F5800 call scheduling (retail sd tex, then lui/addiu s0 adja
 *   Notes for lead: the candidate calls func_L17_002EBF08 through a float-first alias (same EABI registers); reord
 */
extern void func_00234C98(int, long);
extern int func_001F4868(int);
extern long func_001F4868_l(int) __asm__("func_001F4868");
extern void func_001F5800(int, int, int, int, int, int, int, int, long, long);
extern void func_L17_002ED258(int a, char *p);
extern void func_L11_003126D8(void *, void *, void *, int);
extern float func_001FA888(int);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern float func_L00_0025F368(float);
extern void func_L17_002EBF08_f(float, float, float, float, unsigned char, unsigned char, unsigned char, unsigned char) __asm__("func_L17_002EBF08");
extern int func_001F9850(int);
extern void func_L11_00311F98(float, float, float, void *, int, int, long);
extern void func_001F5E60(float, float, float, float, float, int, int, int, int, int, int, int, float, float);
extern void *func_001FE540_id(int) __asm__("func_001FE540");
extern int func_001F6F40_c(int, int, long, void *, int) __asm__("func_001F6F40");
extern float func_001F9B88(float);
extern unsigned char D_0013E15A[];
extern unsigned char D_0013E633[];
extern int D_L17_0015F6B0 MACRO_ADDR;
extern char D_L17_001D9CB0[];
extern int D_L17_001D9E38[];
extern short D_L17_001621F0;
extern short D_L17_001621F4;
extern short D_L17_00162200;

/* Draws the fleet HUD: target marker, ammo icons, gauge sprites and warnings, and refreshes the gauge colour table. */
void func_L17_002ED498(char *moby) {
    char *d = *(char **)(moby + 0x78);
    char *s;
    unsigned char *base;
    unsigned char *g;
    unsigned char *h;
    unsigned char *b;
    int sp10;
    int sp14;
    int tex;
    int i;
    unsigned char n;
    unsigned char c;
    int j;
    int x;
    int y;
    int ex;
    int ey;
    int a;
    int v;
    int old;
    int lo;
    int hi;
    int idx;
    float t;
    float k;
    float m;
    float px;
    float py;
    float f;
    float q;

    func_00234C98(0x42, 0x8000000044L);
    func_001F5800(0x170, (s = (char *)D_0013E15A + 0x4A6, *(int *)(s + 4)) - 0x90, 0x80, 0x80, 0, 0, 0x80, 0x80, 0x70808080, func_001F4868_l(*(int *)(d + 0x104) + 0x28));
    func_001F5800(0x170, *(int *)(s + 4) - 0x90, 0x80, 0x80, 0, 0, 0x80, 0x80, 0x70808080, func_001F4868_l(*(int *)(d + 0x104) + 0x29));
    func_00234C98(0x42, 0x8000000048L);
    func_L17_002ED258((int)moby, d);
    if (*(int *)(d + 0x88) != 0) {
        func_L11_003126D8(*(char **)(d + 0x88) + 0x10, &sp10, &sp14, 0);
        if (*(int *)(d + 0x8C) > *(int *)&D_L17_001621F0) {
            t = func_001FA888(*(int *)(d + 0x8C) - *(int *)&D_L17_001621F0);
            t = t / func_001FA888(*(int *)&D_L17_001621F4);
            m = 2.0f * (1.0f - t);
            k = t * 5.0f + 1.0f;
            if (m > 1.0f) m = 1.0f;
            c = func_001FA898_r(m * 96.0f);
            func_L17_002EBF08_f(func_001FA888(sp10), func_001FA888(sp14), k, func_L00_0025F368(func_001FA888(D_L17_0015F6B0) / 30.0f), 0, 0xFF, 0, c);
            *(int *)(d + 0xEC) = 0;
        } else {
            a = 0xFF;
            if ((*(int *)(d + 0x8C) / func_001F9850(0x14)) & 1) a = 0;
            func_L17_002EBF08_f(func_001FA888(sp10), func_001FA888(sp14), 1.0f, 0.0f, 0xFF, a, 0, 0x60);
            *(int *)(d + 0xEC) = *(int *)(d + 0x88);
        }
    }
    y = 0x40;
    x = 0x18;
    base = D_0013E633 + 0xE1D;
    n = base[0x15F7];
    for (i = 0; i < n; i++) {
        g = D_0013E633 + 0xE1D;
        if (i < g[0x15F6]) {
            func_L11_00311F98(x, y, 0.5f, D_L17_001D9CB0, 0x19, 0xFFFFF3, 0x50008F00);
        } else {
            func_L11_00311F98(x, y, 0.5f, D_L17_001D9CB0, 0x19, 0xFFFFF3, 0x20004F00);
        }
        n = g[0x15F7];
        if (i == n >> 1) {
            y = 0x2E;
            x += 0x1E;
        }
        y += 0x12;
    }
    ex = *(int *)(d + 0xE0);
    ey = *(int *)(d + 0xE4);
    tex = func_001F4868(0x11);
    func_001F5E60(ex, ey, 40.0f, 40.0f, 0.0f, 0x3F, 0x3F, tex, 0xFFFFF3, 0xFF20FF20, 0, 0, 0.5f, 0.5f);
    func_001F5E60(ex, ey, 40.0f, 40.0f, 0.0f, 0x3F, 0x3F, func_001F4868(0x12), 0xFFFFF3, 0xFF20FF20, 0, 0, 0.5f, 0.5f);
    func_001F5E60(ex, ey, 10.0f, 10.0f, 0.0f, 0x1F, 0x1F, func_001F4868(8), 0xFFFFF3, 0xFF20FF20, 0, 0, 0.5f, 0.5f);
    b = D_0013E633 + 0xE1D;
    if (*(float *)(b + 0x15FC) < *(float *)(b + 0x1600) / 10.0f && (D_L17_0015F6B0 / 90) & 1) {
        func_001F6F40_c(0x100, 0x186, 0x80000080L, func_001FE540_id(0x526E), 0x64);
    }
    if (*(unsigned char *)(moby + 0x20) == 8) {
        func_001F6F40_c(0x100, 0xC8, 0x80005080L, func_001FE540_id(0x5243), 0x64);
    }
    h = D_0013E633 + 0xE1D;
    f = *(float *)(h + 0x15FC) - *(float *)(d + 0xF0);
    f *= 0.2f;
    q = func_001F9B88(f);
    if (q > 1.0f) f /= q;
    *(float *)(d + 0xF0) += f;
    v = func_001FA898_r(*(float *)(d + 0xF0) * 251.0f / *(float *)(h + 0x1600)) + 2;
    if (v > 0xFD) v = 0xFD;
    if (v < 2) v = 2;
    old = *(int *)(d + 0xF4);
    lo = old <= v ? old : v;
    hi = old < v ? v : old;
    for (j = lo; j <= hi; j++) {
        idx = (j & 0xE7) | ((j & 0x10) >> 1) | ((j & 8) << 1);
        if (j < v) {
            (*(int **)&D_L17_00162200)[idx] = D_L17_001D9E38[idx];
        } else {
            (*(int **)&D_L17_00162200)[idx] = 0x80000000;
        }
    }
    *(int *)(d + 0xF4) = v;
}
