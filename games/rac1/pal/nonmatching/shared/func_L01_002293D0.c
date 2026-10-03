/* NON_MATCHING func_L01_002293D0 -- src/overlays/shared/help_002274A8.c
 * Best so far: BYTES 79/364 (78.3% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Menu/help per-frame check: reads state at D_0013E633+0xE1D+0x2084, sets a flag from two timer compares (func_0
 *   Everything matches structurally (base pointer re-added before each block, movn, % 4) except one thing: retail 
 *   Wall: one symbol read both gp-relative and through lui/lw; no plain-C spelling found (an asm name like "D_0015
 */
extern int func_001F9850(int);
extern void func_L00_00251358(void *, void *, void *, void *);
extern void func_L00_00251328(void *, int, int, int);
extern void func_L00_002078E0(void);
extern void func_L00_00207948(void *, char *);
extern short D_0015EE84;
extern int D_0015EE84_m __asm__("D_0015EE84") MACRO_ADDR;
extern unsigned char D_0013E15A[];

/* Per-frame check of the menu/help state: flags a prompt when a timer passes a threshold, draws it, and dispatches the pending action. */
void func_L01_002293D0(void) {
    int buf[3];
    char *g = (char *)D_0013E633 + 0xE1D;
    int flag = 0;
    int s = *(int *)(g + 0x2084);
    if (s == 0x80 || s == 0x82) {
        flag = *(int *)(g + 0x198) < func_001F9850(0x1E);
        s = D_0015EE84_m;
    } else {
        s = *(int *)&D_0015EE84;
    }
    if (s == 0xF || s == 0x11) {
        g = (char *)D_0013E633 + 0xE1D;
        if (*(int *)(g + 0x2084) == 0x76) {
            if (*(int *)(g + 0x198) < func_001F9850(0x14)) flag = 1;
        }
    }
    if (flag) {
        g = (char *)D_0013E633 + 0xE1D;
        func_L00_00251358(*(void **)(g + 0x2080), &buf[0], &buf[1], &buf[2]);
        if (*(int *)(g + 0x198) % 4 < 3) {
            buf[0] = 0;
            buf[1] = 0;
            buf[2] = 0;
        } else {
            buf[1] = 0x90;
            buf[2] = 0xF0;
            buf[0] = 0x90;
        }
        g = (char *)D_0013E633 + 0xE1D;
        func_L00_00251328(*(void **)(g + 0x2080), buf[0], buf[1], buf[2]);
        func_L00_002078E0();
    }
    g = (char *)D_0013E633 + 0xE1D;
    {
        int idx = *(int *)(g + 0x10B8);
        if (idx >= 0 && D_0013E15A[0x4C6 + idx] != 0) {
            void *p = *(void **)(g + 0x1090);
            if (p != 0) func_L00_00207948(p, *(char **)(g + 0x2080));
        }
    }
}
