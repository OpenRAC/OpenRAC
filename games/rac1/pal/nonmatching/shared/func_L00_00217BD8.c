/* NON_MATCHING func_L00_00217BD8 -- src/overlays/shared/help_00214D60.c
 * Best so far: BYTES 8/520 (98.5% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawns ten particles per call (random scale/colour via func_002140F8, position from D_0013F450 data, then func
 *   p3/p4/p5 are 8 bytes off (BYTES 8/520): the loop decrement addiu sits in the beq delay slot (ours, +0xfc) wher
 */
extern int func_001F9850(int);
extern float func_002140F8(float, float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern float func_00214158(void);
extern void func_00215C00(void *, float, float, float);
extern float func_001F9F90(float x);
extern float func_001F9FA8(float);
extern void func_L00_0026A7F8(void *, void *, int, int, int, int, int, int);
extern float D_0015EE6C MACRO_ADDR;
extern int D_0015EE84 MACRO_ADDR;
extern char D_0013F4D0[];

/* spawns ten particles around the player */
void func_L00_00217BD8(void) {
    float tmp[4];
    float buf[4];
    int r = func_001F9850(0x28);
    int i;
    char *h = D_0013F4D0;
    char *g = h - 0x80;
    i = 9;
    do {
        int a, b;
        float f23, f21, f20, f22;
        a = func_001FA898_r(func_002140F8(4200.0f, 7350.0f) / 1000.0f);
        b = func_001FA898_r(func_002140F8(21000.0f, 31500.002f) / 1000.0f);
        f23 = func_002140F8(D_0015EE6C * 0.15f, D_0015EE6C * 0.7f);
        qcopy(tmp, h);
        if (D_0015EE84 != 6) tmp[2] = *(float *)(g + 0x2F4);
        f21 = func_00214158();
        i--;
        f20 = func_002140F8(0.34906584f, 1.3962634f);
        f22 = func_002140F8(0.05f, *(float *)(g + 0x234) + 0.2f);
        func_00215C00(buf, f23, f21, f20);
        tmp[0] = tmp[0] + func_001F9F90(f21) * f22;
        tmp[1] = tmp[1] + func_001F9FA8(f21) * f22;
        func_L00_0026A7F8(tmp, buf, 0x7000C0F0, 0x70F0, r, a, b, 1);
    } while (i >= 0);
}
