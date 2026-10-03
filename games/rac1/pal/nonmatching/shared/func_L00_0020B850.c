/* NON_MATCHING func_L00_0020B850 -- src/overlays/shared/help_00203E98.c
 * Best so far: BYTES 33/992 (96.7% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_0020B850: per-frame hero camera/pad update: fabs-threshold tests on six pad floats gate a yaw/pitch c
 *   Best is p4.c (BYTES 33/992, same size, instruction order identical): the only difference is register allocatio
 *   Tried: g/g2/g3/g4/g5 as separate pointer locals (required to get retail's repeated `addiu $x,$s3,lo`), declara
 */
extern char D_L00_0017A780[];
extern int func_L00_0020DC00();
extern float func_001F9B88(float);
extern float func_L00_001FF860(float, float);
extern float func_001FA790(float, float);
extern void func_L00_00233B08(float a, float b, short c, unsigned char d, unsigned char e, unsigned char f);
extern int func_001F9850(int);
extern void func_L00_00250800(void *, int, void *);
extern float func_002140F8(float, float);
extern void func_L00_0026ED30(void *, void *, float);

/* Per-frame update of the hero's camera-pad response and the shake/rumble countdown. */
void func_L00_0020B850(void) {
    char *g, *g2, *g3;
    char *s;
    char *s2;
    float x, y;
    char *t;
    float a, b;
    int i, j;
    float v[4];
    if (func_L00_0020DC00() != 0) return;
    g = (char *)D_0013E633 + 0xE1D;
    if (func_001F9B88(*(float *)(g + 0x2C0)) > 0.03f
        || func_001F9B88(*(float *)(g + 0x2C4)) > 0.03f
        || func_001F9B88(*(float *)(g + 0x2C8)) > 0.06981317f
        || func_001F9B88(*(float *)(g + 0x2CC)) > 0.06981317f
        || func_001F9B88(*(float *)(g + 0x2D0)) > 0.06981317f
        || func_001F9B88(*(float *)(g + 0x2D4)) > 0.06981317f) {
        g2 = (char *)D_0013E633 + 0xE1D;
        if (*(short *)(g2 + 0x308) != 1) {
            s = D_L00_0017A780;
            x = *(float *)(g2 + 0x2C8);
            y = *(float *)(g2 + 0x2CC);
            *(float *)(s + 0x324) = -x;
            *(float *)(s + 0x3D4) = -y;
            a = func_L00_001FF860(0.2f, *(float *)(g2 + 0x2C0)) * 1.2f;
            b = -func_L00_001FF860(0.2f, *(float *)(g2 + 0x2C4)) * 1.2f;
            if (a > 0.7853982f) a = 0.7853982f;
            if (a < -0.17453292f) a = -0.17453292f;
            if (b > 0.17453292f) b = 0.17453292f;
            if (b < -0.7853982f) b = -0.7853982f;
            *(float *)(s + 0x480) = a;
            *(float *)(s + 0x530) = b;
            *(float *)(s + 0x320) = -func_001FA790(*(float *)(g2 + 0x2D0), -a);
            *(float *)(s + 0x3D0) = -func_001FA790(*(float *)(g2 + 0x2D4), -b);
        }
    }
    g3 = (char *)D_0013E633 + 0xE1D;
    if (*(int *)(g3 + 0x10E0) != 0) {
        s2 = D_L00_0017A780;
        *(float *)(s2 + 0x36C) = 0.01f;
        *(float *)(s2 + 0x41C) = 0.01f;
    }
    if (!(*(unsigned short *)(g3 + 0x12AE) & 0x80)) {
    if (*(int *)(g3 + 0x2084) == 3) func_L00_00233B08(0.021f, 0.002f, 1, 2, 1, 0);
    if (*(int *)(g3 + 0x208C) == 4) {
        if (*(int *)(g3 + 0x418) > 0) {
            if (*(int *)(g3 + 0x418) < func_001F9850(0xC))
                func_L00_00233B08(0.016f, 0.002f, 1, 2, 0, 1);
        }
    }
    {
    char *g4 = (char *)D_0013E633 + 0xE1D;
    if (*(int *)(g4 + 0x12A8) > 0 && *(short *)(g4 + 0x12AC) != 0) {
        t = 0;
        if (*(unsigned short *)(g4 + 0x12AE) & 1) t = (char *)*(int *)(g4 + 0x2080) + 0xC0;
        *(int *)(g4 + 0x12A8) = *(int *)(g4 + 0x12A8) - 1;
        func_L00_00250800((void *)*(int *)(g4 + 0x2080), 0x16, v);
        i = 0;
        if (*(short *)(g4 + 0x12AC) > 0) {
            do {
                i++;
                func_L00_0026ED30(v, t, *(float *)(g4 + 0x12A0) + func_002140F8(-*(float *)(g4 + 0x12A4), *(float *)(g4 + 0x12A4)));
            } while (i < *(short *)(g4 + 0x12AC));
        }
        {
        char *g5 = (char *)D_0013E633 + 0xE1D;
        func_L00_00250800((void *)*(int *)(g5 + 0x2080), 0x17, v);
        j = 0;
        if (*(short *)(g5 + 0x12AC) > 0) {
            do {
                j++;
                func_L00_0026ED30(v, t, *(float *)(g5 + 0x12A0) + func_002140F8(-*(float *)(g5 + 0x12A4), *(float *)(g5 + 0x12A4)));
            } while (j < *(short *)(g5 + 0x12AC));
        }
        }
    }
    }
    } else {
        *(int *)(g3 + 0x12A8) = 0;
    }
}
