/* NON_MATCHING func_L00_002A8A20 -- src/overlays/shared/vendor_002A5138.c
 * Best so far: SIZE ours 1232 / retail 1260, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Shared per-tick colour-table update for a moby (1260 bytes): picks one of three 0x60-byte rows of D_L00_001D95
 *   Wall: the DMA packet words are built with 64-bit shifts and ors (dsll, dsll32, or, then sd at 0x70..0x88). SN 
 *   Unblock: a way to write 64-bit shift-and-or in C that gcc 2.95 accepts, or the matched form of the DMA builder
 */
extern char D_0013E633[];
extern char D_L00_001D95E8[];
extern int D_L00_0015F6B0 MACRO_ADDR;
extern int D_L00_00161488 MACRO_ADDR;
extern float D_L00_00161484 SDATA(D_L00_00161484);
extern int func_001F4868(int);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern void func_001F9C30(void *, void *, float);
extern void func_L00_001FD1D8(void *, void *, int);

// Per-tick update of a moby's packed colour table: fills four colour words from D_L00_001D95E8 and sends the DMA packet.
void func_L00_002A8A20(char *s) {
    char *o;
    char *pl;
    char *tbl;
    char *vec;
    char *ea;
    char *eb;
    int *p;
    float *fa;
    float *fb;
    float f0;
    float f1;
    float f2;
    float f3;
    float f4;
    float f12;
    float f20;
    float f21;
    float f22;
    int sel;
    int arg6;
    int i;
    int j;
    int k;
    int n;
    int d;
    int h;
    int word;
    int w0;
    int kind;
    int v;
    int idxv;
    int sv[4];
    unsigned int dma[8];
    char sp[0x40];

    o = *(char **)(s + 0x78);
    if (o == 0) {
        return;
    }
    f2 = *(float *)(o + 0x70);
    if (f2 == 0.0f) {
        return;
    }
    if (f2 == 1.0f) {
        pl = *(char **)(D_0013E633 + 0xE1D + 0x2080);
        if (*(unsigned char *)(pl + 0x52) != *(unsigned char *)(pl + 0x53)) {
            *(float *)(o + 0x70) = 0.0f;
            return;
        }
        if (*(unsigned char *)(pl + 0x52) == 0xFF) {
            *(float *)(o + 0x70) = 0.0f;
            return;
        }
    }
    pl = *(char **)(D_0013E633 + 0xE1D + 0x2080);
    f21 = 1.0f;
    f22 = f21;
    arg6 = 0x14;
    word = 0x808080;
    i = 1;
    kind = *(unsigned char *)(pl + 0x52);
    if ((unsigned int)(kind - 0x17) < 3 || kind == 0x2B) {
        v = kind;
    } else {
        v = *(short *)(o + 0x78);
    }
    if (v == 0x17) {
        *(short *)(o + 0x78) = v;
        sel = 0;
    } else if (v == 0x18) {
        *(short *)(o + 0x78) = v;
        sel = 1;
    } else if (v == 0x19) {
        *(short *)(o + 0x78) = v;
        sel = 2;
    } else {
        *(short *)(o + 0x78) = 0x2B;
        sel = 3;
        arg6 = 0xC;
        word = 0x408080;
        f22 = 0.7f;
        f21 = 1.25f;
        i = 2;
    }
    tbl = D_L00_001D95E8 + sel * 0x60;
    f2 = *(float *)(o + 0x70);
    sv[0] = *(short *)(tbl + 0);
    sv[1] = *(short *)(tbl + 2);
    sv[2] = *(short *)(tbl + 4);
    sv[3] = *(short *)(tbl + 6);
    if (f2 == 1.0f) {
        pl = *(char **)(D_0013E633 + 0xE1D + 0x2080);
        qcopy(o, pl + 0xC0);
        qcopy(o + 0x10, pl + 0xD0);
        qcopy(o + 0x20, pl + 0xE0);
        D_L00_00161488 = D_L00_0015F6B0;
    }
    d = D_L00_0015F6B0 - D_L00_00161488;
    n = (0 < d) ? d : 1;
    f1 = (float)n * *(float *)(tbl + 0x48);
    f1 = f2 + f1;
    *(float *)(o + 0x70) = f1;
    D_L00_00161488 = D_L00_0015F6B0;
    h = *(short *)(tbl + 0x58);
    f0 = (float)(h * 2);
    if (f0 <= f1) {
        pl = *(char **)(D_0013E633 + 0xE1D + 0x2080);
        kind = *(unsigned char *)(pl + 0x52);
        if ((unsigned int)(kind - 0x17) < 3) {
            return;
        }
        if (kind != 0x2B) {
            *(float *)(o + 0x70) = 0.0f;
        }
        return;
    }
    w0 = func_001F4868(arg6);
    pl = *(char **)(D_0013E633 + 0xE1D + 0x2080);
    dma[0] = 5;
    dma[1] = 0;
    dma[2] = w0;
    dma[3] = w0 >> 31;
    dma[4] = 0x260;
    dma[5] = 0x0000FF90;
    dma[6] = (i << 2) | 0x40;
    dma[7] = 0x80;
    qcopy(o + 0x30, pl + 0x10);
    if (h <= 0) {
        return;
    }
    i = 0;
    f20 = 0.5f;
    do {
        vec = sp;
        p = (int *)(sp + 0x40);
        fa = (float *)(sp + 0x50);
        fb = (float *)(sp + 0x54);
        j = 0;
        k = 3;
        do {
            ea = *(char **)(tbl + 0x50) + (i << 4) + j;
            idxv = *(short *)ea;
            if (i == sv[0]) {
                *p = *(int *)(tbl + j + 8);
            } else if (i == sv[1]) {
                *p = *(int *)(tbl + j + 0x18);
            } else if (i == sv[2]) {
                *p = *(int *)(tbl + j + 0x28);
            } else if (i == sv[3]) {
                *p = *(int *)(tbl + j + 0x38);
            } else {
                *p = 0x50808080;
            }
            k = k - 1;
            f12 = (float)(*(signed char *)((char *)p + 3)) * f22;
            w0 = func_001FA898_r(f12);
            *p = (w0 << 24) | word;
            func_001F9C30(vec, *(char **)(tbl + 0x4C) + (idxv << 4), f21);
            vec = vec + 0x10;
            p = p + 1;
            f4 = D_L00_00161484;
            f1 = *(float *)(o + 0x70) * *(float *)(tbl + 0x5C);
            f3 = 1.0f - f1;
            eb = *(char **)(tbl + 0x50) + (i << 4) + j;
            eb = *(char **)(tbl + 0x54) + (*(short *)(eb + 2) << 3);
            f0 = *(float *)eb;
            f0 = (f0 - f20) * f4 + f20;
            *fa = f0;
            fa = fa + 2;
            f0 = *(float *)(eb + 4);
            f0 = (f0 - f20) * f4 + f20 + f3;
            *fb = f0;
            fb = fb + 2;
            j = j + 4;
        } while (k >= 0);
        i = i + 1;
        func_L00_001FD1D8(sp, o, 0);
        h = *(short *)(tbl + 0x58);
    } while (i < h);
}
