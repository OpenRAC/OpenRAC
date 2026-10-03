/* NON_MATCHING func_L15_00216568 -- src/overlays/shared/help_001FFED0.c
 * Best so far: SIZE ours 1388 / retail 1360, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * Cannot land as written (#define in a candidate): rewrite that in plain C first.
 * What the last attempts found:
 *   Per-frame update of the help/cursor state block at D_0013E633+0xE1D (counters ++, 001F9908/9938 init calls, mo
 *   Stopped early: p1.c (one local `base = D_0013E633 + 0xE1D`) compiles to 1308 bytes vs retail 1360, a macro exp
 *   Would need to know the source form that yields a function-wide hi pseudo plus per-region lo adds; logic struct
 */
extern unsigned char D_0013E633[];
extern int D_0013A5E0[];
extern unsigned char D_0013D5E9[];
extern char D_L15_00179F70[];
extern float D_0015EE6C MACRO_ADDR;
extern void func_L00_00216D40(void);
extern void func_001F9908(int *arg0);
extern int func_001F9938(void *);
extern void func_L15_002092E0(void);
extern int func_001F9850(int);
extern int func_L00_00267BA8(int, int, int);
extern struct Moby *func_0020D348_m(int) __asm__("func_0020D348");
extern void func_L00_00251E30(void *);
extern void func_L00_002110C0(int, int, void *);
extern float func_L00_002342F8(float *v);
extern void func_001F9BC0(void *);
extern void func_001F9BF0(void *, void *, void *);
extern void func_001F9BD8(void *, void *, void *);
extern void func_001F9C30(void *, void *, float);
extern float func_001F9CB8(void *);
extern void func_L00_002222F0(void);
extern void func_L00_00233EE0(float *, float, float, float);
extern void func_L00_00211908(void);

#define B (D_0013E633 + 0xE1D)
#define P ((unsigned char *)D_0013A5E0 + 0x2460)
#define G (*(char **)(B + 0xA8C))

