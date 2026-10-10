/* NON_MATCHING func_L01_002E4BA8 -- src/overlays/shared/vendor_002B90A8.c
 * Best so far: SIZE ours 1756 / retail 1772, checked 2026-10-08.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * No longer builds in its file (COMPILE failed, 2026-10-09): match its declarations to the file's first.
 * What the last attempts found:
 *   Run 1-11 (hq9/s22): dispatch on the 0xBC byte (bltz / slti chain), C58 block first, 523C as a shared `sb 0` th
 *   Still differs: the first try_func call's `$4 = $16` is repeated in each arm of the selector in retail (we set 
 *   Would unblock: a way to get the call-site `$4` copied into both selector arms without a second call; or the al
 */
extern char D_0013E633[];
extern char D_L01_001DEE1C[];
extern float D_0015EE70 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern float D_0015EE60 MACRO_ADDR;
extern float D_0015EE64 MACRO_ADDR;
extern short D_L01_001619B0;
extern short D_L01_001619D4;
extern short D_L01_001619D8;
extern short D_L01_001619DC;
extern void func_001F9BF0(void *, void *, void *);
extern float func_001F9D48(void *, void *);
extern float func_L00_001FF860(float, float);
extern void func_L00_002592B0(char *moby, float *vel, float target, float k, float d, float max);
extern int func_001160D8(void);
extern int func_001F9850(int);
extern void func_00213DE0(void *m, int a, int b, int c);
extern void func_001F9C30(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_L00_001FF548(void *, void *, float);
extern int func_L00_00259B08(int a, int b, int g, int e, float c, float d);
extern float func_001F9CB8(void *a);
extern float func_00214358(void *, int, float);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);

