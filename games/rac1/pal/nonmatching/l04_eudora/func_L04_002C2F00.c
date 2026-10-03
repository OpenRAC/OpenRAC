/* NON_MATCHING func_L04_002C2F00 -- src/overlays/l04_eudora/vendor_0029FCF0.c
 * Best so far: SIZE ours 488 / retail 492, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Per-mode smoothing of two values (switch on byte 0x53, cases 0..7), else releases its slot. p0.c matches every
 */
extern int func_L00_0028EB98(void *, int);
extern int func_0022ED80_i(int, int, char *) __asm__("func_0022ED80");
extern float func_001FA888(int);
extern void func_L00_0028EBF0(int);
extern float func_00214D28(float *, float, float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern int func_L00_0028F210(int, int);
extern unsigned char D_0013E633[];
extern float D_0015EE6C MACRO_ADDR;

// Updates a moby's two smoothed values from its mode and releases its slot when done.
void func_L04_002C2F00(char *moby) {
    char *data = *(char **)(moby + 0x78);
    float a;
    float b;
    if (*(unsigned char *)(moby + 0x20) != 9) {
        int r = func_L00_0028EB98(moby, *(int *)(data + 0x238));
        if (r == 0) {
            if (*(unsigned char *)(moby + 0x53) < 8) {
                *(int *)(data + 0x238) = func_0022ED80_i(0, 4, moby);
                *(float *)(data + 0x230) = func_001FA888(0x400);
                *(int *)(data + 0x234) = 0;
            }
        }
        switch (*(unsigned char *)(moby + 0x53)) {
        case 0:
            b = 0.0f;
            a = func_001FA888(0x400) * 0.6f;
            goto common;
        case 1:
            b = 3.0f;
            a = func_001FA888(0x400);
            goto common;
        case 2:
            b = -2.0f;
            a = func_001FA888(0x400);
            goto common;
        case 3:
        case 4:
            b = 4.0f;
            a = func_001FA888(0x400);
            goto common;
        case 5:
        case 6:
        case 7:
            b = 0.0f;
            a = func_001FA888(0x400) * 0.8f;
            goto common;
        default:
            break;
        }
    }
    {
        int idx = *(int *)(data + 0x238);
        if (idx != -1) {
            char *e = (char *)D_0013E633 + 0x1D + idx * 0x70;
            if (*(char **)(e + 0x88) == moby) {
                if (*(unsigned char *)(e + 0x74) != 0)
                    func_L00_0028EBF0(idx);
            }
        }
        *(int *)(data + 0x238) = -1;
    }
    return;
common:
    func_00214D28((float *)(data + 0x230), a, func_001FA888(0x400) * D_0015EE6C * 2.0f);
    func_00214D28((float *)(data + 0x234), b, D_0015EE6C * 8.0f);
    func_L00_0028F210(*(int *)(data + 0x238), func_001FA898_r(*(float *)(data + 0x230)));
    func_L00_0028F210(*(int *)(data + 0x238), func_001FA898_r(*(float *)(data + 0x234)));
}
