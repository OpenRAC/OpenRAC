/* NON_MATCHING func_L01_00231B98 -- src/overlays/shared/help_002274A8.c
 * Best so far: SIZE ours 1076 / retail 1072, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L01_00231B98: registers a hit on the help-slot moby (bumps D_0015EFA4[1] and a per-difficulty counter, bu
 *   Best candidate p7.c: SIZE 1056 vs retail 1072 and otherwise identical in structure (switch on the 0x20A4 byte;
 *   Remaining difference: retail keeps the default arm of the t-chain (t not 0x16/0x12/0x11/3) as its own call sit
 */
extern unsigned char D_0013E633[];
extern char D_0013DE6E[];
extern char D_L01_00178600[];
extern int D_0015EE84 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern int D_L01_0015F6A8 MACRO_ADDR;
extern int D_0015EFA4[] MACRO_ADDR;
extern void func_L00_00207220(void);
extern void func_001F9BF0(void *, void *, void *);
extern float func_001FA748(float, float);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern void func_L01_0023D688(int, int);
extern int func_L00_00217570(int, int);
extern void func_L00_00211338(float *, int, float, float);
extern void func_001F9BD8(void *, void *, void *);

/* Registers a hit on the moby in the help slot: bumps the counters, builds the impact vector and plays the reaction. */
int func_L01_00231B98(int arg) {
    char *g = (char *)D_0013E633 + 0xE1D;
    char *m0;
    char *e;
    int *cnt;
    int hit;
    float v[4];
    int u;
    int t;
    char *m;
    float sc;
    float a;
    float b;

    if (*(int *)(g + 0x208C) == 0x14 || *(int *)(g + 0x208C) == 7) return 0;
    if (*(int *)(g + 0x2084) == 0x32) return 0;
    if (*(int *)(g + 0x1C0) != 0) return 0;
    if (D_L01_0015F6A8 != 0) return 0;
    *(int *)(g + 0x2280) = 0;
    m0 = *(char **)(g + 0x2080);
    if (*(unsigned char *)(m0 + 0xA4) == 0xFF) return 0;
    e = D_L01_00178600 + *(unsigned char *)(m0 + 0xA4) * 64;
    if (*(char **)(e + 0x34) != m0) return 0;
    if (((*(int *)(e + 0x24)) ^ 1) & 1) return 0;
    cnt = (int *)(D_0013DE6E + 0x222);
    *(int *)(g + 0x2280) = *(int *)(e + 0x20);
    D_0015EFA4[1] = D_0015EFA4[1] + 1;
    cnt[D_0015EE84] = cnt[D_0015EE84] + 1;
    if (arg == 0) return 1;
    hit = 0;
    func_L00_00207220();
    if (*(int *)(e + 0x30) & 1) {
        qcopy(v, e + 0x10);
        if (*(float *)(e + 0x1C) == 7000.0f) {
            hit = 1;
        }
    } else {
        m = *(char **)(e + 0x20);
        if (m != 0) {
            func_001F9BF0(v, g + 0x80, m + 0x10);
        } else {
            float pi = 3.1415927f;
            v[0] = func_001F9F90(func_001FA748(*(float *)(g + 0x98), pi));
            v[1] = func_001F9FA8(func_001FA748(*(float *)(g + 0x98), pi));
            v[2] = 0.0f;
        }
    }
    g = (char *)D_0013E633 + 0xE1D;
    u = *(unsigned char *)(g + 0x20A4);
    if (u == 0) {
        g = (char *)D_0013E633 + 0xE1D;
        m = *(char **)(g + 0x2280);
        if (m != 0 && (*(short *)(m + 0xA6) == 0x4EB || *(short *)(m + 0xA6) == 0x558)) {
            func_L01_0023D688(0x80, 1);
            return 1;
        }
        t = *(int *)(g + 0x208C);
        if (t == 0x16) {
            func_L01_0023D688(0x6D, 1);
            *(float *)(g + 0x128) = D_0015EE6C * 7.0f;
            return 1;
        } else if (t == 0x12) {
            m = *(char **)(g + 0x2280);
            if (m != 0 && *(short *)(m + 0xA6) == 0x28F) {
                func_L01_0023D688(0x82, 1);
                return 1;
            }
            func_L01_0023D688(0x75, 1);
            sc = D_0015EE6C;
        } else if (t == 0x11) {
            m = *(char **)(g + 0x2280);
            if (m != 0 && *(short *)(m + 0xA6) == 0x28F) {
                func_L01_0023D688(0x82, 1);
                return 1;
            }
            if (D_0015EE84 == 0xF || D_0015EE84 == 0x11) {
                g = (char *)D_0013E633 + 0xE1D;
                m = *(char **)(g + 0x2280);
                if (m != 0) {
                    short h = *(short *)(m + 0xA6);
                    if (h == 0x28F || h == 0x7B || h == 0x29D) {
                        func_L00_00217570(0x1C, 0);
                    }
                }
            }
            func_L01_0023D688(0x76, 1);
            sc = D_0015EE6C;
        } else if (t == 3) {
            m = *(char **)(g + 0x2280);
            if (m != 0 && *(short *)(m + 0xA6) == 0x28F) {
                func_L01_0023D688(0x82, 1);
                return 1;
            }
            func_L01_0023D688(0x16, 1);
            sc = D_0015EE6C;
        } else {
            func_L01_0023D688(0x16, 1);
            sc = D_0015EE6C;
        }
        a = sc * 5.7f;
        b = sc * 2.4f;
        g = (char *)D_0013E633 + 0xE1D;
        if (*(unsigned char *)(g + 0x12E7) != 0) {
            a = 0.0f;
            b = sc * 1.7f;
        }
        func_L00_00211338(v, hit, a, b);
        if (hit != 0 && *(unsigned char *)(e + 0x28) == 4) {
            v[2] = v[2] + v[2];
        }
    } else if (u == 3) {
        func_L01_0023D688(0x56, 1);
        sc = D_0015EE6C;
        func_L00_00211338(v, hit, sc * 5.0f, sc * 2.4f);
    }
    func_001F9BD8((char *)D_0013E633 + 0xEFD, (char *)D_0013E633 + 0xEFD, v);
    func_001F9BD8((char *)D_0013E633 + 0xF1D, (char *)D_0013E633 + 0xF1D, v);
    return 1;
}
