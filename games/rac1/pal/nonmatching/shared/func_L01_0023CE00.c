/* NON_MATCHING func_L01_0023CE00 -- src/overlays/shared/help_002274A8.c
 * Best so far: SIZE ours 1296 / retail 1292, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L01_0023CE00 (HeroTickStateTimer): per-frame tick of the hero block at D_0013E633+0xE1D: bumps four count
 *   Remaining differences: (1) in the 8-iteration loop retail copies the state pointer (daddu $20,$16,$0 in the fu
 *   Tricks that did matter: a fresh pointer variable per region (one assignment each, so nothing crosses a call), 
 */
extern unsigned char D_0013E633[];
extern char D_0013A5E0[];
extern char D_0013D50F[];
extern char D_L01_00179DF0[];
extern float D_0015EE6C MACRO_ADDR;

extern void func_L00_00216D40(void);
extern void func_001F9908(int *arg0);
extern int func_001F9938(void *);
extern void func_L01_00231A68(void);
extern int func_001F9850(int);
extern int func_L00_00267BA8(int, void *, void *);
extern char *func_0020D348(int);
extern void func_L00_00251E30(void *);
extern float func_L00_002342F8(float *v);
extern void func_001F9BF0(void *, void *, void *);
extern void func_001F9BD8(void *, void *, void *);
extern void func_001F9C30(void *, void *, float);
extern float func_001F9CB8(void *);
extern void func_L00_002222F0(void);
extern void func_L00_00233EE0(float *, float, float, float);
extern void func_L00_00211908(void);

