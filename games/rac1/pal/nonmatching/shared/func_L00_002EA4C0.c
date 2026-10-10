/* NON_MATCHING func_L00_002EA4C0 -- src/overlays/shared/vendor_002E1660.c
 * Best so far: SIZE ours 1276 / retail 1288, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_002EA4C0 (takes `int`, declared so by the caller func_L00_002EB060): turns the moby's basis vectors (
 *   p1.c has the right structure (same call sequence and constants); ours is 1272 vs 1288 bytes. Difference: retai
 *   That is the "repeated lui for one symbol" per-function flag wall; per-use locals (p3.c) and spelling it D_L00_
 *   w16: best is p6.c (g = D_L00_00166F10 hoisted before the if, G locals inlined): prologue now matches. Left: re
 */
extern char D_0013E633[] NOT_SDA;
extern char D_L00_00166F10[];
extern char D_L00_00166F30[] NOT_SDA;
extern short D_L00_00161DF4;
extern void func_001F9BF0(float *, float *, float *);
extern void func_L00_001FF4B0(void *, void *, float);
extern int func_001F9938(void *);
extern float func_001F9C78(void *, void *);
extern void func_001F9C30(void *, void *, float);
extern float func_001F9CB8(void *);
extern void func_001F9CA0(void *, void *, void *);
extern float func_001EC120(void *, float, float, float, float, float);
extern float func_001F9FC0(float);
extern float func_L00_001FF860(float, float);
extern float func_L00_001EB6A8(void *, float, float, float, float, float);
extern float func_001FA790(float, float);
extern void func_002156E0(void *dst, void *vec, void *axis, float angle);
extern float func_001FA888(int);
extern int func_001F9850(int);
extern void func_001F9C08(float, void *, void *, void *);

/* orients the moby's basis vectors toward its target and eases the angle state */
void func_L00_002EA4C0(int a_) {
    char *m = (char *)a_;
    char *d = *(char **)(m + 0x70);
    char *dd = d + 0x40;
    char *s = d + 0x20;
    float v0[4];
    float v1[4];
    float v2[4];
    float v3[4];
    float v4[4];
    char *g;
    char *d2;
    char *k;
    char *up;
    char *fw;
    float a, b, t, bias, ang;
    g = D_L00_00166F10;
    if (*(unsigned char *)(dd + 0xC4) == 0xB) {
        func_001F9BF0((float *)m, (float *)(D_0013E633 + 0xE9D), (float *)(m + 0x30));
        up = m + 0x10;
        func_L00_001FF4B0(m, m, 1.0f);
        fw = m + 0x20;
        func_001F9938(s);
    } else {
        g += 0x20;
        a = func_001F9C78(d + 0x80, g);
        func_001F9C30(v0, g, a);
        func_001F9BF0(v1, (float *)(d + 0x80), v0);
        d2 = *(char **)(m + 0x70);
        k = d2 + 0x130;
        a = func_001F9C78(m + 0x30, g);
        func_001F9C30(v0, g, a);
        func_001F9BF0(v2, (float *)(m + 0x30), v0);
        func_001F9BF0(v3, v1, v2);
        a = func_001F9CB8(v3);
        if (a >= 0.05f) func_001F9C30(m, v3, 1.0f / a);
        up = m + 0x10;
        func_001F9CA0(up, m, D_L00_00166F30);
        func_L00_001FF4B0(up, up, 1.0f);
        *(float *)(s + 8) = func_001EC120(d + 0x30, *(float *)(s + 8), *(float *)(k + 0x30), 0.001f, 0.2f, 0.0f);
        fw = m + 0x20;
        *(float *)(s + 4) = func_001EC120(d + 0x2C, *(float *)(s + 4), *(float *)(dd + 0xB0), 0.001f, 0.2f, 0.0f);
        a = *(float *)(s + 8) - *(float *)(s + 4);
        b = func_001F9C78(d2 + 0x140, D_L00_00166F30);
        func_001F9C30(v0, D_L00_00166F30, -b);
        func_001F9BF0(v0, v0, v3);
        func_001F9BF0(v4, (float *)(m + 0x30), v0);
        func_001F9C30(v0, D_L00_00166F30, -a);
        func_001F9BF0(v4, v4, v0);
        func_001F9BF0(v3, v4, (float *)(m + 0x30));
        a = func_001F9CB8(v3);
        if (a != 0.0f) {
            ang = 1.5707964f - func_001F9FC0(func_001F9C78(m, v3) / a);
            func_001F9CA0(fw, up, m);
            if (func_001F9C78(fw, v3) < 0.0f) ang = -ang;
            b = func_001F9C78(k, D_L00_00166F30);
            t = func_L00_001FF860(func_001F9CB8(k), b) / 0.6981317f;
            bias = 0.0f;
            if (t < -0.1f) {
                t = -t;
                if (0.5f < t) t = 1.0f - t;
                t = t + t;
                bias += t * 0.2617994f;
            }
            *(float *)(s + 0x14) = func_L00_001EB6A8(s + 0x18, *(float *)(s + 0x14), bias, 0.005f, 0.2f, 0.0f);
            ang = func_001FA790(ang, *(float *)(s + 0x14));
            if (1.2217305f < ang) ang = 1.2217305f;
            else if (ang < -1.2217305f) ang = -1.2217305f;
            func_002156E0(m, m, up, ang);
            func_L00_001FF4B0(m, m, 1.0f);
        }
    }
    if (*(short *)s != 0) {
        func_001F9938(s);
        a = func_001FA888(*(short *)s);
        b = func_001FA888(func_001F9850(*(int *)&D_L00_00161DF4));
        func_001F9C08(a / b, m, m, m + 0x40);
        func_L00_001FF4B0(m, m, 1.0f);
    }
    func_001F9CA0(up, m, D_L00_00166F30);
    func_L00_001FF4B0(up, up, 1.0f);
    func_001F9CA0(fw, up, m);
}
