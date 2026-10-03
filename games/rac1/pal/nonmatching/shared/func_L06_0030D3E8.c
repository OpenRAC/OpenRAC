/* NON_MATCHING func_L06_0030D3E8 -- src/overlays/shared/vendor_002FF000.c
 * Best so far: SIZE ours 476 / retail 472, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Draws a fading round marker (4-vertex packed longs, K=0x00FFFFF080008000) at a moby's projected position. Need
 */
extern void func_00234C98(int, long);
extern void func_001F2418(float *, void *);
extern int func_001FA898_r(float) __asm__("func_001FA898");
extern int func_001FA8A8(int, int, float);
extern int func_001F4868(int);
extern void func_L02_0020BF88(void *, void *, void *, int, int);
extern char D_0013E15A[];
extern short D_L06_001624AC;
extern short D_L06_001624B0;
extern short D_L06_001624B4;
extern short D_L06_001624B8;
extern short D_L06_001624A8;
extern short D_L06_001624A4;

// Draws a fading round marker at the moby's screen position.
extern void func_L06_0030D3E8_x(char *) __asm__("func_L06_0030D3E8");
void func_L06_0030D3E8_x(char *moby) {
    char *data = *(char **)(moby + 0x78);
    float pos[4];
    float scr[4];
    int col[4];
    int fill[4];
    long q[4];
    float a;
    float b;
    int x;
    int y;
    int r;
    int z;
    int *bp;
    qcopy(pos, moby + 0x10);
    a = *(float *)data;
    if (a > 1.0f) a = 1.0f;
    b = *(float *)data - 1.0f;
    if (b > 1.0f) {
        b = 1.0f;
    } else if (b < 0.0f) {
        b = 0.0f;
    }
    func_00234C98(0x47, 0x5380B);
    func_001F2418(scr, pos);
    bp = (int *)(D_0013E15A + 0x4A6);
    x = func_001FA898_r(scr[0] - (float)bp[4]);
    x -= 0x1000;
    y = func_001FA898_r(scr[1] - (float)bp[4]) - 0x1000;
    col[0] = *(int *)&D_L06_001624AC;
    col[1] = *(int *)&D_L06_001624B0;
    col[2] = *(int *)&D_L06_001624B4;
    col[3] = *(int *)&D_L06_001624B8;
    r = func_001FA8A8(*(int *)(data + 8), *(int *)(data + 0xC), b);
    fill[0] = r;
    fill[3] = r;
    fill[2] = r;
    fill[1] = r;
    r = func_001FA898_r((float)(*(int *)&D_L06_001624A8 << 4) * a);
    q[3] = (long)((y + r) << 16) + (long)0x00FFFFF080008000L + (x - r);
    q[1] = (long)((y - r) << 16) + (long)0x00FFFFF080008000L + (x - r);
    q[0] = (long)((y - r) << 16) + (long)0x00FFFFF080008000L + (x + r);
    q[2] = (long)((y + r) << 16) + (long)0x00FFFFF080008000L + (x + r);
    z = func_001F4868(*(int *)&D_L06_001624A4);
    func_L02_0020BF88(q, col, fill, z, 1);
    func_00234C98(0x47, 0x5360B);
}
