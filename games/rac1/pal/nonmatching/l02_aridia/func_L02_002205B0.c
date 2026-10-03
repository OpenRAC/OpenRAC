/* NON_MATCHING func_L02_002205B0 -- src/overlays/l02_aridia/help_0021BC90.c
 * Best so far: SIZE ours 832 / retail 836, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * Cannot land as written (#define in a candidate): rewrite that in plain C first.
 * What the last attempts found:
 *   func_L02_002205B0: lock-on/camera offset update (returns 1 when handled, 0 early). Best candidate p3.c: SIZE 8
 *   Difference: block layout of the `return 0` stub. Retail keeps one `b end; daddu $2,0` stub right after the `0x
 *   Would unblock: a source shape that makes gcc cross-jump the later `return 0` into the earlier one (tried: sepa
 */
extern unsigned char D_0013E633[] NOT_SDA;
extern char D_0013DE6E[];
extern char D_L02_00178780[];
extern int D_L02_0015F6A8 MACRO_ADDR;
extern int D_0015EE84_m __asm__("D_0015EE84") MACRO_ADDR;
extern int D_0015EFA4[] MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern void func_L00_00207220(void);
extern void func_001F9BF0(void *, void *, void *);
extern void func_001F9BD8(void *, void *, void *);
extern float func_001FA748(float, float);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern void func_L02_0022BE40(int, int);
extern void func_L00_00211338(float *, int, float, float);

#define G g

// Checks the player's current target and applies a lock-on offset to the camera points; returns 1 when handled.
int func_L02_002205B0(int arg) {
    char *g = (char *)D_0013E633 + 0xE1D;
    char *e;
    char *t;
    char *d;
    char *q;
    char *r;
    int idx;
    int f;
    float v[4];
    float s, a, b;

    if (*(int *)(G + 0x208C) == 0x14 || *(int *)(G + 0x208C) == 7 || *(int *)(G + 0x2084) == 0x32) {
        return 0;
    }
    if (*(int *)(G + 0x1C0) != 0 || D_L02_0015F6A8 != 0) {
        return 0;
    }
    *(int *)(G + 0x2280) = 0;
    t = *(char **)(G + 0x2080);
    idx = *(unsigned char *)(t + 0xA4);
    e = D_L02_00178780 + idx * 0x40;
    if (idx == 0xFF || *(char **)(e + 0x34) != t || ((*(int *)(e + 0x24) ^ 1) & 1)) {
        return 0;
    }
    *(int *)(G + 0x2280) = *(int *)(e + 0x20);
    ((int *)(D_0013DE6E + 0x222))[D_0015EE84_m]++;
    D_0015EFA4[1]++;
    if (arg == 0) {
        return 1;
    }
    func_L00_00207220();
    f = 0;
    if (*(int *)(e + 0x30) & 1) {
        qcopy(v, e + 0x10);
        if (*(float *)(e + 0x1C) == 5627.9248f) {
            f = 1;
        }
    } else if (*(int *)(e + 0x20) != 0) {
        func_001F9BF0(v, G + 0x80, *(char **)(e + 0x20) + 0x10);
    } else {
        v[0] = func_001F9F90(func_001FA748(*(float *)(G + 0x98), 3.1415927f));
        v[1] = func_001F9FA8(func_001FA748(*(float *)(G + 0x98), 3.1415927f));
        v[2] = 0;
    }
    q = (char *)D_0013E633 + 0xE1D;
    switch (*(unsigned char *)(q + 0x20A4)) {
    case 0:
        t = *(char **)(q + 0x2280);
        if (t != 0) {
            int h = *(short *)(t + 0xA6);
            if (h == 0x4EB || h == 0x558) {
                func_L02_0022BE40(0x80, 1);
                return 1;
            }
        }
        r = (char *)D_0013E633 + 0xE1D;
        if (*(int *)(r + 0x208C) == 0x16) {
            func_L02_0022BE40(0x6D, 1);
            *(float *)(r + 0x128) = D_0015EE6C * 7.0f;
            return 1;
        }
        func_L02_0022BE40(0x16, 1);
        s = D_0015EE6C;
        a = s * 5.7f;
        b = s * 2.4f;
        if (*(unsigned char *)(r + 0x12E7) != 0) {
            a = 0;
            b = s * 1.7f;
        }
        func_L00_00211338(v, f, a, b);
        if (f) {
            if (*(unsigned char *)(e + 0x28) == 4) {
                v[2] = v[2] + v[2];
            }
        }
        break;
    case 3:
        func_L02_0022BE40(0x56, 1);
        s = D_0015EE6C;
        func_L00_00211338(v, f, s * 5.0f, s * 2.4f);
        break;
    }
    d = (char *)D_0013E633 + 0xEFD;
    func_001F9BD8(d, d, v);
    d += 0x20;
    func_001F9BD8(d, d, v);
    return 1;
}
