/*
 * EnterSpaceLoadingLoop: sets up the flight's loader state (the blocks at
 * D_001941C0 and D_0013E130, the GS packet in D_0019E7C0), runs the space
 * loading display, then waits on D_0015EE5C for the disc and returns.
 */
typedef struct { long q[2]; } Fill1Quad16;

extern char D_00137C80_f1[] __asm__("D_00137C80");
extern char D_0013DE4B_f1[] __asm__("D_0013DE4B");
extern char D_0013E130_f1[] __asm__("D_0013E130");
extern char D_001414D0_f1[] __asm__("D_001414D0");
extern char D_0015EE5C_f1[] __asm__("D_0015EE5C");
extern char D_0015EE84_f1[] __asm__("D_0015EE84");
extern char D_0015EE88_f1[] __asm__("D_0015EE88");
extern char D_0015EF74_f1[] __asm__("D_0015EF74");
extern char D_0015EF78_f1[] __asm__("D_0015EF78");
extern char D_0015EF8C_f1[] __asm__("D_0015EF8C");
extern char D_0015F53C_f1[] __asm__("D_0015F53C");
extern char D_0015F540_f1[] __asm__("D_0015F540");
extern char D_0015F560_f1[] __asm__("D_0015F560");
extern char D_0015F6FC_f1[] __asm__("D_0015F6FC");
extern char D_00160000_f1[] __asm__("D_00160000");
extern char D_00160008_f1[] __asm__("D_00160008");
extern char D_00160018_f1[] __asm__("D_00160018");
extern char D_0016001C_f1[] __asm__("D_0016001C");
extern char D_00160020_f1[] __asm__("D_00160020");
extern char D_00160028_f1[] __asm__("D_00160028");
extern char D_00160030_f1[] __asm__("D_00160030");
extern char D_001601AC_f1[] __asm__("D_001601AC");
extern char D_001601B0_f1[] __asm__("D_001601B0");
extern char D_001601B4_f1[] __asm__("D_001601B4");
extern char D_00160680_f1[] __asm__("D_00160680");
extern char D_0016100C_f1[] __asm__("D_0016100C");
extern char D_00186410_f1[] __asm__("D_00186410");
extern char D_00186450_f1[] __asm__("D_00186450");
extern char D_0018CC20_f1[] __asm__("D_0018CC20");
extern char D_001941C0_f1[] __asm__("D_001941C0");
extern char D_00194280_f1[] __asm__("D_00194280");
extern char D_0019BEC0_f1[] __asm__("D_0019BEC0");
extern char D_0019C2C0_f1[] __asm__("D_0019C2C0");
extern char D_0019C4C0_f1[] __asm__("D_0019C4C0");
extern char D_0019E7C0_f1[] __asm__("D_0019E7C0");
extern char D_001B3E40_f1[] __asm__("D_001B3E40");
extern char D_001B5D00_f1[] __asm__("D_001B5D00");
extern char D_001B6500_f1[] __asm__("D_001B6500");
extern char D_001B6C00_f1[] __asm__("D_001B6C00");
extern char D_001CAE40_f1[] __asm__("D_001CAE40");
extern char D_001CDB00_f1[] __asm__("D_001CDB00");
extern char D_001D9AD0_f1[] __asm__("D_001D9AD0");
extern int func_001160D8_f1 (void) __asm__("func_001160D8");
extern void func_00118D80_f1 (int) __asm__("func_00118D80");
extern int func_00120F30_f1 (int) __asm__("func_00120F30");
extern void func_00122598_f1 (void) __asm__("func_00122598");
extern void func_0012DDC0_f1 (void) __asm__("func_0012DDC0");
extern void func_0012E1C8_f1 (int, int, long) __asm__("func_0012E1C8");
extern void func_0012E2E8_f1 (void) __asm__("func_0012E2E8");
extern void func_001F3008_f1 (void) __asm__("func_001F3008");
extern void func_001F3140_f1 (void) __asm__("func_001F3140");
extern void func_001F3B90_f1 (void) __asm__("func_001F3B90");
extern void func_001F4E08_f1 (int) __asm__("func_001F4E08");
extern int func_001F98C0_f1 (int) __asm__("func_001F98C0");
extern int func_001F9968_f1 (int) __asm__("func_001F9968");
extern void func_001F99B0_f1 (void *, int, int) __asm__("func_001F99B0");
extern void func_001F99D8_f1 (void *, int) __asm__("func_001F99D8");
extern void func_001F9A98_f1 (void *, void *, int) __asm__("func_001F9A98");
extern void func_00201E10_f1 (void) __asm__("func_00201E10");
extern void func_00202AA8_f1 (int, void *) __asm__("func_00202AA8");
extern void func_00203958_f1 (int, int, void *) __asm__("func_00203958");
extern void func_00202F00_f1 (void *, int, void *, int) __asm__("func_00202F00");
extern void func_00203038_f1 (void *, int) __asm__("func_00203038");
extern void func_00203118_f1 (void *) __asm__("func_00203118");
extern void func_00203E78_f1 (void *, void *, void *, int) __asm__("func_00203E78");
extern void func_00205220_f1 (int) __asm__("func_00205220");
extern int func_0020C468_f1 (void *, void *) __asm__("func_0020C468");
extern int func_002175C8_f1 (int, int, int) __asm__("func_002175C8");
extern void func_0022F090_f1 (int, long) __asm__("func_0022F090");
extern void func_002348E8_f1 (void) __asm__("func_002348E8");

