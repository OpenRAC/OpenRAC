/* NON_MATCHING func_L11_0031FDA0 -- src/overlays/l11_pokitaru/vendor_0031EFC0.c
 * Best so far: SIZE ours 564 / retail 576, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Level 11 HUD gauge draw (gfx packets via func_00234C98, 3 gauge groups, flag from 3 func_00215570 tests). Best
 *   Left: retail keeps D_L11_00167700+0x140 in s0 (addiu s0,v0,0x140) and reloads lui for each D_L11_001626A0 read
 */
extern void func_00234C98(int, long);
extern void func_001F7868(void);
extern int func_00215570(void *arg0, int arg1);
extern int func_001F4868(int);
extern void func_L11_0031FB18(int arg);
extern void func_L11_0031FBF0(int);
extern void func_L11_0031FCC8(int);
extern char D_L11_00167700[];
extern unsigned char D_L11_001626A0 MACRO_ADDR;
extern int D_L11_0016265C MACRO_ADDR;
extern int D_L11_0016267C MACRO_ADDR;
extern short D_L11_00162658;
extern short D_L11_00162678;
extern short D_L11_00162688;

// Draws the level 11 HUD gauges: sets up the gfx packets and per-gauge draws.
void func_L11_0031FDA0(char *m) {
    int *d = *(int **)(m + 0x78);
    int flag = 0;
    func_00234C98(0x42, 0x80000044L);
    func_00234C98(8, 0);
    func_00234C98(0x14, 0xFF9000000260L);
    func_00234C98(0x47, 0x513F1L);
    func_001F7868();
    if (*(float *)(D_L11_00167700 + 0x148) > 255.0f
        || func_00215570(D_L11_00167700 + 0x140, d[0])
        || func_00215570(D_L11_00167700 + 0x140, d[1])
        || func_00215570(D_L11_00167700 + 0x140, d[2])) {
        flag = 1;
    }
    if (flag) {
        func_00234C98(6, func_001F4868(0x2C));
        func_00234C98(0x42, ((long)*(int *)&D_L11_00162658 << 32) | 0x44);
        func_L11_0031FB18(0);
    }
    if (D_L11_001626A0 != 0 && flag) {
        func_00234C98(6, func_001F4868(0x29));
        func_00234C98(0x42, ((long)((D_L11_0016265C * D_L11_001626A0) >> 8) << 32) | 0x68);
        func_L11_0031FB18(1);
        func_00234C98(0x42, ((long)((D_L11_0016265C * D_L11_001626A0) >> 8) << 32) | 0x62);
        func_L11_0031FB18(2);
    }
    func_00234C98(6, func_001F4868(0x2A));
    func_00234C98(0x42, ((long)*(int *)&D_L11_00162678 << 32) | 0x48);
    func_L11_0031FBF0(0);
    func_00234C98(0x42, ((long)D_L11_0016267C << 32) | 0x48);
    func_L11_0031FBF0(1);
    if (flag) {
        func_00234C98(6, func_001F4868(0x2B));
        func_00234C98(0x42, ((long)*(int *)&D_L11_00162688 << 32) | 0x48);
        func_L11_0031FCC8(0);
    }
}
