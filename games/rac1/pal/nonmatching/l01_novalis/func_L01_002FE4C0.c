/* NON_MATCHING func_L01_002FE4C0 -- src/overlays/l01_novalis/vendor_002FABE8.c
 * Best so far: SIZE ours 2208 / retail 2232, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   run 9 (p7): init stores reordered to retail's order (0x34/0x38, 0x28/0x2C, 0x3C, D1358, D1288.. D1354): SIZE 2
 *   run 10 (p8): -4/4 and -6/6 loop constants held in locals before their loops (retail sets f22/f21 before the lo
 *   run 11 (p9): outer-loop base recomputed from the global (retail's lui %hi before the loop): SIZE 2208.
 *   run 12 (p10): stored constants written inline at the stores (no locals): identical diff to p9, SIZE 2208.
 *   run 13 (p11): flag-5 constants (-11, 176, 39, 178, 0.05) held in locals: SIZE 2172, worse; layout of phase 2 c
 *   Stop after run 13: p9 and p10 leave the same first differences (init block: retail loads the -0.577, 32768, 25
 *   Still differing, unseen past the first ~25 hunks (try_func truncates the diff at ~100 lines): the 24-byte size
 *   Wall-like: the scheduler ties in the init constant stores (rewording did not move them in three tries).
 */
typedef int u128_FE4C0 __attribute__((mode(TI)));
typedef union { u128_FE4C0 q; float f[4]; } Vec4_FE4C0;

extern char D_L01_001E3780[];
extern char D_L01_001FA980[];
extern float D_L01_001803C0[];
extern float D_L01_001CAF80[];
extern int D_L01_00161358_s __asm__("D_L01_00161358") MACRO_ADDR;
extern int D_L01_00161350_s __asm__("D_L01_00161350") MACRO_ADDR;
extern float D_L01_00161288;
extern float D_L01_0016128C;
extern float D_L01_00161290;
extern float D_L01_00161294;
extern int D_L01_00161354;
extern unsigned char D_L01_00161280[];
extern unsigned char D_L01_00161281[];
extern unsigned char D_L01_00161282[];
extern unsigned char D_L01_00161283[];
extern unsigned char D_L01_00161284 MACRO_ADDR;
extern unsigned char D_L01_00161285[];
extern unsigned char D_L01_00161286[];
extern short D_L01_00161D14;
extern int D_L01_0015F6B0 MACRO_ADDR;
extern char D_L01_001672C0[];
extern int D_L01_001FA8C8[];
extern int D_L01_001FA8A8[];
extern char D_L01_001E379E[];
extern int D_L01_001FA8E8[];
extern char D_L01_001FA850[];
extern char D_L01_001FA910[];
extern unsigned char D_L01_001FA960[];

extern void func_L01_002B8C00(void *records, int count);
extern void func_L01_002B90A8(float s);
extern float func_L00_001FF860_303590(float, float) __asm__("func_L00_001FF860");
extern float func_001F9F90_FB158c(float) __asm__("func_001F9F90");
extern float func_001F9FA8_FB158c(float) __asm__("func_001F9FA8");
extern void func_L01_002B8E20(float *in, int *out);
extern float func_002140F8(float, float);
extern void func_L01_002B9440_x(void *, int, float, float, float, float) __asm__("func_L01_002B9440");
extern int func_00215570(void *arg0, int arg1);
extern int func_002140B0(int);
extern void func_L00_002A5158(int, int, int, float, float, float, float);
extern void func_L01_002B9198(char *p, int n);
extern void func_001F49B0(void *, void *);
extern void func_L01_002FE498(void);
extern char *func_L01_003010A8(void *position, float scale);
extern int func_L00_00258BC8(int lo, int hi);
extern unsigned char *func_L01_00288A48(void *a, void *b, float f, float g);
extern void func_L01_002888C8(void *a, void *b, float f);

