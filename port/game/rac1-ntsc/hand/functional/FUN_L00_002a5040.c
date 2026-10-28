/* Carried by port/tools/versionmap/carry_functions.py from the decompilation of Ratchet & Clank (PAL), src/overlays/shared/bolts_and_wrench.c (games/rac1): func_L00_002A62D0, matched C there.
 * Kind `same` in the version map (../../README.md): the two versions have the same instructions for it.
 * Names and addresses are this version's. Functional C, not matched C. */
/* carry, not from a `same` function or an `exact` data address (read them):
 *   0x00808080: no place found, left as it is
 *   D_L00_0016145C -> 00161394: between
 *   D_L00_00161460 -> 00161398: between
 */

extern void func_001FA030(void *, void *);
typedef struct { char pad[0x254]; int p; char pad2[0x18]; short s; char pad3[1]; char c; char pad4[0x14]; float f0; char pad5[8]; float f1; } S_c00_2a5040;
extern S_c00_2a5040 D_L00_00166C80;
extern void func_001F9A10(void *, void *, void *);
typedef unsigned int Q_2A62D0_c00_2a5040 __attribute__((mode(TI), aligned(16)));
typedef union { Q_2A62D0_c00_2a5040 q; float f[4]; } V_2A62D0_c00_2a5040;
typedef struct {
    float v[4][4];
    int col[4];
    float uv[4][2];
} Quad_2A62D0_c00_2a5040;
extern short D_L00_00161394;
extern short D_L00_00161398;
extern float D_L00_001D7360[][4];
extern float D_L00_001D73F0[][2];
extern int D_L00_001D7438[];
extern int D_L00_001D7460[][4];
extern void func_001FA030(void *, void *);
extern int func_001F44B8(int);
extern void func_001F9CF8(void *, void *, void *);
extern void func_001F9A10(void *, void *, void *);
extern void func_L00_001FD228(void *, void *, int);
extern float func_001F9DC8(float);
extern float func_001F9DE0(float);
extern void func_L00_00263618(void *, int, float, float);
typedef struct { float x, y, z, w; } __attribute__((aligned(16))) VendorGlowCorner_c00_2a5040;
void FUN_L00_002a5040(char *m) {
    char *p = *(char **)(m + 0x78);
    Quad_2A62D0_c00_2a5040 qd;
    long pk[4];
    float q[4][4];
    float d0[4];
    float mtx[3][4];
    float pi;
    float half;
    float zero;
    char *g, *h;
    float (*t4)[2]; float (*t0)[2];
    int izero;
    float off;
    int k, i, i0, i1, i2, i3, col;
    h = (char *)&D_L00_00166C80;
    d0[2] = *(float *)(h + 0x158);
    d0[0] = 0.0f;
    d0[1] = 0.0f;
    func_001FA030(mtx, d0);
    (*(float *)&D_L00_00161394) = (*(float *)&D_L00_00161394) + 0.01f;
    if (1.0f < (*(float *)&D_L00_00161394)) {
        (*(float *)&D_L00_00161394) = (*(float *)&D_L00_00161394) - 1.0f;
    }
    pk[2] = 1;
    pk[3] = 0x4000000044L;
    pk[0] = 0;
    pk[1] = func_001F44B8(0x18);
    for (k = 0; k < 4; k++) {
        func_001F9CF8(q[0], D_L00_001D7360[D_L00_001D7460[k][0]], mtx);
        func_001F9CF8(q[1], D_L00_001D7360[D_L00_001D7460[k][1]], mtx);
        func_001F9CF8(q[2], D_L00_001D7360[D_L00_001D7460[k][2]], mtx);
        func_001F9CF8(q[3], D_L00_001D7360[D_L00_001D7460[k][3]], mtx);
        qd.v[0][0] = *(float *)(m + 0x10) + q[0][0] * *(float *)(p + 0x90);
        qd.v[0][1] = *(float *)(m + 0x14) + q[0][1] * *(float *)(p + 0x90);
        qd.v[0][2] = *(float *)(m + 0x18) + q[0][2];
        qd.v[1][0] = *(float *)(m + 0x10) + q[1][0] * *(float *)(p + 0x90);
        qd.v[1][1] = *(float *)(m + 0x14) + q[1][1] * *(float *)(p + 0x90);
        qd.v[1][2] = *(float *)(m + 0x18) + q[1][2];
        qd.v[2][0] = *(float *)(m + 0x10) + q[2][0] * *(float *)(p + 0x90);
        qd.v[2][1] = *(float *)(m + 0x14) + q[2][1] * *(float *)(p + 0x90);
        qd.v[2][2] = *(float *)(m + 0x18) + q[2][2];
        qd.v[3][0] = *(float *)(m + 0x10) + q[3][0] * *(float *)(p + 0x90);
        qd.v[3][1] = *(float *)(m + 0x14) + q[3][1] * *(float *)(p + 0x90);
        qd.v[3][2] = *(float *)(m + 0x18) + q[3][2];
        i0 = D_L00_001D7460[k][0];
        i1 = D_L00_001D7460[k][1];
        i2 = D_L00_001D7460[k][2];
        i3 = D_L00_001D7460[k][3];
        off = (*(float *)&D_L00_00161394);
        qd.col[0] = D_L00_001D7438[i0];
        qd.col[1] = D_L00_001D7438[i1];
        qd.col[2] = D_L00_001D7438[i2];
        qd.col[3] = D_L00_001D7438[i3];
        t0 = D_L00_001D73F0;
        t4 = (float (*)[2])&t0[0][1];
        qd.uv[0][0] = t0[i0][0];
        qd.uv[0][1] = t4[i0][0] + off;
        qd.uv[1][0] = t0[i1][0];
        qd.uv[1][1] = t4[i1][0] + off;
        qd.uv[2][0] = t0[i2][0];
        qd.uv[2][1] = t4[i2][0] + off;
        qd.uv[3][0] = t0[i3][0];
        qd.uv[3][1] = t4[i3][0] + off;
        func_L00_001FD228(&qd, 0, 1);
    }
    pk[1] = func_001F44B8(0x1B);
    (*(float *)&D_L00_00161398) = (*(float *)&D_L00_00161398) + 0.01f;
    if (1.0f < (*(float *)&D_L00_00161398)) {
        (*(float *)&D_L00_00161398) = 0.0f;
    }
    {
        float ph = (*(float *)&D_L00_00161398);
        float n = -ph * 1.2f;
        float pp = ph * 1.2f;
        float z = ph * 1.4f + 1.1f;
        float zz = z - 0.2f;
        VendorGlowCorner_c00_2a5040 c[4] = {
            { n * *(float *)(p + 0x90), n * *(float *)(p + 0x90), z },
            { pp * *(float *)(p + 0x90), n * *(float *)(p + 0x90), zz },
            { n * *(float *)(p + 0x90), pp * *(float *)(p + 0x90), z },
            { pp * *(float *)(p + 0x90), pp * *(float *)(p + 0x90), zz },
        };
    g = (char *)&D_L00_00166C80;
    off = *(float *)(g + 0x158);
    d0[0] = 0.0f;
    d0[2] = off;
    d0[1] = 0.0f;
    func_001FA030(mtx, d0);
    func_001F9CF8(q[0], &c[0], mtx);
    func_001F9CF8(q[1], &c[1], mtx);
    func_001F9CF8(q[2], &c[2], mtx);
    func_001F9CF8(q[3], &c[3], mtx);
    }
    col = ((int)((1.0f - (*(float *)&D_L00_00161398)) * 128.0f) << 24) | 0x808080;
    qd.v[0][0] = *(float *)(m + 0x10) + q[0][0];
    qd.v[0][1] = *(float *)(m + 0x14) + q[0][1];
    qd.v[0][2] = *(float *)(m + 0x18) + q[0][2];
    qd.v[1][0] = *(float *)(m + 0x10) + q[1][0];
    qd.v[1][1] = *(float *)(m + 0x14) + q[1][1];
    qd.v[1][2] = *(float *)(m + 0x18) + q[1][2];
    qd.v[2][0] = *(float *)(m + 0x10) + q[2][0];
    qd.v[2][1] = *(float *)(m + 0x14) + q[2][1];
    qd.v[2][2] = *(float *)(m + 0x18) + q[2][2];
    qd.v[3][0] = *(float *)(m + 0x10) + q[3][0];
    qd.v[3][1] = *(float *)(m + 0x14) + q[3][1];
    qd.v[3][2] = *(float *)(m + 0x18) + q[3][2];
    qd.col[0] = col;
    qd.col[1] = col;
    qd.col[2] = col;
    qd.col[3] = col;
    qd.uv[0][0] = 0.0f;
    qd.uv[0][1] = 0.0f;
    qd.uv[1][0] = 1.0f;
    qd.uv[1][1] = 0.0f;
    qd.uv[2][0] = 0.0f;
    qd.uv[2][1] = 1.0f;
    qd.uv[3][0] = 1.0f;
    qd.uv[3][1] = 1.0f;
    func_L00_001FD228(&qd, 0, 1);
    izero = 0;
    zero = *(float *)&izero;
    pi = 3.14159f;
    half = 0.5f;
    for (i = 0; i < 4; i++) {
        V_2A62D0_c00_2a5040 S_c00_2a5040;
        float ang = (float)i * pi * half - pi;
        S_c00_2a5040.f[0] = func_001F9DC8(ang) * 1.1f;
        S_c00_2a5040.f[1] = func_001F9DE0(ang) * 1.1f;
        S_c00_2a5040.f[2] = 0.59f;
        func_001F9CF8(S_c00_2a5040.f, S_c00_2a5040.f, m + 0xC0);
        func_001F9A10(S_c00_2a5040.f, S_c00_2a5040.f, m + 0x10);
        func_L00_00263618(S_c00_2a5040.f, *(int *)(m + 0x90) & 0xFFFF0000, 0.1333f, zero);
    }
}
extern float func_001F9DE0(float);
extern void FUN_L00_002a5040();
extern int func_001F44B8(int);
extern void func_L00_001FD228(void *, void *, int);
extern float func_001F9DC8(float);
extern int func_001F44B8(int);
extern void func_L00_001FD228(void *, void *, int);
