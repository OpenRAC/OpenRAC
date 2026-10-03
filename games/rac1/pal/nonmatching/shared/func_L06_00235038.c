/* NON_MATCHING func_L06_00235038 -- src/overlays/shared/help_0021D6B8.c
 * Best so far: SIZE ours 1332 / retail 1340, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * Cannot land as written (#define in a candidate): rewrite that in plain C first.
 * What the last attempts found:
 *   Per-frame update: bumps the counters at base+0x198.., ticks the timers with func_001F9908/9938, reads the pad 
 *   Matched so far: per-statement `char *g = (char *)D_0013E633 + 0xE1D;` locals, a single `pad` variable assigned
 *   Open: (1) the 0x1D4 pad test has pad/value registers swapped ($v1/$v0); (2) retail copies the block base into 
 */
extern unsigned char D_0013E633[];
extern char D_0013A5E0[];
extern char D_L06_0017A170[];
extern short D_L06_0015F6A8;
extern float D_0015EE6C MACRO_ADDR;
extern unsigned char D_0013D5E9[];
extern void func_L00_00216D40(void);
extern void func_001F9908(int *arg0);
extern int func_001F9938(void *);
extern void func_L06_00228390(void);
extern int func_001F9850(int);
extern int func_L00_00267BA8(int, int, int *);
extern char *func_0020D348(int);
extern void func_L00_00251E30(void *);
extern void func_L01_00231960(int, int, void *);
extern float func_L00_002342F8(float *v);
extern void func_001F9BC0(void *);
extern void func_001F9BF0(void *, void *, void *);
extern void func_001F9BD8(void *, void *, void *);
extern void func_001F9C30(void *, void *, float);
extern float func_001F9CB8(void *);
extern void func_L00_002222F0(void);
extern void func_L00_00233EE0(float *, float, float, float);
extern void func_L00_00211908(void);

#define G g
#define U8(o) (*(unsigned char *)(G + (o)))
#define S16(o) (*(short *)(G + (o)))
#define I32(o) (*(int *)(G + (o)))
#define F32(o) (*(float *)(G + (o)))

