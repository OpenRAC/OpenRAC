/* NON_MATCHING func_L00_002B1290 -- src/overlays/shared/vendor_002AB910.c
 * Best so far: SIZE ours 1008 / retail 1012, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Finds the nearest aimable moby in the D_L00_001ABD80 list (distance < 40, valid, line-of-sight via func_L00_00
 *   p3.c matches retail in structure and register allocation (range test as nested ifs, b = D_0013E633+0xE1D in th
 *   Wall: the dead load has no plain-C form I found; would need knowing what the original did with D_L00_00173F40[
 */
extern float func_001F9D10(void *, void *);
extern float func_L00_001FF860(float, float);
extern float func_001F9D48(void *, void *);
extern float func_001FA850(float, float);
extern float func_001F9B88(float);
extern float func_001FA888(int);
extern void func_00215C00(void *, float, float, float);
extern void func_001F9BD8(void *, void *, void *);
extern float func_001F9FC0(float);
extern int func_L00_001EFFF0(void *, void *, int, int, int);
extern int func_L00_002608F0(char *);
extern char *D_L00_001ABD80[];
extern char D_L00_00166EC0[];
extern int D_L00_00173F40[];
extern char D_0013E633[];
extern unsigned char D_0013E15A[];

/* Finds the closest target moby in the list that the object at P can aim at, filling OUT with its aim. */
void func_L00_002B1290(char *p, char *out, char *exclude) {
    float v[4] __attribute__((aligned(16)));
    float w[4] __attribute__((aligned(16)));
    float best = 10000.0f;
    float f24 = *(float *)(p + 0x44);
    float f25 = *(float *)(p + 0x48);
    unsigned char *flag = D_0013E15A + 0x4C6;
    int i;
    char *m;

    for (i = 0; (m = D_L00_001ABD80[i]) != 0; i++) {
        char *b = D_0013E633 + 0xE1D;
        char *mp;
        char *pp;
        int q;
        float f20, f21, f22, f23, h;
        int k;
        if (m[0x20] < 0) continue;
        if (m == exclude) continue;
        mp = m + 0x10;
        qcopy(v, mp);
        pp = p + 0x10;
        if (func_001F9D10(v, pp) > 40.0f) continue;
        q = func_L00_0025D390(m);
        if (q != 0) {
            if (q > 0x100000) {
                if (q <= 0x3FFFFF) {
                    v[2] = v[2] + *(float *)(q + 0x10);
                    goto ok;
                }
            }
        }
        q = 0;
        v[2] = v[2] + 0.5f;
    ok:
        if (m != 0 && *(char **)(m + 0x24) != 0 && *(short *)(*(char **)(m + 0x24) + 0x46) == 5) {
            char *pl;
            f23 = func_L00_001FF860(v[0] - *(float *)(p + 0x10), v[1] - *(float *)(p + 0x14));
            f22 = func_L00_001FF860(func_001F9D48(pp, v), v[2] - *(float *)(p + 0x18));
            f21 = func_001F9D10(pp, v);
            if (f21 < 2.5f) {
                pl = *(char **)(b + 0x2080);
                if (func_001FA850(*(float *)(pl + 0x48), func_L00_001FF860(*(float *)(m + 0x10) - *(float *)(b + 0x80), *(float *)(m + 0x14) - *(float *)(b + 0x84))) < 1.0471976f) {
                    if (func_001F9B88(f22) < 0.7853982f) return;
                }
            }
            if (f21 > best) continue;
            f20 = func_001FA888(flag[0xB]) * 0.6981317f;
            func_00215C00(w, f21, f25, f24 + *(float *)(b + 0x2E4) * 0.5f);
            func_001F9BD8(w, w, pp);
            h = func_001F9FC0(func_001F9D10(w, v) / (f21 + f21));
            h = h + h;
            f20 = f20 + 1.5707964f;
            if (f20 < h) {
                if (q != 0) {
                    f20 = func_001FA888(*(unsigned char *)(q + 0xA) << 3) * 0.125f;
                    func_001F9FC0(f20 / f21);
                }
            }
            k = 1;
            if (func_L00_001EFFF0(D_L00_00166EC0, v, 6, (int)p, 0)) {
                (void)*(volatile int *)&D_L00_00173F40[6];
                continue;
            }
            best = f21;
            f24 = -f22;
            f25 = f23;
            if (func_L00_002608F0(m) == 0) {
                float hh = 0.5f;
                *(char **)(out + 0x18) = m;
                if (q != 0) hh = *(float *)(q + 0x10);
                *(float *)(out + 0x2C) = f23;
                *(int *)(out + 0x38) = k;
                *(float *)(out + 0x30) = f24;
                *(int *)(out + 0x24) = 0;
                *(int *)(out + 0x28) = 0;
                *(int *)(out + 0x20) = 0;
                qcopy(out, mp);
                *(float *)(out + 0x1C) = hh;
            }
        }
    }
}
