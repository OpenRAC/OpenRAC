/* NON_MATCHING func_L03_002DBC38 -- src/overlays/l03_kerwan/vendor_002CB280.c
 * Best so far: SIZE ours 1956 / retail 1968, checked 2026-10-08.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * No longer builds in its file (COMPILE failed, 2026-10-09): match its declarations to the file's first.
 * What the last attempts found:
 *   Helga (class 890) update on level 03: state dispatch on m[0x20] (pause help via func_L01_0029C2A0, moby spawns
 *   Best so far p4.c: 1956 bytes against 1968. Prologue saves more registers than retail (s0, s2, s3, s4, s6, s7 a
 *   Would unblock: knowing the original shape of the m[0x20] switch and the source of the tail's register-carried 
 */
typedef struct {
    int key;
    int pad4;
    unsigned short a;
    short flag;
    short c;
    short d;
} Entry;

typedef struct {
    int n;
    Entry e[1];
} Table;

extern int func_L00_002676E8(void *, void *);
extern int func_L00_00267290(void *, void *);
extern void func_L01_00279398(float, void *);
extern void func_L00_002512D8(int);
extern void func_L00_00286128(void *, void *);
extern void func_L00_002618D8(int, int);
extern void func_L00_00264DB8(int, int);
extern void func_0020D678(void *);
extern int func_0020BFC8(int, int);
extern float func_L00_001FF860(float, float);
extern float func_001FA850(float, float);
extern float func_001F9CB8(void *);
extern int func_001F9850(int);
extern void func_001F9908(int *);
extern float func_002140F8(float, float);
extern float func_001F9878(float);
extern int func_001FA898(float);
extern float func_001FA748(float, float);
extern void func_00215C00(void *, float, float, float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_001F9BF0(void *, void *, void *);
extern float func_001FA790(float, float);
extern float func_001F9CE8(void *);
extern void func_L00_00263950(void *, void *, int, float, float);
extern void func_L02_0025D750(void *);
extern void func_L01_0029C2A0(char *a0, int a1, char *p, int mode, Table *t);

extern unsigned char D_0014171B[];
extern int D_0015EE84 MACRO_ADDR;
extern char D_0013E633[];
extern char *D_L03_00160058 MACRO_ADDR;
extern Table D_L03_001BAA30;
extern char D_L03_00161C38[];
extern char D_L03_001BA9E0[];
extern unsigned char D_L03_0015FD08[];
extern unsigned char D_0015EEB0[] MACRO_ADDR;
extern float D_0015EE64 MACRO_ADDR;
extern int D_L03_0015F6A8 MACRO_ADDR;
extern char *D_L03_0016016C MACRO_ADDR;

/* Level 03 helga moby update: state dispatch, pause help and the float motion tail. */
void func_L03_002DBC38(unsigned char *m) {
    unsigned char *d = *(unsigned char **)(m + 0x78);
    unsigned char *p16 = d + 0x80;
    int st;
    float r, r2, r3;
    int t2;
    int gate = 0;
    int s20 = 0;
    float f22, f23;
    u128 loc;
    float v2[4];
    float v3[4];
    char *P;

    func_L03_002DBAD8(m);
    st = m[0x20];
    if (st != 1) {
    if (st < 2) {
    if (st != 0) { } else {
        if (D_0013D5CA[10] != 0) {
            int w = *(int *)(d + 0xD4);
            if (w != -1) {
                unsigned char *e = (unsigned char *)D_L03_00160058 + (w << 8);
                if (*(short *)(e + 0xA6) == 0x3F4) {
                    unsigned char *q = *(unsigned char **)(e + 0x78);
                    *(int *)(q + 0x8C) = 1;
                    func_L01_0029C2A0((char *)q + 0x8C, 4, (char *)m, 2, &D_L03_001BAA30);
                }
            }
            func_0020D678(m);
            return;
        }
        {
            unsigned char b = (unsigned char)(D_0014171B + 0xAA35)[m[0xB0] + (D_0015EE84 << 4)];
            if (b == 0xFF) {
                int w = *(int *)(d + 0xD4);
                if (w != -1) {
                    unsigned char *e = (unsigned char *)D_L03_00160058 + (w << 8);
                    if (*(short *)(e + 0xA6) == 0x3F4) {
                        unsigned char *q = *(unsigned char **)(e + 0x78);
                        *(int *)(q + 0x8C) = 1;
                        func_L01_0029C2A0((char *)q + 0x8C, 4, (char *)m, 2, &D_L03_001BAA30);
                    }
                }
            }
        }
        {
            unsigned char *G = *(unsigned char **)&D_0013E633[0x2E9D];
            m[0x20] = 1;
            *(long long *)(m + 0x38) = *(long long *)(G + 0x38);
            *(char **)(p16 + 0x20) = D_L03_00161C38;
            *(int *)(d + 0xC8) = 5;
            func_L00_002676E8(m, p16);
            func_L02_0025D750(m);
        }
    }
    } else if (st == 2) {
        if (D_L03_0015F6A8 == 2) {
            if (*(short *)(d + 0xB6) == 2) {
                int w = *(int *)(d + 0xD4);
                if (w != -1) {
                    unsigned char *e = (unsigned char *)D_L03_00160058 + (w << 8);
                    if (*(short *)(e + 0xA6) == 0x3F4) {
                        unsigned char *q = *(unsigned char **)(e + 0x78);
                        *(int *)(q + 0x7C) = 0;
                        *(int *)(q + 0x8C) = 0;
                        *(int *)(q + 0xA0) = *(int *)(d + 0xD8);
                    }
                }
            }
        } else if (*(short *)(d + 0x84) != 2) {
            func_L00_002512D8(m[0xB0]);
            {
                int w = *(int *)(d + 0xD4);
                if (w != -1) {
                    unsigned char *e = (unsigned char *)D_L03_00160058 + (w << 8);
                    if (*(short *)(e + 0xA6) == 0x3F4) {
                        unsigned char *q = *(unsigned char **)(e + 0x78);
                        *(int *)(q + 0x8C) = 1;
                        func_L01_0029C2A0((char *)q + 0x8C, 4, (char *)m, 2, &D_L03_001BAA30);
                    }
                }
            }
            {
                int w3 = *(int *)(d + 0xCC);
                if (w3 != -1) {
                    unsigned char *base = (unsigned char *)D_L03_0016016C + (w3 << 7);
                    func_L00_00286128(base + 0x30, base + 0x70);
                }
            }
            m[0x20] = 1;
        } else {
            m[0x20] = 1;
            func_L00_002618D8(12, 1);
            func_L00_00264DB8(0xBC8, -1);
            {
                int w0 = *(int *)(d + 0xD0);
                if (w0 != -1) {
                    unsigned char *E = (unsigned char *)D_L03_00160058 + (w0 << 8);
                    unsigned char b6 = E[0xB0];
                    int store = 0;
                    D_L03_001BA9E0[*(short *)(E + 0xB2) + 0x454] = E[0xB0] + 2;
                    if (b6 == 0xFF) {
                        store = 1;
                    } else if (D_L03_0015FD08[b6] != 0xFF
                               && (D_0014171B + 0xAA35)[b6 + (D_0015EE84 << 4)] == 0xFF) {
                        store = 1;
                    }
                    if (store) {
                        D_L03_001BB640[*(short *)(E + 0xB2) + 0x454] = b6 + 2;
                    }
                    func_0020D678(E);
                }
            }
            {
                int w = *(int *)(d + 0xD4);
                if (w != -1) {
                    unsigned char *e = (unsigned char *)D_L03_00160058 + (w << 8);
                    if (*(short *)(e + 0xA6) == 0x3F4) {
                        unsigned char *q = *(unsigned char **)(e + 0x78);
                        *(int *)(q + 0x7C) = 0;
                        *(int *)(q + 0x8C) = 0;
                        *(int *)(q + 0xA0) = *(int *)(d + 0xD8);
                    }
                }
            }
            func_0020BFC8(0, -1);
            func_0020D678(m);
            return;
        }
    }
    } else {
        if (func_L00_00267290(m, p16) != 0) {
            func_L01_00279398(2.3f, m);
            m[0x20] = 2;
            {
                int w = *(int *)(d + 0xD4);
                if (w != -1) {
                    unsigned char *e = (unsigned char *)D_L03_00160058 + (w << 8);
                    if (*(short *)(e + 0xA6) == 0x3F4) {
                        unsigned char *q = *(unsigned char **)(e + 0x78);
                        *(int *)(q + 0x8C) = 1;
                        func_L01_0029C2A0((char *)q + 0x8C, 4, (char *)m, 2, &D_L03_001BAA30);
                    }
                }
            }
        }
    }

    /* tail: the float motion block runs only when m[0x53] is zero */
    P = &D_0013E633[0xE9D];
    f22 = 0.02f;
    f23 = 0.3f;
    if (m[0x53] == 0) {
        s20 = 1;
        r = func_001F9D48((float *)(m + 0x10), (float *)P);
        if (r < 8.0f) {
            float f1v = *(float *)(P - 0x80 + 0xD4);
            float f12v = *(float *)(P - 0x80 + 0xD0);
            r2 = func_L00_001FF860(f12v - *(float *)(m + 0x10), f1v - *(float *)(m + 0x14));
            r2 = func_001FA850(*(float *)(m + 0x48), r2);
            if (r2 < 1.5707964f) {
                gate = 1;
                r3 = func_001F9CB8(P + 0x80);
                if (0.01f < r3) {
                    t2 = func_001F9850(0x78);
                    *(int *)(d + 0x1F0) = t2;
                } else {
                    func_001F9908((int *)(d + 0x1F0));
                    t2 = 1;
                }
            }
        }
        if (!gate) {
            t2 = *(int *)(d + 0x1F0);
            if (t2 != 0) {
                *(int *)(d + 0x1F0) = 0;
                *(u128 *)(d + 0x1E0) = *(u128 *)(P + 0x50);
            }
        }
        func_001F9908((int *)(d + 0x1F4));
        if (t2 != 0) {
            float k;
            k = 0.0174532925f;
            r = func_001F9878(func_002140F8(180.0f, 300.0f));
            r2 = func_001FA898(r);
            *(int *)(d + 0x1F4) = (int)r2;
            r = func_002140F8(-90.0f, 90.0f) * k;
            r = func_001FA748(*(float *)(m + 0x48), r);
            r2 = func_002140F8(0.0f, 30.0f);
            func_00215C00(d + 0x1E0, 6.0f, r, r2 * k);
            func_001F9BD8(d + 0x1E0, d + 0x1E0, m + 0x10);
        }
        if (*(int *)(d + 0x1F0) != 0) {
            loc = *(u128 *)(P + 0x50);
            f22 = 0.04f;
            f23 = 0.3f;
        } else {
            loc = *(u128 *)(d + 0x1E0);
        }
    }
    if (s20) {
        float a, b, c, e2, f1c;
        *(u128 *)v2 = *(u128 *)(m + 0x10);
        v2[2] = v2[2] + 1.0f;
        func_001F9BF0(v3, &loc, v2);
        a = func_L00_001FF860(v3[0], v3[1]);
        b = func_001FA790(a, *(float *)(m + 0x48));
        c = func_001F9CE8(v3);
        e2 = func_L00_001FF860(c, v3[2]);
        f1c = -e2;
        if (b > 1.5707964f) {
            b = 1.5707964f;
        } else if (b < -1.5707964f) {
            b = -1.5707964f;
        }
        if (0.5235988f < f1c) {
            f1c = 0.5235988f;
        } else if (f1c < -0.5235988f) {
            f1c = -0.5235988f;
        }
        *(float *)(d + 0x144) = f1c;
        *(float *)(d + 0x148) = b * 0.5f;
    }
    if (D_0015EEB0[0] != 0) {
        *(float *)(d + 0x150) = 2.75f;
    }
    func_L00_00263950(m, d + 0xE0, 0, f22 * D_0015EE64, f23 * D_0015EE64);
    func_L00_00263950(m, d + 0x160, 1, f22 * D_0015EE64, f23 * D_0015EE64);
}
