/* NON_MATCHING func_L05_0031D450 -- src/overlays/l05_rilgar/vendor_0030EB68.c
 * Best so far: SIZE ours 2412 / retail 2460, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   - func_L05_0031D450 (level 05 moby class 1347 update): timers, table-slot claims (D_0015EE84 bit sets), hint a
 *   - Stopped after 9 runs (p0-p8): tried locals at top, inline globals, per-block pointers, bit temporary, top ho
 */
extern char D_0013E633[];
extern char D_0014171B[];
extern char D_0013D50F[];
extern unsigned char D_0013D355[];
extern int func_00215570(void *, int);
extern int func_001F9850(int);
extern int func_L00_00203F20(int a, int b);
extern void func_001F9BC0(void *);
extern void func_L00_002D6CE0(int);
extern int D_0015EE84 MACRO_ADDR;
extern int D_L05_0015F6B0 MACRO_ADDR;
extern int D_0015EFA4 MACRO_ADDR;
extern short D_L05_00160098;

// Level 05 moby update, class 1347: timers, table slot claims and the hint and fail sounds.
void func_L05_0031D450(char *m) {
    char *dat = *(char **)(m + 0x78);
    char *p;
    char *e;
    int lim;
    int q;
    int n;
    int v7;
    unsigned short v;

    *(unsigned char *)(m + 0x30) = 0xFF;
    if (func_00215570(D_0013E633 + 0xE9D, *(int *)(dat + 0x50))) {
        if (*(int *)(D_0013E633 + 0xE9D + 0x200C) == 0x12) {
            n = *(int *)(dat + 0x68);
            *(int *)(dat + 0x68) = n + 1;
            if (func_001F9850(0x168) < n) {
                char *b = D_0014171B + 0x34D;
                { int bit = 1 << D_0015EE84;
                if (!(*(int *)(b + 0x224) & bit)) func_L00_00203F20(0x3F0, 0x44); }
                goto L54C;
            }
        }
        {
            char *b = D_0014171B + 0x34D;
            if (*(int *)(D_0013E633 + 0x2EA9) == 0x11 || D_0013D355[0x148] != 0) *(unsigned short *)(b + 0x220) = 0xFFFF;
        }
    }
L54C:
    {
        char *c = D_0014171B + 0x22D;
        char *b = D_0014171B + 0x34D;
        if (func_00215570(D_0013E633 + 0xE9D, *(int *)(dat + 0x40))) {
            lim = func_001F9850(D_0015EFA4) - *(unsigned short *)(c + 0x4A) * 600;
            if ((int)((float)func_001F9850(0xE10) * 60.0f) < lim) {
                { int bit = 1 << D_0015EE84;
                if (!(*(int *)(b + 0x244) & bit)) func_L00_00203F20(0x7DD, 0x48); }
            }
        }
    }
    lim = func_001F9850(0x3C);
    if (D_L05_0015F6B0 < lim) {
        char *c = D_0014171B + 0x22D;
        char *b = D_0014171B + 0x34D;
        if (*(unsigned short *)(c + 0x60) != 0 && *(unsigned short *)(c + 0x58) == 0 && *(int *)(b + 0x28C) >= 0)
            func_L00_00203F20(0x4E27, 0x51);
    }
    lim = func_001F9850(0x3C);
    if (D_L05_0015F6B0 < lim) {
        char *c = D_0014171B + 0x22D;
        char *b = D_0014171B + 0x34D;
        if (*(unsigned short *)(c + 0xE8) != 0) {
            if (*(int *)(b + 0x394) >= 0) func_L00_00203F20(0x4E28, 0x72);
            *(unsigned short *)(c + 0xE8) = 0;
        }
    }
    {
        char *d = D_0013D50F + 0xB9;
        char *g = D_0013E633 + 0xE9D;
        if (*(unsigned char *)(d + 0x16) == 0) {
            if (func_00215570(g, *(int *)(dat + 0x24)) && *(int *)(g + 0x27C) == 0) {
                char *b = D_0014171B + 0x34D;
                lim = func_001F9850(D_0015EFA4) - *(unsigned short *)(b + 0x11A) * 600;
                if ((int)((float)func_001F9850(0x12) * 60.0f) < lim || *(unsigned short *)(b + 0x11A) == 0) {
                    func_L00_00203F20(0x138A, 0x23);
                } else {
                    q = func_001F9850(D_0015EFA4) / 600;
                    if (*(unsigned short *)(b + 0x11A) < q) *(unsigned short *)(b + 0x11A) = func_001F9850(D_0015EFA4) / 600;
                }
            }
        } else {
            if (func_00215570(g, *(int *)(dat + 0x24)) && *(int *)(g + 0x27C) == 0) {
                char *b = D_0014171B + 0x34D;
                if (*(unsigned short *)(b + 0x120) != 0) {
                    lim = func_001F9850(D_0015EFA4) - *(unsigned short *)(b + 0x122) * 600;
                    if ((int)((float)func_001F9850(0x12) * 60.0f) < lim || *(unsigned short *)(b + 0x122) == 0) {
                        func_L00_00203F20(0x138B, 0x24);
                    } else {
                        q = func_001F9850(D_0015EFA4) / 600;
                        if (*(unsigned short *)(b + 0x122) < q) *(unsigned short *)(b + 0x122) = func_001F9850(D_0015EFA4) / 600;
                    }
                } else {
                    *(unsigned short *)(b + 0x120) = *(unsigned short *)(b + 0x120) + 1;
                    q = func_001F9850(D_0015EFA4) / 600;
                    if (*(unsigned short *)(b + 0x122) < q) *(unsigned short *)(b + 0x122) = func_001F9850(D_0015EFA4) / 600;
                    *(int *)(b + 0x124) |= (1 << D_0015EE84) | 0x80000000;
                }
            }
            if (*(int *)(D_0013E633 + 0x2EA9) == 0x13) *(unsigned short *)(D_0014171B + 0x34D + 0x120) = 0xFFFF;
        }
    }
    {
        char *b = D_0014171B + 0x34D;
        if (*(unsigned short *)(b + 0x130) < 2) {
            if (*(unsigned char *)(D_0014171B + 0xAA35 + *(int *)(dat + 0x20) + (D_0015EE84 << 4)) == 0xFF) {
                v = *(unsigned short *)(b + 0x128);
                if (v <= 0xFFFE) *(unsigned short *)(b + 0x128) = v + 1;
                q = func_001F9850(D_0015EFA4) / 600;
                if (*(unsigned short *)(b + 0x12A) < q) *(unsigned short *)(b + 0x12A) = func_001F9850(D_0015EFA4) / 600;
                *(int *)(b + 0x12C) |= (1 << D_0015EE84) | 0x80000000;
            } else {
                if (*(int *)(D_0014171B + 0xD875 + *(int *)(dat + 0x20) * 4) >= 3) {
                    if (func_00215570(D_0013E633 + 0xE9D, *(int *)(dat + 0x2C))) {
                        e = D_0014171B + 0x105;
                        lim = func_001F9850(D_0015EFA4) - *(unsigned short *)(e + 0x82) * 600;
                        if (func_001F9850(0x4650) < lim) {
                            lim = func_001F9850(D_0015EFA4) - *(unsigned short *)(e + 0x7A) * 600;
                            if (func_001F9850(0x4650) < lim) {
                                lim = func_001F9850(D_0015EFA4) - *(unsigned short *)(e + 0xA2) * 600;
                                if (func_001F9850(0x4650) < lim) {
                                    lim = func_001F9850(D_0015EFA4) - *(unsigned short *)(e + 0x52) * 600;
                                    if (func_001F9850(0x4650) < lim) {
                                        lim = func_001F9850(D_0015EFA4) - *(unsigned short *)(e + 0x8A) * 600;
                                        if (func_001F9850(0x4650) < lim) {
                                            if (*(int *)(b + 0x134) >= 0) func_L00_00203F20(0x138D, 0x26);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if (*(unsigned char *)(D_0013D355 + 0x13B) != 0) {
        if (*(unsigned short *)(D_0014171B + 0x34D + 0x110) == 0 && *(int *)(D_0013E633 + 0x2EA9) == 0x16)
            func_L00_00203F20(0x1389, 0x22);
    }
    {
        char *b = D_0014171B + 0x34D;
        if (*(unsigned short *)(b + 0x2F0) == 0) {
            char *h = D_0013E633 + 0xE1D;
            if (*(int *)(h + 0x2084) == 10) {
                if (func_00215570(h + 0x80, *(int *)(dat + 0x34)) || func_00215570(h + 0x80, *(int *)(dat + 0x38))) {
                    v = *(unsigned short *)(b + 0x2F0);
                    if (v <= 0xFFFE) *(unsigned short *)(b + 0x2F0) = v + 1;
                    q = func_001F9850(D_0015EFA4) / 600;
                    if (*(unsigned short *)(b + 0x2F2) < q) *(unsigned short *)(b + 0x2F2) = func_001F9850(D_0015EFA4) / 600;
                    *(int *)(b + 0x2F4) |= (1 << D_0015EE84) | 0x80000000;
                }
            }
            if (*(int *)(h + 0x208C) == 0x12) {
                p = dat + 0x10;
                if (func_00215570(p, *(int *)(dat + 0x34)) || func_00215570(p, *(int *)(dat + 0x38))) {
                    func_001F9BC0(p);
                    *(int *)(dat + 0x60) = *(int *)(dat + 0x60) + 1;
                }
            } else if (*(int *)(dat + 0x60) >= 2) {
                if (func_00215570(h + 0x80, *(int *)(dat + 0x34)) || func_00215570(h + 0x80, *(int *)(dat + 0x38)))
                    func_L00_00203F20(0x138E, 0x5E);
            }
        }
    }
    {
        char *c = D_0014171B + 0x22D;
        char *b = D_0014171B + 0x34D;
        char *d = D_0013D50F + 0xB9;
        if (*(unsigned short *)(c + 0x90) == 0 && *(unsigned short *)(b + 0x250) == 0 && *(unsigned char *)(d + 0xF) != 0) {
            if (func_00215570(D_0013E633 + 0xE9D, *(int *)(dat + 0x48))) {
                if (*(int *)(D_0013D50F + 0x5D) >= 20) func_L00_00203F20(0x4E20, 0x4A);
            }
        }
    }
    v7 = *(int *)(dat + 0x54);
    if (v7 != -1) {
        if (*(unsigned char *)(D_0014171B + 0xAA35 + *(int *)(dat + 0x58) + (D_0015EE84 << 4)) == 0xFF &&
            *(unsigned char *)(D_0014171B + 0xAA35 + *(int *)(dat + 0x20) + (D_0015EE84 << 4)) == 0xFF) {
            if (D_L05_0015F6B0 < 5 ||
                ((p = *(char **)(D_0013E633 + 0x1119)) != 0 && (*(short *)(p + 0xA6) == 0x3E6 || *(short *)(p + 0xA6) == 0x33E))) {
                func_L00_002D6CE0(*(int *)&D_L05_00160098 + (v7 << 8));
                *(int *)(dat + 0x54) = -1;
            }
        }
    }
    qcopy(dat, D_0013E633 + 0xE9D);
    if (*(short *)(D_0013E633 + 0xE9D + 0x28E) == 0) qcopy(dat + 0x10, D_0013E633 + 0xE9D);
}
