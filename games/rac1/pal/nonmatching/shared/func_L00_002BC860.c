/* NON_MATCHING func_L00_002BC860 -- src/overlays/shared/vendor_002BA7C8.c
 * Best so far: BYTES 9/1448 (99.4% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * Cannot land as written (#define in a candidate): rewrite that in plain C first.
 * What the last attempts found:
 *   UpdateMoby_180: sparkle-quad spawner. State 0 clears three 10-entry short tables; state 2 reads the hero dista
 *   Left: (1) alive branch: retail has `addiu $s7,$sp,0x30` (w base) before `addiu $s6,$s4,1` (i+1); ours swaps th
 *   Idioms that mattered: switch (case 0 / case 2) for the state; D_L00_0016179C/17A0/17A4 as `int/float MACRO_ADD
 */
extern char D_0013E633[];
extern short D_L00_001DC1C0[];
extern short D_L00_001DC1D8[];
extern short D_L00_001DC1F0[];
extern short D_L00_001DC208[];
extern char D_L00_001DBEA0[];
extern char D_L00_001DBE90[];
extern float D_L00_00166EC0[];
extern short D_L00_00161778;
extern short D_L00_0016177C;
extern int D_L00_0016179C MACRO_ADDR;
extern int D_L00_001617A0 MACRO_ADDR;
extern float D_L00_001617A4 MACRO_ADDR;
extern short D_L00_00161774;
extern float D_L00_0017AFBC;
extern int func_001F9850(int);
extern float func_0020D830(void *);
extern int func_001F9908_r(void *) __asm__("func_001F9908");
extern float func_001FA888(int);
extern void func_00213DE0(void *, int, int, int);
extern void func_001F9C30(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_002141A8(void *, float, float);
extern void func_001F9BF0(void *, void *, void *);
extern float func_002140F8(float, float);
extern void func_002156E0(void *dst, void *vec, void *axis, float angle);
extern float func_L00_00258C80(float lo, float hi);
extern void func_001F49B0(void *, void *);
extern void func_L00_002BCE08(int);

#define AA(i) ((float (*)[4])(D_L00_001DBEA0 + (i) * 0x50))
#define BB(i) ((float (*)[4])((i) * 0x50 + D_L00_001DBE90))
// Per-frame update of a moby that scatters ten fading sparkle quads around the hero.
void func_L00_002BC860(char *m) {
    int flag = 0;
    float vec0[4] __attribute__((aligned(16)));
    float vec1[4] __attribute__((aligned(16)));
    float t[4] __attribute__((aligned(16)));
    float w[5][4] __attribute__((aligned(16)));
    char *g = D_0013E633 + 0xE1D;
    int i;
    int n;
    int j;
    int q;
    int any;

    if (*(unsigned char *)(*(char **)(g + 0x2080) + 0x53) != 0x82) {
        D_L00_0017AFBC = 0.1f;
    }
    switch (*(unsigned char *)(m + 0x20)) {
    case 0:
        m[0x20] = 2;
        for (q = 0; q < 10; q++) {
            D_L00_001DC1C0[q] = 0;
            D_L00_001DC1F0[q] = 2;
            D_L00_001DC208[q] = 1;
        }
        D_L00_0016179C = func_001F9850(*(int *)&D_L00_00161778);
        D_L00_001617A0 = func_001F9850(*(int *)&D_L00_0016177C);
        break;
    case 2:
        if (*(int *)(g + 0x2084) == 0x20) {
            float f = func_0020D830(m);
            if (6.0f <= f) {
                flag = 1;
                if (func_001F9908_r(&D_L00_0016179C) == 0) {
                    float a = func_001FA888(D_L00_0016179C);
                    a = a / func_001FA888(func_001F9850(*(int *)&D_L00_00161778));
                    D_L00_001617A4 = 1.0f - a;
                } else if (12.0f <= f) {
                    if (func_001F9908_r(&D_L00_001617A0) == 0) {
                        float a = func_001FA888(D_L00_001617A0);
                        a = a / func_001FA888(func_001F9850(*(int *)&D_L00_0016177C));
                        D_L00_001617A4 = a;
                    } else {
                        D_L00_001617A4 = 0.0f;
                    }
                } else {
                    D_L00_001617A4 = 1.0f;
                }
            }
        } else {
            D_L00_0016179C = func_001F9850(*(int *)&D_L00_00161778);
            D_L00_001617A0 = func_001F9850(*(int *)&D_L00_0016177C);
            func_00213DE0(m, 1, 0, func_001F9850(12));
        }
    }
    func_001F9C30(vec0, m + 0xD0, -0.5f);
    func_001F9BD8(vec0, vec0, m + 0x10);
    any = 0;
    for (j = 0; j < 10; j++) {
        if (D_L00_001DC1C0[j] != 0) {
            any = 1;
            break;
        }
    }
    if (!flag && !any) return;
    for (i = 0; i < 10; i = n) {
        D_L00_001DC1C0[i]--;
        if (D_L00_001DC1C0[i] < 0) D_L00_001DC1C0[i] = 0;
        if (D_L00_001DC1C0[i] == 0) {
            if (flag) {
                float sign;
                int k;
                D_L00_001DC1C0[i] = 2;
                D_L00_001DC1D8[i] = 0x40;
                func_002141A8(vec1, *(float *)&D_L00_00161774, *(float *)&D_L00_00161774);
                qcopy(D_L00_001DBEA0 + i * 0x50, vec0);
                n = i + 1;
                sign = 1.0f;
                for (k = 1; k < 5; k++) {
                    float ang;
                    func_001F9BF0(t, BB(i)[k], D_L00_00166EC0);
                    ang = func_002140F8(0.17453292f, 0.7853982f) * sign;
                    sign = -sign;
                    func_002156E0(vec1, vec1, t, ang);
                    func_001F9BD8(AA(i)[k], BB(i)[k], vec1);
                }
            } else {
                n = i + 1;
            }
        } else {
            int k;
            n = i + 1;
            for (k = 1; k < 5; k++) {
                float *v = w[k];
                func_001F9BF0(v, AA(i)[k], BB(i)[k]);
                v[0] += func_L00_00258C80(0.0f, 0.1f);
                v[1] += func_L00_00258C80(0.0f, 0.1f);
                v[2] += func_L00_00258C80(0.0f, 0.1f);
            }
            qcopy(D_L00_001DBEA0 + i * 0x50, vec0);
            for (k = 1; k < 5; k++) {
                func_001F9BD8(AA(i)[k], BB(i)[k], w[k]);
            }
            if (D_L00_001DC1C0[i] > 0) D_L00_001DC1D8[i] = 0x20;
            else D_L00_001DC1D8[i] = 0;
        }
    }
    func_001F49B0(func_L00_002BCE08, m);
}
