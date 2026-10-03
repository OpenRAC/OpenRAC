/* NON_MATCHING func_L17_002F1D20 -- src/overlays/l17_fleet/vendor_002F1558.c
 * Best so far: SIZE ours 556 / retail 564, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Unblock: find the source form that keeps the dead sltiu (it likely perturbs register allocation too) and the d
 *   n08 (p5-p8, budget spent): best is p6.c (switch(mode){case 2:A; case 0:B; default:A}) at 556/564 bytes; its sa
 *   Left: the dead `lbu; sltiu 0x10` of moby[0x20] before the first call (an unused local `int ok = moby[0x20] < 1
 *   Unblock: a source form that keeps the dead sltiu (likely a cast or macro whose result is discarded) would also
 *   t05 (q0-q4, budget spent): q2.c (p6 with separate locals rr=x+0x32, gg=y+0xB4, bb=z+0x14 clamped in their own 
 *   Unblock: a source form that keeps the dead sltiu and the double mode test in retail's A-first layout.
 *   u05 (p9, 4 runs used by compile slips): p6 plus an expression statement `moby[0x20] < 16;` ahead of the data l
 *   v06 (p10, p11; 2 runs, stopped): best.c plus `(void)(moby[0x20] < 16);` (p10) and `n = moby[0x20] < 16;` befor
 */
extern int func_001F9850(int);
extern float func_001FA888(int);
extern float func_001F9FA8(float);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern int func_001FA8A8(int, int, float);
extern void func_L00_00250800(void *, int, void *);
extern void func_001F49B0(void (*)(void), void *);
extern void func_L17_002F1F58(char *);
extern int D_L17_0015F6B0;

// Pulses a moby's colour from a random phase and queues its draw callback.
void func_L17_002F1D20(unsigned char *moby) {
    char *data;
    int n;
    int alpha;
    int mode;
    int i;
    int x, y, z;
    float f;
    int color;
    n = moby[0x20] < 16;
    data = *(char **)(moby + 0x78);
    n = func_001F9850(0xAA);
    alpha = 0x80;
    mode = 0;
    switch (moby[0x20]) {
    case 4: case 5: case 6:
        mode = 1;
        break;
    case 7: case 8: case 9: case 10: case 11: case 13: case 14: case 15:
        mode = 2;
        break;
    case 12:
        mode = -1;
        break;
    }
    if (mode >= 0) {
        if (mode == 2) {
            n = func_001F9850(0x32);
        } else if (mode == 1) {
            n = func_001F9850(0x5A);
        }
        f = func_001F9FA8(((float)(D_L17_0015F6B0 % n) / func_001FA888(n)) * 2.0f * 3.1415927f + -3.1415927f);
        x = func_001FA898_r(f * 20.0f);
        y = func_001FA898_r(f * 70.0f);
        z = func_001FA898_r(f * 10.0f);
        y += 0xB4;
        z += 0x14;
        x += 0x32;
        if (y > 255) y = 255;
        if (x > 255) x = 255;
        switch (mode) {
        case 2:
            color = (alpha << 24) | (z << 16) | ((x / 2) << 8) | y;
            break;
        case 0:
            color = (alpha << 24) | (z << 16) | (y << 8) | x;
            break;
        default:
            color = (alpha << 24) | (z << 16) | ((x / 2) << 8) | y;
            break;
        }
        *(int *)(moby + 0x90) = func_001FA8A8(*(int *)(moby + 0x90), color, 0.1f);
        for (i = 0; i < 3; i++) {
            func_L00_00250800(moby, i + 1, data + 0x200 + i * 16);
        }
        func_001F49B0(func_L17_002F1F58, moby);
    }
}
