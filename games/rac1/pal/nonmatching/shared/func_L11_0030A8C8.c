/* NON_MATCHING func_L11_0030A8C8 -- src/overlays/shared/vendor_002C99E0.c
 * Best so far: SIZE ours 1456 / retail 1480, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Rocket moby update (1480 bytes): eases the moby toward its target with three 002140F8 samples, a flight loop (
 *   Remaining differences: about 40 regions, mostly constant loads and float register choices (retail reloads 1021
 *   Unblock: a full diff listing for the size gap, then the constant and register order of the loop.
 */
extern char D_L11_001B2D80[];
extern char D_L11_001748E0[];
extern float D_0015EE70 MACRO_ADDR;
extern float D_0015EE6C_e __asm__("D_0015EE6C") MACRO_ADDR;
extern float func_002140F8(float, float);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern int func_001F9850(int);
extern char *func_L00_00272158(void *, void *, int, int, int, int, float, float, float, float, float);
extern void func_001F9EC0(void *, void *, void *);
extern float func_00214158(void);
extern void func_002156E0(void *, void *, void *, float);
extern void func_L00_0026DA50(void *, void *, int, int, int, int, float);
extern float func_001F9D10(void *, void *);
extern float func_L00_001FF860(float, float);
extern float func_001F9D48(void *, void *);
extern float func_L00_0025CE58(float *, float *, float, float, float, float);
extern void func_00215C00(void *, float, float, float);
extern void func_L00_00250800(void *, int, void *);
extern int func_L00_001EFFF0(void *, void *, int, void *, void *);
extern void func_L00_0025F4A8(void *, void *, void *, float, float, int, int, int, float, float, float, float, int, float, int, int, int, int);
extern void func_0020D678(void *);
extern int func_001F9908(int *);

// Rocket moby update: eases toward its target, runs the flight loop and spawns its effect.
void func_L11_0030A8C8(char *moby)
{
    char *data;
    char *p5;
    char *p22;
    char *p30;
    char *p24;
    float loc20[4];
    float loc30[4];
    float v40[4];
    float v50[4];
    float loc60[4];
    float f0;
    float f13;
    float f23;
    float t;
    int s;
    char *r5;
    int st;

    data = *(char **)(moby + 0x78);
    qcopy(loc20, moby + 0x10);
    p24 = *(char **)(moby + 0x24);
    p30 = moby + 0x10;
    p22 = moby + 0xC0;
    f0 = *(float *)(p24 + 0x24) - *(float *)(moby + 0x2C);
    *(float *)(moby + 0x2C) = *(float *)(moby + 0x2C) + f0 * 0.1f;
    qzero(loc60);
    loc60[0] = func_002140F8(-1.0f, 1.0f);
    loc60[1] = func_002140F8(-1.0f, 1.0f);
    loc60[2] = func_002140F8(-1.0f, 1.0f);
    qcopy(v50, loc60);
    f0 = func_002140F8(0.1f, 0.2f);
    func_L00_001FF4B0(v50, v50, f0 * D_0015EE6C_e);
    f0 = func_002140F8(D_0015EE6C_e * 0.1f, D_0015EE6C_e);
    func_L00_001FF4B0(v40, data, -f0);
    func_001F9BD8(v50, v50, v40);
    s = func_001F9850(6);
    f23 = 0.0f;
    func_L00_00272158(p30, v50, s, 0x7F, 0xB0B0B0, 3, 40000.0f, 1000.0f, 1.0f, -0.00039999999f, f23);

    do {
        t = f23;
        f23 = t + 0.33333334f;
        f0 = func_002140F8(t, f23);
        func_L00_001FF4B0(v40, data, f0 * *(float *)(data + 0x2C));
        func_001F9BD8(v40, v40, p30);
        s = func_001F9850(0x3C);
        r5 = func_L00_00272158(v40, v50, s, 0x7F, 0x606060, 3, 40000.0f, 1000.0f, 1.0f, -0.000199999995f, 0.0f);
        if (r5 != 0) {
            r5[2] = *(unsigned char *)*(char **)(D_L11_001B2D80 + 0x5C);
            r5[3] = 0x44;
        }
        qzero(loc60);
        loc60[2] = 0.0199999996f;
        func_001F9EC0(loc60, loc60, p22);
        f0 = func_00214158();
        func_002156E0(loc60, loc60, p22, f0);
        s = func_001F9850(5);
        func_L00_0026DA50(v40, loc60, 0x4F007FFF, 0x1FFFFFFF, s, 1, 20000.0f);
    } while (f23 < 1.0f);

    p5 = *(char **)(data + 0x24);
    st = 0;
    if (p5 != 0 && (unsigned char)p5[0x20] != 0xFE && (unsigned char)p5[0x20] != 0xFD) {
        f0 = func_001F9D10(data + 0x10, p5 + 0x10);
        if (f0 < 3.0f) {
            *(float *)(moby + 0x48) = func_L00_001FF860(*(float *)(p5 + 0x10) - *(float *)(moby + 0x10), *(float *)(p5 + 0x14) - *(float *)(moby + 0x14));
            f0 = func_001F9D48(p30, p5 + 0x10);
            *(float *)(moby + 0x44) = -func_L00_001FF860(f0, *(float *)(p5 + 0x18) - *(float *)(moby + 0x18));
            qcopy(data + 0x10, p5 + 0x10);
            goto ACE4_;
        }
    }
    f13 = D_0015EE70;
    f13 = f13 * 251.327408f;
    func_L00_0025CE58((float *)(moby + 0x48), (float *)(data + 0x30), *(float *)(moby + 0x48), f13, f13, D_0015EE6C_e * 2513.27417f);
    f13 = D_0015EE70 * 251.327408f;
    func_L00_0025CE58((float *)(moby + 0x44), (float *)(data + 0x30), *(float *)(moby + 0x44), f13, f13, D_0015EE6C_e * 2513.27417f);
    *(int *)(data + 0x24) = 0;

ACE4_:
    func_00215C00(data, *(float *)(data + 0x2C), *(float *)(moby + 0x48), -*(float *)(moby + 0x44));
    func_001F9BD8(p30, p30, data);
    if (*(float *)(moby + 0x10) < 2.0f) {
        goto AE24;
    }
    if (1021.0f < *(float *)(moby + 0x10)) {
        goto AE24;
    }
    if (*(float *)(moby + 0x14) < 2.0f) {
        goto AE24;
    }
    if (1021.0f < *(float *)(moby + 0x14)) {
        goto AE24;
    }
    if (*(float *)(moby + 0x18) < 2.0f) {
        goto AE24;
    }
    if (1021.0f < *(float *)(moby + 0x18)) {
        goto AE24;
    }
    func_L00_00250800(moby, 1, loc30);
    s = func_L00_001EFFF0(loc20, loc30, 0, (void *)*(int *)(data + 0x20), 0);
    if (s == 0) {
        if (func_001F9908((int *)(data + 0x28)) == 0) {
            goto EPI;
        }
        func_0020D678(moby);
        goto EPI;
    }
    func_L00_0025F4A8(moby, data, D_L11_001748E0, 2.0f, 10.0f, 10, 3, 16, 4.0f, 2.0f, 9.0f, 1.0f, -1, 20.0f, 0, 1, -1, 0);

AE24:
    func_0020D678(moby);

EPI:
    ;
}
