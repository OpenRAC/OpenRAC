/* NON_MATCHING func_L18_002190C8 -- src/overlays/l18_veldin2/help_00214138.c
 * Best so far: SIZE ours 1332 / retail 1328, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   - -mno-split-addresses on p8: 1276 (worse, dropped).
 *   - static inline accessor (p12) and folded-address first read (p13): gcse still reuses the first load (1288).
 *   - What worked (p14-p17): make the first p read (case 0 head) through g2 (the counting-block pointer, own pseud
 *   reloads through g3 (s0 = s3+lo, the retail form `lw 0x2280($s0)`); the 0x11 inner (t==0xF/0x11) read via a blo
 *   (`addiu v0,s3,lo; lw 0x2280(v0)`, as retail). Then use separate temporaries r0..r3 per read instead of the sha
 *   - Remaining: retail has two separate `func_L18_002284E0(0x16,1)` call sites (mode 3 block and the final else);
 *   Writing snd=0x16 vs literal (p18) no change; calling after loading s in the else (p19) gave 1332 (spill). Also
 *   0x12 bne delay slot and beql/bnel flavours near +254, +4c (prologue early-return chain) differ.
 */
extern unsigned char D_0013E633[];
extern char D_0013DE6E[];
extern char D_L18_00178B80[];
extern int D_L18_0015F6A8 MACRO_ADDR;
extern int D_0015EE84 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern int D_0015EFA8 MACRO_ADDR;
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern void func_L00_00207220(void);
extern void func_001F9BF0(void *dst, void *a, void *b);
extern float func_001FA748(float, float);
extern float func_001F9F90(float);
extern float func_001F9FA8(float);
extern void func_L18_002284E0(int, int);
extern int func_L00_00217570(int, int);
extern void func_L00_00211338(float *v, int each, float s, float z);
extern void func_L00_002AAE80(void);
extern void func_001F9BC0(void *);
extern void func_001F9BD8(void *, void *, void *);

