/* NON_MATCHING func_L13_002E2980 -- src/overlays/l13_gemlik/vendor_002C2638.c
 * Best so far: SIZE ours 1552 / retail 1560, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   s07 (hq8): level 13 asteroid update, switch on state 0..4 with a shared clamp tail (20..1003) and FA748 blends
 *   Remaining: the ?: for the scale_ticks argument (retail: bnez with the constant in the delay slot, no condition
 */
extern float func_001F9D10(void *, void *);
extern int func_L00_00258BC8(int, int);
extern float func_002140F8(float, float);
extern void func_L00_001FF4B0(void *, void *, float);
extern int func_001F9938(void *);
extern void func_001F9BD8(void *, void *, void *);
extern int func_001F9850(int);
extern int func_L00_001F10E0(float, void *, int, void *);
extern void func_L00_001FF610(void *, void *, void *);
extern void func_L13_002E2690(char *, char *, float);
extern char *func_L00_0025B478(void *, int, int);
extern void func_L00_001FF548(void *, void *, float);
extern void func_0020D678(void *);
extern int func_001160D8(void);
extern float func_00214158(void);
extern int func_L00_00200290(char *, float);
extern float func_001FA748(float, float);
extern char D_0013E633[];
extern float D_0015EE6C MACRO_ADDR;
extern char *D_L13_00160058 MACRO_ADDR;
extern int D_L13_0015F6B0 MACRO_ADDR;
extern char D_L13_001741C0[];

