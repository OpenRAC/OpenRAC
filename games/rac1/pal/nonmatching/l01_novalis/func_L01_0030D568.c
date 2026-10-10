/* NON_MATCHING func_L01_0030D568 -- src/overlays/l01_novalis/vendor_002FABE8.c
 * Best so far: SIZE ours 1752 / retail 1796, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Moby update for class 1546: state 0 init, state 1 runs a cutscene state machine on the table at D_L01_0016CD60
 *   Differences: retail keeps the hi half of &D_L01_0016CD60 in $fp and re-adds the low half per block (addiu $x,$
 *   Would unblock: the source shape that makes GCC hoist the hi half of the global address into a saved register a
 */
extern int D_L01_0015F6A8 MACRO_ADDR;
extern float D_L01_0015F660[] MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern char D_L01_00162100[];
extern char func_00233AB8[];
extern int func_001F9850(int);
extern void func_L00_00264870(int);
extern void func_L00_00250800(void *, int, void *);
extern float func_002140F8(float, float);
extern int func_002140B0(int);
extern unsigned char *func_L00_00272770(void *, void *, void *, float, float);
extern char *func_L00_002D9340(void *, float);
extern f32 func_00214158(void);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern int func_L00_00258BC8(int lo, int hi);
extern void func_L00_002703E8(void *, void *, int, int);
extern void func_001F49B0(void *, void *);
extern char *func_L00_0026DEA0(void *, int, void *, int, float, float, float, float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern void func_001F9BF0(void *, void *, void *);
extern void func_001F9BC0(void *);
extern void func_L00_0025F4A8(void *, void *, void *, float, float, int, int, int, float, float, float, float, int, float, int, int, int, int);
extern void func_001F9C30(void *, void *, float);
extern void func_L00_00264BE8(void *, void *, void *, float, float);
extern void func_001F9BD8(void *, void *, void *);
extern G D_L01_0016CD60;

/* Update for moby class 1546 on level 01: a cutscene effect state machine over the level table at D_L01_0016CD60. */
void func_L01_0030D568(char *moby) {
    char a[0x10];
    char b[0x10];
    char c[0x10];
    char d[0x10];
    char *m;
    char *q;
    char *ret;
    char *r17;
    int v;
    int k;
    int i;
    int k2;
    int k3;
    int k4;
    int r;
    int r2;
    int r16;
    int x;
    int s2;
    float f20, f21, f22, f23, f24;
    float e, t, t2;
    float fb2;

    if (*(unsigned char *)(moby + 0x20) == 0) {
        *(unsigned char *)(moby + 0x20) = 1;
        *(unsigned char *)(moby + 0x30) = 0xFF;
        return;
    }
    if (*(unsigned char *)(moby + 0x20) != 1) return;
    if (D_L01_0015F6A8 != 2) return;

    {
    char *g = (char *)&D_L01_0016CD60;
    v = *(int *)(g + 0x30);
    if (v == 1) {
        func_L00_00264870(*(int *)(g + 0x178 + 3 * 4));
    } else if (v == 2) {
        r = func_001F9850(0x94);
        if (!(r < *(int *)(g + 0x34))) {
            func_L00_00264870(*(int *)(g + 0x178 + 2 * 4));
        }
    } else if (v == 3 || v == 4) {
        func_L00_00264870(*(int *)(g + 0x178 + 3 * 4));
    }
    }

    if (*(int *)((char *)&D_L01_0016CD60 + 0x30) == 1) {
        char *g = (char *)&D_L01_0016CD60;
        r = func_001F9850(0x1D8);
        if (!(*(int *)(g + 0x34) < r)) {
            r = func_001F9850(0x219);
            if (!(r < *(int *)(g + 0x34))) {
                if (*(int *)(g + 0x178) != 0) {
                    func_L00_00250800(*(void **)(g + 0x178), 1, a);
                    f21 = 2.0f;
                    f20 = func_002140F8(0.5f, f21);
                    fb2 = (func_002140B0(2) != 0) ? f21 : -2.0f;
                    q = (char *)func_L00_00272770(a, D_L01_0015F660, a + 8, f20, fb2);
                    if (q != 0) *(short *)(q + 0xA) = func_001F9850(0x3C);
                }
            }
        }
    }

    if (*(int *)((char *)&D_L01_0016CD60 + 0x30) == 1) {
        char *g = (char *)&D_L01_0016CD60;
        r = func_001F9850(0x1D8);
        if (*(int *)(g + 0x34) == r && *(int *)(g + 0x178) != 0) {
            func_L00_00250800(*(void **)(g + 0x178), 1, a);
            f20 = 3.0f;
            q = func_L00_002D9340(a, f20);
            if (q != 0) *(unsigned char *)(q + 0x23) = 0x70;
            f24 = 0.0f;
            f22 = f20;
            f23 = 6.5f;
            k = 15;
            do {
                k--;
                f20 = func_00214158();
                e = D_0015EE6C;
                f21 = func_002140F8(e * f24, e * f22);
                t = func_001F9F90(f20);
                *(float *)(b + 0) = t * f21;
                t2 = func_001F9FA8(f20);
                e = D_0015EE6C;
                *(float *)(b + 4) = t2 * f21;
                *(float *)(b + 8) = func_002140F8(e * f22, e * f23);
                r16 = func_L00_00258BC8(0x5A, 0x78);
                s2 = func_002140B0(2);
                func_L00_002703E8(a, b, s2, r16);
            } while (k >= 0);
                }
    }

    if (*(int *)((char *)&D_L01_0016CD60 + 0x30) == 4) {
        char *g = (char *)&D_L01_0016CD60;
        m = *(char **)(g + 0x188);
        if (m != 0 && *(short *)(m + 0xA6) == 0x213) {
            func_001F49B0(func_00233AB8, m);
            r = func_001F9850(0x352);
            if (!(*(int *)(g + 0x34) < r)) {
                r = func_001F9850(0x488);
                if (!(r < *(int *)(g + 0x34))) {
                    for (i = 1; i < 7; i++) {
                        func_L00_00250800(m, i, a);
                        k2 = 1;
                        do {
                            r = func_002140B0(0x10);
                            s2 = func_002140B0(2);
                            x = (s2 == 0) ? r : -r;
                            ret = func_L00_0026DEA0(a, x, D_L01_0015F660, 0x7F204080, 0.1f, 1.0f, 0.9f, 200000.0f);
                            if (ret != 0) {
                                r17 = ret + 0x20;
                                r = func_001F9850(0x44C);
                                k4 = (*(int *)(g + 0x34) < r) ? 6 : 0x1E;
                                r2 = func_001F9850(k4);
                                *(short *)(ret + 0xA) = r2;
                                v = func_001FA898_r(4.0f);
                                *(unsigned char *)(ret + 9) = (unsigned char)(v + 0x40);
                                *(int *)(r17 + 4) = 2;
                                *(unsigned char *)(r17 + 0xA) = 0x7F;
                                *(unsigned char *)(r17 + 0xB) = *(unsigned char *)(ret + 0xA);
                            }
                            k2--;
                        } while (k2 >= 0);
                    }
                }
            }
        }
    }

    if (*(int *)((char *)&D_L01_0016CD60 + 0x30) == 5) {
        char *g = (char *)&D_L01_0016CD60;
        r = func_001F9850(0x78);
        if (*(int *)(g + 0x34) < r) {
            m = *(char **)(g + 0x180);
            if (m != 0) {
                qzero(a);
                func_L00_00250800(m, 5, b);
                if (!(*(int *)(g + 0x34) < 2)) func_001F9BF0(a, b, D_L01_00162100);
                qcopy(D_L01_00162100, b);
                r = func_001F9850(0x76);
                if (*(int *)(g + 0x34) == r) {
                    func_001F9BC0(c);
                    f20 = 0.0f;
                    f23 = 9.0f;
                    f21 = 1.0f;
                    f22 = 60.0f;
                    func_L00_0025F4A8(m, c, b, f20, f20, 3, 0xC8, 0xC8, 5.0f, 2.5f, f23, f21, -1, f22, 0, 0x3C, -1, 0);
                    qcopy(d, b);
                    *(float *)(d + 8) = *(float *)(d + 8) - f21;
                    func_L00_0025F4A8(m, c, d, f20, f20, 0x12C, 2, 2, f21, 0.5f, f23, f21, -1, f22, 0, 0, -1, 0);
                }
                func_001F49B0(func_00233AB8, m);
                f21 = 150000.0f;
                f20 = 60000.0f;
                func_L00_00250800(m, 1, c);
                func_L00_00250800(m, 2, d);
                func_001F9BF0(c, c, a);
                func_001F9BF0(d, d, a);
                func_001F9C30(a, a, 0.25f);
                k3 = 3;
                do {
                    k3--;
                    func_L00_00264BE8(c, c, D_L01_0015F660, f21, f20);
                    func_L00_00264BE8(d, d, D_L01_0015F660, f21, f20);
                    func_001F9BD8(c, c, a);
                    func_001F9BD8(d, d, a);
                } while (k3 >= 0);
            }
        }
    }
}
