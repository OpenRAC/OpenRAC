/* NON_MATCHING func_L00_002E8210 -- src/overlays/shared/vendor_002E1660.c
 * Best so far: SIZE ours 976 / retail 980, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_002E8210 (CamType0Init / ActivateCamera_0): zeroes and fills the camera's data blocks (offsets 0x40/0
 *   Best is p6.c/p7.c: BYTES 317/980, same size, identical instruction multiset (mnemonic counts match exactly); e
 *   First remaining difference (+0x8c): retail emits sb 0xD6 before sw 0xC0, loads D_L00_00161D70 into $f0 (ours $
 */
extern char D_0013E633[];
extern short D_L00_00161E5C;
extern short D_L00_00161E60;
extern short D_L00_00161D70;
extern short D_L00_00161DEC;
extern short D_L00_00161DAC;
extern short D_L00_00161DB4;
extern short D_L00_00161DB0;
extern short D_L00_00161DF4;
extern float D_L00_0015F040 MACRO_ADDR;
extern float D_L00_0015F044_m __asm__("D_L00_0015F044") MACRO_ADDR;
extern int D_L00_0015F060;
extern void *D_L00_00166F04;
extern int func_001F9850(int);
extern void func_001F9BC0(void *);
extern void func_001F9BF0(float *, float *, float *);
extern float func_001F9C78(void *, void *);
extern void func_001F9C30(void *, void *, float);
extern float func_001F9CB8(void *);
extern void func_L00_002E80A8(char *);
extern void func_L00_002E58E0(char *);
extern void func_L00_002E7B68(int);