/* WaterSurfaceFxUpdate: update of moby class 751 on level 01, the water surface effect. */
void func_L01_002FE4C0(char *m) {
    int *data;
    int flag;
    int i, j, k, n;
    int cnt;
    char *p;
    char *base;

    data = *(int **)(m + 0x78);
    switch (((unsigned char *)m)[0x20]) {
    case 0: {
        float one = 1.0f;
        float zero = 0.0f;
        float kk = -0.00159999996f;
        base = D_L01_001E3780;
        *(float *)(m + 0x18) = *(float *)(m + 0x18) + 0.5f;
        m[0x20] = 1;
        ((unsigned char *)m)[0x30] = 0xFF;
        func_L01_002B8C00(base, 0x15);
        *(int *)(base + 0x107B4) = 8;
        *(float *)(base + 0xE478) = kk;
        *(float *)(base + 0x1079C) = zero;
        *(float *)(base + 0xE47C) = zero;
        *(float *)(base + 0x11928) = kk;
        *(float *)(base + 0x10798) = kk;
        *(float *)(base + 0xF608) = kk;
        *(float *)(base + 0x1192C) = zero;
        *(float *)(base + 0xF60C) = zero;
        func_L01_002B90A8(one);
        D_L01_001CAF80[0] = 16.0f;
        D_L01_001CAF80[3] = -8.0f;
        D_L01_001CAF80[6] = 0.899999976f;
        D_L01_001CAF80[8] = 0.100000001f;
        D_L01_001CAF80[2] = -8.0f;
        D_L01_001CAF80[5] = one;
        D_L01_001CAF80[4] = one;
        D_L01_001CAF80[1] = 16.0f;
        {
            float a0, b0, x, y, t, u, k6n, k6p;
            float *fv;
            char *p20;
            char *p18;
            a0 = func_L00_001FF860_303590(D_L01_001803C0[4], D_L01_001803C0[5]);
            t = func_001F9F90_FB158c(a0) * 0.57735002f;
            D_L01_001CAF80[12] = t;
            u = func_001F9FA8_FB158c(a0) * 0.57735002f;
            ((unsigned char *)D_L01_001CAF80)[0x3D] = 0x40;
            D_L01_001CAF80[13] = u;
            D_L01_001CAF80[14] = -0.57735002f;
            ((int *)D_L01_001CAF80)[10] = 41;
            ((int *)D_L01_001CAF80)[11] = 42;
            ((unsigned char *)D_L01_001CAF80)[0x3C] = 28;
            D_L01_00161358_s = 21;
            D_L01_00161288 = zero;
            D_L01_0016128C = 32768.0f;
            D_L01_00161290 = 255.0f;
            D_L01_00161294 = 48.0f;
            D_L01_00161350_s = (int)base;
            D_L01_00161354 = (int)D_L01_001FA980;

            p20 = base;
            p18 = D_L01_001FA980;
            i = 20;
            do {
                func_L01_002B8E20((float *)p20, (int *)p18);
                p18 += 0x10;
                i--;
                p20 += 0x1190;
            } while (i >= 0);

            k6n = -6.0f;
            k6p = 6.0f;
            for (i = 0; i < 21; i++) {
                fv = (float *)(D_L01_001E3780 + i * 0x1190);
                j = 1;
                do {
                    a0 = func_002140F8(k6n, k6p);
                    x = fv[0] + a0;
                    b0 = func_002140F8(k6n, k6p);
                    j--;
                    y = fv[1] + b0;
                    func_L01_002B9440_x(D_L01_001E3780, 0x15, x, y, 2.0f, 0.200000003f);
                } while (j >= 0);
            }
        }
        }
        break;
    case 1:
    flag = -1;
    for (i = 0; i < 7; i++) {
        if (data[i] != -1 && func_00215570(D_L01_001672C0, data[i])) {
            float k4n, k4p;
            flag = i;
            k4n = -4.0f;
            k4p = 4.0f;
            for (k = 0; k < D_L01_001FA8C8[i]; k++) {
                int idx = D_L01_001FA8A8[i] + k;
                float c1, c2, x, y;
                *(unsigned short *)(D_L01_001E3780 + idx * 0x1190 + 0x1E) = *(unsigned short *)(D_L01_001FA850 + idx * 4);
                if (func_002140B0(D_L01_001FA8E8[i]) == 0) {
                    c1 = func_002140F8(k4n, k4p);
                    x = *(float *)(D_L01_001E3780 + idx * 0x1190) + c1;
                    c2 = func_002140F8(k4n, k4p);
                    y = *(float *)(D_L01_001E3780 + idx * 0x1190 + 4) + c2;
                    func_L00_002A5158(D_L01_00161350_s + D_L01_001FA8A8[i] * 0x1190 + k * 0x1190, 1, 1, x, y, 1.0f, -0.0500000007f);
                }
            }
        } else {
            cnt = D_L01_001FA8C8[i];
            if (cnt > 0) {
                p = D_L01_001E379E + D_L01_001FA8A8[i] * 0x1190;
                do {
                    *(short *)p = 0;
                    p += 0x1190;
                    cnt--;
                } while (cnt != 0);
            }
        }
    }

    if (flag >= 0) {
        if ((unsigned)(flag - 1) < 3) {
            D_L01_00161285[0] = 0x28;
            D_L01_00161283[0] = 0x40;
            D_L01_00161286[0] = 0x30;
            D_L01_00161280[0] = 0;
            D_L01_00161281[0] = 0x28;
            D_L01_00161282[0] = 0x30;
            D_L01_00161284 = 0;
        } else {
            D_L01_00161284 = 0x18;
            D_L01_00161285[0] = 0x30;
            D_L01_00161286[0] = 0x90;
            D_L01_00161280[0] = 0x18;
            D_L01_00161281[0] = 0x30;
            D_L01_00161282[0] = 0x90;
            D_L01_00161283[0] = 0x30;
        }
    }

    func_L01_002B9198(D_L01_001E3780, 0x15);
    func_001F49B0((void *)func_L01_002FE498, m);

    if (flag == 0 || flag == 6) {
        n = *(int *)&D_L01_00161D14 - 1;
        *(int *)&D_L01_00161D14 = n;
        if (n <= 0) {
            Vec4_FE4C0 v0;
            float x2;
            int idx2 = func_002140B0(5);
            v0.q = *(u128_FE4C0 *)(D_L01_001FA910 + (idx2 << 4));
            v0.f[0] = v0.f[0] + func_002140F8(-0.150000006f, 0.150000006f);
            x2 = func_002140F8(-0.150000006f, 0.150000006f);
            v0.f[1] = v0.f[1] + x2;
            func_L01_003010A8(&v0, 0.200000003f);
            *(int *)&D_L01_00161D14 = func_L00_00258BC8(0x12C, 0x4B0);
        }
    }

    if (flag == 5) {
        Vec4_FE4C0 v0;
        Vec4_FE4C0 v1;
        int rem;
        float f24, f20, c1, c2, c3, x1, y1, yy, zz;
        rem = D_L01_0015F6B0 % 20;
        v0.f[0] = 0.0199999996f;
        *(int *)&v0.f[1] = 0;
        *(int *)&v0.f[2] = 0;
        f24 = (float)D_L01_001FA960[rem] / 20.0f;
        c1 = func_002140F8(-0.600000024f, 0.600000024f);
        v1.f[0] = c1 + 177.0f;
        c2 = func_002140F8(-0.100000001f, 0.100000001f);
        f20 = f24 * -11.0f;
        v1.f[2] = 39.0f;
        f20 = f20 + 176.0f;
        f20 = f20 + c2;
        v1.f[1] = f20;
        c3 = func_002140F8(0.400000006f, 0.5f);
        func_L01_00288A48(&v1, &v0, 1.0f, c3);
        *(int *)&v0.f[0] = 0;
        *(int *)&v0.f[1] = 0;
        *(int *)&v0.f[2] = 0;
        f24 = v0.f[0];
        do {
            if (func_002140B0(0x20) == 0) {
                x1 = func_002140F8(-0.600000024f, 0.600000024f);
                v1.f[0] = x1 + 178.0f;
                y1 = func_002140F8(-0.100000001f, 0.100000001f);
                v1.f[1] = f24 * -11.0f + 176.0f + y1;
                yy = func_002140F8(-0.200000003f, 0.0f);
                v1.f[2] = yy + 39.0f;
                zz = func_002140F8(4.0f, 10.0f);
                func_L01_002888C8(&v1, &v0, zz);
            }
            f24 = f24 + 0.0500000007f;
        } while (f24 < 1.0f);
    }
        break;
    default:
        return;
    }
}
