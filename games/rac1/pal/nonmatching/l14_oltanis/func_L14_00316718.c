/* NON_MATCHING func_L14_00316718 -- src/overlays/l14_oltanis/vendor_002FF358.c
 * Best so far: SIZE ours 1680 / retail 1676, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Stopped: p0, p2, p3 and p5 compile to 1680 bytes against retail's 1676 with the same instruction diff (the dif
 */
extern char *D_L14_0015F050 MACRO_ADDR;
extern char *D_L14_001B0F30[];
extern int D_L14_0015F504;
extern char D_L14_001677D0[];
extern char *D_L14_00160098 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern char D_0013E633[];
extern char D_L14_0016D240[];
extern float D_L14_0016D2F0;
extern float D_L14_001676D8;
extern void func_001F9EC0(void *, void *, void *);
extern void func_001F9BD8(void *, void *, void *);
extern void func_001F9BF0(void *dst, void *a, void *b);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9CA0(void *, void *, void *);
extern int func_L00_0025E860(void *, void *, int *, float *, float, int);
extern int func_001F9938(void *);
extern float func_001F9FA8(float);
extern float func_001F9F90(float x);
extern void func_001F3140(void);
extern void func_L14_002D7880(void *);
extern float func_00214220(float, float, float);
extern float func_001F9D10(void *, void *);
extern void func_001F9C30(void *, void *, float);

