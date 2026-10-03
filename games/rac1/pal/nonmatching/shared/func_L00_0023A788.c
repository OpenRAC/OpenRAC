/* NON_MATCHING func_L00_0023A788 -- src/overlays/shared/hud_00235960.c
 * Best so far: SIZE ours 1012 / retail 1020, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   HudBoltsDraw: draws the two-digit bolt meter bar (textures 0x7580/0x754F) and the bolt total with a drop shado
 *   Structure decoded fully (p6.c, 573/1020 bytes, same instruction stream); difference is saved-register allocati
 *   Unblock: find a wording that lowers x0's allocation priority below e (p0-p7 tried reuse of locals, c/e->cnt mi
 */
extern float func_001FA888(int);
extern int func_001FA898(float);
extern int func_00200198(int, int);
extern void func_00200650(int, int, int, int, int, int);
extern void func_00200468(int, int, int, int, int, int);
extern void func_L00_0023BAB8(char *, int, int, int, int, int);
extern int func_001FA8A8(int, int, float);
extern int func_00116248(char *, const char *, ...);
extern void func_001F6CF8_c(int, int, int, char *, int) __asm__("func_001F6CF8");
extern char D_0013E633[];
extern int D_0015EE80 MACRO_ADDR;
extern int D_0015EE98 MACRO_ADDR;
extern int D_0015EE88 MACRO_ADDR;
extern short D_L00_0015F904;
extern short D_L00_0015F908;
extern short D_L00_0015F90C;
extern short D_L00_0015F910;
extern short D_L00_0015F914;
extern short D_L00_0015F918;
extern short D_L00_0015F91C;
extern short D_L00_0015F920;
extern short D_L00_0015F924;
extern short D_L00_0015F928;
extern short D_L00_0015F92C;
extern char D_L00_0015F930[];
extern char D_L00_0015F938[];
extern char D_L00_0015F940[];

/* Draws the bolt counter: a bar of two digit meters, then the bolt total with a shadow. */
int func_L00_0023A788(char *e) {
    short *p = *(short **)(e + 0x80);
    char buf[16];
    unsigned char *c = (unsigned char *)(e + 0x70);
    int flag;
    if (*(int *)(D_0013E633 + 0x2EA1) != 0x32 && c[0] != 0) {
        int x0 = *(int *)(e + 0x50);
        int y = D_0015EE80 ? 10 : 0x12;
        float r1, r2;
        int n = 1;
        int v;
        int alpha, w, xa, xb, xc;
        r1 = func_001FA888(c[0]) / func_001FA888(*(int *)&D_L00_0015F904);
        if (r1 > 1.0f) r1 = 1.0f;
        else if (r1 < 0.0f) r1 = 0.0f;
        r2 = func_001FA888(c[1]) / func_001FA888(*(int *)&D_L00_0015F908);
        if (r2 > 1.0f) r2 = 1.0f;
        else if (r2 < 0.0f) r2 = 0.0f;
        xa = x0 - 0x1C;
        xb = x0 - 0xC;
        xc = x0 - 0x20;
        alpha = func_001FA898((float)func_001FA898(r1 * 128.0f) * 0.7f);
        v = D_0015EE98;
        while (v >= 10) {
            v /= 10;
            n++;
        }
        flag = 0;
        w = func_001FA898((float)(*(int *)&D_L00_0015F92C * n) * r1);
        xa -= w;
        func_00200650(func_00200198(0x7580, 1), xb, y, 0x20, 0x20, alpha);
        func_00200468(func_00200198(0x7580, 0), xa, y, w + 0x10, 0x20, alpha);
        func_00200468(func_00200198(0x7580, 1), xa - 0x20, y, 0x20, 0x20, alpha);
        func_L00_0023BAB8(e, func_00200198(0x754F, *p >> 1), xc, y, 0, 0x80);
        xc = func_001FA8A8(*(int *)&D_L00_0015F90C, *(int *)&D_L00_0015F910, r2);
        xb = func_001FA8A8(0, 0x80000000, r2);
        func_00116248(buf, D_L00_0015F930, D_0015EE98);
        func_001F6CF8_c(x0 + *(int *)&D_L00_0015F914 + 1, y + *(int *)&D_L00_0015F918 + 1, xb, buf, -1);
        func_001F6CF8_c(x0 + *(int *)&D_L00_0015F914, y + *(int *)&D_L00_0015F918, xc, buf, -1);
        if (D_0015EE88 != 3) flag = D_0015EE88 != 5;
        func_00116248(buf, flag ? D_L00_0015F938 : D_L00_0015F940);
        {
            int tx = flag ? *(int *)&D_L00_0015F91C : *(int *)&D_L00_0015F924;
            int ty = flag ? *(int *)&D_L00_0015F920 : *(int *)&D_L00_0015F928;
            int i;
            for (i = 3; i < n; i += 3) {
                func_001F6CF8_c(x0 + tx - *(int *)&D_L00_0015F92C * i + 2, y + ty + 2, xb, buf, -1);
                func_001F6CF8_c(x0 + tx - *(int *)&D_L00_0015F92C * i, y + ty, xc, buf, -1);
            }
        }
    }
    return *(int *)(e + 0x58);
}
