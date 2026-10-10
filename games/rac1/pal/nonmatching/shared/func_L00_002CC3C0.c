/* NON_MATCHING func_L00_002CC3C0 -- src/overlays/shared/vendor_002C96D0.c
 * Best so far: SIZE ours 1180 / retail 1188, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_002CC3C0 (1188 bytes): builds a moby's data block from a source moby and a 16-byte position vector, r
 *   Stopped after 5 runs (best p3.c, SIZE 1180 vs 1188). Differences are structural, not one tie: retail keeps a c
 *   Harness note: the candidate prelude already holds a u128 typedef at the end of the file; a u128 typedef in the
 */
typedef int w128 __attribute__((mode(TI)));

extern char *func_0020D348_2CD3B8(int) __asm__("func_0020D348");
extern float func_001FA748(float, float);
extern int func_L00_0025D390(char *);
extern void func_001F9BC0(void *);
extern int func_001F9850(int);
extern int func_L00_001EFFF0(void *, void *, int, int, int);
extern void func_00215C00(void *, float, float, float);
extern float func_002140F8(float, float);
extern void func_L00_001FF610(void *, void *, void *);
extern float func_001F9CB8(void *a);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern int func_L00_00258BC8(int, int);
extern char *func_L00_0026EBC0(char *pos, char *vel, int c, int d, float f);
extern void func_0020D678(void *);
extern float func_L00_001FF860(float, float);
extern float func_001F9CE8(void *);
extern char *func_L00_0026E940(char *, int, int, int, float);
extern unsigned char *func_L00_00275FF8(char *a, char *b, int c, float f);
extern float func_001FA888(int);
extern float func_L00_00258E58(float, float, float, float, float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern void func_L00_002CD110(void *, int);
extern float D_L00_00161964 MACRO_ADDR;
extern char D_L00_00173F60[];
extern unsigned char D_L00_00173F80[];
extern float D_0015EE6C MACRO_ADDR;
extern float D_L00_0015F660[] MACRO_ADDR;
extern short D_L00_00161960;
extern short D_L00_00161958;
extern short D_L00_00161940;
extern short D_L00_00161944;
extern short D_L00_00161948;

// Builds a moby's data block from its source moby and a position vector; returns the new moby or 0.
char *func_L00_002CC3C0(char *a, char *vec, char *b, float f12, float f13, float f14, float f15) {
    w128 tv;
    float lf[4];
    float out[4];
    w128 tmp;
    float v[4];
    char *m;
    char *d;
    char *q;
    unsigned char *p;
    int r;
    int r2;
    int r3;
    int i;
    int *arr;
    float len;
    float ret;
    float fi;
    float f16;

    tv = *(w128 *)vec;
    m = func_0020D348_2CD3B8(0x131);
    if (m == 0) {
        return 0;
    }
    d = *(char **)(m + 0x78);
    *(char **)(d + 0x20) = a;
    *(short *)(m + 0x32) = 0x7F;
    m[0x30] = 0x7F;
    m[0x31] = 1;
    *(long long *)(m + 0x38) = *(long long *)(a + 0x38);
    *(float *)(m + 0x40) = *(float *)&D_L00_00161960;
    *(float *)(m + 0x44) = func_001FA748(f15, D_L00_00161964);
    *(float *)(m + 0x48) = f14;
    *(w128 *)(m + 0x10) = tv;
    if (b != 0) {
        *(w128 *)d = *(w128 *)(b + 0x10);
        r = func_L00_0025D390(b);
        *(float *)(d + 0x8) = *(float *)(d + 0x8) + (r != 0 ? *(float *)(r + 0x10) : 0.5f);
    } else {
        func_001F9BC0(d);
    }
    *(float *)(d + 0x18) = f12;
    *(float *)(d + 0x1C) = f13;
    *(int *)(d + 0x10) = 0;
    *(int *)(d + 0x14) = 0;
    r = func_001F9850(0x23);
    p = (unsigned char *)D_0013E15A + 0x4C6;
    *(int *)(d + 0x28) = 0;
    r = r + p[0xF] * 10;
    *(short *)(d + 0x26) = r;
    *(short *)(d + 0x24) = r;
    *(w128 *)lf = *(w128 *)(a + 0x10);
    lf[2] = ((float *)&tv)[2];
    r = func_L00_001EFFF0(lf, &tv, 0, (int)a, 0);
    if (r != 0) {
        func_00215C00(out, D_0015EE6C * 40.0f, *(float *)(m + 0x48), -*(float *)(m + 0x44));
        *(w128 *)(m + 0x10) = *(w128 *)D_L00_00173F60;
        i = 4;
        do {
            *(w128 *)v = 0;
            v[0] = func_002140F8(-1.0f, 1.0f);
            i--;
            v[1] = func_002140F8(-1.0f, 1.0f);
            v[2] = func_002140F8(-1.0f, 1.0f);
            tmp = *(w128 *)v;
            func_L00_001FF610(v, out, D_L00_00173F80);
            len = func_001F9CB8(v);
            func_L00_001FF4B0(&tmp, &tmp, len * 0.5f);
            func_001F9BD8(v, v, &tmp);
            func_L00_001FF4B0(v, v, func_002140F8(D_0015EE6C * 3.0f, D_0015EE6C * 6.0f));
            r = func_001F9850(10);
            r2 = func_001F9850(15);
            r3 = func_L00_00258BC8(r, r2);
            func_L00_0026EBC0(m + 0x10, (char *)v, 0x7F2F4F6F, r3, 30000.0f);
        } while (i >= 0);
        p = (unsigned char *)D_0013E15A + 0x4C6;
        if (p[0xF] == 0) {
            func_L00_002CD110(m, 0);
            func_0020D678(m);
            return 0;
        }
        *(int *)(d + 0x28) = *(int *)(d + 0x28) + 1;
        if (p[0xF] + 5 < *(int *)(d + 0x28)) {
            func_L00_002CD110(m, 1);
            func_0020D678(m);
            return 0;
        }
        func_L00_001FF610(out, out, D_L00_00173F80);
        *(float *)(m + 0x48) = func_L00_001FF860(out[0], out[1]);
        len = func_001F9CE8(out);
        *(float *)(m + 0x44) = -func_L00_001FF860(len, out[2]);
        return m;
    } else {
        arr = (int *)(d + 0x2C);
        func_L00_0026E940(m, 0x2F4F7F7F, *(short *)(d + 0x24), -1, *(float *)&D_L00_00161958);
        for (i = 0; i < 17; i++) {
            q = (char *)func_L00_00275FF8(m + 0x10, (char *)D_L00_0015F660, 1, 0.0f);
            if (q != 0) {
                *(float *)(q + 0xC) = func_002140F8(*(float *)&D_L00_00161940, *(float *)&D_L00_00161944);
                fi = func_001FA888(i);
                f16 = fi / func_001FA888(16);
                ret = func_L00_00258E58(-1.5f, 0.0f, 1.0f, 0.0f, f16);
                *(float *)(q + 0xC) = *(float *)(q + 0xC) + (*(float *)&D_L00_00161948 - *(float *)(q + 0xC)) * ret;
                r = func_001FA898_r(ret * -64.0f + 128.0f);
                *(int *)(q + 4) = (r << 24) | 0xFFFFFF;
                arr[i] = (int)q;
            } else {
                arr[i] = 0;
            }
        }
    }
    return m;
}
