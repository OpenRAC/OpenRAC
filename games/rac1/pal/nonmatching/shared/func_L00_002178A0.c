/* NON_MATCHING func_L00_002178A0 -- src/overlays/shared/help_00214D60.c
 * Best so far: SIZE ours 584 / retail 588, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Hero velocity brake: takes (speed, lim), scales/clamps the velocity vector into local v, builds a second vecto
 *   p5.c is instruction-for-instruction retail (same size) except for one register swap: retail keeps `D_0013E633+
 *   Tried: r reused after `r = r - 0x80` (p5), separate r2 (constant-folds, size 584), w as pointer local (much wo
 */
extern char D_0013E633[] NOT_SDA;
extern int D_L00_00173F40[];
extern float func_L00_00234250(float *v);
extern void func_001F9C30(void *, void *, float);
extern void func_L00_00234420(float *dst, float *src, float z);
extern void func_L00_00233F88(float *dst, float *src, float r);
extern void func_L00_002343A0(float *dst, float *src, float z);
extern void func_001F9BD8(void *, void *, void *);
extern void func_L00_00234090(float *dst, float *src, float dz);
extern int func_L00_001EFFF0(void *, void *, int, int, int);
extern float func_L00_002345B0(float *v);
extern void func_L00_002144A0(float s);

// Brakes the hero's velocity vector toward a computed target, then calls the brake effect if the test passes.
void func_L00_002178A0(float speed, float lim) {
    Vx v;
    Vx w;
    char *q;
    char *q2;
    char *r;
    char *r2;
    float *p;
    float f;
    float g;
    float z2;
    float z;
    p = (float *)(D_0013E633 + 0xEFD);
    if (func_L00_00234250(p) == 0.0f) return;
    q = (char *)p - 0xE0;
    if (*(float *)(q + 0x229C) > 0.5f) {
        if (*(int *)(q + 0x208C) != 6 && *(short *)(q + 0x1F4) == 0 && *(unsigned char *)(q + 0x12E7) == 0) return;
    }
    q2 = D_0013E633 + 0xE1D;
    if (*(short *)(q2 + 0x30E) != 0) return;
    if (*(int *)(q2 + 0x2084) == 0x20) speed = 2.0f;
    z = 0.0f;
    func_001F9C30(&v, q2 + 0xE0, speed);
    if (z < lim) {
        if (func_L00_00234250(&v.x) < lim) {
            func_L00_00234420(&v.x, &v.x, z);
            func_L00_00233F88(&v.x, &v.x, lim);
        }
    }
    z2 = 0.0f;
    func_L00_002343A0(&v.x, &v.x, z2);
    r = D_0013E633 + 0xE9D;
    func_001F9BD8(&v, &v, r);
    r2 = r - 0x80;
    qcopy(&w, &v);
    func_L00_00234090(&v.x, &v.x, 0.3f);
    f = -0.2f;
    if (*(int *)(r2 + 0x2084) == 0x20) f = -0.35f;
    if (*(unsigned char *)(r2 + 0x20A4) == 2) f = -0.7f;
    func_L00_00234090(&w.x, &w.x, f);
    g = 1.0f;
    if (func_L00_001EFFF0(&v, &w, 4, *(int *)(r2 + 0x2080), 0)) {
        if (D_L00_00173F40[7] > 0) {
            if (func_L00_002345B0((float *)&D_L00_00173F40[16]) <= 0.8726646304f) g = z2;
        }
    }
    if (g == 0.0f) return;
    func_L00_002144A0(0.0f);
}