/* per-frame timer tick of the hero state block: bumps counters, decays timers, tracks the camera-facing test and sets a few tuning floats */
void func_L01_0023CE00(void) {
    unsigned char *g = (unsigned char *)D_0013E633 + 0xE1D;
    char *pad;
    char *h;
    char *q;
    char *s;
    char *m;
    char *p;
    float a;
    float b;
    int i;

    if (g[0x20AD] == 0) func_L00_00216D40();
    *(int *)(g + 0x198) += 1;
    *(int *)(g + 0x19C) += 1;
    *(int *)(g + 0x1A0) += 1;
    *(int *)(g + 0x1A4) += 1;
    func_001F9908((int *)(g + 0x1C4));
    func_001F9908((int *)(g + 0x1C0));
    func_001F9938(g + 0x1B2);
    func_001F9908((int *)(g + 0x1B8));
    func_001F9938(g + 0x1D8);
    func_001F9938(g + 0x1DE);
    func_001F9908((int *)(g + 0x1B4));
    func_001F9938(g + 0x1F0);
    func_001F9938(g + 0x1B0);
    func_001F9938(g + 0x1C8);
    func_001F9938(g + 0x1CA);
    func_001F9938(g + 0x1E4);
    func_001F9938(g + 0x1E6);
    func_001F9908((int *)(g + 0x1CC));
    func_001F9908((int *)(g + 0x1D0));
    func_001F9908((int *)(g + 0x1D4));
    func_001F9938(g + 0x1E8);
    func_001F9938(g + 0x1E0);
    func_001F9938(g + 0x1EE);
    func_001F9938(g + 0x1EA);
    func_001F9938(g + 0x1F4);
    func_001F9938(g + 0x1F2);
    func_001F9938(g + 0x1DC);
    func_001F9938(g + 0x1F6);
    func_001F9938(g + 0x1DA);
    func_001F9938(g + 0x1F8);

    pad = D_0013A5E0 + 0x2460;
    if ((*(int *)(pad + 0x1A0) & 5) == 0) *(short *)(g + 0x1EC) = 0;
    if (g[0x20A8] != 0) *(int *)(g + 0x1BC) += 1;
    else *(int *)(g + 0x1BC) = 0;
    h = (char *)D_0013E633 + 0xE1D;
    pad = D_0013A5E0 + 0x2460;
    if (*(int *)(pad + 0x1D4) != 0) {
        *(int *)(h + 0x1A8) = 0;
        *(int *)(h + 0x1AC) += 1;
    } else {
        *(int *)(h + 0x1AC) = 0;
        *(int *)(h + 0x1A8) += 1;
    }

    g = (unsigned char *)D_0013E633 + 0xE1D;
    if (*(short *)(g + 0x22DE) != 0 && func_001F9938(g + 0x22DE) != 0) {
        if (*(int *)(g + 0x22A8) != 0 && *(int *)&D_L01_0015F6A8 == 0) {
            if (g[0x20A4] == 3) {
                func_L01_00231A68();
                if (func_L00_00267BA8(0x80, (void *)func_001F9850(0x12), 0) != 0) g[0x20A6] = 1;
            } else {
                m = func_0020D348(0x27A);
                *(char **)(g + 0xA8C) = m;
                if (m != 0) {
                    qcopy(m + 0x10, g + 0x80);
                    qcopy(m + 0x40, g + 0x90);
                    *(short *)(m + 0x32) = 0x40;
                    *(*(char **)(g + 0xA8C) + 0x31) = 1;
                    *(int *)(*(char **)(g + 0xA8C) + 0x90) = 0;
                    *(long *)(*(char **)(g + 0xA8C) + 0x38) = *(long *)(*(char **)(g + 0x2080) + 0x38);
                    func_L00_00251E30(*(char **)(g + 0xA8C));
                    func_L01_00231960(3, 0x53, *(char **)(g + 0xA8C));
                }
            }
        }
    }

    for (i = 0; i < 5; i++) func_001F9908((int *)(D_L01_00179DF0 + i * 0x70));

    g = (unsigned char *)D_0013E633 + 0xE1D;
    *(float *)(g + 0x22B8) = 10000.0f;
    if (*(short *)(g + 0x30E) != 0 && func_L00_002342F8((float *)(g + 0xE0)) < 0.0f) {
        if (*(int *)(g + 0x21B4) == 0x20) {
            char *t;
            float v[4];
            float w[4];
            func_001F9BC0(v);
            t = (char *)g + 0x1B00;
            for (i = 0; i < 8; i++) {
                int n = *(int *)(g + 0x21B0) - i;
                func_001F9BF0(w, t + ((n + 0x1F) % 32) * 16, t + ((n + 0x1E) % 32) * 16);
                func_001F9BD8(v, v, w);
            }
            func_001F9C30(v, v, 1.0f / i);
            if (func_001F9CB8(v) < D_0015EE6C) *(unsigned short *)(D_0013E633 + 0xE1D + 0x1E2) += 1;
            else *(short *)(D_0013E633 + 0xE1D + 0x1E2) = 0;
        }
    } else {
        *(short *)(D_0013E633 + 0xE1D + 0x1E2) = 0;
    }

    func_L00_002222F0();
    q = (char *)D_0013E633 + 0xEED;
    func_L00_00233EE0((float *)q, 0.0f, 0.0f, 0.7f);
    s = q - 0xD0;
    if (*(unsigned char *)(D_0013D50F + 0xB9 + 0x22) != 0) {
        a = 12.0f;
        b = 4.5f;
    } else {
        a = 3.0f;
        b = 1.75f;
    }
    *(float *)(s + 0x2288) = a;
    *(float *)(s + 0x228C) = b;

    g = (unsigned char *)D_0013E633 + 0xE1D;
    if (*(float *)(g + 0x88) - *(float *)(g + 0x2D8) < 4.0f) {
        qcopy(g + 0xC0, g + 0x80);
        *(float *)(g + 0xC8) = *(float *)(g + 0x2D8) + 0.5f;
    } else {
        qcopy(g + 0xC0, g + 0xD0);
    }
    if (*(float *)(D_0013E633 + 0xE1D + 0x88) < 1.0f) func_L00_00211908();
}
