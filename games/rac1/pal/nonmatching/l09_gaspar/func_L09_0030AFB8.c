/* NON_MATCHING func_L09_0030AFB8 -- src/overlays/l09_gaspar/vendor_002C2B08.c
 * Best so far: SIZE ours 1556 / retail 1580, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Level 9 shuttle moby update (class 1766): state machine (0 start, 1 and 2 steer with func_L00_00263B78/BF8, 3 
 *   Attempts: p0 goto ladder, 1556 bytes vs 1580 (24 short); p1 table base in a local and the bnel shape for the t
 *   Unblock: the saved-register set, which needs the entry order of the $29 copy and the table base; a sweep of th
 */
extern char *D_L09_001B0930[];
extern int D_0015EE84 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern int D_L09_0015F6B0 MACRO_ADDR;
extern unsigned char D_0014171B[] NOT_SDA;
extern void func_001F9C30(void *, void *, float);
extern void func_0020D960(char *, int, unsigned char *);
extern float func_L00_001FF860(float, float);
extern float func_001FA748(float, float);
extern void func_L00_00263B78(float, float, char *, float *, float *);
extern void func_L00_00263BF8(void *, char *, char *, float, float, float);
extern int func_00215570(void *, int);
extern int func_L00_0028EB98(void *, int);
extern int func_0022ED80(int, int, int);
extern float func_001F9D48(void *, void *);
extern int func_L09_0030B648(void *);
extern void func_L00_0028EBF0(int);
extern void func_001F9BD8(void *, void *, void *);
extern void func_L00_002617B0(char *, void *, void *, void *);

