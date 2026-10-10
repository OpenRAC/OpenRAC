/* NON_MATCHING func_L05_0031DDF0 -- src/overlays/shared/vendor_002CF2C0.c
 * Best so far: SIZE ours 1628 / retail 1656, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Lamp moby update (1656 bytes): reads a moby's light and spark state, calls the sprite/effect helpers in a 24-s
 *   Wall for the next worker: find a source form that keeps those six constants live across the calls without cons
 */
extern char D_L05_001672C0[];
extern char D_L05_0015F660[] MACRO_ADDR;
extern float D_0015EE70 MACRO_ADDR;
extern float D_0015EE60 MACRO_ADDR;
extern short D_L05_00162088;
extern short D_L05_00162090;
extern short D_L05_00162094;
extern float func_001F9D48(float *, float *);
extern float func_001FA888(int);
extern int func_001FA898(float);
extern void func_001F49B0(void *, void *);
extern char *func_L00_0025B478(void *, int, int);
extern void func_001F9C30(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern float func_001F9D10(void *, void *);
extern void func_0022ED80(int, int, int);
extern char *func_L05_0031E670(unsigned char *a0);
extern int func_001F9850(int);
extern void *func_L00_00265050(char *, int, float *, void *, int, int, float, float *, float *, float *);
extern void func_L01_00279790(void *);
extern float func_001FA748(float, float);
extern float func_002140F8(float, float);
extern int func_002140B0(int);
extern float func_00214158(void);
extern void func_00215C00(void *, float, float, float);
extern float func_L00_00258C80(float lo, float hi);
extern unsigned char *func_L00_00272F00(float *pos, int a, int col, int mode, int b, float *vel, float f0, float f1, float f2);
extern char *func_L00_0026DEA0(void *, int, void *, int, float, float, float, float);
extern int func_L00_00258BC8(int, int);
extern void func_0020D678(void *);
extern void func_L05_0031E468(char *m);

/* Lamp moby update: sets up its light and sparks, and deletes it when done. */
void func_L05_0031DDF0(unsigned char *m) {
    unsigned char *d = *(unsigned char **)(m + 0x78);
    float v10[4];
    float vA[4];
    float vB[4];
    float vC[4];
    float f0, f20, f21, f22, f28, f27, f26, f23, f24, f25;
    char *r16, *r17, *r;
    int i, n, s, t, r1, r2;

    if (!m[0x20]) {
        m[0x20] = 1;
        *(int *)(m + 0x90) = 0x7F000000 | (d[0x65] << 8) | (d[0x66] << 16) | d[0x64];
        *(int *)(d + 0x68) = (d[0x6E] << 16) | (d[0x6D] << 8) | d[0x6C];
    }
    f20 = func_001F9D48((float *)D_L05_001672C0, (float *)(m + 0x10));
    if (f20 >= 48.0f) return;
    if (f20 >= 32.0f) {
        f20 = (f20 - 32.0f) * 0.0625f;
        f0 = func_001FA888(d[0x6F]);
        *(int *)(d + 0x60) = func_001FA898(f0 * (1.0f - f20));
    } else {
        *(int *)(d + 0x60) = d[0x6F];
    }
    func_001F49B0(func_L05_0031E468, m);
    r16 = func_L00_0025B478(m, 0x810000, 0);
    m[0xA4] = 0xFF;
    if (r16 == 0) return;
    func_001F9C30(v10, m + 0xE0, *(float *)&D_L05_00162088);
    func_001F9BD8(v10, m + 0x10, v10);
    if (!(*(int *)(r16 + 0x24) & 0x800000)) {
        f20 = 0.8f;
        f0 = func_001F9D10(v10, r16);
        if (f0 > f20) {
            char *m2 = *(char **)(r16 + 0x20);
            if (*(short *)(m2 + 0xA6) != 0x47) return;
            f0 = func_001F9D10(v10, m2 + 0x10);
            if (f0 > f20) return;
        }
    }
    f20 = 12.0f;
    func_0022ED80(0, 0, (int)m);
    func_L05_0031E670((unsigned char *)m);
    s = func_001F9850(0x5A);
    r = func_L00_00265050(m, 0x5ED, (float *)(m + 0x10), m + 0x40, s, 0, *(float *)&D_0015EE70 * f20, (float *)D_L05_0015F660, (float *)D_L05_0015F660, (float *)D_L05_0015F660);
    if (r) *(s64 *)(r + 0x38) = *(s64 *)(m + 0x38);
    func_L01_00279790(m);
    f0 = *(float *)&D_L05_00162090;
    func_001F9C30(vA, m + 0xC0, f0 * D_0015EE60);
    vA[2] = vA[2] + *(float *)&D_L05_00162094 * D_0015EE60;
    s = func_001F9850(0x5A);
    r = func_L00_00265050(m, 0x5EE, (float *)(m + 0x10), m + 0x40, s, 0, D_0015EE70 * f20, vA, (float *)D_L05_0015F660, (float *)D_L05_0015F660);
    if (r) *(s64 *)(r + 0x38) = *(s64 *)(m + 0x38);
    vA[0] = -vA[0];
    vA[1] = -vA[1];
    *(float *)(m + 0x48) = func_001FA748(*(float *)(m + 0x48), 3.1415927f);
    s = func_001F9850(0x5A);
    r = func_L00_00265050(m, 0x5EE, (float *)(m + 0x10), m + 0x40, s, 0, D_0015EE70 * f20, vA, (float *)D_L05_0015F660, (float *)D_L05_0015F660);
    if (r) *(s64 *)(r + 0x38) = *(s64 *)(m + 0x38);

    f28 = 0.3f;
    f27 = 0.05f;
    f26 = 0.1f;
    f23 = 0.0f;
    f24 = 0.15f;
    f25 = 0.003f;
    i = 0x17;
    do {
        f22 = func_002140F8(f28, 4.0f);
        r1 = func_002140B0(2);
        n = r1 - 1;
        if (r1) n = r1;
        func_00214158();
        f21 = func_002140F8(0.3490658f, 1.5358897f);
        f20 = f21;
        f0 = func_002140F8(f27, f26);
        func_00215C00(vB, f0 * D_0015EE60, f21, f20);
        qcopy(vC, v10);
        f0 = func_L00_00258C80(f23, f24);
        vC[0] = vC[0] + f0;
        f0 = func_L00_00258C80(f23, f24);
        vC[1] = vC[1] + f0;
        f0 = func_L00_00258C80(f23, f24);
        vC[2] = vC[2] + f0;
        s = func_001F9850(0x3C);
        func_L00_00272F00(vC, s, *(int *)(d + 0x68) | 0x7F000000, 0, n, vB, f22 * f26, f22, f25);
        s = func_001F9850(0x3C);
        func_L00_00272F00(vC, s, 0x7F7F7F7F, 1, -n, vB, f22 * 0.07f, f22 * 0.7f, f25);
        vB[0] = 0.0f;
        vB[1] = 0.0f;
        f0 = func_002140F8(0.01f, f27);
        vB[2] = f0 * D_0015EE60;
        f20 = func_002140F8(60000.0f, 200000.0f);
        r1 = func_002140B0(2);
        r2 = func_002140B0(2);
        t = -r1;
        if (r2 == 0) t = r1;
        r17 = func_L00_0026DEA0(v10, t, vB, 0x407F7F7F, f28, 1.0f, 1.02f, f20);
        if (r17) {
            char *p;
            s = func_L00_00258BC8(0x5A, 0x78);
            s = func_001F9850(s);
            *(short *)(r17 + 0xA) = s;
            p = r17 + 0x20;
            *(int *)(p + 4) = 2;
            *(char *)(p + 0xA) = func_L00_00258BC8(0x40, 0x60);
            *(char *)(p + 0xB) = ((unsigned char *)r17)[0xA];
            if (func_002140B0(2)) *(char *)(r17 + 3) = 0x44;
        }
    } while (--i >= 0);
    func_0020D678(m);
}
