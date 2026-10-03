/* NON_MATCHING func_L18_002EBBF0 -- src/overlays/l18_veldin2/vendor_002A8400.c
 * Best so far: BYTES 129/1236 (89.6% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * Cannot land as written (#define in a candidate): rewrite that in plain C first.
 * What the last attempts found:
 *   ## Round 1 (match worker, 14 of 16 runs)
 *   Function: Veldin 2 effect spawner. Copies self+0x10 (QVec aligned struct assign, no sq $zero: not a wall), spa
 *   Best: p10.c, same size (1236), BYTES 153/1236; all remaining diffs are register/schedule ties, no structural d
 *   What mattered: -0.34906584f constants (0x3EB2B8C2); 'v20[2] += ...' before the v10[3]/v20[3] stores (fixes loa
 *   Remaining: (1) saved-reg numbering (self in $s5 vs retail $s4, loop counters), f20/f21 swap for the two func_0
 *   Declarations: gp ints/floats declared short and read via *(int*)&x as in the neighbouring func_L18_002EB988; b
 */
extern int func_001F9850(int);
extern void func_L06_0030D338(void *, int, int, int);
extern int func_L00_0028EF68(int i, int a1, int v, int k);
extern float func_002140F8(float, float);
extern float func_00214158(void);
extern void func_00215C00(void *, float, float, float);
extern void func_001F9C30(void *, void *, float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern char *func_00219780(void *, void *, void *, int, int, int, int, int, int);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern void func_001F9BD8(void *, void *, void *);
extern int func_002140B0(int);
extern int func_L00_00258BC8(int, int);
extern void func_L00_0026B890(void *, void *, int, int, int, int, int, int, float, float);
extern void func_L00_002ADBB0(void *, void *, void *, int, int, int, int, int, float);
extern float D_0015EE6C __attribute__((section(".sdata")));
extern short D_L18_00161FF8, D_L18_00161FFC, D_L18_00162000, D_L18_00162004, D_L18_00162008, D_L18_0016200C;
extern short D_L18_00162010, D_L18_00162014, D_L18_00162018, D_L18_0016201C, D_L18_00162020, D_L18_00162024;
extern short D_L18_00162028, D_L18_00162030, D_L18_00162034, D_L18_00162038, D_L18_0016203C, D_L18_00162040;
extern short D_L18_00162044, D_L18_00162048, D_L18_0016204C, D_L18_00162050, D_L18_00162054, D_L18_00162058;
extern short D_L18_0016205C, D_L18_00162060, D_L18_00162064, D_L18_00162068, D_L18_0016206C, D_L18_00162070;
extern short D_L18_00162074, D_L18_00162078;
extern unsigned char D_L18_00162059[], D_L18_0016205A[], D_L18_0016205B[];
extern unsigned char D_L18_00162065[], D_L18_00162066[], D_L18_00162067[];
extern unsigned char D_L18_00162071[], D_L18_00162072[], D_L18_00162073[];
extern int D_L18_001D9F40[], D_L18_001D9F58[];
extern char D_L18_0015F660[];
extern char S2[] __asm__("D_L18_0015F660");
extern char S3[] __asm__("D_L18_0015F660");
extern char D_L18_00167700[];

#define MACRO_ADDR __attribute__((section(".sdata")))
#define I(x) (*(int *)&(x))
#define F(x) (*(float *)&(x))

void func_L18_002EBBF0(char *self) {
    QVec v10;
    float v20[4];
    int i, j;
    *(QVec *)&v10 = *(QVec *)(self + 0x10);
    func_L06_0030D338(&v10, func_001F9850(30), 0x80806060, 0x801010);
    func_L00_0028EF68(0x10, 0, (int)self, 0x58E);
    for (i = 0; i < I(D_L18_00161FFC);) {
        float a, b, k;
        int x, y, z;
        a = func_002140F8(-0.34906584f, 0.34906584f);
        b = func_00214158();
        func_00215C00(&v10, func_002140F8(F(D_L18_00162000), F(D_L18_00162004)) * D_0015EE6C, b, a);
        func_001F9C30(v20, &v10, F(D_L18_00162024));
        i++;
        v20[2] += F(D_L18_00162020) * func_002140F8(0.8f, 1.2f) * D_0015EE6C;
        v10.v[3] = F(D_L18_0016201C);
        v20[3] = F(D_L18_0016201C);
        k = func_002140F8(0.8f, 1.2f);
        x = func_001FA898_r(func_001F9850(I(D_L18_00162010)) * k);
        y = func_001FA898_r(func_001F9850(I(D_L18_00162014)) * k);
        z = func_001FA898_r(func_001F9850(I(D_L18_00162018)) * k);
        func_00219780(self + 0x10, &v10, v20, I(D_L18_00162008), I(D_L18_0016200C), x, y, z, I(D_L18_00161FF8));
    }
    for (j = 0; j < I(D_L18_00162028);) {
        float a, c, d, e, g, h;
        int p, q, r, t;
        j++;
        a = func_002140F8(F(D_L18_00162030), F(D_L18_00162034)) * D_0015EE6C;
        e = func_002140F8(1.0471976f, 1.3962634f);
        func_00215C00(&v10, a, func_00214158(), e);
        c = func_00214158();
        g = func_001F9F90(c);
        g = g * func_002140F8(0.0f, F(D_L18_0016204C));
        v20[0] = g;
        h = func_001F9FA8(c);
        h = h * func_002140F8(0.0f, F(D_L18_0016204C));
        v20[2] = 0.0f;
        v20[1] = h;
        func_001F9BD8(v20, v20, self + 0x10);
        p = D_L18_001D9F40[func_002140B0(6)];
        q = D_L18_001D9F58[func_002140B0(6)];
        d = F(D_L18_00162038) * 400000.0f;
        r = func_001F9850(func_L00_00258BC8(I(D_L18_0016203C), I(D_L18_00162040)));
        t = func_001F9850(func_L00_00258BC8(I(D_L18_00162044), I(D_L18_00162048)));
        func_L00_0026B890(v20, &v10, p, q, r, t, 0, 0, d, a);
    }
    if (F(D_L18_00162050) > 0.0f) {
        char *p = self + 0x10;
        int t = func_001F9850(I(D_L18_00162054));
        func_L00_002ADBB0(self, p, D_L18_0015F660, t, *(unsigned char *)&D_L18_00162058, D_L18_00162059[0], D_L18_0016205A[0], D_L18_0016205B[0], F(D_L18_00162050));
    }
    if (F(D_L18_0016205C) > 0.0f) {
        char *p = self + 0x10;
        int t = func_001F9850(I(D_L18_00162060));
        func_L00_002ADBB0(self, p, S2, t, *(unsigned char *)&D_L18_00162064, D_L18_00162065[0], D_L18_00162066[0], D_L18_00162067[0], F(D_L18_0016205C));
    }
    if (F(D_L18_00162068) > 0.0f) {
        char *p = self + 0x10;
        int t = func_001F9850(I(D_L18_0016206C));
        func_L00_002ADBB0(self, p, S3, t, *(unsigned char *)&D_L18_00162070, D_L18_00162071[0], D_L18_00162072[0], D_L18_00162073[0], F(D_L18_00162068));
    }
    if (F(D_L18_00162074) > 0.0f) {
        char *s = D_L18_00167700;
        *(float *)(s + 0x160) = F(D_L18_00162074);
        *(int *)(s + 0x168) = func_001F9850(I(D_L18_00162078));
    }
}
