/* NON_MATCHING func_L06_00228390 -- src/overlays/shared/help_0021D6B8.c
 * Best so far: BYTES 16/280 (94.3% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L06_00228390: resets the game-state block at D_0013F450 (mode 1 swaps 22A8/22AC, deletes mobys at 1620/16
 *   p3/p4/p5 (switch, separate ifs, inverted ||) and p2 all give the same bytes (16 instrs differ, only in the fin
 *   Looks like a register-allocation/delay-slot tie (a0 set before the test in retail). Unblock: find a source sha
 */
extern unsigned char D_0013F450[] __attribute__((section(".data")));
extern int D_L06_0015F6A8;
extern void func_0020D678(void *);
extern void func_L00_00205B50(void);
extern void func_L00_00207220(void);
extern void func_L00_00232EA8(void);
extern void func_L06_00235E08(int, int);

// Resets the shared game-state block and its mobys, then reloads the current level screen.
void func_L06_00228390(void) {
    char *g = D_0013F450;
    char *p;
    char *m;
    if (*(unsigned char *)(g + 0x20A4) == 1) {
        *(int *)(g + 0x22AC) = *(int *)(g + 0x22A8);
        *(int *)(g + 0x22A8) = *(short *)(g + 0x22B0);
        if (*(int *)(g + 0x1620)) {
            func_0020D678(*(void **)(g + 0x1620));
            *(int *)(g + 0x1620) = 0;
        }
        if (*(int *)(g + 0x1624)) {
            func_0020D678(*(void **)(g + 0x1624));
            *(int *)(g + 0x1624) = 0;
        }
    }
    func_L00_00205B50();
    g = D_0013F450;
    p = *(char **)(g + 0xA84);
    g[0x20A4] = 0;
    if (p) {
        *(unsigned short *)(p + 0x34) &= 0xFFF9;
    }
    *(int *)(g + 0xA84) = 0;
    if (*(int *)(g + 0xA8C)) {
        func_0020D678(*(void **)(g + 0xA8C));
        *(int *)(g + 0xA8C) = 0;
    }
    func_L00_00207220();
    m = *(char **)(g + 0xA88);
    *(char **)(g + 0x2080) = m;
    qcopy(m + 0x10, g + 0x80);
    *(int *)(m + 0x98) = 0;
    *(float *)(g + 0xA94) = 1.0f;
    func_L00_00232EA8();
    if (*(int *)(g + 0x2084) != 100 || (D_L06_0015F6A8 != 2 && D_L06_0015F6A8 != 6)) {
        func_L06_00235E08(0, 1);
        return;
    }
}
