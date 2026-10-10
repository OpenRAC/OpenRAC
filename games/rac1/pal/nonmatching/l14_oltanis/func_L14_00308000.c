/* NON_MATCHING func_L14_00308000 -- src/overlays/l14_oltanis/vendor_002FF358.c
 * Best so far: SIZE ours 860 / retail 872, checked 2026-10-08.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * No longer builds in its file (COMPILE failed, 2026-10-09): match its declarations to the file's first.
 * What the last attempts found:
 *   Button moby update (class 1416, level 14): state 0 calls func_00214158 and checks level flags, then a table lo
 *   Left: one short-lived pseudo (regalloc.py pseudo 198, 4 refs) is pushed into saved $18 because it crosses a ca
 */
extern float func_00214158(void);
extern float func_001FA748(float, float);
extern float func_001F9FA8(float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern int func_0022ED80(int, int, int);
extern char *D_L14_001B0F30[];
extern int D_L14_001BAF60[];
extern float D_0015EE6C MACRO_ADDR;
extern int D_0015EE84 MACRO_ADDR;
extern char D_0013E633[];

// Button moby update (class 1416, level 14): aims at its target, tests the level flags, runs the list loop.
void func_L14_00308000(unsigned char *moby) {
    char *data;
    int i;
    int r;
    int x;
    int idx;
    float t;

    data = *(char **)(moby + 0x78);
    if (moby[0x20] == 0) {
        unsigned short b2;
        int g;
        *(float *)(data + 0xC) = func_00214158();
        *(float *)(moby + 0x18) = *(float *)(moby + 0x18) - 0.35f;
        b2 = *(unsigned short *)(moby + 0xB2);
        if (D_L14_001BBCC0[(short)b2 + 0x454] == 0) {
            g = D_0015EE84;
            if (((*(int *)(D_0014171B + 0xAB75 + ((((short)b2) >> 5) << 2) + (g << 8)) >> (b2 & 0x1F)) & 1) == 0) {
                if (*(int *)(data + 8) == 0) {
                    moby[0x20] = 1;
                    return;
                }
                if (*(unsigned char *)(D_0014171B + 0xAA35 + moby[0xB0] + (g << 4)) != 0xFF) {
                    moby[0x20] = 1;
                    return;
                }
            }
        }
        moby[0x20] = 2;
        *(int *)(moby + 0x90) = 0x80208020;
        moby[0xBC] = 2;
        idx = *(int *)data;
        if (idx == -1) {
            return;
        }
        if (*(int *)D_L14_001B0F30[idx] > 0) {
            i = 0;
            for (;;) {
                idx = *(int *)data;
                {
                    char *e = D_L14_001B0F30[idx] + (i << 4);
                    if (*(float *)(e + 0x1C) == *(float *)(data + 4)) {
                        *(int *)(e + 0x1C) = 0;
                    }
                }
                i++;
                idx = *(int *)data;
                if (!(i < *(int *)D_L14_001B0F30[idx])) {
                    break;
                }
            }
        }
    } else if ((signed char)moby[0x20] == 1) {
        unsigned short b2;
        int g;
        t = func_001FA748(*(float *)(data + 0xC), D_0015EE6C * 6.2831855f);
        *(float *)(data + 0xC) = t;
        t = func_001F9FA8(t) * 4.0f - 3.0f;
        x = func_001FA898_r(t * 128.0f);
        if (x >= 0x81) {
            r = 0x80;
        } else {
            r = 32;
            if (x > 31) {
                r = x;
            }
        }
        *(int *)(moby + 0x90) = 0x80000000 | (r << 8) | (r << 16) | r;
        if (*(char **)(D_0013E633 + 0xE1D + 0x2FC) != moby) {
            return;
        }
        if (*(short *)(D_0013E633 + 0xE1D + 0x30E) != 0) {
            return;
        }
        b2 = *(unsigned short *)(moby + 0xB2);
        g = D_0015EE84;
        *(int *)(D_0014171B + 0xAB75 + ((((short)b2) >> 5) << 2) + (g << 8)) |= 1 << (b2 & 0x1F);
        D_L14_001BAF60[(short)b2 >> 5] |= 1 << (b2 & 0x1F);
        moby[0x20] = 2;
        moby[0xBC] = 1;
        *(int *)(moby + 0x90) = 0x80208020;
        func_0022ED80(0, 0, (int)moby);
        idx = *(int *)data;
        if (idx == -1) {
            return;
        }
        if (*(int *)D_L14_001B0F30[idx] > 0) {
            i = 0;
            for (;;) {
                idx = *(int *)data;
                {
                    char *e = D_L14_001B0F30[idx] + (i << 4);
                    if (*(float *)(e + 0x1C) == *(float *)(data + 4)) {
                        *(int *)(e + 0x1C) = 0;
                    }
                }
                i++;
                idx = *(int *)data;
                if (!(i < *(int *)D_L14_001B0F30[idx])) {
                    break;
                }
            }
        }
    }
}
