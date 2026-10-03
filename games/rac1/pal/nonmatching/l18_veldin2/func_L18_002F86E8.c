/* NON_MATCHING func_L18_002F86E8 -- src/overlays/l18_veldin2/vendor_002F2AE0.c
 * Best so far: BYTES 38/1048 (96.4% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Best: p7.c, BYTES 61/1048 (size matches). 12 runs used.
 *   Mattered: gp floats as `short` decls read via *(float*)&; D_L18_00162430 needs a gp alias
 *   (`short X_g __asm__("D_L18_00162430")`) because the file declares it MACRO_ADDR; 0.003f constant;
 *   unsigned char moby (no sll/sra); table as 32-byte struct array (index-first addu mult fixed).
 *   Remaining diffs (alloc/schedule only): float reg numbering in the lerp (ours t=$f3, retail $f5),
 *   order of the three loop-prelude luis (retail A310,167BD0,167700; ours 167700 first), v0/v1 for
 *   the g+0x180 load, and daddu $a0 / mov.s $f12 order before the two func_00214D88 calls.
 *   p3..p8 variants (locals for out pointers, g inside loop, x2/y2 locals) compiled to identical bytes.
 */
extern float func_001F9D48(void *, void *);
extern void func_L06_00317770(char *moby);
extern void func_001FFDA0(int arg0, int arg1);
extern void func_L00_002E9900(float x, float y, int flag);
extern void func_L00_002E9968(float x, float y);
extern int func_001FFB38(int, int, int, int, int, int, int);
extern float func_00214D88(float, float, float, float, float *, float *);
extern void func_L00_00236710(void);
extern void func_L00_00236830(void);
extern void func_L06_0024A080(void);
typedef struct { char pad[0x1C]; char *p; } Rec32;
extern Rec32 *D_L18_0015F050 MACRO_ADDR;
extern char *D_L18_0016016C MACRO_ADDR;
extern int D_L18_0016A310[];
extern char D_L18_00167BD0[];
extern char D_L18_00167700[];
extern float D_0015EE70 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern char D_0013E633[];
extern short D_L18_00162440;
extern short D_L18_00162444;
extern short D_L18_00162448;
extern short D_L18_0016244C;
extern short D_L18_00162450;
extern short D_L18_00162454;
extern short D_L18_00162458;
extern short D_L18_0016245C;
extern short D_L18_00162460;
extern short D_L18_00162464;
extern short D_L18_00162468;
extern short D_L18_0016246C;
extern short D_L18_00162470;
extern short D_L18_00162474;
extern short D_L18_00162430_g __asm__("D_L18_00162430");

void func_L18_002F86E8(unsigned char *m) {
    char *d = *(char **)(m + 0x78);
    char *p = D_L18_0015F050[*(int *)(d + 0x224)].p;
    float f, t, fx, fy, a, b;
    func_001F9908((int *)(d + 0x360));
    f = func_001F9D48(D_0013E633 + 0xE9D, d + 0x3C0);
    if (f > *(float *)&D_L18_0016244C) {
        f = *(float *)&D_L18_0016244C;
    } else if (f < *(float *)&D_L18_00162448) {
        f = *(float *)&D_L18_00162448;
    }
    f = (f - *(float *)&D_L18_00162448) / (*(float *)&D_L18_0016244C - *(float *)&D_L18_00162448);
    a = *(float *)&D_L18_00162458;
    b = *(float *)&D_L18_00162450;
    fx = (b - a) * f + a;
    fy = (*(float *)&D_L18_00162454 - *(float *)&D_L18_0016245C) * f + *(float *)&D_L18_0016245C;
    if (m[0x20] == 0x13) {
        char *a = D_0013E633 + 0xE1D;
        if (*(int *)(a + 0x208C) == 0xF || *(int *)(a + 0x2084) == 0x42) {
            if (*(int *)(d + 0x364) == 0 && *(float *)(d + 0x390) <= 25.0f && *(int *)(d + 0x360) == 0) {
                fx = *(float *)&D_L18_00162460;
                fy = *(float *)&D_L18_00162464;
                goto done;
            }
        }
        if (func_001F9D48(D_0013E633 + 0xE9D, D_L18_0016016C + (*(int *)(d + 0x344) << 7) + 0x30) < 28.0f) {
            fx = *(float *)&D_L18_00162468;
            fy = *(float *)&D_L18_0016246C;
        }
    } else if (m[0x20] == 9 || m[0x20] == 6) {
        fx = *(float *)&D_L18_00162470;
        fy = *(float *)&D_L18_00162474;
    } else if (m[0x20] == 0 || m[0x20] == 0xA || m[0x20] == 0xB || m[0x20] == 1 || m[0x20] == 0x1B) {
        char *a = D_0013E633 + 0xE1D;
        int i;
        for (i = 0; i < 0x30; i++) {
            char *r = D_L18_00167BD0 + i * 0xA0;
            if (*(int *)(d + 0x3EC) == 0 && *(int *)(a + 0x2FC) != 0) {
                if (*(short *)(*(char **)(a + 0x2FC) + 0xA6) == 0x24B) {
                    *(int *)(d + 0x3EC) = 1;
                }
            }
            if (D_L18_0016A310[i] != 0) {
                if (*(short *)(r + 0x86) == 0x12) {
                    int n = -1;
                    char *g;
                    func_L06_00317770((char *)r);
                    if (*(int *)(d + 0x368) != n) {
                        func_001FFDA0(*(int *)(d + 0x368), 0);
                        *(int *)(d + 0x368) = n;
                    }
                    if (m[0x20] < 2) {
                        goto done;
                    }
                    if (*(int *)(d + 0x3EC) == 0 && *(int *)&D_L18_00162430_g == 0) {
                        goto done;
                    }
                    g = D_L18_00167700;
                    if (*(char **)(g + 0x180) == 0) {
                        goto done;
                    }
                    if (*(short *)(*(char **)(g + 0x180) + 0x86) == 0) {
                        func_L00_002E9900(*(float *)&D_L18_00162470, 0.003f, 0);
                        func_L00_002E9968(*(float *)&D_L18_00162474, 0.003f);
                    }
                    goto done;
                }
            }
        }
    } else if (*(float *)(d + 0x38C) > 0.0f) {
        *(int *)(d + 0x368) = func_001FFB38(0x16, 0xFFFF, (int)func_L00_00236710, (int)func_L00_00236830,
                                             (int)func_L06_0024A080 + 0x20, (int)(d + 0x36C), 0x15E);
    }
done:
    func_00214D88(fx, *(float *)&D_L18_00162444 * D_0015EE70, *(float *)&D_L18_00162444 * D_0015EE70,
                  *(float *)&D_L18_00162440 * D_0015EE6C, (float *)(p + 0x34), (float *)(d + 0x378));
    func_00214D88(fy, *(float *)&D_L18_00162444 * D_0015EE70, *(float *)&D_L18_00162444 * D_0015EE70,
                  *(float *)&D_L18_00162440 * D_0015EE6C, (float *)(p + 0x38), (float *)(d + 0x37C));
}
