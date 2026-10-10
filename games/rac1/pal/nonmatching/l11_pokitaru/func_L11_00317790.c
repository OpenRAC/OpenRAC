/* NON_MATCHING func_L11_00317790 -- src/overlays/l11_pokitaru/vendor_00312BD8.c
 * Best so far: SIZE ours 1464 / retail 1444, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Level 11 moby update (class 1248): seven-state float machine that builds two child mobys (func_0020D348 twice,
 *   Stopped at 6 runs of 10. Still open: retail keeps 0.01745 in $f20 from case 0 onward and computes it after the
 */
extern float D_0015EE70 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern float D_L11_0016239C SDATA(D_L11_0016239C);
extern float D_L11_001623A0 SDATA(D_L11_001623A0);
extern float D_L11_00162398 SDATA(D_L11_00162398);
extern char D_0013E633[];
extern char *func_0020D348(int);
extern float func_001F9D48(float *, float *);
extern int func_00215570(void *arg0, int arg1);
extern void func_0022ED80(int, int, int);
extern float func_00214D88(float *, float *, float, float, float, float);
extern void func_L00_001FF4B0(void *, void *, float);
extern void func_001F9BD8(void *, void *, void *);
extern void func_L00_00251E30(void *);

/* Update function for moby class 1248 on level 11: a seven-state machine that builds two child mobys. */
void func_L11_00317790(unsigned char *moby)
{
    char *data = *(char **)(moby + 0x78);
    float buf[4];
    float k = 0.01745329238474369f;

    switch (moby[0x20]) {
    case 0: {
        char *p16;
        float v = *(float *)(moby + 0x2C);
        char *rd;

        v = v + v;
        *(float *)(moby + 0x2C) = v;
        *(char **)data = func_0020D348(0x4DF);
        p16 = D_0013E633 + 0xE1D;
        *(short *)(*(char **)data + 0x32) = 0x40;
        (*(char **)data)[0x31] = 1;
        *(long *)(*(char **)data + 0x38) = *(long *)(*(char **)(p16 + 0x2080) + 0x38);
        *(unsigned short *)(*(char **)data + 0x34) = *(unsigned short *)(moby + 0x34) | 0x20;
        qcopy(*(char **)data + 0x10, moby + 0x10);
        *(float *)(*(char **)data + 0x18) = *(float *)(*(char **)data + 0x18) - 4.0f;
        qcopy(*(char **)data + 0x40, moby + 0x40);
        *(float *)(*(char **)data + 0x40) = -(D_L11_0016239C * k);
        *(float *)(*(char **)data + 0x44) = D_L11_001623A0 * k;
        rd = *(char **)(*(char **)data + 0x78);
        *(char **)(rd + 8) = rd + 0x20;
        *(int *)(rd + 0x5C) = *(int *)(rd + 0x5C) | 1;
        *(char **)(data + 4) = func_0020D348(0x4DF);
        *(short *)(*(char **)(data + 4) + 0x32) = 0x40;
        (*(char **)(data + 4))[0x31] = 1;
        *(long *)(*(char **)(data + 4) + 0x38) = *(long *)(*(char **)(p16 + 0x2080) + 0x38);
        *(unsigned short *)(*(char **)(data + 4) + 0x34) = *(unsigned short *)(moby + 0x34) | 0x8020;
        qcopy(*(char **)(data + 4) + 0x10, moby + 0x10);
        *(float *)(*(char **)(data + 4) + 0x18) = *(float *)(*(char **)(data + 4) + 0x18) - 4.0f;
        qcopy(*(char **)(data + 4) + 0x40, moby + 0x40);
        *(float *)(*(char **)(data + 4) + 0x40) = D_L11_0016239C * k;
        *(float *)(*(char **)(data + 4) + 0x44) = D_L11_001623A0 * k;
        rd = *(char **)(*(char **)(data + 4) + 0x78);
        *(char **)(rd + 8) = rd + 0x20;
        *(int *)(rd + 0x5C) = *(int *)(rd + 0x5C) | 1;
        moby[0x20] = 1;
        break;
    }
    case 1: {
        char *p16 = D_0013E633 + 0xE9D;
        if (func_001F9D48((float *)(moby + 0x10), (float *)p16) < 12.0f) {
            if (func_00215570(p16, *(int *)(data + 0x10)) != 0) {
                *(int *)(data + 8) = 0;
                moby[0x20] = 2;
                func_0022ED80(0, 4, *(int *)data);
            }
        }
        break;
    }
    case 2:
        func_00214D88((float *)(data + 8), (float *)(data + 0xC), 1.0f,
                      D_0015EE70 * 8.0f, D_0015EE70 * 16.0f, D_0015EE6C * 8.0f);
        func_L00_001FF4B0(buf, moby + 0xC0, 0.5f);
        func_001F9BD8(*(char **)data + 0x10, moby + 0x10, buf);
        *(float *)(*(char **)data + 0x18) = (*(float *)(moby + 0x18) + *(float *)(data + 8) * 4.0f) - 4.0f;
        func_001F9BD8(*(char **)(data + 4) + 0x10, moby + 0x10, buf);
        *(float *)(*(char **)(data + 4) + 0x18) = (*(float *)(moby + 0x18) + *(float *)(data + 8) * 4.0f) - 4.0f;
        func_L00_00251E30(*(char **)data);
        func_L00_00251E30(*(char **)(data + 4));
        if (1.0f <= *(float *)(data + 8)) {
            *(int *)(data + 8) = 0;
            moby[0x20] = 3;
        }
        break;
    case 3:
        func_00214D88((float *)(data + 8), (float *)(data + 0xC), 1.0f,
                      D_0015EE70 * 10.0f, D_0015EE70 * 20.0f, D_0015EE6C * 10.0f);
        *(float *)(*(char **)data + 0x44) = (1.0f - *(float *)(data + 8)) * (D_L11_001623A0 * k);
        *(float *)(*(char **)(data + 4) + 0x44) = (1.0f - *(float *)(data + 8)) * (D_L11_001623A0 * k);
        func_L00_00251E30(*(char **)data);
        func_L00_00251E30(*(char **)(data + 4));
        if (1.0f <= *(float *)(data + 8)) {
            *(int *)(data + 8) = 0;
            moby[0x20] = 4;
        }
        break;
    case 4:
        func_00214D88((float *)(data + 8), (float *)(data + 0xC), 1.0f,
                      D_0015EE70 * 12.0f, D_0015EE70 * 24.0f, D_0015EE6C * 12.0f);
        *(float *)(*(char **)data + 0x18) = *(float *)(moby + 0x18) + *(float *)(data + 8) * 0.25f;
        *(float *)(*(char **)(data + 4) + 0x18) = *(float *)(moby + 0x18) + *(float *)(data + 8) * 0.25f;
        *(float *)(*(char **)data + 0x40) = -(*(float *)(data + 8) * 1.5707963705062866f);
        *(float *)(*(char **)(data + 4) + 0x40) = *(float *)(data + 8) * 1.5707963705062866f;
        func_L00_00251E30(*(char **)data);
        func_L00_00251E30(*(char **)(data + 4));
        if (1.0f <= *(float *)(data + 8)) {
            *(int *)(data + 8) = 0;
            moby[0x20] = 5;
        }
        break;
    case 5:
        break;
    case 6:
        func_L00_001FF4B0(buf, moby + 0xC0, 0.5f);
        func_001F9BD8(*(char **)data + 0x10, moby + 0x10, buf);
        *(float *)(*(char **)data + 0x18) = *(float *)(moby + 0x18) + D_L11_00162398;
        *(float *)(*(char **)data + 0x40) = -(D_L11_0016239C * k);
        *(float *)(*(char **)data + 0x44) = D_L11_001623A0 * k;
        func_L00_00251E30(*(char **)data);
        func_001F9BD8(*(char **)(data + 4) + 0x10, moby + 0x10, buf);
        *(float *)(*(char **)(data + 4) + 0x18) = *(float *)(moby + 0x18) + D_L11_00162398;
        *(float *)(*(char **)(data + 4) + 0x40) = -(D_L11_0016239C * k);
        *(float *)(*(char **)(data + 4) + 0x44) = D_L11_001623A0 * k;
        func_L00_00251E30(*(char **)(data + 4));
        break;
    }
}
