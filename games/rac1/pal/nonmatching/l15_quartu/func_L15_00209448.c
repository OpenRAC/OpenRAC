/* NON_MATCHING func_L15_00209448 -- src/overlays/l15_quartu/help_00204AC0.c
 * Best so far: SIZE ours 1292 / retail 1296, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * Cannot land as written (#define in a candidate): rewrite that in plain C first.
 * What the last attempts found:
 *   func_L15_00209448: per-frame camera/effect update for the current target entry (64-byte records at D_L15_00178
 *   Best candidate p10.c: 1288 vs retail 1296 bytes (8 short), structure matches through the first part and case 0
 *   Unblock: find what makes the +0x2280 reloads survive CSE (a different base pseudo per arm) and the $5/$6 swap;
 */
extern unsigned char D_0013F450[] NOT_SDA;
extern char D_L15_00178780[];
extern int D_L15_0015F6A8;
extern int D_0013E090[];
extern int D_0015EE84_m __asm__("D_0015EE84") MACRO_ADDR;
extern int D_0015EFA8_m __asm__("D_0015EFA8") MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern int func_L00_00207220(void);
extern void func_001F9BF0(void *, void *, void *);
extern float func_001FA748(float, float);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern void func_L15_00217348(int, int);
extern int func_L00_00217570(int a, int b);
extern void func_L00_00211338(float *v, int each, float s, float z);
extern int func_L00_002AAE80(void);
extern void func_001F9BC0(void *);
extern void func_001F9BD8(void *, void *, void *);

#define GB(o) (*(unsigned char *)(g + (o)))
#define GH(o) (*(short *)(g + (o)))
#define GW(o) (*(int *)(g + (o)))
#define GF(o) (*(float *)(g + (o)))

// per-frame update of the camera/effect state for the current target entry; returns 1 when it ran
int func_L15_00209448(int a) {
    char *g = D_0013F450;
    float tmp[4];
    char *q;
    int idx, w, mode, t;
    float s;
    if (GW(0x208C) == 0x14 || GW(0x208C) == 7 || GW(0x2084) == 0x32) return 0;
    if (GW(0x1C0) != 0) return 0;
    if (D_L15_0015F6A8 != 0) return 0;
    GW(0x2280) = 0;
    w = GW(0x2080);
    idx = *(unsigned char *)(w + 0xA4);
    if (idx == 0xFF) return 0;
    q = D_L15_00178780 + idx * 64;
    if (*(int *)(q + 0x34) != w) return 0;
    if (GB(0x20A4) == 2) {
        if (!(*(int *)(q + 0x24) & 2)) return 0;
    } else {
        if ((*(int *)(q + 0x24) ^ 1) & 1) return 0;
    }
    t = D_0015EE84_m;
    {
        char *g = D_0013F450;
        int *cnt = &D_0013E090[t];
        GW(0x2280) = *(int *)(q + 0x20);
        *cnt = *cnt + 1;
        D_0015EFA8_m = D_0015EFA8_m + 1;
        if (GB(0x20A4) == 2) {
            GW(0x1630) = GW(0x1630) - func_001FA898_r(*(float *)(q + 0x2C));
            if (GW(0x1630) < 0) GW(0x1630) = 0;
        }
    }
    if (a == 0) return 1;
    func_L00_00207220();
    a = 0;
    if (*(int *)(q + 0x30) & 1) {
        qcopy(tmp, q + 0x10);
        if (*(float *)(q + 0x1C) == 5627.9248f) a = 1;
    } else {
        w = *(int *)(q + 0x20);
        if (w != 0) {
            func_001F9BF0(tmp, D_0013F450 + 0x80, (char *)w + 0x10);
        } else {
            char *g = D_0013F450;
            tmp[0] = func_001F9F90(func_001FA748(GF(0x98), 3.1415927f));
            tmp[1] = func_001F9FA8(func_001FA748(GF(0x98), 3.1415927f));
            tmp[2] = 0;
        }
    }
    g = D_0013F450;
    mode = GB(0x20A4);
    switch (mode) {
    case 0: {
        int id;
        w = GW(0x2280);
        if (w != 0) {
            int h = *(short *)(w + 0xA6);
            if (h == 0x4EB || h == 0x558) {
                id = 0x80;
                goto ret1;
            }
        }
        goto next;
    ret1:
        func_L15_00217348(id, 1);
        return 1;
    next:
        {
            char *g = D_0013F450;
            int v = GW(0x208C);
            if (v == 0x16) {
                func_L15_00217348(0x6D, 1);
                GF(0x128) = D_0015EE6C * 7.0f;
                return 1;
            } else if (v == 0x12) {
                id = 0x75;
                w = GW(0x2280);
                if (w != 0 && *(short *)(w + 0xA6) == 0x28F) {
                    id = 0x82;
                    goto ret1;
                }
                func_L15_00217348(id, 1);
            } else if (v == 0x11) {
                id = 0x76;
                w = GW(0x2280);
                if (w != 0 && *(short *)(w + 0xA6) == 0x28F) {
                    id = 0x82;
                    goto ret1;
                }
                if (D_0015EE84_m == 0xF || D_0015EE84_m == 0x11) {
                    w = GW(0x2280);
                    if (w != 0) {
                        int h = *(short *)(w + 0xA6);
                        if (h == 0x28F || h == 0x7B || h == 0x29D) {
                            func_L00_00217570(0x1C, 0);
                        }
                    }
                }
                func_L15_00217348(id, 1);
            } else if (v == 3) {
                id = 0x16;
                w = GW(0x2280);
                if (w != 0 && *(short *)(w + 0xA6) == 0x28F) {
                    id = 0x82;
                    goto ret1;
                }
                func_L15_00217348(id, 1);
            } else {
                func_L15_00217348(0x16, 1);
            }
        }
        s = D_0015EE6C;
        {
            float f12 = s * 5.7f;
            float f13 = s * 2.4f;
            if (GB(0x12E7) != 0) {
                f12 = 0.0f;
                f13 = s * 1.7f;
            }
            func_L00_00211338(tmp, a, f12, f13);
        }
        if (a != 0) {
            if (*(unsigned char *)(q + 0x28) == 4) tmp[2] = tmp[2] + tmp[2];
        }
        break;
    }
    case 2: {
        if ((*(int *)(q + 0x24) & 4) || GW(0x1630) <= 0) {
            func_L15_00217348(0x5D, 1);
            func_L00_002AAE80();
            if (GH(0x30E) == 0) {
                s = D_0015EE6C;
                func_L00_00211338(tmp, a, s * 7.0f, s * 3.5f);
                break;
            }
        }
        func_001F9BC0(tmp);
        break;
    }
    case 3:
        func_L15_00217348(0x56, 1);
        s = D_0015EE6C;
        func_L00_00211338(tmp, a, s * 5.0f, s * 2.4f);
        break;
    }
    func_001F9BD8(D_0013F450 + 0xE0, D_0013F450 + 0xE0, tmp);
    func_001F9BD8(D_0013F450 + 0x100, D_0013F450 + 0x100, tmp);
    return 1;
}