int func_L18_002190C8(int each) {
    char *g = D_0013E633 + 0xE1D;
    char *g2;
    char *g3;
    char *e;
    char *p;
    int *cnt;
    int idx;
    int t;
    int mode;
    int snd;
    char *r0;
    char *r1;
    char *r2;
    char *r3;
    float v[4];
    float s;
    float a;
    float b;

    mode = *(int *)(g + 0x208C);
    if (mode == 0x14 || mode == 7 || *(int *)(g + 0x2084) == 0x32 || *(int *)(g + 0x1C0) != 0 || D_L18_0015F6A8 != 0) {
        return 0;
    }
    *(int *)(g + 0x2280) = 0;
    p = *(char **)(g + 0x2080);
    idx = *(unsigned char *)(p + 0xA4);
    if (idx == 0xFF) {
        return 0;
    }
    e = D_L18_00178B80 + idx * 64;
    if (*(char **)(e + 0x34) != p) {
        return 0;
    }
    if (*(unsigned char *)(g + 0x20A4) == 2) {
        if (!(*(int *)(e + 0x24) & 2)) {
            return 0;
        }
    } else {
        if ((*(int *)(e + 0x24) ^ 1) & 1) {
            return 0;
        }
    }
    t = D_0015EE84;
    g2 = D_0013E633 + 0xE1D;
    cnt = (int *)(D_0013DE6E + 0x222);
    cnt[t]++;
    D_0015EFA8 = D_0015EFA8 + 1;
    *(int *)(g2 + 0x2280) = *(int *)(e + 0x20);
    if (*(int *)(g2 + 0x208C) == 0xF) {
        *(short *)(g2 + 0x5BE) = 1;
        return 0;
    }
    if (*(unsigned char *)(g2 + 0x20A4) == 2) {
        int r = func_001FA898_r(*(float *)(e + 0x2C));
        int q = *(int *)(g2 + 0x1630) - r;
        *(int *)(g2 + 0x1630) = q;
        if (q < 0) {
            *(int *)(g2 + 0x1630) = 0;
        }
    }
    if (each == 0) {
        return 1;
    }
    func_L00_00207220();
    each = 0;
    if (*(int *)(e + 0x30) & 1) {
        qcopy(v, e + 0x10);
        if (*(float *)(e + 0x1C) == 5627.9248f) {
            each = 1;
        }
    } else {
        p = *(char **)(e + 0x20);
        if (p != 0) {
            func_001F9BF0(v, D_0013E633 + 0xE9D, p + 0x10);
        } else {
            char *gq = D_0013E633 + 0xE1D;
            v[0] = func_001F9F90(func_001FA748(*(float *)(gq + 0x98), 3.1415927f));
            v[1] = func_001F9FA8(func_001FA748(*(float *)(gq + 0x98), 3.1415927f));
            v[2] = 0.0f;
        }
    }
    g3 = D_0013E633 + 0xE1D;
    switch (*(unsigned char *)(g3 + 0x20A4)) {
    case 0:
        p = *(char **)(g2 + 0x2280);
        if (p != 0 && (*(short *)(p + 0xA6) == 0x4EB || *(short *)(p + 0xA6) == 0x558)) {
            snd = 0x80;
play:
            func_L18_002284E0(snd, 1);
            return 1;
        }
        mode = *(int *)(g3 + 0x208C);
        if (mode == 0x16) {
            func_L18_002284E0(0x6D, 1);
            *(float *)(g3 + 0x128) = D_0015EE6C * 7.0f;
            return 1;
        } else if (mode == 0x12) {
            {
                r0 = *(char **)(g3 + 0x2280);
            }
            snd = 0x75;
            if (r0 != 0 && *(short *)(r0 + 0xA6) == 0x28F) {
                snd = 0x82;
                goto play;
            }
            func_L18_002284E0(snd, 1);
            s = D_0015EE6C;
        } else if (mode == 0x11) {
            {
                r1 = *(char **)(g3 + 0x2280);
            }
            if (r1 != 0 && *(short *)(r1 + 0xA6) == 0x28F) {
                snd = 0x82;
                goto play;
            }
            t = D_0015EE84;
            if (t == 0xF || t == 0x11) {
                char *q3 = D_0013E633 + 0xE1D;
                r2 = *(char **)(q3 + 0x2280);
                if (r2 != 0) {
                    int h = *(short *)(r2 + 0xA6);
                    if (h == 0x28F || h == 0x7B || h == 0x29D) {
                        func_L00_00217570(0x1C, 0);
                    }
                }
            }
            snd = 0x76;
            func_L18_002284E0(snd, 1);
            s = D_0015EE6C;
        } else if (mode == 3) {
            {
                r3 = *(char **)(g3 + 0x2280);
            }
            if (r3 != 0 && *(short *)(r3 + 0xA6) == 0x28F) {
                snd = 0x82;
                goto play;
            }
            func_L18_002284E0(0x16, 1);
            s = D_0015EE6C;
        } else {
            s = D_0015EE6C;
            func_L18_002284E0(0x16, 1);
        }
        a = s * 5.7f;
        b = s * 2.4f;
        if (*(unsigned char *)(D_0013E633 + 0xE1D + 0x12E7) != 0) {
            a = 0.0f;
            b = s * 1.7f;
        }
        func_L00_00211338(v, each, a, b);
        if (each != 0) {
            if (*(unsigned char *)(e + 0x28) == 4) {
                v[2] = v[2] + v[2];
            }
        }
        break;
    case 2:
        if (!(*(int *)(e + 0x24) & 4) && *(int *)(g3 + 0x1630) > 0) {
            func_001F9BC0(v);
        } else {
            func_L18_002284E0(0x5D, 1);
            func_L00_002AAE80();
            if (*(short *)(g3 + 0x30E) != 0) {
                func_001F9BC0(v);
            } else {
                s = D_0015EE6C;
                func_L00_00211338(v, each, s * 7.0f, s * 3.5f);
            }
        }
        break;
    case 3:
        func_L18_002284E0(0x56, 1);
        s = D_0015EE6C;
        func_L00_00211338(v, each, s * 5.0f, s * 2.4f);
        break;
    default:
        break;
    }
    func_001F9BD8(D_0013E633 + 0xEFD, D_0013E633 + 0xEFD, v);
    func_001F9BD8(D_0013E633 + 0xF1D, D_0013E633 + 0xF1D, v);
    return 1;
}
