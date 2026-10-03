/* NON_MATCHING func_L00_002EB3E8 -- src/overlays/shared/vendor_002EB0D8.c
 * Best so far: BYTES 26/1356 (98.1% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Tried: T2* via typedef (struct pointer), nested if, int-typed alias of func_L00_001FF4B0's return
 *   (plain and with the result assigned), reorderings. Looks like a local-alloc priority tie.
 *   2. The second smoothing block loads D_L00_00161E90/94 into $f2/$f1 where retail has $f1/$f2 (first
 *   block matches); operand order of add.s fixed by writing (B - A) * s + A.
 *   Idioms that worked: unsigned char *g (lbu), G = (char*)D_0013E633 + 0xE1D recomputed per block in
 *   separate locals (g2, g3, g4), pad = D_0013A5E0 + 0x2460, exact float constants (1.1170107f etc.),
 *   `z = 0; a[3] = z; a[2] = z;` so the zero stays in a float register, `s = 1.466f; if (!(0 < v)) s = -1.466f;`
 *   Would unblock: a variant that changes the pseudo numbering/priority of the D_L00_00166D80 pointer.
 */
#include "common.h"

extern int D_0015EED0[] MACRO_ADDR;
extern char D_0013E633[];
extern char D_0013A5E0[];
typedef struct { char pad[0x270]; short s; } T2;
extern T2 D_L00_00166D80_t __asm__("D_L00_00166D80");
extern short D_L00_00161E90;
extern short D_L00_00161E94;
extern short D_L00_00161E98;
extern short D_L00_00161E9C;
extern short D_L00_00161EA0;
extern void func_L00_002EB170(void *);
extern int func_L00_001FF4B0_i(void *, void *, float) __asm__("func_L00_001FF4B0");
extern float func_001FA888(int);
extern float func_001F9B88(float);
extern float func_001EC120(void *, float, float, float, float, float);
extern void func_002156E0(void *, void *, void *, float);
extern void func_001F9CA0(void *, void *, void *);
extern float func_001FA748(float, float);
extern float func_001FA790(float, float);
extern void func_001F9BD8(void *, void *, void *);
extern int func_L00_001F10E0(float, void *, int, void *);

/* camera update: turns the pad stick input into yaw and pitch of the camera and rebuilds its matrix */
void func_L00_002EB3E8(char *m) {
    float mat[4];
    float w[4];
    float a[4];
    float *d = *(float **)(m + 0x70);
    unsigned char *g;
    T2 *h;
    unsigned char *g2, *g3, *g4;
    char *pad;
    float v, k, r, s, x, f, z;
    int r0;
    func_L00_002EB170(m);
    g = (unsigned char *)D_0013E633 + 0xE1D;
    r0 = func_L00_001FF4B0_i(mat, *(char **)(g + 0x2080) + 0xE0, 1.0f);
    h = &D_L00_00166D80_t;
    if (h->s == 0) {
        if (g[0x20A5] == 0)
            g[0x20A5] = 1;
    }
    pad = D_0013A5E0 + 0x2460;
    v = -*(float *)(pad + 0x108);
    if (v == 0)
        v = -*(float *)(pad + 0x100);
    if (v == 0)
        v = func_001FA888((*(int *)(pad + 0x1A0) >> 15) & 1);
    if (v == 0)
        v = -func_001FA888((*(int *)(pad + 0x1A0) >> 13) & 1);
    s = func_001F9B88(d[0]);
    k = *(float *)&D_L00_00161E90 + (*(float *)&D_L00_00161E94 - *(float *)&D_L00_00161E90) * s;
    r = func_001EC120(d + 1, d[0], v, *(float *)&D_L00_00161E98, *(float *)&D_L00_00161E9C, *(float *)&D_L00_00161EA0);
    d[0] = r;
    if (r != 0)
        func_002156E0(m, m, mat, k * r);
    g2 = (unsigned char *)D_0013E633 + 0xE1D;
    f = *(float *)(g2 + 0x14C);
    if (f != 0 && *(short *)(g2 + 0x308) == 0)
        func_002156E0(m, m, mat, f);
    pad = D_0013A5E0 + 0x2460;
    v = -*(float *)(pad + 0x10C);
    if (v == 0)
        v = -*(float *)(pad + 0x104);
    if (v == 0)
        v = func_001FA888((*(int *)(pad + 0x1A0) >> 12) & 1);
    if (v == 0)
        v = -func_001FA888((*(int *)(pad + 0x1A0) >> 14) & 1);
    h = &D_L00_00166D80_t;
    g3 = (unsigned char *)D_0013E633 + 0xE1D;
    if (h->s != 0 && *(int *)(g3 + 0x2090) == 2)
        v = 0;
    if (D_0015EED0[3] == 0)
        v = -v;
    s = func_001F9B88(d[2]);
    k = (*(float *)&D_L00_00161E94 - *(float *)&D_L00_00161E90) * s + *(float *)&D_L00_00161E90;
    r = func_001EC120(d + 3, d[2], v, *(float *)&D_L00_00161E98, *(float *)&D_L00_00161E9C, *(float *)&D_L00_00161EA0);
    d[2] = r;
    if (r != 0) {
        func_001F9CA0(w, m, mat);
        x = k * d[2];
        v = func_001F9B88(func_001FA748(d[4], x));
        if (1.1170107f < v && func_001F9B88(d[4]) < v)
            x = x + (0.0f - x) * (func_001FA790(v, 1.1170107f) / 0.3490659f);
        if (1.1152654f <= v && 0 < d[4])
            d[5] = func_001EC120(d + 6, d[5], 0.5f, 0.005f, 0.2f, 0);
        else
            d[5] = func_001EC120(d + 6, d[5], 0, 0.005f, 0.2f, 0);
        a[0] = *(float *)m;
        a[1] = *(float *)(m + 4);
        z = 0;
        a[3] = z;
        a[2] = z;
        func_L00_001FF4B0_i(a, a, d[5]);
        func_001F9BD8(m + 0x30, m + 0x30, a);
        g4 = (unsigned char *)D_0013E633 + 0xE1D;
        if (func_L00_001F10E0(0.3f, m + 0x30, 0x12, *(char **)(g4 + 0x2080)) != 0)
            d[5] = func_001EC120(d + 6, d[5], 0, 0.005f, 0.2f, 0);
        v = func_001FA748(d[4], x);
        if (1.4660766f < func_001F9B88(v))
            {
            s = 1.4660766f;
            if (!(0 < v))
                s = -1.4660766f;
            x = func_001FA790(s, d[4]);
        }
        func_002156E0(m, m, w, x);
        d[4] = func_001FA748(d[4], x);
    }
    func_L00_001FF4B0_i(m, m, 1.0f);
    func_001F9CA0(m + 0x10, m, mat);
    func_L00_001FF4B0_i(m + 0x10, m + 0x10, 1.0f);
    func_001F9CA0(m + 0x20, m + 0x10, m);
}
