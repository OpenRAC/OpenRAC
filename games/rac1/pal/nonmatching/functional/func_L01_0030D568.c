extern int D_L01_0015F6A8_30D568 __asm__("D_L01_0015F6A8") MACRO_ADDR;
extern float D_L01_0015F660_30D568[] __asm__("D_L01_0015F660") MACRO_ADDR;
extern float D_0015EE6C_30D568 __asm__("D_0015EE6C") MACRO_ADDR;
extern char D_L01_00162100_30D568[] __asm__("D_L01_00162100");
extern char func_00233AB8_30D568[] __asm__("func_00233AB8");
extern int func_001F9850_30D568(int) __asm__("func_001F9850");
extern void func_L00_00264870_30D568(int) __asm__("func_L00_00264870");
extern void func_L00_00250800_30D568(void *, int, void *) __asm__("func_L00_00250800");
extern float func_002140F8_30D568(float, float) __asm__("func_002140F8");
extern int func_002140B0_30D568(int) __asm__("func_002140B0");
extern unsigned char *func_L00_00272770_30D568(void *, void *, void *, float, float) __asm__("func_L00_00272770");
extern char *func_L00_002D9340_30D568(void *, float) __asm__("func_L00_002D9340");
extern f32 func_00214158_30D568(void) __asm__("func_00214158");
extern float func_001F9F90_30D568(float) __asm__("func_001F9F90");
extern float func_001F9FA8_30D568(float) __asm__("func_001F9FA8");
extern int func_L00_00258BC8_30D568(int lo, int hi) __asm__("func_L00_00258BC8");
extern void func_L00_002703E8_30D568(void *, void *, int, int) __asm__("func_L00_002703E8");
extern void func_001F49B0_30D568(void *, void *) __asm__("func_001F49B0");
extern char *func_L00_0026DEA0_30D568(void *, int, void *, int, float, float, float, float) __asm__("func_L00_0026DEA0");
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern void func_001F9BF0_30D568(void *, void *, void *) __asm__("func_001F9BF0");
extern void func_001F9BC0_30D568(void *) __asm__("func_001F9BC0");
extern void func_L00_0025F4A8_30D568(void *, void *, void *, float, float, int, int, int, float, float, float, float, int, float, int, int, int, int) __asm__("func_L00_0025F4A8");
extern void func_001F9C30_30D568(void *, void *, float) __asm__("func_001F9C30");
extern void func_L00_00264BE8_30D568(void *, void *, void *, float, float) __asm__("func_L00_00264BE8");
extern void func_001F9BD8_30D568(void *, void *, void *) __asm__("func_001F9BD8");
typedef struct {
    unsigned char pad0[0x30];
    int mode;
    int frame;
    unsigned char pad38[0x178 - 0x38];
    void *ents[4];
    void *e188;
} G_30D568;
extern G_30D568 GL_30D568 __asm__("D_L01_0016CD60");

/* func_L01_0030D568 -- src/overlays/l01_novalis/vendor_002FABE8.c (functional C for the port, not a match)
 * Novalis cutscene effects (class 1546): while a cutscene plays (D_L01_0015F6A8 == 2) it follows the
 * cutscene record D_L01_0016CD60 (+0x30 the scene, +0x34 its frame, +0x178.. the actors): the
 * ambient hum per scene, the dust burst and the 16-puff kick at given frames of scene 1, the
 * sparks off the six joints of actor class 0x213 in scene 4, and in scene 5 the explosion pair and
 * the smoke trails that follow actor +0x180's joints.
 * From the staged near miss (nonmatching/l01_novalis/func_L01_0030D568.c), its logic checked against
 * retail block by block; restructured the hum (one call with the chosen actor, as retail) and made
 * the record a struct.
 * equiv: DIFFERENT by shape only: this compiler re-reads the scene word (+0x30) more often than retail
 * (a read with no side effect), keeps the two smoke constants per iteration, and some tests have the
 * other branch polarity; every call, its arguments and every store match on reading. */
