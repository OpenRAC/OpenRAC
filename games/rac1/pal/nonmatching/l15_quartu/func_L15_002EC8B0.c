/* NON_MATCHING func_L15_002EC8B0 -- src/overlays/l15_quartu/vendor_0029C1D0.c
 * Best so far: BYTES 5/500 (99.0% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   UpdateMoby_1419 state machine (5-case jump table, case order 0,2,1,3,4 in source). p1.c/p3.c: whole function m
 *   retail has D_0015EE84 in $v1 and moby[0xB0] in $v0 ($v0 = $v0 + $v1, then + table base); ours swaps $v0/$v1 (5
 *   Reordering operands, int-cast form and a hoisted local did not move it (a local made it worse). Register alloc
 */
extern char D_0013E633[];
extern char D_0014171B[];
extern int D_0015EE84 MACRO_ADDR;
extern int D_L15_0015F6A8 MACRO_ADDR;
extern short D_L15_00161B48;
extern short D_L15_00160058;
extern char *D_L15_0016016C MACRO_ADDR;
extern int func_00215570(void *arg0, int arg1);
extern void func_L00_00299B68(int);
extern void func_L00_002512D8(int idx);
extern void func_L00_00233868(void);
extern void func_L00_0029A7D0(int);
extern void func_L00_00233950(void);
extern void func_L00_00217718(void *, void *, int, int);
extern void func_L00_00261848(int);
extern void func_L00_00286128(void *a0, void *a1);
extern void func_L00_00263DB0(int a);
extern int func_0020BFC8(int slot, int flags);
extern void func_001F49B0(void (*)(void), void *);
extern void func_L15_002ECAA8(void);

/* UpdateMoby_1419: state machine for a timed object */
void func_L15_002EC8B0(unsigned char *moby) {
    int *data = *(int **)(moby + 0x78);
    moby[0x30] = 0x80;
    switch (moby[0x20]) {
    case 0:
        if (*(unsigned char *)(moby[0xB0] + D_0015EE84 * 16 + (int)(D_0014171B + 0xAA35)) != 0xFF
            && *(int *)&D_L15_00161B48 == 0
            && func_00215570(D_0013E633 + 0xE9D, data[0])) {
            if (*(unsigned char *)(*(char **)&D_L15_00160058 + data[2] * 256 + 0x20) == 3) {
                func_L00_00299B68(3);
                moby[0x20] = 2;
                func_L00_002512D8(moby[0xB0]);
                func_L00_00233868();
            }
        }
        break;
    case 2:
        if (D_L15_0015F6A8 != 2) {
            func_L00_0029A7D0(0x11);
            moby[0x20] = 1;
        }
        break;
    case 1:
        if (D_L15_0015F6A8 != 2) {
            func_L00_00299B68(4);
            moby[0x20] = 3;
            func_L00_00233950();
            if (data[1] != -1) {
                char *p = D_L15_0016016C + data[1] * 128;
                func_L00_00217718(p + 0x30, p + 0x70, 0, 1);
            }
        }
        break;
    case 3:
        if (D_L15_0015F6A8 != 2) {
            func_L00_00261848(0x11);
            if (data[1] != -1) {
                char *p = D_L15_0016016C + data[1] * 128;
                func_L00_00286128(p + 0x30, p + 0x70);
            }
            func_L00_00263DB0(0x11);
            func_0020BFC8(0, -1);
            moby[0x20] = 4;
        }
        break;
    case 4:
        break;
    }
    if (D_L15_0015F6A8 == 2) func_001F49B0(func_L15_002ECAA8, moby);
}