/* steers a moby toward a point and nudges it, with a counter that resets its state */
void func_L01_002E4BA8(char *a, char *b) {
    float v[4];
    char *q = a + 0x10;
    char *ap;
    char *bp;
    char *g;
    char *sel;
    char *tb;
    float s04;
    float f20, f21, f22, f0, f2, t, dx, dy;
    int bb, m, r20;
    unsigned char u53;

    func_001F9BF0(v, b, q);
    if (*(unsigned char *)(a + 0x20) == 6) {
        sel = b + 0x30;
    } else {
        sel = D_0013E633 + 0xE9D;
    }
    f22 = func_001F9D48(q, sel);
    ap = a + 0x10;
    func_001F9D48(ap, b);
    bb = *(unsigned char *)(a + 0xBC);
    if (bb != 0) {
        if (bb < 0) goto L523C;
        if (bb >= 4) goto L523C;
        if (bb < 2) goto L523C;
        m = *(int *)(b + 0x68);
        goto M4D80;
    }

    dx = *(float *)b - *(float *)(a + 0x10);
    dy = *(float *)(b + 4) - *(float *)(a + 0x14);
    t = func_L00_001FF860(dx, dy);
    func_L00_002592B0(a, (float *)D_L01_001DEE1C, t,
                      D_0015EE70 * 11.5191727f, D_0015EE70 * 3.1415927f, D_0015EE6C * 11.5191727f);
    if (*(unsigned char *)(a + 0x70) & 2) {
        int three = 3;
        *(float *)(a + 0x58) = 1.0f;
        u53 = *(unsigned char *)(a + 0x53);
        if (u53 != func_001160D8() % three) {
            int three2 = 3;
            int k = func_001160D8() % three2;
            func_00213DE0(a, k, 0, func_001F9850(20));
        }
    }
    if (1.29999995f < f22) {
        if (*(unsigned char *)(a + 0x53) != 5) {
            func_00213DE0(a, 5, 0, func_001F9850(10));
        }
        goto L5260;
    }
    return;

M4D80:
    if (m == 0) {
        dx = *(float *)b - *(float *)(a + 0x10);
        dy = *(float *)(b + 4) - *(float *)(a + 0x14);
        t = func_L00_001FF860(dx, dy);
        func_L00_002592B0(a, (float *)D_L01_001DEE1C, t,
                          D_0015EE70 * 11.5191727f, D_0015EE70 * 3.1415927f, D_0015EE6C * 11.5191727f);
        goto E68;
    }
    dx = *(float *)(m + 0x10) - *(float *)(a + 0x10);
    dy = *(float *)(m + 0x14) - *(float *)(a + 0x14);
    t = func_L00_001FF860(dx, dy);
    func_L00_002592B0(a, (float *)D_L01_001DEE1C, t,
                      *(float *)&D_L01_001619D4 * 0.0174532924f * D_0015EE70,
                      *(float *)&D_L01_001619D8 * 0.0174532924f * D_0015EE70,
                      *(float *)&D_L01_001619DC * 0.0174532924f * D_0015EE6C);

E68:
    s04 = D_0015EE60 * 0.0399999991f;
    tb = b + 0x10;
    func_001F9C30(v, v, s04);
    bp = tb;
    func_001F9BF0(v, v, bp);
    if (1.29999995f < f22) {
        func_001F9C30(v, v, D_0015EE64 * 0.129999995f);
    } else {
        func_001F9C30(v, v, D_0015EE64 * 0.25f);
    }
    f21 = 1.0f;
    v[2] = v[2] * (D_0015EE60 * -0.100000024f + f21);
    func_001F9BD8(bp, bp, v);
    if (*(unsigned char *)(a + 0x20) == 6) {
        func_L00_001FF548(bp, bp, D_0015EE6C * 2.70000005f);
    }
    r20 = func_L00_00259B08((int)a, (int)bp, *(int *)D_L01_001DEE08, 0, f21,
                            *(float *)(D_L01_001DEE08 + 0x30));
    f21 = func_001F9CB8(bp);
    if (r20 != 0) {
        if (f21 < 0.00499999989f) {
            int raw = *(unsigned short *)(b + 0x4A) + 1;
            int c = (short)raw;
            *(short *)(b + 0x4A) = raw;
            if (func_001F9850(20) < c) {
                if (*(unsigned char *)(a + 0x31) == 0) {
                    qcopy(ap, b);
                    *(short *)(b + 0x4A) = 0;
                }
            }
        }
        f2 = func_00214358(ap, 0, 0.5f);
        if (!(r20 & 2) && !(*(float *)(a + 0x18) - f2 < *(float *)&D_L01_001619B0)) goto L50D4;
    L503C:
        if (f21 < 0.00499999989f) {
            if (*(unsigned char *)(a + 0x53) >= 3) {
                int three = 3;
                u53 = *(unsigned char *)(a + 0x53);
                if (u53 != func_001160D8() % three) {
                    int three2 = 3;
                    int k = func_001160D8() % three2;
                    func_00213DE0(a, k, 0, func_001F9850(10));
                }
            }
            goto L5188;
        }
        goto L5128;
    L50D4:
        if (*(unsigned char *)(a + 0x53) != 5) {
            func_00213DE0(a, 5, 0, func_001F9850(7));
        }
        goto L5188;
    }
    f0 = func_00214358(ap, 0, 0.5f);
    if (*(float *)(a + 0x18) - f0 < *(float *)&D_L01_001619B0) goto L5128;
    goto L5158;

L5128:
    if (*(unsigned char *)(a + 0x53) != 3) {
        func_00213DE0(a, 3, 0, func_001F9850(11));
    }
    goto L5188;

L5158:
    if (*(unsigned char *)(a + 0x53) != 5) {
        func_00213DE0(a, 5, 0, func_001F9850(7));
    }
    *(short *)(b + 0x4A) = 0;

L5188:
    if (*(unsigned char *)(a + 0x20) == 6) return;
    if (f22 < 1.10000002f) {
        g = D_0013E633 + 0xE1D;
        f20 = 1.10000002f - f22;
        f22 = 0.100000001f;
        t = func_L00_001FF860(*(float *)(g + 0x80) - *(float *)(a + 0x10),
                              *(float *)(g + 0x84) - *(float *)(a + 0x14));
        f21 = t;
        f0 = func_001F9F90(f21);
        *(float *)(b + 0x10) = *(float *)(b + 0x10) - (f20 * f0) * (D_0015EE64 * f22);
        f0 = func_001F9FA8(f21);
        f20 = f20 * f0;
        *(float *)(b + 0x14) = *(float *)(b + 0x14) - f20 * (D_0015EE64 * f22);
    }
    return;

L523C:
    *(unsigned char *)(a + 0xBC) = 0;
    if (1.29999995f < f22) {
L5260:
        *(unsigned char *)(a + 0xBC) = 2;
    }
    return;
}