/* per-frame update of the help/cursor state block */
void func_L15_00216568(void) {
    char *m;
    char *p;
    char *end;
    int i;
    float vec[4];
    float tmp[4];
    if (B[0x20AD] == 0) {
        func_L00_00216D40();
    }
    *(int *)(B + 0x198) += 1;
    *(int *)(B + 0x19C) += 1;
    *(int *)(B + 0x1A0) += 1;
    *(int *)(B + 0x1A4) += 1;
    func_001F9908((int *)(B + 0x1C4));
    func_001F9908((int *)(B + 0x1C0));
    func_001F9938(B + 0x1B2);
    func_001F9908((int *)(B + 0x1B8));
    func_001F9938(B + 0x1D8);
    func_001F9938(B + 0x1DE);
    func_001F9908((int *)(B + 0x1B4));
    func_001F9938(B + 0x1F0);
    func_001F9938(B + 0x1B0);
    func_001F9938(B + 0x1C8);
    func_001F9938(B + 0x1CA);
    func_001F9938(B + 0x1E4);
    func_001F9938(B + 0x1E6);
    func_001F9908((int *)(B + 0x1CC));
    func_001F9908((int *)(B + 0x1D0));
    func_001F9908((int *)(B + 0x1D4));
    func_001F9938(B + 0x1E8);
    func_001F9938(B + 0x1E0);
    func_001F9938(B + 0x1EE);
    func_001F9938(B + 0x1EA);
    func_001F9938(B + 0x1F4);
    func_001F9938(B + 0x1F2);
    func_001F9938(B + 0x1DC);
    func_001F9938(B + 0x1F6);
    func_001F9938(B + 0x1DA);
    func_001F9938(B + 0x1F8);
    if ((*(int *)(P + 0x1A0) & 5) == 0) {
        *(short *)(B + 0x1EC) = 0;
    }
    if (B[0x20A8] != 0) {
        *(int *)(B + 0x1BC) += 1;
    } else {
        *(int *)(B + 0x1BC) = 0;
    }
    if (*(int *)(P + 0x1D4) != 0) {
        *(int *)(B + 0x1A8) = 0;
        *(int *)(B + 0x1AC) += 1;
    } else {
        *(int *)(B + 0x1AC) = 0;
        *(int *)(B + 0x1A8) += 1;
    }
    if (*(short *)(B + 0x22DE) != 0) {
        if (func_001F9938(B + 0x22DE) != 0) {
            if (*(int *)(B + 0x22A8) != 0) {
                if (*(int *)&D_L15_0015F6A8 == 0) {
                    if (B[0x20A4] == 3) {
                        func_L15_002092E0();
                        if (func_L00_00267BA8(0x80, func_001F9850(0x12), 0) != 0) {
                            B[0x20A6] = 1;
                        }
                    } else {
                        m = (char *)func_0020D348_m(0x27A);
                        G = m;
                        if (m != 0) {
                            qcopy(m + 0x10, B + 0x80);
                            qcopy(m + 0x40, B + 0x90);
                            *(short *)(m + 0x32) = 0x40;
                            G[0x31] = 1;
                            *(int *)(G + 0x90) = 0;
                            *(long *)(G + 0x38) = *(long *)(*(char **)(B + 0x2080) + 0x38);
                            func_L00_00251E30(G);
                            func_L00_002110C0(3, 0x53, G);
                        }
                    }
                }
            }
        }
    }
    if (B[0x20A4] == 2) {
        func_001F9938(B + 0x1636);
    }
    p = D_L15_00179F70;
    end = p + 0x230;
    do {
        func_001F9908((int *)p);
        p += 0x70;
    } while (p < end);
    *(float *)(B + 0x22B8) = 10000.0f;
    if (*(short *)(B + 0x30E) != 0 && func_L00_002342F8((float *)(B + 0xE0)) < 0.0f) {
        if (*(int *)(B + 0x21B4) == 0x20) {
            func_001F9BC0(vec);
            for (i = 0; i < 8; i++) {
                int x = *(int *)(B + 0x21B0) - i;
                func_001F9BF0(tmp, B + 0x1B00 + ((x + 0x1F) % 32) * 16, B + 0x1B00 + ((x + 0x1E) % 32) * 16);
                func_001F9BD8(vec, vec, tmp);
            }
            func_001F9C30(vec, vec, 1.0f / 8);
            if (func_001F9CB8(vec) < D_0015EE6C) {
                *(unsigned short *)(B + 0x1E2) += 1;
            } else {
                *(short *)(B + 0x1E2) = 0;
            }
        }
    } else {
        *(short *)(B + 0x1E2) = 0;
    }
    func_L00_002222F0();
    if (B[0x20A4] == 2) {
        func_L00_00233EE0((float *)(B + 0xD0), 0.0f, 0.0f, 4.0f);
        *(float *)(B + 0x2288) = 15.0f;
        *(float *)(B + 0x228C) = 3.0f;
    } else {
        func_L00_00233EE0((float *)(B + 0xD0), 0.0f, 0.0f, 0.7f);
        if (D_0013D5E9[1] != 0) {
            *(float *)(B + 0x2288) = 12.0f;
            *(float *)(B + 0x228C) = 4.5f;
        } else {
            *(float *)(B + 0x2288) = 3.0f;
            *(float *)(B + 0x228C) = 1.75f;
        }
    }
    if (*(float *)(B + 0x88) - *(float *)(B + 0x2D8) < 4.0f) {
        qcopy(B + 0xC0, B + 0x80);
        *(float *)(B + 0xC8) = *(float *)(B + 0x2D8) + 0.5f;
    } else {
        qcopy(B + 0xC0, B + 0xD0);
    }
    if (*(float *)(B + 0x88) < 1.0f) {
        func_L00_00211908();
    }
}
