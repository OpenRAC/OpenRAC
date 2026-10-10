/* NON_MATCHING func_L14_002FE798 -- src/overlays/l14_oltanis/vendor_002E0538.c
 * Best so far: SIZE ours 1264 / retail 1272, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Level 14 emitter: three/four bursts of func_L00_0026DEA0 sprites (two loops of 2 and 3 steps, one with a negat
 *   Left: the 1.0f constant is re-materialised at each call instead of kept in $f21 (retail keeps it as a register
 */
typedef int q128 __attribute__((mode(TI)));

extern short D_L14_00162030;
extern short D_L14_00162034;
extern short D_L14_00162038;
extern short D_L14_0016203C;
extern short D_L14_00162040;
extern short D_L14_00162044;
extern float D_L14_0015F660[] MACRO_ADDR;
extern void func_001F9C30(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern int func_002140B0(int);
extern char *func_L00_0026DEA0(void *, int, void *, int, float, float, float, float);
extern int func_001F9850(int);
extern int func_L00_00258BC8(int, int);
extern void func_001F9BF0(void *, void *, void *);

/* Spawns the effect sprites around a moby in state 2 or 4 and up. */
void func_L14_002FE798(char *m) {
    float v0[4];
    float v1[4];
    q128 v2;
    char *pd0;
    char *q;
    char *e;
    char *p;
    int k;
    int r;
    int r16;
    int r18;
    int r19;
    int a5;
    int s20;
    int s22;
    int s23;
    float f20;
    float f21;
    float f22;
    float one;

    unsigned char st = m[0x20];

    if (st == 0 || st == 3 || st == 1) return;

    q = m + 0xC0;
    func_001F9C30(v0, q, *(float *)&D_L14_00162030);
    func_001F9BD8(v1, m + 0x10, v0);
    func_001F9C30(v0, m + 0xE0, *(float *)&D_L14_00162038);
    func_001F9BD8(v1, v1, v0);
    v2 = *(q128 *)v1;
    pd0 = m + 0xD0;
    func_001F9C30(v0, pd0, *(float *)&D_L14_00162034);
    func_001F9BD8(v1, v1, v0);
    s20 = 0x7F;
    s22 = 2;
    k = 1;

    /* first burst */
    do {
        r18 = func_002140B0(0x10);
        r = func_002140B0(2);
        a5 = (r == 0) ? r18 : -r18;
        e = func_L00_0026DEA0(v1, a5, D_L14_0015F660, *(int *)&D_L14_00162044, 0.2f, 1.0f, 0.9f, 200000.0f);
        if (e != 0) {
            *(short *)(e + 0xA) = func_001F9850(12);
            p = e + 0x20;
            *(int *)(p + 4) = s22;
            p[0xA] = s20;
            p[0xB] = e[0xA];
        }
        k = k - 1;
    } while (k >= 0);

    f20 = 100000.0f;
    func_001F9C30(v0, q, *(float *)&D_L14_0016203C);
    s23 = 2;
    r18 = 0x10;
    f22 = 20000.0f;
    one = 1.0f;
    f21 = one;
    func_001F9BD8(v1, v1, v0);
    r19 = func_001F9850(2);
    s22 = 0x7F;
    s20 = 2;
    k = 2;
    do {
        e = func_L00_0026DEA0(v1, r18, D_L14_0015F660, 0x7FFFFFFF, 0.05f, one, f21, f20);
        if (e != 0) {
            *(short *)(e + 0xA) = r19;
            r = func_002140B0(0xFF);
            e[8] = (char)r;
            p = e + 0x20;
            *(int *)(p + 4) = s23;
            p[0xA] = s22;
            p[0xB] = e[0xA];
        }
        r18 = -r18;
        r19 = r19 << 1;
        k = k - 1;
        f20 = f20 - f22;
    } while (k >= 0);

    f20 = 300000.0f;
    func_001F9C30(v0, q, *(float *)&D_L14_00162040);
    func_001F9BD8(v1, &v2, v0);
    r16 = func_L00_00258BC8(0x40, 0x80);
    r16 = r16 | ((r16 << 16) | ((r16 << 8) | 0x7F000000));
    if (func_002140B0(3) != 0) {
        r18 = func_002140B0(4);
        r = func_002140B0(2);
        a5 = (r == 0) ? r18 : -r18;
        e = func_L00_0026DEA0(v1, a5, D_L14_0015F660, r16, 0.05f, 1.0f, 1.0f, f20);
        r16 = (int)e;
        if (e != 0) {
            p = e + 0x20;
            *(short *)(e + 0xA) = func_001F9850(0x3C);
            r = func_002140B0(2);
            if (r != 0) e[3] = 0x44;
            p[0xA] = 0x40;
            *(int *)(p + 4) = 2;
            p[0xB] = (char)func_001F9850(0x3C);
        }
    }

    func_001F9C30(v0, pd0, *(float *)&D_L14_00162034);
    s22 = 2;
    s20 = 0x7F;
    k = 1;
    func_001F9BF0(v1, &v2, v0);

    /* second burst */
    do {
        r18 = func_002140B0(0x10);
        r = func_002140B0(2);
        a5 = (r == 0) ? r18 : -r18;
        e = func_L00_0026DEA0(v1, a5, D_L14_0015F660, *(int *)&D_L14_00162044, 0.2f, 1.0f, 0.9f, 200000.0f);
        if (e != 0) {
            *(short *)(e + 0xA) = func_001F9850(12);
            p = e + 0x20;
            *(int *)(p + 4) = s22;
            p[0xA] = s20;
            p[0xB] = e[0xA];
        }
        k = k - 1;
    } while (k >= 0);

    f20 = 100000.0f;
    func_001F9C30(v0, q, *(float *)&D_L14_0016203C);
    f22 = 20000.0f;
    one = 1.0f;
    f21 = one;
    func_001F9BD8(v1, v1, v0);
    r18 = 0x10;
    r19 = func_001F9850(2);
    s23 = 2;
    s22 = 0x7F;
    s20 = 2;
    k = 2;
    do {
        e = func_L00_0026DEA0(v1, r18, D_L14_0015F660, 0x7FFFFFFF, 0.05f, one, f21, f20);
        if (e != 0) {
            *(short *)(e + 0xA) = r19;
            r = func_002140B0(0xFF);
            e[8] = (char)r;
            p = e + 0x20;
            *(int *)(p + 4) = s23;
            p[0xA] = s22;
            p[0xB] = e[0xA];
        }
        r18 = -r18;
        r19 = r19 << 1;
        k = k - 1;
        f20 = f20 - f22;
    } while (k >= 0);
}
