/* NON_MATCHING func_L14_002235D0 -- src/overlays/l14_oltanis/help_0021E3A8.c
 * Best so far: BYTES 8/608 (98.7% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L14_002235D0: per-frame camera follow update. Picks target values (0x228/0x22C/0x230 of the D_0013F450 bl
 *   Budget spent at p8.c: same size (608), 18 bytes differ. Shape that got there: switch(t){case 0,case 3,default:
 *   Remaining: (1) 0.35 constant is 0x3EB33334, write 0.35000002f (not 0.35f); (2) block-2 loads: retail loads 0x1
 *   z07: p14.c best (8 bytes): 0.35000002f and 0x198 > 0x420 fixed. Left: join-block float regs (retail a=f0,.45=f
 */
extern float D_0015EE60 MACRO_ADDR;
extern float D_0015EE64 MACRO_ADDR;
extern float D_0015EE6C MACRO_ADDR;
extern float func_001F9D48(float *, float *);
extern f32 func_00214D28(f32 *p, f32 target, f32 maxstep);
extern float func_L00_0025C918(float *p, float *v, float t, float u1, float u2, float eps);

/* Picks the camera follow targets from the game state and eases the camera toward them. */
void func_L14_002235D0(void) {
    char *g = D_0013E633 + 0xE1D;
    char *g2;
    char *g3;
    char *g4;
    char *g5;
    float *p;
    int t = *(unsigned char *)(g + 0x20A4);
    float d, a, b;

    switch (t) {
    case 0:
        a = 0.8f;
        b = 0.7f;
        break;
    case 3:
        a = 0.8f;
        b = 0.6f;
        break;
    default:
        goto skip;
    }
    *(float *)(g + 0x230) = 0.45f;
    *(float *)(g + 0x22C) = b;
    *(float *)(g + 0x228) = a;
skip:
    g2 = D_0013E633 + 0xE1D;
    if (*(int *)(g2 + 0x208C) == 4) {
        if (*(int *)(g2 + 0x198) > *(int *)(g2 + 0x420)) {
            if (*(short *)(g2 + 0x41E) == 0) {
                *(float *)(g2 + 0x22C) = *(float *)(g2 + 0x434);
            }
        }
    } else if (*(int *)(g2 + 0x2084) == 6) {
        *(float *)(g2 + 0x22C) = 0.5f;
    } else if (*(int *)(g2 + 0x2084) == 4) {
        *(float *)(g2 + 0x228) = 0.35000002f;
    } else if (*(unsigned char *)(g2 + 0x12E2) != 0 && *(int *)(g2 + 0x300) != 0) {
        *(float *)(g2 + 0x228) = 0.8f;
        *(float *)(g2 + 0x22C) = 0.9f;
    } else if ((g5 = D_0013E633 + 0xE1D, *(int *)(g5 + 0x2084) == 0x7F)) {
        *(float *)(g5 + 0x230) = 0.8f;
    }
    g3 = D_0013E633 + 0xE1D;
    if (*(unsigned char *)(g3 + 0x257) != 0 && *(int *)(g3 + 0x2094) != 0x12
        && *(int *)(g3 + 0x208C) != 0x11 && *(unsigned char *)(g3 + 0x12E4) == 0) {
        d = func_001F9D48((float *)(g3 + 0x210), (float *)(g3 + 0x80));
        if (!(*(float *)(g3 + 0x234) * 0.5f < d)) return;
    }
    p = (float *)(D_0013E633 + 0x103D);
    g4 = D_0013E633 + 0xE1D;
    func_00214D28(p, *(float *)(g4 + 0x228), D_0015EE60 * 0.02f);
    func_00214D28(p + 1, *(float *)(g4 + 0x22C), D_0015EE60 * 0.02f);
    func_L00_0025C918(p + 5, p + 6, *(float *)(g4 + 0x230),
                      D_0015EE64 * 0.02f, D_0015EE64 * 0.3f, D_0015EE6C * 4.0f);
}