/* Per-frame update of the camera/pad counters, the help-marker spawn and the camera blend parameters. */
void func_L06_00235038(void) {
    float v[4];
    float t[4];
    char *m;
    char *g = (char *)D_0013E633 + 0xE1D;
    char *ring;
    char *pad;
    char *h;
    int n;
    int i;
    int p;
    int end;
    int r;

    if (U8(0x20AD) == 0) func_L00_00216D40();
    I32(0x198) = I32(0x198) + 1;
    I32(0x19C) = I32(0x19C) + 1;
    I32(0x1A0) = I32(0x1A0) + 1;
    I32(0x1A4) = I32(0x1A4) + 1;
    func_001F9908((int *)(G + 0x1C4));
    func_001F9908((int *)(G + 0x1C0));
    func_001F9938(G + 0x1B2);
    func_001F9908((int *)(G + 0x1B8));
    func_001F9938(G + 0x1D8);
    func_001F9938(G + 0x1DE);
    func_001F9908((int *)(G + 0x1B4));
    func_001F9938(G + 0x1F0);
    func_001F9938(G + 0x1B0);
    func_001F9938(G + 0x1C8);
    func_001F9938(G + 0x1CA);
    func_001F9938(G + 0x1E4);
    func_001F9938(G + 0x1E6);
    func_001F9908((int *)(G + 0x1CC));
    func_001F9908((int *)(G + 0x1D0));
    func_001F9908((int *)(G + 0x1D4));
    func_001F9938(G + 0x1E8);
    func_001F9938(G + 0x1E0);
    func_001F9938(G + 0x1EE);
    func_001F9938(G + 0x1EA);
    func_001F9938(G + 0x1F4);
    func_001F9938(G + 0x1F2);
    func_001F9938(G + 0x1DC);
    func_001F9938(G + 0x1F6);
    func_001F9938(G + 0x1DA);
    func_001F9938(G + 0x1F8);
    pad = D_0013A5E0 + 0x2460;
    if ((*(int *)(pad + 0x1A0) & 5) == 0) S16(0x1EC) = 0;
    if (U8(0x20A8) != 0) {
        I32(0x1BC) = I32(0x1BC) + 1;
    } else {
        I32(0x1BC) = 0;
    }
    pad = D_0013A5E0 + 0x2460;
    if (*(int *)(pad + 0x1D4) != 0) {
        char *g = (char *)D_0013E633 + 0xE1D;
        I32(0x1A8) = 0;
        I32(0x1AC) = I32(0x1AC) + 1;
    } else {
        char *g = (char *)D_0013E633 + 0xE1D;
        I32(0x1AC) = 0;
        I32(0x1A8) = I32(0x1A8) + 1;
    }
    {
    char *g = (char *)D_0013E633 + 0xE1D;
    if (S16(0x22DE) != 0 && func_001F9938(G + 0x22DE)) {
        if (I32(0x22A8) != 0) {
            if (*(int *)&D_L06_0015F6A8 == 0) {
                if (U8(0x20A4) == 3) {
                    func_L06_00228390();
                    if (func_L00_00267BA8(0x80, func_001F9850(0x12), 0)) {
                        U8(0x20A6) = 1;
                    }
                } else {
                    m = func_0020D348(0x27A);
                    I32(0xA8C) = (int)m;
                    if (m) {
                        qcopy(m + 0x10, G + 0x80);
                        qcopy(m + 0x40, G + 0x90);
                        *(short *)(m + 0x32) = 0x40;
                        *(char *)(I32(0xA8C) + 0x31) = 1;
                        *(int *)(I32(0xA8C) + 0x90) = 0;
                        *(long *)(I32(0xA8C) + 0x38) = *(long *)(I32(0x2080) + 0x38);
                        func_L00_00251E30((void *)I32(0xA8C));
                        func_L01_00231960(3, 0x53, (void *)I32(0xA8C));
                    }
                }
            }
        }
    }
    }
    p = (int)D_L06_0017A170;
    end = p + 0x230;
    do {
        func_001F9908((int *)p);
        p += 0x70;
    } while (p < end);
    {
    char *g = (char *)D_0013E633 + 0xE1D;
    F32(0x22B8) = 9999.0f;
    if (S16(0x30E) != 0 && func_L00_002342F8((float *)(G + 0xE0)) < 0.0f) {
        if (I32(0x21B4) == 0x20) {
            func_001F9BC0(v);
            h = (char *)D_0013E633 + 0xE1D;
            ring = h + 0x1B00;
            n = 8;
            for (i = 0; i < n; i++) {
                int x = *(int *)(h + 0x21B0) - i;
                func_001F9BF0(t, ring + ((x + 0x1F) % 32) * 16, ring + ((x + 0x1E) % 32) * 16);
                func_001F9BD8(v, v, t);
            }
            func_001F9C30(v, v, 1.0f / n);
            if (func_001F9CB8(v) < D_0015EE6C) {
                char *g = (char *)D_0013E633 + 0xE1D;
                S16(0x1E2) = (unsigned short)S16(0x1E2) + 1;
            } else {
                char *g = (char *)D_0013E633 + 0xE1D;
                S16(0x1E2) = 0;
            }
        }
    } else {
        char *g = (char *)D_0013E633 + 0xE1D;
        S16(0x1E2) = 0;
    }
    }
    func_L00_002222F0();
    {
    char *g = (char *)D_0013E633 + 0xE1D;
    if (U8(0x20A4) == 1) {
        func_L00_00233EE0((float *)(G + 0xD0), 0.0f, 0.0f, 0.4f);
        F32(0x2288) = 2.125f;
        F32(0x228C) = 1.25f;
    } else {
        func_L00_00233EE0((float *)(G + 0xD0), 0.0f, 0.0f, 0.7f);
        if (D_0013D5E9[1] != 0) {
            F32(0x2288) = 12.0f;
            F32(0x228C) = 4.5f;
        } else {
            F32(0x2288) = 3.0f;
            F32(0x228C) = 1.75f;
        }
    }
    }
    {
        char *g = (char *)D_0013E633 + 0xE1D;
        float z = F32(0x2D8);
        if (F32(0x88) - z < 4.0f) {
            qcopy(G + 0xC0, G + 0x80);
            F32(0xC8) = z + 0.5f;
        } else {
            qcopy(G + 0xC0, G + 0xD0);
        }
    }
    { char *g = (char *)D_0013E633 + 0xE1D;
    if (F32(0x88) < 1.0f) func_L00_00211908();
    }
}
