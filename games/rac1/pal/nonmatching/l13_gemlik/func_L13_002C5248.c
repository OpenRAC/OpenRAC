/* NON_MATCHING func_L13_002C5248 -- src/overlays/l13_gemlik/vendor_002C2638.c
 * Best so far: SIZE ours 1140 / retail 1128, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Picks a path point for a moby (path via d+0xA0, sphere/entry checks, two func_001F9D10 probes, then turn-rate 
 *   Runs: 10 of 10 spent. Tried: fewer locals (n, start folded to reads), block-scoped temporaries, a shared D_001
 */
extern char *D_L13_001B0AB0[];
extern char D_0013E633[];
extern float D_0015EE60 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern char *D_L13_00160058 MACRO_ADDR;
extern void func_001F9BF0(void *, void *, void *);
extern void func_001F9C30(void *, void *, float);
extern void func_L00_001FF548(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
void func_L13_002C5190(char *out, float scale);
int func_L13_002C4D50(char *pt, int *tbl, int start, float ref);
extern float func_001F9D10(void *, void *);
extern int func_001F9938(void *);
extern float func_001F9D48(void *, void *);
int func_L13_002C4F10(char *m, char *d, int aim);

// Picks a path point for a moby, steers it toward the best one and updates its turn rate.
int func_L13_002C5248(char *moby, char *d, int aim) {
    char buf[48];
    char *path = D_L13_001B0AB0[*(int *)(d + 0xA0)];
    char *p5;
    char *e;
    char *ref;
    int r17, r19, r21;
    float f0, f20, f3;
    if (path == 0) return 0;
    ref = D_0013E633 + 0xE9D;
    p5 = *(char **)(D_0013E633 + 0x240D);
    if (p5 != 0 && *(short *)(p5 + 0xA6) == 0x45 && *(int *)(d + 0xBC) != -1 &&
        (e = D_L13_00160058 + (*(int *)(d + 0xBC) << 8)) != 0 &&
        *(unsigned char *)(e + 0x20) != 0xFE && *(unsigned char *)(e + 0x20) != 0xFD) {
        func_001F9BF0(buf, p5 + 0x10, e + 0x10);
        func_001F9C30(buf, buf, 0.5f);
        func_L00_001FF548(buf, buf, 100.0f);
        func_001F9BD8(buf, buf, e + 0x10);
    } else if (p5 != 0 && *(short *)(p5 + 0xA6) == 0x45) {
        func_L13_002C5190(buf, 140.0f);
    } else {
        char *src = D_0013E633 + 0xE9D;
        if (*(int *)(d + 0xBC) != -1) {
            e = D_L13_00160058 + (*(int *)(d + 0xBC) << 8);
            if (e != 0) {
                unsigned char st = *(unsigned char *)(e + 0x20);
                if (st != 0xFE && st != 0xFD) src = e + 0x10;
            }
        }
        qcopy(buf, src);
    }
    {
        int r16 = func_L13_002C4D50(buf, (int *)path, *(short *)(d + 0xB4), 0.0f);
        r17 = r16 + 5;
        if (r17 >= *(int *)path) r17 = r17 - *(int *)path;
        qcopy(buf + 0x10, path + 0x10 + (r17 << 4));
        f20 = func_001F9D10(buf + 0x10, ref);
        r16 = r16 - 5;
        if (r16 < 0) r16 = r16 + *(int *)path;
        qcopy(buf + 0x20, path + 0x10 + (r16 << 4));
        f0 = func_001F9D10(buf + 0x20, ref);
        if (f0 < f20) {
            r16 = r17;
            r21 = 1;
            r17 = r16;
        } else {
            r21 = -1;
            if (f20 < f0) {
                r17 = r16;
            } else if (*(signed char *)(d + 0xD0) == 1) {
                r16 = r17;
                r21 = 1;
                r17 = r16;
            } else {
                r17 = r16;
            }
        }
    }
    r19 = 0;
    for (;;) {
        int t = r17 << 4;
        f0 = func_001F9D10(buf, path + t + 0x10);
        if (!(f0 < 30.0f)) break;
        if (!(r19 < *(int *)path)) break;
        r17 = (r21 + r17) % *(int *)path;
        r19++;
    }
    {
    int r16 = *(short *)(d + 0xB4);
    r21 = 1;
    if (r16 < r17) {
        r21 = -1;
        if ((r17 - r16) < (*(int *)path - r17 + r16)) r21 = 1;
    } else {
        if ((r16 - r17) < (*(int *)path - r16 + r17)) r21 = -1;
    }
    }
    f20 = func_001F9D10(moby + 0x10, path + (r17 << 4) + 0x10);
    if (r21 == *(signed char *)(d + 0xD0)) {
        float f2, f5;
        f0 = f20 / 60.0f;
        f3 = *(float *)(d + 0xCC);
        f2 = D_0015EE60 * 0.07f;
        f5 = D_0015EE6C * 100.0f;
        f0 = (f0 - f3) * f2;
        f3 = f3 + f0;
        *(float *)(d + 0xCC) = f3;
        if (f5 < f3) *(float *)(d + 0xCC) = f5;
    } else if (func_001F9938(d + 0xD2) != 0) {
        float f1, f2;
        *(short *)(d + 0xD2) = 10;
        f1 = D_0015EE60 * -0.04000002145767212f + 1.0f;
        f0 = *(float *)(d + 0xCC);
        f0 = f0 * f1;
        *(float *)(d + 0xCC) = f0;
        f3 = func_001F9D48(moby + 0x10, ref);
        f2 = D_0015EE60 * 0.1f;
        f1 = *(float *)(d + 0xCC);
        if ((f1 < f2) | (*(unsigned char *)(moby + 0x31) == 0) | (f3 < 40.0f)) {
            short b4 = *(short *)(d + 0xB4);
            unsigned short b6 = *(unsigned short *)(d + 0xB6);
            *(signed char *)(d + 0xD0) = r21;
            *(short *)(d + 0xB4) = b6;
            *(short *)(d + 0xB6) = b4;
        }
    }
    return func_L13_002C4F10(moby, d, aim);
}
