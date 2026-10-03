/* NON_MATCHING func_L00_0023AC68 -- src/overlays/shared/hud_00235960.c
 * Best so far: SIZE ours 888 / retail 880, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_0023AC68 (HudBoltAlertDraw): draws a three-part bolt-alert icon (two fades clamped to 0..1 from m[0x7
 *   Best is p2/p3 (113/880 bytes): size and all integer registers match. Left: (1) the two clamped floats are swap
 *   Would unblock: the source shape that makes the allocator give the second (long-lived) float the lower callee-s
 */
extern int D_0013E600[];
extern int D_0015EE80 MACRO_ADDR;
extern int D_L00_0015F6B0 MACRO_ADDR;
extern short D_L00_0015F9CC;
extern short D_L00_0015F9D0;
extern short D_L00_0015F9D4;
extern short D_L00_0015F9D8;
extern short D_L00_0015F9DC;
extern short D_L00_0015F9E0;
extern short D_L00_0015F9E4;
extern short D_L00_0015F9E8;
extern char D_L00_0015F9F0[];
extern void func_00200650(int, int, int, int, int, int);
extern void func_L00_0023BAB8(char *, int, int, int, int, int);
extern int func_00116248(char *str, const char *fmt, ...);
extern float func_001F9FA8(float);
extern int func_001FA8A8(int, int, float);
extern void func_L00_001FB7F8(void *, void *, void *, void *, void *);

// Draws the bolt-alert icon pieces, the label and the pulsing bolt counter text.
int func_L00_0023AC68(char *m) {
    char buf[0x80];
    int y;
    int x;
    int a;
    int b;
    int c;
    int d;
    int xx;
    int yy;
    int col1;
    int col2;
    float f20;
    float f21;
    int tex;
    char *p;

    y = D_0013E600[1] - 0x2A;
    if (D_0015EE80 == 0) {
        y = D_0013E600[1] - 0x32;
    }
    x = *(int *)(m + 0x50);
    if (*(unsigned char *)(D_0013F450 + 0x20A4) != 2 && *(int *)(D_0013F450 + 0x2084) != 0x32 && *(unsigned char *)(m + 0x70) != 0) {
        p = m + 0x70;
        f20 = func_001FA888(*(unsigned char *)(m + 0x70));
        f21 = f20 / func_001FA888(D_L00_0015F8D0);
        if (f21 > 1.0f) {
            f21 = 1.0f;
        } else if (f21 < 0.0f) {
            f21 = 0.0f;
        }
        f20 = func_001FA888(*(unsigned char *)(p + 1));
        f20 = f20 / func_001FA888(D_L00_0015F8D0);
        if (f20 > 1.0f) {
            f20 = 1.0f;
        } else if (f20 < 0.0f) {
            f20 = 0.0f;
        }
        d = func_001FA898(f20 * 128.0f);
        col1 = func_001FA898((float)*(int *)&D_L00_0015F9CC * f21);
        a = x + *(int *)&D_L00_0015F9D4;
        b = func_001FA898((float)*(int *)&D_L00_0015F9D0 * f21);
        c = a - b;
        func_00200650(func_00200198(0x7580, 1), a + 0x10, y, 0x20, 0x20, col1);
        func_00200468(func_00200198(0x7580, 0), c, y, b + 0x10, 0x20, col1);
        func_00200468(func_00200198(0x7580, 1), c - 0x20, y, 0x20, 0x20, col1);
        tex = func_00200198(0x754F, (D_L00_0015F6B0 % 0x3C) >> 1);
        func_L00_0023BAB8(m, tex, x - 0x20, y, 0, d);
        func_00116248(buf, D_L00_0015F9F0);
        xx = x + *(int *)&D_L00_0015F9D8;
        yy = y + *(int *)&D_L00_0015F9DC;
        a = func_001F9850(*(int *)&D_L00_0015F9E8);
        {
            float t = func_001FA888(a);
            float r = func_001F9FA8((float)(D_L00_0015F6B0 % a) / t * 6.2831855f - 3.1415927f) * 4.0f - 3.0f;
            if (r < 0.0f) {
                r = 0.0f;
            }
            f20 = r * f20;
        }
        col1 = func_001FA8A8(0, 0x80000000, f20);
        col2 = func_001FA8A8(*(int *)&D_L00_0015F9E0, *(int *)&D_L00_0015F9E4, f20);
        func_L00_001FB7F8((void *)(xx + 1), (void *)(yy + 1), (void *)col1, buf, (void *)-1);
        func_L00_001FB7F8((void *)xx, (void *)yy, (void *)col2, buf, (void *)-1);
    }
    return *(int *)(m + 0x58);
}