/* Initialises the type-0 camera: fills its data blocks and seeds the first position. */
void func_L00_002E8210(char *m) {
    char *a, *b, *c, *e, *f, *g, *p, *d, *pad;
    int r;
    float t;
    float v0[4];
    float v1[4];
    *(int *)&D_L00_00161E5C = 0;
    *(int *)&D_L00_00161E60 = 0;
    a = *(char **)(m + 0x70) + 0x40;
    qcopy(a, D_0013E633 + 0xE9D);
    *(float *)(a + 0xB0) = 1.5f;
    *(char *)(a + 0xC4) = 0;
    *(char *)(a + 0xD6) = 0;
    pad = D_0013E633 + 0xE1D;
    *(int *)(a + 0xC0) = *(int *)(pad + 0x2080);
    *(float *)(a + 0xE0) = 0.2f;
    *(float *)(a + 0xE4) = *(float *)&D_L00_00161D70;
    *(float *)(a + 0xDC) = *(float *)&D_L00_00161D70;
    *(float *)(a + 0xE8) = 0.2f;
    *(float *)(a + 0xB8) = 0.005f;
    *(short *)(a + 0xC6) = 0;
    *(short *)(a + 0xD8) = 0;
    *(int *)(a + 0xB4) = 0;
    *(short *)(a + 0xDA) = 0;
    *(int *)(a + 0xBC) = 0;
    b = *(char **)(m + 0x70) + 0x130;
    *(float *)(b + 0x54) = 0.003f;
    *(float *)(b + 0x30) = 2.0f;
    *(float *)(b + 0x58) = 4.64f;
    *(float *)(b + 0x2C) = 4.64f;
    *(float *)(b + 0x48) = 0.003f;
    *(int *)(b + 0x34) = 0;
    *(int *)(b + 0x38) = 0;
    *(int *)(b + 0x28) = 0;
    *(int *)(b + 0x20) = 0;
    *(int *)(b + 0x24) = 0;
    *(short *)(b + 0x3C) = 0;
    *(short *)(b + 0x3E) = 0;
    *(int *)(b + 0x44) = 0;
    *(int *)(b + 0x50) = 0;
    c = *(char **)(m + 0x70);
    *(short *)(c + 0x10) = 1;
    *(short *)(c + 0x12) = 0;
    *(short *)(c + 0x14) = 0;
    *(short *)(c + 0x16) = 0;
    D_L00_0015F040 = 0.75f;
    e = *(char **)(m + 0x70);
    *(short *)(e + 0x20) = 0;
    f = e + 0x20;
    *(int *)(f + 0x10) = 0;
    *(int *)(f + 0xC) = 0;
    *(short *)(f + 0x2) = 0;
    *(int *)(f + 0x14) = 0;
    *(int *)(f + 0x18) = 0;
    p = *(char **)(m + 0x70);
    *(int *)(p + 0x190) = 0;
    p += 0x190;
    *(float *)(p + 0x14) = 14.2857f;
    *(float *)(p + 0x8) = 0.005f;
    *(float *)(p + 0xC) = 0.2f;
    *(float *)(p + 0x10) = 0.15707964f;
    *(int *)(p + 0x4) = 0;
    p = *(char **)(m + 0x70);
    *(int *)(p + 0x1A8) = 0;
    p += 0x1A8;
    *(float *)(p + 0x14) = D_L00_0015F044_m;
    *(float *)(p + 0x18) = 0.5235988f;
    *(int *)(p + 0x24) = 0;
    *(int *)(p + 0x8) = 0;
    *(int *)(p + 0x4) = 0;
    *(int *)(p + 0xC) = 0;
    *(int *)(p + 0x10) = 0;
    *(int *)(p + 0x1C) = 0;
    *(int *)(p + 0x20) = 0;
    p = *(char **)(m + 0x70);
    *(int *)(p + 0x220) = 0;
    p += 0x220;
    *(int *)(p + 0x10) = 0;
    *(short *)(p + 0x4) = 0;
    *(short *)(p + 0x6) = 0;
    *(float *)(p + 0x8) = *(float *)(b + 0x2C);
    *(float *)(p + 0xC) = *(float *)&D_L00_00161DEC;
    r = func_001F9850(0x7D0);
    d = *(char **)(m + 0x70);
    g = d + 0x1D0;
    *(int *)(g + 0x30) = 0;
    *(short *)(g + 0x3A) = 0;
    *(short *)(g + 0x36) = 0;
    *(short *)(g + 0x38) = 0;
    *(int *)(g + 0x4C) = 0;
    *(int *)(g + 0x48) = D_L00_0015F060;
    *(short *)(g + 0x34) = r;
    *(float *)(g + 0x44) = *(float *)&D_L00_00161DAC;
    *(float *)(g + 0x3C) = *(float *)&D_L00_00161DB4;
    *(float *)(g + 0x40) = *(float *)&D_L00_00161DB0;
    func_001F9BC0(d + 0x1E0);
    p = *(char **)(m + 0x70);
    *(short *)(p + 0x240 + 0x28) = 0;
    *(int *)(p + 0x240 + 0x24) = 0;
    qcopy(p + 0x240, D_0013E633 + 0xE9D);
    qcopy(p + 0x250, D_0013E633 + 0xE9D);
    if (*(unsigned char *)(m + 0x7D) == 2) {
        func_L00_002E80A8(m);
        qcopy(m + 0x40, m);
        if (*(short *)((char *)D_L00_00166F04 + 0x86) == 7 && *(short *)(pad + 0x30C) == 0) {
            *(short *)(e + 0x20) = 0;
        } else {
            *(short *)f = func_001F9850(*(int *)&D_L00_00161DF4);
        }
    } else {
        func_L00_002E58E0(m);
        func_L00_002E7B68(1);
    }
    func_001F9BF0((float *)b, (float *)(m + 0x30), (float *)(a + 0x50));
    qcopy(b + 0x10, b);
    qcopy(c, m + 0x30);
    qcopy(g + 0x20, m + 0x30);
    func_001F9C30(v0, D_0013E633 + 0x10AD, func_001F9C78(b, D_0013E633 + 0x10AD));
    func_001F9BF0(v1, (float *)b, v0);
    t = *(float *)(b + 0x2C) - func_001F9CB8(v1);
    *(float *)(g + 0x30) = t;
    if (t < -0.1f) *(short *)(g + 0x3A) = 1;
    *(float *)(f + 8) = *(float *)(b + 0x30);
    *(float *)(f + 4) = *(float *)(a + 0xB0);
    *(short *)(m + 0x7E) = 0;
}