void func_00232278(void)
{
    char *P;     /* $16: the loader block, D_001941C0_f1 */
    char *S;     /* $20: the chunk the loader block points at (+0x14) */
    char *R21;   /* $21: S + S[+4] */
    char *R22;   /* $22: R21 + S[+0x30], then S + 0x50 */
    char *R18;   /* $18: S + S[+0x1C] */
    char *R16;
    char *R17;
    char *Y;
    char *Z;
    char *dst;
    char *p5;
    char *X;
    char *CC;
    int r, r23, n3, n10, k, w, lim, t, tmp_i, f58;
    long tmp, pk;
    int *outp;
    char *e;
    int ctr;
    int a4;
    unsigned short hv;

    /* Start: the four timer writes and the first calls. */
    *(short *)(D_0013E130_f1 + 0x24) = -1;
    *(int *)(D_0013E130_f1 + 0x20) = 4;
    *(float *)D_0015F53C_f1 = 1.0f;
    *(int *)D_001414D0_f1 = 0;
    *(int *)D_0016100C_f1 = 0x100000;
    *(int *)D_0015F6FC_f1 = 0;
    *(int *)D_0015F540_f1 = 0;
    func_00201E10_f1();

    {
        int v = *(int *)D_0015EF8C_f1;
        *(int *)D_0015EF78_f1 = v;
        *(int *)D_0015EF74_f1 = v;
    }
    func_001F99B0_f1(D_00194280_f1, 0x87654321, 0x10);
    func_001F99B0_f1(D_001B3E40_f1, -1, 0x800);
    func_001F99B0_f1(D_001B6C00_f1, -1, 0xE00);
    func_001F99B0_f1(D_001B6500_f1, 0, 0xE0);
    func_001F3008_f1();
    func_001F3140_f1();
    func_002348E8_f1();

    P = D_001941C0_f1;

    r = func_002175C8_f1(*(int *)(P + 0x14) + 0x400000,
                      *(int *)(D_00137C80_f1 + 0x13B8),
                      *(int *)(D_00137C80_f1 + 0x13BC));
    func_001F4E08_f1(func_001F98C0_f1(0xC));
    func_00120F30_f1(0);
    func_00118D80_f1(0);
    r23 = func_0020C468_f1((void *)(*(int *)(P + 0x14) + 0x400000),
                        (void *)*(int *)(P + 0x14));
    func_00118D80_f1(0);

    S = *(char **)(P + 0x14);
    func_00203958_f1(*(int *)S + (int)S, *(int *)(S + 0x8), S + *(int *)(S + 0xC));
    R21 = S + *(int *)(S + 0x4);
    R22 = R21 + *(int *)(S + 0x30);
    R18 = S + *(int *)(S + 0x1C);
    n3 = *(int *)(S + 0x18);

    /* The GS packet: a PACKED-mode tag and two registers, into D_0019E7C0_f1. */
    {
        int v40 = *(int *)D_0015EF8C_f1 + *(int *)(S + 0x40);
        int v44 = *(int *)D_0015EF8C_f1 + *(int *)(S + 0x44);
        long a = (long)(v44 >> 8) << 37;
        long b = (long)(v40 >> 8);
        pk = b | 0x1D308000L | a | ((long)0xB800 << 19) | (-1L << 63);
    }
    *(long *)D_0019E7C0_f1 = pk;
    *(long *)(D_0019E7C0_f1 + 0x10) = (((0x8000L << 20) | 0x8000L) << 19) | 0x4000L;
    *(long *)(D_0019E7C0_f1 + 0x8) = ((long)0xFFA0 << 32) | 0xE0L;
    *(int *)D_00160008_f1 = n3;

    ctr = 0;
    if (n3 > 0) {
        outp = (int *)D_001B5D00_f1;
        e = R18;
        do {
            hv = *(unsigned short *)(e + 4);
            ctr++;
            r = func_001F9968_f1((int)hv);
            w = *(int *)e;
            t = r << 28;
            e += 0x10;
            *outp = (int)R22 + w + t;
            outp++;
        } while (ctr < *(int *)D_00160008_f1);
    }
    func_001F9A98_f1(D_001CAE40_f1, R18, *(int *)D_00160008_f1 << 4);

    R22 = S + 0x50;
    func_00203118_f1(R21 + *(int *)(S + 0x48));

    R16 = S + *(int *)(S + 0x14);
    n10 = *(int *)(S + 0x10);
    *(int *)D_00160000_f1 = 0;
    k = 0;
    if (n10 > 0) {
        do {
            w = *(int *)R16;
            dst = R16 + 0x10;
            k++;
            if (w == 0) {
                a4 = 0;
            } else {
                a4 = (int)(R21 + w);
            }
            func_00203E78_f1((void *)a4, S + *(int *)(S + 0x1C), dst, *(int *)(R16 + 4));
            R16 += 0x20;
        } while (k < *(int *)(S + 0x10));
    }

    *(int *)D_0015F560_f1 = (int)(R21 + *(int *)(S + 0x38));
    func_00203038_f1(S + *(int *)(S + 0x2C), *(int *)(S + 0x28));
    R18 = D_0019BEC0_f1;
    func_00202F00_f1(S + *(int *)(S + 0x3C), (int)(R21 + *(int *)(S + 0x34)),
                  S + *(int *)(S + 0x24), *(int *)(S + 0x20));

    R17 = R21 + *(int *)(S + 0x4C);
    {
        int e84 = *(int *)D_0015EE84_f1;
        lim = *(int *)D_0015EE88_f1 - 1;
        if (!(-1 < lim)) {
            lim = 0;
        }
        {
            long *tp = &tmp;
            func_00202AA8_f1((int)(R17 + *(int *)(R17 + 4 * e84 + 4)), (void *)tp);
        }
        t = lim * 19;
        t = t + e84;
        t = t << 2;
        X = R17 + t;
        *(long *)D_00160680_f1 = tmp;
        {
            long *tp = &tmp;
            func_00202AA8_f1((int)(R17 + *(int *)(X + 0x50)), (void *)tp);
        }
    }
    {
        char *Pb = P;
        int v2 = *(int *)D_0015EF74_f1 + 0x2000;
        Y = *(char **)(Pb + 0x14);
        *(int *)D_0015EF78_f1 = v2;
        Y = Y + r23;
        *(int *)(Pb + 0x18) = (int)Y;
        *(int *)D_0015EF74_f1 = v2;
        *(long *)0x00160688 = tmp;
    }
    func_001F99D8_f1(D_0019C2C0_f1, 0x100);
    func_001F99D8_f1(D_0019C4C0_f1, 0x180);
    func_001F99D8_f1(D_0019BEC0_f1, 0x400);
    func_001F9A98_f1(D_0019BEC0_f1, D_001D9AD0_f1, 0x40);

    *(int *)D_00160018_f1 = (int)Y;
    func_001F99B0_f1(Y, 0, 0x4000);
    Y += 0x4000;
    Z = *(char **)D_00160018_f1;
    *(int *)D_0016001C_f1 = (int)Z;
    *(unsigned char *)(Z + 0x20) = 0xFF;
    *(int *)D_00160028_f1 = (int)Y;
    Z = *(char **)D_00160018_f1;
    Y += 0x2000;
    *(int *)D_001601AC_f1 = (int)Y;
    *(int *)D_001601B4_f1 = -1;
    *(int *)D_00160020_f1 = (int)(Z + 0x3F00);
    *(int *)D_001601B0_f1 = 0;
    *(int *)0x001601B8 = 0;
    func_001F99D8_f1(D_001CDB00_f1, 0x200);

    func_001F3B90_f1();
    *(int *)D_00160030_f1 = 0x1F4;
    *(int *)0x001601BC = 0x1F4000;
    r = func_001160D8_f1();
    *(int *)(D_0013E130_f1 + 0x5C) = 0;
    *(int *)(D_0013E130_f1 + 0x58) = (r >> 16) & 3;
    *(int *)(D_0013E130_f1 + 0x50) = 0;
    *(int *)(D_0013E130_f1 + 0x54) = 0;
    *(Fill1Quad16 *)0x001605F0 = *(Fill1Quad16 *)0x001605E0;

    {
        int e84b = *(int *)D_0015EE84_f1;
        if (e84b == 0 || (e84b == 1 && ((unsigned char *)D_0013DE4B_f1)[0] == 0)) {
            *(int *)(D_0013E130_f1 + 0x5C) = 2;
            *(int *)(D_0013E130_f1 + 0x58) = 4;
        }
    }

    func_001F99B0_f1(D_0018CC20_f1, 0, 0x1C0);
    func_001F99B0_f1(D_00186410_f1, 0, 0x40);
    func_001F99B0_f1(D_00186450_f1, 0, 0x40);

    CC = D_0018CC20_f1;
    {
        int d = *(int *)D_0016100C_f1;
        int a5 = d + (int)0xFFFA0000;
        int P8 = *(int *)(P + 0x8);
        int P4 = *(int *)(P + 0x4);
        f58 = *(int *)(D_0013E130_f1 + 0x58);
        *(int *)(CC + 0x5C) = P8 + a5;
        *(int *)(CC + 0x58) = P4 + a5;
        *(int *)D_0016100C_f1 = a5;
        p5 = R21 + *(int *)(R22 + 4 * f58);
    }
    {
        char *pp = p5;
        k = 0;
        if (*(int *)(pp + 4) != 0) {
            int v3;
            w = *(int *)pp;
            dst = CC + 0x60;
            for (;;) {
                k++;
                v3 = *(int *)(R22 + 4 * f58);
                pp += 8;
                w += 0x800;
                *(int *)dst = (int)(R21 + v3) + w;
                dst += 4;
                if (!(k < 0x46)) {
                    break;
                }
                if (*(int *)(pp + 4) == 0) {
                    break;
                }
                w = *(int *)pp;
            }
        }
    }

    func_00205220_f1(0);
    *(int *)D_0015EE5C_f1 = -1;
    func_0012E1C8_f1((int)(R21 + *(int *)(S + 0x64)), (int)func_0022F090_f1,
                  (long)(unsigned int)D_0015EE5C_f1);
    do {
        func_00118D80_f1(0);
        func_00122598_f1();
        func_0012DDC0_f1();
    } while (*(int *)D_0015EE5C_f1 == -1);
    func_0012E2E8_f1();
}
