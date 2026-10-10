/* NON_MATCHING func_L11_00318C90 -- src/overlays/l11_pokitaru/vendor_00312BD8.c
 * Best so far: SIZE ours 1156 / retail 1144, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Pokitaru list update (1144 bytes): for each entry of the moby's list, finds the two nearest neighbours by dist
 *   Left: the loop head and the first divide-by-zero checks (retail keeps the table pointer and index in saved reg
 *   Unblock: rebuild the loop head from the assembly (index in $30, 2i in $21, 4i in $20) and check the neighbour-
 */
extern void func_L00_001FF4B0_313a70(void *, void *, float) __asm__("func_L00_001FF4B0");
extern void func_001F9BD8_313a70(void *, void *, void *) __asm__("func_001F9BD8");
extern float func_001F9D10(void *, void *);
extern int func_0022ED80_c(int, int, int) __asm__("func_0022ED80");
extern int func_001F9850(int);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern void func_001F9C08(void *, void *, void *, float);
extern float func_L00_001FF860(float, float);
extern float func_001F9D48(float *, float *);
extern float func_001FA790(float, float);
extern float func_001FA748(float, float);
extern void func_L00_00251E30(void *);
extern int D_L11_0015F6A8 MACRO_ADDR;
extern float D_L11_001623B8 SDATA(D_L11_001623B8);
extern float D_L11_001623B4 SDATA(D_L11_001623B4);
extern int D_L11_001623BC SDATA(D_L11_001623BC);

/* Pokitaru: for each entry of the moby's list, finds the nearest neighbours and sets their blend weights. */
void func_L11_00318C90(char *m)
{
    char *d = *(char **)(m + 0x78);
    char *tbl;
    short *X;
    int *Y;
    float *Z;
    float *W;
    float v[4];
    int i;
    int x, nx, nn, dv, t, t2;
    float f20, f21, f22, f23, f0;
    char *o;
    char *d2;

    func_L00_001FF4B0_313a70(v, m + 0xC0, D_L11_001623B8);
    func_001F9BD8_313a70(v, v, m + 0x10);
    tbl = *(char **)(d + 0xF0);
    if (*(int *)(d + 0x64) <= 0) return;
    X = (short *)(d + 0x90);
    Y = (int *)(d + 0x70);
    Z = (float *)(d + 0xA0);
    W = (float *)(d + 0xC0);
    for (i = 0; i < *(int *)(d + 0x64); i++) {
        dv = *(int *)tbl;
        x = X[i];
        nx = (x + 1) % dv;
        nn = (x + 2) % dv;
        f21 = func_001F9D10(tbl + x * 16 + 0x10, v);
        f20 = func_001F9D10(tbl + nx * 16 + 0x10, v);
        while (D_L11_001623B4 < f20) {
            x = nx;
            nx = (x + 1) % dv;
            f21 = f20;
            nn = (nx + 1) % dv;
            f20 = func_001F9D10(tbl + nx * 16 + 0x10, v);
        }
        if (*(float *)(tbl + x * 16 + 0x1C) == 42.0f) {
            o = (char *)Y[i];
            d2 = *(char **)(o + 0x78);
            if (*(int *)(d2 + 0x28) == 0) {
                if (D_L11_0015F6A8 == 0) func_0022ED80_c(2, 0, (int)o);
                *(int *)(d2 + 0x28) = func_001F9850(D_L11_001623BC);
            }
        }
        f0 = D_L11_001623B4;
        if (f0 < f21) f23 = (f21 - f0) / (f21 - f20);
        else f23 = 0.0f;
        X[i] = x;
        f21 = 0.0f;
        Z[i] = f23;
        if (W[i] != 0.0f) {
            f20 = f23 - W[i];
            t = func_001FA898_r(f20);
            t2 = func_001FA898_r(f20);
            f20 = f20 - (float)t2;
            if (f20 < f21) {
                t = t - 1;
                f20 = f20 + 1.0f;
            }
            f23 = f20;
            x = (x + t + dv) % dv;
            nx = (nx + t + dv) % dv;
            nn = (nn + t + dv) % dv;
        }
        o = (char *)Y[i];
        func_001F9C08(o + 0x10, tbl + x * 16 + 0x10, tbl + nx * 16 + 0x10, f23);
        qcopy(v, o + 0x10);
        f22 = func_L00_001FF860(*(float *)(tbl + x * 16 + 0x10) - *(float *)(tbl + nx * 16 + 0x10),
                                *(float *)(tbl + x * 16 + 0x14) - *(float *)(tbl + nx * 16 + 0x14));
        f0 = func_L00_001FF860(*(float *)(tbl + nx * 16 + 0x10) - *(float *)(tbl + nn * 16 + 0x10),
                               *(float *)(tbl + nx * 16 + 0x14) - *(float *)(tbl + nn * 16 + 0x14));
        f21 = func_001F9D48((float *)(tbl + nx * 16 + 0x10), (float *)(tbl + x * 16 + 0x10));
        f0 = func_L00_001FF860(f21, *(float *)(tbl + x * 16 + 0x18) - *(float *)(tbl + nx * 16 + 0x18));
        f20 = -f0;
        f0 = func_001F9D48((float *)(tbl + nn * 16 + 0x10), (float *)(tbl + nx * 16 + 0x10));
        f0 = func_L00_001FF860(f0, *(float *)(tbl + nx * 16 + 0x18) - *(float *)(tbl + nn * 16 + 0x18));
        f0 = func_001FA790(-f0, f20);
        f0 = func_001FA748(f0 * f23, f20);
        *(float *)(o + 0x44) = f0;
        f0 = func_001FA790(f21, f22);
        f0 = func_001FA748(f0 * f23, f22);
        *(float *)(o + 0x48) = f0;
        func_L00_00251E30(o);
        func_L00_001FF4B0_313a70(v, o + 0xC0, D_L11_001623B8);
        func_001F9BD8_313a70(v, v, o + 0x10);
    }
}