void func_L01_0030D568(char *moby) {
    char a[0x10] __attribute__((aligned(16)));
    char b[0x10] __attribute__((aligned(16)));
    char c[0x10] __attribute__((aligned(16)));
    char d[0x10] __attribute__((aligned(16)));
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
    G_30D568 *g = &GL_30D568;

    if (*(unsigned char *)(moby + 0x20) == 0) {
        *(unsigned char *)(moby + 0x20) = 1;
        *(unsigned char *)(moby + 0x30) = 0xFF;
        return;
    }
    if (*(unsigned char *)(moby + 0x20) != 1) return;
    if (D_L01_0015F6A8_30D568 != 2) return;

    v = g->mode;
    if (v == 1) {
        goto hum;
    }
    if (v == 2) {
        r = func_001F9850_30D568(0x94);
        if (!(r < g->frame)) {
            goto hum;
        }
    }
    v = g->mode;
    if (v == 3 || v == 4) {
    hum:
        v = g->mode;
        k = 3;
        if (v != 1 && v != 3 && v != 4) {
            k = (v ^ 2) == 0 ? 2 : 3;
        }
        func_L00_00264870_30D568(*(int *)&g->ents[k]);
    }

    if (g->mode == 1) {
        r = func_001F9850_30D568(0x1D8);
        if (!(g->frame < r)) {
            r = func_001F9850_30D568(0x219);
            if (!(r < g->frame)) {
                if (*(int *)&g->ents[0] != 0) {
                    func_L00_00250800_30D568(g->ents[0], 1, a);
                    f21 = 2.0f;
                    f20 = func_002140F8_30D568(0.5f, f21);
                    fb2 = (func_002140B0_30D568(2) != 0) ? f21 : -2.0f;
                    q = (char *)func_L00_00272770_30D568(a, D_L01_0015F660_30D568, a + 8, f20, fb2);
                    if (q != 0) *(short *)(q + 0xA) = func_001F9850_30D568(0x3C);
                }
            }
        }
    }

    if (g->mode == 1) {
        r = func_001F9850_30D568(0x1D8);
        if (g->frame == r && *(int *)&g->ents[0] != 0) {
            func_L00_00250800_30D568(g->ents[0], 1, a);
            f20 = 3.0f;
            q = func_L00_002D9340_30D568(a, f20);
            if (q != 0) *(unsigned char *)(q + 0x23) = 0x70;
            f24 = 0.0f;
            f22 = f20;
            f23 = 6.5f;
            k = 15;
            do {
                k--;
                f20 = func_00214158_30D568();
                e = D_0015EE6C_30D568;
                f21 = func_002140F8_30D568(e * f24, e * f22);
                t = func_001F9F90_30D568(f20);
                *(float *)(b + 0) = t * f21;
                t2 = func_001F9FA8_30D568(f20);
                e = D_0015EE6C_30D568;
                *(float *)(b + 4) = t2 * f21;
                *(float *)(b + 8) = func_002140F8_30D568(e * f22, e * f23);
                r16 = func_L00_00258BC8_30D568(0x5A, 0x78);
                s2 = func_002140B0_30D568(2);
                func_L00_002703E8_30D568(a, b, s2, r16);
            } while (k >= 0);
                }
    }

    if (g->mode == 4) {
        m = (char *)g->e188;
        if (m != 0 && *(short *)(m + 0xA6) == 0x213) {
            func_001F49B0_30D568(func_00233AB8_30D568, m);
            r = func_001F9850_30D568(0x352);
            if (!(g->frame < r)) {
                r = func_001F9850_30D568(0x488);
                if (!(r < g->frame)) {
                    for (i = 1; i < 7; i++) {
                        func_L00_00250800_30D568(m, i, a);
                        k2 = 1;
                        do {
                            r = func_002140B0_30D568(0x10);
                            s2 = func_002140B0_30D568(2);
                            x = (s2 == 0) ? r : -r;
                            ret = func_L00_0026DEA0_30D568(a, x, D_L01_0015F660_30D568, 0x7F204080, 0.1f, 1.0f, 0.9f, 200000.0f);
                            if (ret != 0) {
                                r17 = ret + 0x20;
                                r = func_001F9850_30D568(0x44C);
                                k4 = 6;
                                if (!(g->frame < r)) {
                                    k4 = 0x1E;
                                }
                                r2 = func_001F9850_30D568(k4);
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

    if (g->mode == 5) {
        r = func_001F9850_30D568(0x78);
        if (g->frame < r) {
            m = (char *)g->ents[2];
            if (m != 0) {
                *(unsigned int __attribute__((mode(TI))) *)a = 0;
                func_L00_00250800_30D568(m, 5, b);
                if (!(g->frame < 2)) func_001F9BF0_30D568(a, b, D_L01_00162100_30D568);
                qcopy(D_L01_00162100_30D568, b);
                r = func_001F9850_30D568(0x76);
                if (g->frame == r) {
                    func_001F9BC0_30D568(c);
                    f20 = 0.0f;
                    f23 = 9.0f;
                    f21 = 1.0f;
                    f22 = 60.0f;
                    func_L00_0025F4A8_30D568(m, c, b, f20, f20, 3, 0xC8, 0xC8, 5.0f, 2.5f, f23, f21, -1, f22, 0, 0x3C, -1, 0);
                    qcopy(d, b);
                    *(float *)(d + 8) = *(float *)(d + 8) - f21;
                    func_L00_0025F4A8_30D568(m, c, d, f20, f20, 0x12C, 2, 2, f21, 0.5f, f23, f21, -1, f22, 0, 0, -1, 0);
                }
                func_001F49B0_30D568(func_00233AB8_30D568, m);
                f21 = 150000.0f;
                f20 = 60000.0f;
                func_L00_00250800_30D568(m, 1, c);
                func_L00_00250800_30D568(m, 2, d);
                func_001F9BF0_30D568(c, c, a);
                func_001F9BF0_30D568(d, d, a);
                func_001F9C30_30D568(a, a, 0.25f);
                k3 = 3;
                do {
                    k3--;
                    func_L00_00264BE8_30D568(c, c, D_L01_0015F660_30D568, f21, f20);
                    func_L00_00264BE8_30D568(d, d, D_L01_0015F660_30D568, f21, f20);
                    func_001F9BD8_30D568(c, c, a);
                    func_001F9BD8_30D568(d, d, a);
                } while (k3 >= 0);
            }
        }
    }
}
