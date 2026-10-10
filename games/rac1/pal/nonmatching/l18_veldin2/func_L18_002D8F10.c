/* NON_MATCHING func_L18_002D8F10 -- src/overlays/l18_veldin2/vendor_002A8400.c
 * Best so far: SIZE ours 1084 / retail 1092, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Best candidate p3.c: 1084 of 1092 bytes, a jump-table update (moby[0x20] states 0..5). The state bodies, the D
 *   Left: GCC lays the state-1/3 body (shared through cross-jumping) after the state-2 code, where retail puts it 
 */
extern float func_00214D88(float *, float *, float, float, float, float);
extern int func_0022ED80_2F0F78(int, int, int) __asm__("func_0022ED80");
extern void func_L00_0028EBF0(int);
extern void func_001F49B0(void (*)(void), void *);
extern void func_L18_002D9488(char *);
extern int func_001F9908(int *);
extern float func_001F9D48(void *, void *);
extern int func_001F9850(int);
extern int func_L00_00203F20(int a, int b);
extern float func_00214D28(float *p, float target, float maxstep);
extern char D_0013E633[];
extern char D_0014171B[];
extern float D_0015EE70 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern int D_0015EE84 MACRO_ADDR;
extern int D_0015EFA4 MACRO_ADDR;
extern unsigned char D_0013D50F NOT_SDA;
extern short D_L18_00161AA0;
extern short D_L18_00161AA4;
extern short D_L18_00161AA8;
extern short D_L18_00161420;

// Update for the thruster pack lock moby: a state machine on moby[0x20] that steers and retargets its spring.
void func_L18_002D8F10(char *moby) {
    char *d = *(char **)(moby + 0x78);
    char *x;
    char *w;
    int r, r2, r3, r4, r6, t;
    float f0;

    switch ((unsigned char)moby[0x20]) {
    case 1:
        func_00214D88((float *)(moby + 0x18), (float *)(d + 0x68), *(float *)(d + 0x64) + *(float *)&D_L18_00161AA8, D_0015EE70 * 20.0f, D_0015EE70 * 40.0f, D_0015EE6C * 20.0f);
        break;
    case 3:
        func_00214D88((float *)(moby + 0x18), (float *)(d + 0x68), *(float *)(d + 0x64) + *(float *)&D_L18_00161AA8, D_0015EE70 * 20.0f, D_0015EE70 * 40.0f, D_0015EE6C * 20.0f);
        break;
    case 0:
        *(float *)(d + 0x64) = *(float *)(moby + 0x18);
        *(short *)(d + 0x3E) = *(short *)(d + 0x3E) | 8;
        if (*(int *)(d + 0x60) != 0) {
            moby[0x20] = 1;
            *(float *)(moby + 0x2C) = *(float *)(*(char **)(moby + 0x24) + 0x24) * *(float *)&D_L18_00161AA0;
        } else {
            moby[0x20] = 4;
        }
        break;
    case 2:
        func_00214D88((float *)(moby + 0x18), (float *)(d + 0x68), *(float *)(d + 0x64) + *(float *)&D_L18_00161AA4, D_0015EE70 * 5.0f, D_0015EE70 * 10.0f, D_0015EE6C * 20.0f);
        if (*(short *)(D_0013E633 + 0xE1D + 0x30E) == 0 && *(int *)(D_0013E633 + 0xE1D + 0x2FC) == (int)moby && *(int *)(D_0013E633 + 0xE1D + 0x2084) == 0x22) {
            moby[0x20] = 3;
            func_0022ED80_2F0F78(0, 0, (int)moby);
            r = *(int *)(d + 0x70);
            if (r != -1) {
                char *p = D_0013E633 + 0x1D + r * 0x70;
                if (*(int *)(p + 0x88) == (int)moby && *(unsigned char *)(p + 0x74) != 0) {
                    func_L00_0028EBF0(r);
                }
            }
            *(int *)(d + 0x70) = -1;
        } else {
            if (*(int *)(D_0013E633 + 0xE1D + 0x2084) != 0x72) {
                func_001F49B0((void (*)(void))func_L18_002D9488, moby);
                if (func_001F9908((int *)(d + 0x6C)) != 0) {
                    *(int *)&D_L18_00161420 = 1;
                }
            }
        }
        break;
    case 4:
        if (*(short *)(D_0013E633 + 0xE1D + 0x30E) == 0) {
            if (*(int *)(D_0013E633 + 0xE1D + 0x2FC) == (int)moby) {
                if (*(int *)(D_0013E633 + 0xE1D + 0x2084) == 0x22) {
                    f0 = *(float *)(moby + 0x18) - 1.5f;
                    moby[0x20] = 5;
                    moby[0xBC] = 1;
                    *(float *)(moby + 0x18) = f0;
                    if (D_0015EE84 == 0x12) {
                        D_0013D50F = 1;
                    }
                }
            }
        }
        f0 = func_001F9D48(moby + 0x10, D_0013E633 + 0xE9D);
        if (f0 < 2.5f) {
            w = D_0014171B + 0x34D;
            if ((*(int *)(w + 0x15814) & (1 << D_0015EE84)) == 0) {
                if (*(unsigned short *)(w + 0x388) != 0) {
                    r = func_001F9850(D_0015EFA4);
                    r2 = func_001F9850(0x12);
                    r6 = r - *(unsigned short *)(w + 0x38A) * 600;
                    t = (int)((float)r2 * 60.0f);
                    if (t < r6 || *(unsigned short *)(w + 0x38A) == 0) {
                        func_L00_00203F20(0x2B02, 0x71);
                    } else {
                        r3 = func_001F9850(D_0015EFA4);
                        if (*(unsigned short *)(w + 0x38A) < r3 / 600) {
                            r4 = func_001F9850(D_0015EFA4);
                            *(short *)(w + 0x38A) = r4 / 600;
                        }
                    }
                } else {
                    *(short *)(w + 0x388) = 1;
                    r = func_001F9850(D_0015EFA4);
                    if (*(unsigned short *)(w + 0x38A) < r / 600) {
                        r2 = func_001F9850(D_0015EFA4);
                        *(short *)(w + 0x38A) = r2 / 600;
                    }
                    *(int *)(w + 0x38C) = *(int *)(w + 0x38C) | (1 << D_0015EE84) | 0x80000000;
                }
            }
        }
        break;
    case 5:
        if (func_00214D28((float *)(moby + 0x18), *(float *)(d + 0x64) - 1.5f, 0.15f) == 0.0f) {
            moby[0x20] = 6;
        }
        break;
    }
}
