/* NON_MATCHING func_L13_00306EF0 -- src/overlays/l13_gemlik/vendor_002EBD00.c
 * Best so far: SIZE ours 1644 / retail 1628, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Level 13 rocket moby update (class 1233): flies toward its target (func_L00_001FF4B0, func_L00_00272158 trail 
 *   Attempts: p0 goto ladder 1668 bytes vs 1628; p1 plain u128 copies for 0xC0/0xD0/0xE0 (qcopy was the wrong form
 *   Unblock: find which local retail keeps in $22 instead of $30 (the sp+0x20 buffer used as the loop quad) so the
 */
extern float func_001FA888(int);
extern char D_L13_001741C0[];
extern char D_L13_00174200[];
extern float D_L13_0015F660[] MACRO_ADDR;
extern int D_0015EE84 MACRO_ADDR;
extern unsigned char *func_L00_00272158(void *pos, float *vec, int s, int a, int col, int n, float x, float y, float z, float w, float pw);
extern unsigned char *func_L07_0029B070(char *pos, char *vec);
extern void func_L13_00307550(char *moby);
extern void func_L13_003075E0(char *moby);

/* Rocket moby update (level 13): flies toward the target, sprays its trail and checks its bounds. */
void func_L13_00306EF0(char *m) {
    char *p16 = m + 0x10;
    char *d = *(char **)(m + 0x78);
    char *p5;
    char *e;
    char *rp;
    char *rr;
    char *rr2;
    int i17;
    int i19;
    int r;
    int r10;
    int r15;
    int u2;
    int flag16;
    int h;
    float f0;
    float f1;
    float f20;
    float f21;
    float f22;
    float f23;
    unsigned char s0[16] __attribute__((aligned(16)));
    unsigned char s10[16] __attribute__((aligned(16)));
    unsigned char s20[16] __attribute__((aligned(16)));
    unsigned char s30[16] __attribute__((aligned(16)));
    unsigned char s60[16] __attribute__((aligned(16)));
    qcopy(s0, p16);
    if (*(unsigned char *)(m + 0x20) != 1) goto L7004;
    e = *(char **)(d + 0x20);
    if (e == 0) goto L746C;
    if (*(short *)(e + 0xA6) != 0x65) goto L746C;
    if (*(unsigned char *)(e + 0x20) == 0xFE) goto L746C;
    if (*(unsigned char *)(e + 0x20) == 0xFD) goto L746C;
    qcopy(m + 0x40, e + 0x40);
    *(u128 *)(m + 0xC0) = *(u128 *)(e + 0xC0);
    *(u128 *)(m + 0xD0) = *(u128 *)(e + 0xD0);
    *(u128 *)(m + 0xE0) = *(u128 *)(e + 0xE0);
    func_L00_00250800(*(void **)(d + 0x20), *(short *)(d + 0x26), p16);
    qcopy(d + 0x10, p16);
    if (*(char **)(d + 0x20) == 0) goto L6FF8;
    if (*(unsigned char *)(*(char **)(d + 0x20) + 0x20) == 0xFE) goto L6FF8;
    if (*(unsigned char *)(*(char **)(d + 0x20) + 0x20) == 0xFD) goto L6FF8;
    if ((*(unsigned short *)(*(char **)(d + 0x20) + 0x34) & 1) == 0) goto L6FF8;
    *(unsigned short *)(m + 0x34) |= 1;
    goto L747C;
L6FF8:
    *(unsigned short *)(m + 0x34) &= 0xFFFE;
    goto L747C;

L7004:
    rp = (char *)func_L00_0025B478(m, 0x230000, 0);
    p5 = p16;
    if (rp == 0) goto L7034;
    e = *(char **)(rp + 0x20);
    if (e == 0) goto L7038;
    if (*(short *)(e + 0xA6) == 0x1DF) goto L7454;
L7034:
L7038:
    func_001F9BD8(p5, p5, d);
    f21 = 0.25f;
    i17 = 0;
L7070:
    f0 = func_001FA888(i17);
    f0 = -f0;
    func_001F9C30(s20, d, f0 * f21);
    func_001F9BD8(s10, p5, s20);
    f20 = func_002140F8(2.5e4f, 3.5e4f);
    r10 = func_001F9850(15);
    rr = (char *)func_L00_00272158(s10, D_L13_0015F660, r10, 0x60, 0x4040FF, 3, f20, 5e2f, 1.0f, -0.0004f, 0.0f);
    i17++;
    if (rr == 0) goto L7134;
    *(unsigned char *)(rr + 9) = (unsigned char)(func_001FA898_r(4.0f) - 0x80);
L7134:
    if (i17 < 4) goto L7070;
    qcopy(s10, d);
    f22 = 1.0f;
    *(int *)(s10 + 8) = 0;
    func_L00_001FF500((float *)s10, (float *)s10, 1.0f);
    *(float *)(s10 + 8) = 1.0f;
    func_L00_0025A8C0(s30, m, 0x10001, 1.0f, s10);
    h = *(short *)(d + 0x24);
    flag16 = (h == 0);
    f20 = func_001F9D48(p5, d + 0x10);
    f1 = func_001F9D48(*(char **)(d + 0x2C) + 0x10, d + 0x10);
    f0 = *(float *)(d + 0x28);
    if (f0 < f20) {
        u2 = *(unsigned char *)(m + 0x31);
        goto L71D8;
    }
    if (!(f1 < f20)) goto L72C0;
    u2 = *(unsigned char *)(m + 0x31);
L71D8:
    if (u2 == 0) goto L72B0;
    f20 = 1.0f;
    f21 = -1.0f;
    f23 = 0.1f;
    f22 = 4.0f;
    i19 = 2;
    do {
        *(u128 *)s60 = 0;
        f0 = func_002140F8(f21, f20);
        i19--;
        *(float *)(s60 + 0) = f0;
        f0 = func_002140F8(f21, f20);
        *(float *)(s60 + 4) = f0;
        f0 = func_002140F8(f21, f20);
        *(float *)(s60 + 8) = f0;
        qcopy(s20, s60);
        f0 = func_001F9CB8((void *)s60);
        func_L00_001FF4B0(s20, s20, f0 * f23);
        func_001F9BD8(s20, s60, s20);
        f0 = func_002140F8(D_0015EE6C + D_0015EE6C, D_0015EE6C * f22);
        func_L00_001FF4B0(s20, s20, f0);
        func_L07_0029B070(p5, (char *)s20);
    } while (i19 >= 0);
    func_L13_00307550(m);
    goto L746C;
L72B0:
    func_L13_00307550(m);
    goto L746C;
L72C0:
    rr2 = (char *)func_L00_001EFFF0(s0, p5, flag16, m, s30);
    if (rr2 == 0) goto L747C;
    if (*(int *)(D_L13_001741C0 + 0x38) == *(int *)(d + 0x20)) goto L7480;
    qcopy(p5, D_L13_001741C0 + 0x20);
    if (*(unsigned char *)(m + 0x31) == 0) goto L7434;
    f21 = -1.0f;
    f20 = f22;
    i17 = 0;
    do {
        *(u128 *)s20 = 0;
        f0 = func_002140F8(f21, f20);
        *(float *)(s20 + 0) = f0;
        f0 = func_002140F8(f21, f20);
        *(float *)(s20 + 4) = f0;
        f0 = func_002140F8(f21, f20);
        *(float *)(s20 + 8) = f0;
        qcopy(s60, s20);
        func_L00_001FF610(s20, d, D_L13_00174200);
        f0 = func_001F9CB8((void *)s20);
        func_L00_001FF4B0(s60, s60, f0 * 0.5f);
        func_001F9BD8(s60, s20, s60);
        f0 = func_002140F8(D_0015EE6C * 3.0f, D_0015EE6C * 6.0f);
        func_L00_001FF4B0(s60, s60, f0);
        r10 = func_001F9850(10);
        r15 = func_001F9850(15);
        r = func_L00_00258BC8(r10, r15);
        func_L00_0026EBC0(p5, (char *)s60, 0x7F2F4F6F, r, 3e4f);
        i17++;
    } while (i17 < 5);
L7434:
    rp = *(char **)(D_L13_001741C0 + 0x18);
    if (rp == 0) goto L7464;
    if (*(short *)(rp + 0xA6) != 0x1DF) goto L7464;
    goto L7454;
L7454:
    func_L13_003075E0(m);
    goto L746C;
L7464:
    func_L13_00307550(m);
L746C:
    func_0020D678(m);
    goto L7510;
L747C:
    f0 = *(float *)(m + 0x10);
L7480:
    f20 = 2.0f;
    if (f0 < f20) goto L7504;
    f1 = 1021.0f;
    if (f1 < f0) goto L7504;
    f0 = *(float *)(m + 0x14);
    if (f0 < f20) goto L7504;
    if (f1 < f0) goto L7504;
    f0 = *(float *)(m + 0x18);
    if (f0 < f20) goto L7504;
    if (!(f1 < f0)) goto L7510;
L7504:
    func_0020D678(m);
L7510:
    return;
}