// Update function for asteroid mobies (classes 212 and 1412) on level 13: state machine for approach, bounce and retire.
void func_L13_002E2980(unsigned char *m) {
    char *d = *(char **)(m + 0x78);
    float f20;

    f20 = func_001F9D10(m + 0x10, D_0013E633 + 0xE9D);
    switch (m[0x20]) {
    case 0: {
        float v;
        qcopy(d + 0x10, m + 0x10);
        *(float *)(d + 0x1C) = 20.0f;
        qcopy(d + 0x20, m + 0xC0);
        *(float *)(d + 0x2C) = *(float *)(m + 0x2C);
        *(short *)(d + 0x4E) = func_L00_00258BC8(2, 3);
        v = *(float *)(*(char **)(m + 0x24) + 0x24);
        if (*(float *)(m + 0x2C) < v) m[0x72] = 0;
        m[0x20] = 3;
        break;
    }
    case 1: {
        char *x = D_0013E633 + 0xE1D;
        char *p;
        float f0;
        char *d20;
        if (*(int *)(x + 0x2084) != 0x32) break;
        p = *(char **)(x + 0x15F0);
        if (p == 0) break;
        if (*(short *)(p + 0xA6) != 0x45) break;
        if (m[0x31] == 0 && !(f20 < 40.0f)) break;
        d20 = d + 0x20;
        f0 = func_002140F8(1.0f, 10.0f);
        func_L00_001FF4B0(d, d20, f0 * D_0015EE6C);
        m[0x20] = 2;
        break;
    }
    case 2: {
        int t;
        char *m10 = m + 0x10;
        func_001F9938(d + 0x4C);
        func_001F9BD8(m10, m10, d);
        if (m[0x31] == 0 && !(f20 < 40.0f)) goto c9c;
        t = 0x78;
        if (*(int *)(d + 0x40) == 0) t = 0x168;
        t = func_001F9850(t);
        *(int *)(d + 0x44) = t;
        if (*(short *)(d + 0x4C) != 0) break;
        {
            char *p2 = *(char **)(m + 0x24);
            char *q;
            float f1;
            int pv = *(int *)(p2 + 0x10);
            int gd = (int)m - (int)D_L13_00160058;
            *(int *)(m + 0x94) = pv;
            f1 = *(float *)(m + 0x2C);
            f20 = f1 / *(float *)(p2 + 0x24);
            if ((D_L13_0015F6B0 & 1) != ((gd >> 8) & 1)) goto c14;
            if (func_L00_001F10E0(f20 * 6.2f, m + 0x10, 0, m) == 0) goto c14;
            q = D_L13_001741C0;
            if (*(int *)(q + 0x18) == *(int *)(d + 0x48)) break;
            func_L00_001FF610(d, d, q + 0x40);
            qcopy(m + 0x10, q + 0x30);
            if (*(char **)(q + 0x18) != 0) {
                if (*(short *)(*(char **)(q + 0x18) + 0xA6) == 0xD4) break;
            }
            func_L13_002E2690((char *)m, d, f20);
            break;
        }
    c14:
        {
            char *r = func_L00_0025B478(m, 0x330000, 0);
            char *p2;
            if (r == 0) goto c90;
            if (*(short *)(*(char **)(r + 0x20) + 0xA6) == 0xD4) goto c90;
            r += 0x10;
            p2 = *(char **)(m + 0x24);
            f20 = *(float *)(m + 0x2C) / *(float *)(p2 + 0x24);
            func_L00_001FF548(r, r, D_0015EE6C * 20.0f);
            func_001F9BD8(d, d, r);
            func_L13_002E2690((char *)m, d, f20);
        }
    c90:
        m[0xA4] = 0xFF;
        break;
    c9c:
        if (func_001F9908((int *)(d + 0x44)) == 0) break;
        if (*(int *)(d + 0x40) == 1) {
            func_0020D678(m);
            return;
        }
        m[0x20] = 4;
        *(unsigned short *)(m + 0x34) = *(unsigned short *)(m + 0x34) | 1;
        *(int *)(m + 0x94) = 0;
        m[0x31] = 0;
        *(float *)(m + 0x2C) = *(float *)(d + 0x2C);
        break;
    }
    case 3: {
        float k;
        float r;
        qcopy(m + 0x10, d + 0x10);
        qzero(d + 0x30);
        r = func_002140F8(-30.0f, 30.0f);
        k = r * 0.01745329238474369f;
        f20 = k * D_0015EE6C;
        switch (func_001160D8() % 3) {
        case 0: *(float *)(d + 0x30) = f20; break;
        case 1: *(float *)(d + 0x34) = f20; break;
        case 2: *(float *)(d + 0x38) = f20; break;
        }
        r = func_002140F8(-180.0f, 180.0f);
        k = r * 0.01745329238474369f;
        f20 = k * D_0015EE6C;
        switch (func_001160D8() % 3) {
        case 0: *(float *)(d + 0x30) = f20; break;
        case 1: *(float *)(d + 0x34) = f20; break;
        case 2: *(float *)(d + 0x38) = f20; break;
        }
        *(float *)(m + 0x40) = func_00214158();
        *(float *)(m + 0x44) = func_00214158();
        *(float *)(m + 0x48) = func_00214158();
        qzero(d);
        *(float *)(m + 0x2C) = *(float *)(d + 0x2C);
        if (*(float *)(*(char **)(m + 0x24) + 0x24) < *(float *)(d + 0x2C)) m[0x72] = 100;
        m[0x20] = 1;
        break;
    }
    case 4: {
        if (func_L00_00200290(d + 0x10, (float)*(short *)(m + 0x32)) >= 0) break;
        qcopy(m + 0x10, d + 0x10);
        {
            char *p2 = *(char **)(m + 0x24);
            m[0x31] = 1;
            *(int *)(m + 0x94) = *(int *)(p2 + 0x10);
            *(unsigned short *)(m + 0x34) = *(unsigned short *)(m + 0x34) & 0xFFFE;
            m[0x20] = 3;
        }
        break;
    }
    default:
        break;
    }
    if (*(float *)(m + 0x10) < 20.0f) *(float *)(m + 0x10) = 20.0f;
    if (1003.0f < *(float *)(m + 0x10)) *(float *)(m + 0x10) = 1003.0f;
    if (*(float *)(m + 0x14) < 20.0f) *(float *)(m + 0x14) = 20.0f;
    if (1003.0f < *(float *)(m + 0x14)) *(float *)(m + 0x14) = 1003.0f;
    if (*(float *)(m + 0x18) < 20.0f) *(float *)(m + 0x18) = 20.0f;
    if (1003.0f < *(float *)(m + 0x18)) *(float *)(m + 0x18) = 1003.0f;
    *(float *)(m + 0x40) = func_001FA748(*(float *)(m + 0x40), *(float *)(d + 0x30));
    *(float *)(m + 0x44) = func_001FA748(*(float *)(m + 0x44), *(float *)(d + 0x34));
    *(float *)(m + 0x48) = func_001FA748(*(float *)(m + 0x48), *(float *)(d + 0x38));
}
