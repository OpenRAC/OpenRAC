/* NON_MATCHING func_L01_0030CDF0 -- src/overlays/l01_novalis/vendor_002FABE8.c
 * Best so far: SIZE ours 1096 / retail 1112, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Moby class 1510 update on level 01 (1112 bytes): steers the moby's three float values toward targets, checks t
 *   Differences left: f21 (1.0) is constant-folded into the calls so retail's saved $f21 and the `mov.s $f13,$f21`
 *   Unblock: a way to keep 1.0f live across the calls without folding it (retail keeps it in $f21), and a shared b
 */
extern char D_L01_00174340[];
extern float D_L01_0015F660[] MACRO_ADDR;
extern float D_0015EE60 MACRO_ADDR;
extern float D_0015EE70_s __asm__("D_0015EE70") MACRO_ADDR;
extern float D_L01_001620F0 SDATA(D_L01_001620F0);
extern float D_L01_001620EC SDATA(D_L01_001620EC);
extern float D_L01_001620E8 SDATA(D_L01_001620E8);
extern int func_002140B0(int);
extern void func_001F49B0(void *, void *);
extern void func_L01_0030D380(void);
extern void func_001F9BD8(void *, void *, void *);
extern float func_001F9B88(float);
extern float func_001FA748(float, float);
extern int func_L00_001F10E0(float, void *, int, void *);
extern int func_001F9908(int *);
extern void func_L00_001FF610(void *, void *, void *);
extern void func_001F9BC0(void *);
extern void func_L00_0025F4A8(void *, void *, void *, float, float, int, int, int, float, float, float, float, int, float, int, int, int, int);
extern void func_0020D678(void *);
extern float func_002140F8(float, float);
extern int func_001F9850(int);
extern int func_L00_00258BC8(int, int);
extern void func_L00_0026DA50(void *, void *, int, int, int, int, float);
extern char *func_L00_0026DEA0(void *, int, void *, int, float, float, float, float);

// Update function for moby class 1510 on level 01: steers the moby toward its target and emits its effect.
void func_L01_0030CDF0(char *m) {
    char *c;
    int v;
    int ret2;
    int v16;
    int v2;
    int r17;
    int r16;
    int t;
    int i;
    int k23;
    float f0;
    float f1;
    float f2;
    float f3;
    float f20;
    float f21;

    if (func_002140B0(2) != 0) {
        func_001F49B0(func_L01_0030D380, m);
    }
    c = *(char **)(m + 0x78);
    func_001F9BD8(m + 0x10, m + 0x10, c);
    f0 = func_001F9B88(*(float *)c);
    f3 = D_0015EE60;
    f1 = D_L01_001620F0 * f3;
    if (f1 < f0) {
        f2 = 1.0f;
        f0 = D_L01_001620EC;
        f1 = *(float *)c;
        f0 = (f0 - f2) * f3 + f2;
        *(float *)c = f1 * f0;
    }
    f0 = func_001F9B88(*(float *)(c + 4));
    f3 = D_0015EE60;
    f1 = D_L01_001620F0 * f3;
    if (f1 < f0) {
        f2 = 1.0f;
        f0 = D_L01_001620EC;
        f1 = *(float *)(c + 4);
        f0 = (f0 - f2) * f3 + f2;
        *(float *)(c + 4) = f1 * f0;
    }
    f3 = *(float *)(c + 8);
    if (0.0f < f3) {
        f2 = 1.0f;
        f0 = D_L01_001620EC;
        f1 = D_0015EE60;
        f0 = (f0 - f2) * f1 + f2;
        *(float *)(c + 8) = f3 * f0;
    }
    f2 = D_0015EE70_s;
    f0 = D_L01_001620E8 * f2;
    f1 = *(float *)(c + 8);
    f1 = f1 - f0;
    *(float *)(c + 8) = f1;
    f0 = func_001FA748(*(float *)(m + 0x40), *(float *)(c + 0x10));
    *(float *)(m + 0x40) = f0;
    f0 = func_001FA748(*(float *)(m + 0x44), *(float *)(c + 0x14));
    *(float *)(m + 0x44) = f0;
    f0 = func_001FA748(*(float *)(m + 0x48), *(float *)(c + 0x18));
    *(float *)(m + 0x48) = f0;
    if (!(*(float *)(m + 0x10) < 2.0f) && !(1020.0f < *(float *)(m + 0x10)) && !(*(float *)(m + 0x14) < 2.0f) && !(1020.0f < *(float *)(m + 0x14)) && !(*(float *)(m + 0x18) < 2.0f) && !(1020.0f < *(float *)(m + 0x18))) {
        v = func_L00_001F10E0(2.0f, m + 0x10, 0, (void *)*(int *)(c + 0x24));
        ret2 = func_001F9908((int *)(c + 0x20));
        if (ret2 == 0 && v == 0) {
            goto d0d0;
        }
        if (v != 0) {
            if (*(int *)(D_L01_00174340 + 0x1C) == 0) {
                func_001F9BC0(c);
            } else {
                func_L00_001FF610(c, c, D_L01_00174340 + 0x40);
            }
        }
        func_L00_0025F4A8(m, c, m + 0x10, 0.0f, 0.0f, 20, 3, 4, 4.0f, 2.0f, 100000.0f, 3.0f, 0, 15.0f, 1, 1, -1, 0);
        func_0020D678(m);
        return;
    }
    func_0020D678(m);
    return;
d0d0:
    k23 = 0x60000000;
    f21 = 1.0f;
    i = 1;
    f0 = func_002140F8(0.5f, 1.5f);
    f20 = f0 * 210052.0f;
    v16 = func_001F9850(10);
    v2 = func_001F9850(20);
    func_L00_0026DA50(m + 0x10, D_L01_0015F660, 0x4F007FFF, 0x1FFFFFFF, func_L00_00258BC8(v16, v2), 1, f20);
    do {
        v16 = func_002140B0(6);
        t = func_002140B0(2);
        if (t != 0) {
            v16 = -v16;
        }
        f0 = func_002140F8(40000.0f, 100000.0f);
        f20 = f0;
        v2 = func_L00_00258BC8(48, 255);
        r17 = func_L00_0026DEA0(m + 0x10, v16, D_L01_0015F660, v2 | (v2 << 16) | ((v2 << 8) | k23), 0.1f, f21, f21, f20);
        if (r17 != 0) {
            *(unsigned char *)(r17 + 3) = 0x44;
            r16 = r17 + 0x20;
            *(short *)(r17 + 0xA) = func_001F9850(60);
            *(int *)(r16 + 4) = 2;
            *(unsigned char *)(r16 + 0xA) = 0x60;
            *(unsigned char *)(r16 + 0xB) = *(unsigned char *)(r17 + 0xA);
        }
        i = i - 1;
    } while (i >= 0);
}
