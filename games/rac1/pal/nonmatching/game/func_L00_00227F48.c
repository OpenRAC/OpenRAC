extern unsigned char D_0013D5CA[] NOT_SDA;
extern float D_0015EE6C MACRO_ADDR;
extern unsigned char D_0013E633[] NOT_SDA;
extern int D_0013A5E0[];
extern int D_L00_00167114;
extern int func_001F9850(int);
extern int func_001FFB38(int, int, void *, void *, void *, void *, int, int);
extern int func_L00_00222B80(int, int);
extern int func_L00_001EFFF0(void *, void *, int, int, int);
extern void func_L00_00236BF8(void);
extern void func_L00_00236DE8(void);
extern void func_L00_00236F38(void);

/* Hero camera-or-state timer step: in state 0x11 it counts the timer at +0x22A0
   down and, when it runs out, plays a sound through func_L00_00222B80; in state
   0x12 it tests a probe through func_L00_001EFFF0. Returns 1 when it played. */
int func_L00_00227F48(void) {
    char *s = (char *)D_0013E633 + 0xE1D;
    int q4;
    int q;
    int r, d, quo, v;
    int call;
    int tmp[4];
    int v2[4];
    float *tf = (float *)tmp;
    float *vf = (float *)v2;

    call = 0;
    if (*(int *)(s + 0x208C) == 0x11) {
        if (D_0013D5CA[4] == 0) {
            r = func_001F9850(0x3C);
            d = (r << 4) - r;
            quo = 10000 / d;
            func_001FFB38(4, 0x753F, (void *)func_L00_00236BF8, (void *)func_L00_00236DE8,
                          (void *)func_L00_00236F38, s + 0x22A0, 10000, quo);
            q = quo;
        } else {
            q = 0;
        }
        v = *(int *)(s + 0x22A0) - q;
        *(int *)(s + 0x22A0) = v;
        if (v < 0) {
            *(int *)(s + 0x22A0) = 0;
        }
        q4 = 0x6A;
        if (*(int *)(s + 0x22A0) == 0) {
            if (!(*(float *)(s + 0x2F0) - 2.0f < *(float *)(s + 0x88)
                  && *(float *)(s + 0x94) < -0.87266463f
                  && D_0015EE6C < *(float *)(s + 0x108))) {
                call = 1;
            }
        }
        if (!call) {
            if (*(float *)(s + 0x2F0) + 0.4f < *(float *)(s + 0x88)) {
                D_L00_00167114 = 0;
                q4 = 6;
                call = 1;
            }
        }
        if (call) {
            func_L00_00222B80(q4, 1);
            return 1;
        }
    } else {
        int st = *(int *)(s + 0x2084);
        if (st != 0x6A && st != 0x76) {
            *(int *)(s + 0x22A0) = 10000;
        }
    }

    if (*(int *)(s + 0x208C) != 0x12) {
        return 0;
    }
    tmp[0] = ((int *)(s + 0x80))[0];
    tmp[1] = ((int *)(s + 0x80))[1];
    tmp[2] = ((int *)(s + 0x80))[2];
    tmp[3] = ((int *)(s + 0x80))[3];
    tf[2] = *(float *)(s + 0x2F0) + 0.009999999776482582f;
    v2[0] = tmp[0];
    v2[1] = tmp[1];
    v2[2] = tmp[2];
    v2[3] = tmp[3];
    vf[2] = tf[2] + 0.30000001192092896f;
    if (!func_L00_001EFFF0(tmp, v2, 2, *(int *)(s + 0x2080), 0)) {
        return 0;
    }
    if (D_0013D5CA[2] != 0 && (*(int *)((char *)D_0013A5E0 + 0x2600) & 0xA) != 0) {
        q4 = 0x35;
    } else {
        q4 = 0x33;
    }
    func_L00_00222B80(q4, 1);
    return 1;
}