/* Camera 20 update: eases the camera toward its target, with the interval search over the d+0x58 table. */
void func_L14_00316718(char *c) {
    char *d;
    char *cur;
    char *tbl;
    char *p21;
    char *p18;
    char *pad;
    char *arr;
    char *d2;
    char *q;
    int t20, t24;
    int r16;
    int i, i1, v, si, n;
    int *p58;
    float vec0[4], vec10[4], vec20[4];
    int i40, i48;
    float f44, f4c;
    float fk, f20, f21, f14, f1, f2, ff;
    char *fp;
    char *s50, *s54, *tab;
    float vec30[4];
    float *fv;

    arr = D_L14_0015F050;
    d = *(char **)(c + 0x70);
    cur = *(char **)(arr + (*(short *)(c + 0x84) << 5) + 0x1C);
    if (*(short *)(d + 0x54) != 0) goto L_B;

    /* path A: the first copy of the table entry, then the target fork */
    t20 = *(int *)(cur + 0x20);
    t24 = *(int *)(cur + 0x24);
    tbl = (char *)D_L14_001B0F30;
    {
        char *src = ((char **)D_L14_001B0F30)[t20];
        char *dst3 = ((char **)D_L14_001B0F30)[t24];
        char *p20 = c + 0x30;
        D_L14_0015F504 = 1;
        src += 0x10;
        *(short *)(d + 0x54) = 1;
        qcopy(p20, src);
        if (*(int *)(cur + 0x28) < 0) goto L_840;
        fk = *(float *)(cur + 0x2C);
        d2 = D_L14_00160098 + (*(int *)(cur + 0x28) << 8);
        vec10[0] = fk;
        vec10[1] = *(float *)(cur + 0x30);
        vec10[2] = *(float *)(cur + 0x34);
        vec10[3] = 0.0f;
        func_001F9EC0(vec10, vec10, D_L14_001677D0);
        func_001F9BD8(vec20, d2 + 0x10, vec10);
        func_001F9BF0(vec0, vec20, p20);
        goto L_854;
    L_840:
        func_001F9BF0(vec0, dst3 + 0x10, c + 0x30);
    }
L_854:
    func_001F9C30(vec10, D_0013E633 + 0x10AD, -1.0f);
    func_L00_001FF4B0(c, vec0, 1.0f);
    func_001F9CA0(c + 0x10, c, vec10);
    func_L00_001FF4B0(c + 0x10, c + 0x10, 1.0f);
    func_001F9CA0(c + 0x20, c + 0x10, c);
    goto L_END;

L_B:
    /* path B: a blend of the two entries */
    t20 = *(int *)(cur + 0x20);
    p21 = ((char **)D_L14_001B0F30)[t20];
    qcopy(vec0, c + 0x30);
    p58 = (int *)(c + 0x30);
    r16 = func_L00_0025E860(p21, c + 0x30, (int *)(d + 0x40), (float *)(d + 0x44), *(float *)(cur + 0x3C) * D_0015EE6C, 0);
    if (*(short *)(d + 0x56) != 0) {
        r16 = (func_001F9938(d + 0x56) != 0) ? 1 : r16;
    }
    r16 = (*(unsigned char *)(cur + 0x39) != 0) ? 1 : r16;
    if (r16 == 0) goto L_AA0;

    /* B1 */
    f21 = *(float *)(d + 0x88) * 0.5f;
    p21 = c + 0x10;
    fk = func_001F9FA8(f21);
    f20 = fk;
    f20 = f20 / func_001F9F90(f21);
    D_L14_0016D2F0 = f20;
    func_001F3140();
    D_L14_001676D8 = 0.25f;
    *(short *)(c + 0x7E) = 4;
    D_L14_0015F504 = 0;
    func_L14_002D7880(D_L14_00160098 + (*(int *)(cur + 0x50) << 8));
    pad = D_0013E633 + 0xE1D;
    p18 = *(char **)(pad + 0x560);
    f44 = *(float *)(pad + 0x568);
    i40 = *(int *)(pad + 0x564);
    func_L00_0025E860(p18, p58, &i40, &f44, 10.0f, 0);
    *(float *)(c + 0x38) = *(float *)(c + 0x38) + 2.0f;
    f44 = *(float *)(pad + 0x568);
    i40 = *(int *)(pad + 0x564);
    func_L00_0025E860(p18, vec20, &i40, &f44, 0.0f, 0);
    vec20[2] = vec20[2] + 1.5f;
    func_001F9BF0(vec10, vec20, p58);
    func_L00_001FF4B0(c, vec10, 1.0f);
    func_001F9CA0(p21, c, pad + 0x290);
    func_L00_001FF4B0(p21, p21, -1.0f);
    func_001F9CA0(c + 0x20, p21, c);
    goto L_END;

L_AA0:
    /* B2: interval search over the short pairs at d+0x58 */
    s50 = c + 0x10;
    s54 = c + 0x20;
    i = 0;
    if (*(short *)(d + 0x58) < 0) goto L_C28;
    if (*(short *)(d + 0x5A) < 0) goto L_C28;
    tab = D_L14_0016D240;
    p21 = ((char **)D_L14_001B0F30)[*(int *)(cur + 0x20)];
L_AD8:
    i1 = i + 1;
    v = *(int *)(d + 0x40);
    si = *(short *)(d + 0x58 + 2 * i);
    if (v < si) goto L_BF4;
    if (!(v < *(short *)(d + 0x58 + 2 * i1))) goto L_BF4;
    f14 = 0.0f;
    f2 = *(float *)(d + 0x44);
    if (si < v) {
        float *pp = (float *)(p21 + (si << 4) + 0x1C);
        n = v - si;
        do {
            f14 = f14 + *pp;
            pp = (float *)((char *)pp + 0x10);
        } while (--n != 0);
    }
    f1 = f14;
    f14 = f14 + f2;
    if (v < *(short *)(d + 0x58 + 2 * i1)) {
        float *pp = (float *)(p21 + (v << 4) + 0x1C);
        n = *(short *)(d + 0x58 + 2 * i1) - v;
        do {
            f1 = f1 + *pp;
            pp = (float *)((char *)pp + 0x10);
        } while (--n != 0);
    }
    f14 = f14 / f1;
    fv = (float *)(d + 0x68);
    fk = func_00214220(fv[i], fv[i1], f14);
    f20 = fk * 0.017453292f * 0.5f;
    fk = func_001F9FA8(f20);
    f21 = fk;
    fk = func_001F9F90(f20);
    f21 = f21 / fk;
    *(float *)(tab + 0xB0) = f21;
    func_001F3140();
L_BF4:
    i1 = i + 1;
    if (!(i1 < 7)) goto L_C28;
    i = i1;
    if (*(short *)(d + 0x58 + 2 * i1) < 0) goto L_C28;
    if (*(short *)(d + 0x58 + 2 * (i1 + 1)) >= 0) goto L_AD8;

L_C28:
    ff = func_001F9D10(vec0, p58);
    *(float *)(d + 0x50) = *(float *)(d + 0x50) + ff;
    if (*(int *)(cur + 0x28) < 0) goto L_CB8;
    fk = *(float *)(cur + 0x2C);
    d2 = D_L14_00160098 + (*(int *)(cur + 0x28) << 8);
    vec20[0] = fk;
    vec20[1] = *(float *)(cur + 0x30);
    vec20[2] = *(float *)(cur + 0x34);
    vec20[3] = 0.0f;
    func_001F9EC0(vec20, vec20, D_L14_001677D0);
    func_001F9BD8(vec30, d2 + 0x10, vec20);
    func_001F9BF0(vec10, vec30, p58);
    goto L_D10;
L_CB8:
    f21 = *(float *)(d + 0x50);
    fk = *(float *)(d + 0x48);
    f20 = *(float *)(d + 0x4C);
    fk = f21 / fk;
    tab = ((char **)D_L14_001B0F30)[*(int *)(cur + 0x24)];
    i48 = 0;
    f4c = 0.0f;
    func_L00_0025E860(tab, vec20, &i48, &f4c, fk * f20, 0);
    func_001F9BF0(vec10, vec20, p58);
L_D10:
    func_001F9C30(vec20, D_0013E633 + 0x10AD, -1.0f);
    func_L00_001FF4B0(c, vec10, 1.0f);
    func_001F9CA0(s50, c, vec20);
    func_L00_001FF4B0(s50, s50, 1.0f);
    func_001F9CA0(s54, s50, c);
    goto L_END;
L_END:
    ;
}
