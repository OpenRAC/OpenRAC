/* NON_MATCHING func_L03_002DD158 -- src/overlays/l03_kerwan/vendor_002CB280.c
 * Best so far: SIZE ours 1392 / retail 1408, checked 2026-10-08.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * No longer builds in its file (COMPILE failed, 2026-10-09): match its declarations to the file's first.
 * What the last attempts found:
 *   Robot Qwark update (moby class 914): a bit test on a shared table, then a switch on moby[0x20] (0 start, 1 wai
 *   Differences left: state 3 keeps one float constant in $f24 and multiplies by it where retail passes it as the 
 *   Unblock: a way to force the three float constants (0.0, 0.25, 0.5, 1.0) into $f21-$f24 as retail does, and the
 */
typedef int u128z __attribute__((mode(TI)));
typedef int s64x __attribute__((mode(DI)));
extern char D_L03_00166F40[];
extern int D_L03_001BA8E0[];
extern int D_0015EE84 MACRO_ADDR;
extern char D_0014171B[];
extern float D_0015EE6C MACRO_ADDR;
extern int D_L03_0015F6A8 MACRO_ADDR;
extern short D_L03_0015F16C;
extern unsigned char D_0013D5CA[];
extern unsigned char D_0013D4A5[];
extern char D_0013E633[];
extern float func_001F9D10(void *, void *);
extern void func_00213D28(void *, int, int);
extern char *func_L00_0025B478(void *, int, int);
extern void func_L00_00264DB8(int, int);
extern int func_L00_002676E8(void *, void *);
extern int func_L00_00267290(void *, void *);
extern void func_L01_00279398(float, void *);
extern void func_L00_00286128(void *, void *);
extern float func_00214158(void);
extern float func_001F9F90(float);
extern float func_002140F8(float, float);
extern float func_001F9FA8(float);
extern void func_L00_001FF240(void *, void *, void *);
extern int func_002140B0(int);
extern int func_L00_00258BC8(int, int);
extern int func_001F9850(int);
extern int func_L00_00258BC8(int, int);

/* Robot Qwark (moby class 914) update: a small state machine that drives its shots and targeting. */
void func_L03_002DD158(char *m) {
    char *d = *(char **)(m + 0x78);
    float vec[4];
    float blk[4];
    unsigned short h;
    int bit;
    char *it;
    unsigned char itype;

    if (*(unsigned char *)(m + 0x31) != 0) {
        if (func_001F9D10(m + 0x10, D_L03_00166F40) < 26.0f) {
            func_L00_0025B178(m);
            m[0x7F] = 21;
        }
    }
    h = *(unsigned short *)(m + 0xB2);
    if (D_L03_001BB640[0x454 + (short)h] != 0 ||
        ((*(int *)((char *)D_0014171B + 0xAB75 + ((short)h >> 5) * 4 + (D_0015EE84 << 8)) >> (h & 0x1F)) & 1)) {
        if (*(unsigned char *)(m + 0x20) != 3) {
            m[0x20] = 3;
            func_00213D28(m, 1, 0);
        }
    }
    it = func_L00_0025B478(m, -1, 0);
    if (it != 0 && *(char **)(it + 0x20) != 0 && (itype = *(unsigned char *)(it + 0x28)) == 3 &&
        D_L03_001BB640[0x454 + (short)h] == 0 &&
        (((*(int *)((char *)D_0014171B + 0xAB75 + ((short)h >> 5) * 4 + (D_0015EE84 << 8)) >> (h & 0x1F)) ^ 1) & 1)) {
        unsigned char *p5;
        p5 = (unsigned char *)D_0013D50F + 1;
        if (p5[5] == 0) {
            p5[5] = 1;
            func_0022EE28(1, 0, 0);
            func_L00_00264DB8(0x53DB, -1);
        }
        *(u128z *)blk = *(u128z *)(m + 0x10);
        *(u128z *)vec = 0;
        blk[2] = blk[2] + 1.0f;
        func_L00_0025F4A8(m, vec, blk, 0.0f, 0.0f, 10, 3, 16, 4.0f, 2.0f, 9.0f, 1.0f, 0, 15.0f, 1, 1, -1, 0);
        m[0x20] = itype;
        func_00213D28(m, 1, 0);
        h = *(unsigned short *)(m + 0xB2);
        {
            int rnd = D_0015EE84;
            int *t1 = (int *)((char *)D_0014171B + 0xAB75 + ((short)h >> 5) * 4 + (rnd << 8));
            *t1 = *t1 | (1 << (h & 0x1F));
            D_L03_001BA8E0[(short)h >> 5] = D_L03_001BA8E0[(short)h >> 5] | (1 << (h & 0x1F));
        }
    }
    m[0xA4] = 0xFF;
    if (D_0013D5CA[0xA] != 0) {
        if (*(unsigned char *)(m + 0x20) != 3) return;
    }
    switch (*(unsigned char *)(m + 0x20)) {
    case 0: {
        char *p = *(char **)(D_0013E633 + 0x2E9D);
        s64x t = *(s64x *)(p + 0x38);
        m[0x20] = 1;
        *(s64x *)(m + 0x38) = t;
        func_L00_002676E8(m, d);
        if (D_0013D4A5[4] != 0) {
            *(short *)(d + 4) = 1;
            *(short *)(d + 0x36) = 2;
        }
        break;
    }
    case 1: {
        if (func_L00_00267290(m, d)) {
            D_0013D4A5[4] = 1;
            func_L01_00279398(2.7f, m);
            m[0x20] = 2;
        }
        break;
    }
    case 2: {
        int v;
        char *p;
        if (D_L03_0015F6A8 == 2) break;
        m[0x20] = 1;
        if (*(short *)(d + 4) != 2) break;
        v = *(int *)(d + 0x4C);
        if (v == -1) break;
        p = *(char **)&D_L03_0015F16C + v * 128;
        func_L00_00286128(p + 0x30, p + 0x70);
        break;
    }
    case 3: {
        float a[4];
        float b[4];
        float c[4];
        float f20;
        float z = 0.0f;
        float k25 = 0.25f;
        float k5 = 0.5f;
        float k1 = 1.0f;
        float t;
        t = func_00214158();
        f20 = func_001F9F90(t);
        t = func_002140F8(z, k25) * D_0015EE6C;
        f20 = f20 * t;
        a[0] = f20;
        t = func_00214158();
        f20 = func_001F9FA8(t);
        t = func_002140F8(z, k25) * D_0015EE6C;
        a[2] = z;
        f20 = f20 * t;
        a[1] = f20;
        t = func_00214158();
        f20 = func_001F9F90(t);
        t = func_002140F8(z, k5);
        t = t * func_002140F8(z, k1);
        f20 = f20 * t;
        b[0] = f20;
        t = func_00214158();
        f20 = func_001F9FA8(t);
        t = func_002140F8(z, k5);
        f20 = f20 * t;
        t = func_002140F8(z, k1);
        b[2] = z;
        f20 = f20 * t;
        b[1] = f20;
        func_L00_001FF240(c, b, m + 0x10);
        if (func_002140B0(3) == 0) {
            int r;
            r = func_001F9850(func_L00_00258BC8(0x3C, 0x78));
            func_L00_0026DD70(b, a, 0x20202020, 0x7F7F7F, 157500.0f, r);
        }
        break;
    }
    }
}