/* Gaspar shuttle moby update: steers along its path by state, with its sound and camera hooks. */
void func_L09_0030AFB8(char *m) {
    char *d = *(char **)(m + 0x78);
    char *p18 = m + 0x10;
    char *g;
    unsigned char buf[32] __attribute__((aligned(16)));
    char *sp0 = (char *)buf;
    char *sp10 = (char *)buf + 0x10;
    char *e19;
    char **tab;
    char *p6;
    char *p22;
    char *ptr3;
    char *ep;
    int st;
    int r;
    int c5;
    int h;
    int i;
    float f0;
    float f1;
    float f20;
    unsigned char bt;
    unsigned char b0;
    unsigned char *q;

    tab = D_L09_001B0930;
    e19 = tab[*(int *)(d + 0xAC)];
    func_001F9C30(sp0, p18, -1.0f);
    qcopy(sp10, m + 0x40);
    if (d == 0) goto L5B8;
    p22 = sp10;
    st = *(unsigned char *)(m + 0x20);
    if (st == 0) goto L070;
    if (st == 1) goto L234;
    if (st == 2) goto L170;
    if (st == 3) goto L380;
    goto L_end;

L070:
    *(unsigned char *)(m + 0x30) = 0xFF;
    *(float *)(d + 0xBC) = 3.1415927f;
    *(int *)(d + 0xC4) = -1;
    *(int *)(d + 0xC8) = 0;
    *(int *)(d + 0xCC) = 0;
    if (*(int *)(d + 0xA0) == 0) goto L150;
    func_0020D960(m, 0, (unsigned char *)(d + 0x60));
    p6 = tab[*(int *)(d + 0xAC)];
    q = D_0014171B + 0xAA35;
    b0 = *(unsigned char *)(m + 0xB0);
    *(unsigned char *)(m + 0x20) = 1;
    bt = *(unsigned char *)(q + (b0 + (D_0015EE84 << 4)));
    if (bt != 0xFF) *(short *)(d + 0xBA) = 1;
    if (bt != 0xFF) {
        ptr3 = p6 + 0x10;
        *(short *)(d + 0xB8) = 1;
    } else {
        *(short *)(d + 0xB8) = -1;
        *(short *)(d + 0xBA) = *(unsigned short *)p6 - 2;
        ptr3 = p6 + (*(int *)p6 << 4);
    }
    qcopy(p18, ptr3);
    h = *(short *)(d + 0xBA);
    ep = p6 + (h << 4);
    *(float *)(m + 0x48) = func_L00_001FF860(*(float *)(ep + 0x10) - *(float *)(m + 0x10),
                                             *(float *)(ep + 0x14) - *(float *)(m + 0x14));
L150:
    *(float *)(m + 0x48) = func_001FA748(*(float *)(m + 0x48), 3.1415927f);
    goto L_end;

L170:
    f1 = *(float *)(d + 0xCC);
    f0 = 1.0f - f1;
    f0 = f0 * 0.01f;
    f20 = D_0015EE6C * 0.5235988f;
    f1 = f1 + f0;
    *(float *)(d + 0xCC) = f1;
    func_L00_00263B78(f1 * 0.14f, D_0015EE6C * 0.5235988f, (char *)m, (float *)(d + 0xD8), (float *)(d + 0xDC));
    f1 = D_0015EE6C;
    f0 = *(float *)(d + 0xCC);
    func_L00_00263BF8(m, d + 0xD0, d + 0xD4, f0 * 0.08726646f, f1 * 0.31415927f, f1 * 0.4886922f);
    if (*(int *)(D_0013E633 + 0x1119) != (int)m) *(unsigned char *)(m + 0x20) = 1;
    goto L_end;

L234:
    f1 = *(float *)(d + 0xCC);
    f0 = 1.0f - f1;
    f0 = f0 * 0.01f;
    f20 = D_0015EE6C * 0.5235988f;
    f1 = f1 + f0;
    *(float *)(d + 0xCC) = f1;
    func_L00_00263B78(f1 * 0.14f, D_0015EE6C * 0.5235988f, (char *)m, (float *)(d + 0xD8), (float *)(d + 0xDC));
    f1 = D_0015EE6C;
    f0 = *(float *)(d + 0xCC);
    func_L00_00263BF8(m, d + 0xD0, d + 0xD4, f0 * 0.08726646f, f1 * 0.31415927f, f1 * 0.4886922f);
    if (*(int *)(d + 0xC8) == 0) goto L350;
    h = *(short *)(d + 0xB8);
    if (h <= 0) goto L318;
    c5 = *(int *)(d + 0xB4);
    if (c5 == -1) goto L31C;
    r = func_00215570(D_0013E633 + 0xE9D, c5);
    if (r != 0) {
        *(unsigned char *)(m + 0x20) = 3;
        goto L350;
    }
L318:
    h = *(short *)(d + 0xB8);
L31C:
    if (h >= 0) goto L350;
    c5 = *(int *)(d + 0xB0);
    if (c5 == -1) goto L354;
    r = func_00215570(D_0013E633 + 0xE9D, c5);
    if (r == 0) goto L350;
    *(unsigned char *)(m + 0x20) = 3;
L350:
L354:
    g = D_0013E633 + 0xE1D;
    if (*(int *)(g + 0x2FC) != (int)m) goto L_end;
    h = *(short *)(g + 0x30E);
    if (h != 0) goto L_end;
    *(int *)(d + 0xC8) = 1;
    *(unsigned char *)(m + 0x20) = 3;
    goto L_end;

L380:
    if (((D_L09_0015F6B0 & 7)) != (((int)m >> 8) & 7)) goto L3E0;
    c5 = *(int *)(d + 0xC4);
    if (c5 == -1) goto L3CC;
    r = func_L00_0028EB98(m, c5);
    if (r != 0) goto L3E0;
    if (*(int *)(d + 0xC4) != -1) goto L3E0;
L3CC:
    r = func_0022ED80(0, 4, (int)m);
    *(int *)(d + 0xC4) = r;
L3E0:
    f1 = *(float *)(d + 0xBC);
    if (f1 != 0.0f) goto L4F8;
    h = *(short *)(d + 0xB8);
    if (h >= 0) goto L47C;
    c5 = *(int *)(d + 0xB4);
    if (c5 == -1) goto L478;
    r = func_00215570(D_0013E633 + 0xE9D, c5);
    if (r == 0) goto L478;
    f20 = 8.0f;
    f0 = func_001F9D48(p18, e19 + (*(int *)e19 << 4));
    if (!(f20 < f0)) goto L4F8;
    f0 = func_001F9D48(p18, e19 + 0x10);
    if (!(f20 < f0)) goto L4F8;
    *(short *)(d + 0xB8) = 1;
    *(float *)(d + 0xBC) = 3.1415927f;
    goto L4F8;
L478:
    h = *(short *)(d + 0xB8);
L47C:
    if (h <= 0) goto L4F8;
    c5 = *(int *)(d + 0xB0);
    if (c5 == -1) goto L4F8;
    r = func_00215570(D_0013E633 + 0xE9D, c5);
    if (r == 0) goto L4F8;
    f20 = 8.0f;
    f0 = func_001F9D48(p18, e19 + (*(int *)e19 << 4));
    if (!(f20 < f0)) goto L4F8;
    f0 = func_001F9D48(p18, e19 + 0x10);
    if (!(f20 < f0)) goto L4F8;
    *(short *)(d + 0xB8) = -1;
    *(float *)(d + 0xBC) = 3.1415927f;
L4F8:
    r = func_L09_0030B648(m);
    if (r == 0) goto L594;
    c5 = *(int *)(d + 0xC4);
    if (c5 == -1) goto L548;
    ep = D_0013E633 + 0x1D + c5 * 0x70;
    if (*(int *)(ep + 0x88) != (int)m) goto L548;
    if (*(unsigned char *)(ep + 0x74) == 0) goto L548;
    func_L00_0028EBF0(c5);
L548:
    *(int *)(d + 0xC4) = -1;
    *(int *)(d + 0xCC) = 0;
    *(int *)(d + 0xDC) = 0;
    *(unsigned char *)(m + 0x20) = 2;
    *(float *)(d + 0xBC) = 3.1415927f;
    h = -*(short *)(d + 0xB8);
    *(short *)(d + 0xB8) = h;
    if ((h << 16) <= 0) {
        *(short *)(d + 0xBA) = *(unsigned short *)e19 - 1;
        goto L594;
    }
    *(short *)(d + 0xBA) = 0;
    goto L594;

L594:
L_end:
    func_001F9BD8(sp0, sp0, p18);
    func_L00_002617B0(d + 0x20, sp0, p22, m + 0x40);
L5B8:
    return;
}
