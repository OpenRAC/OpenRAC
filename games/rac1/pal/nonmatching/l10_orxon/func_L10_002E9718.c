/* NON_MATCHING func_L10_002E9718 -- src/overlays/l10_orxon/vendor_002E30F8.c
 * Best so far: SIZE ours 600 / retail 608, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   UpdateMoby for a bolt-cost pickup: state 0 waits for the player in range/cone and buys option 5 or 8 (subtract
 *   bolts, sets D_0015EEA0, state 1); state 1 shows a message and resets. Best p1.c/p2.c (56 bytes differ, same si
 *   Left: m/d saved regs swapped ($s0/$s1: retail m=$s1, d=$s0), the sltu result lands in $a2 not $a0, and the
 *   0x271C/0x271D select comes out as movz instead of retail's beqz + delay-slot li. Control flow otherwise identi
 *   Moving d's assignment, int* vs char* for d and if/else forms did not move the allocation.
 */
extern float func_L00_001FF860(float, float);
extern float func_001FA850(float, float);
extern float func_001F9D48(void *, void *);
extern void func_L00_002676A0(void *, int);
extern int func_00215F80(int, int);
extern void func_L00_00299B68(int);
extern int func_0020BFC8(int, int);
extern void func_L00_00264DB8(int, int);
extern int func_001F9850(int);
extern unsigned char D_0013E633[];
extern unsigned char D_0013D355[];
extern char D_0013A5E0[];
extern int D_L10_0015F6A8 MACRO_ADDR;
extern int D_0015EE98 MACRO_ADDR;
extern short D_0015EEA0;
extern short D_L10_0015F720;

// Update of a pickup-style moby: waits for the player to come near, then spends bolts on a purchase.
void func_L10_002E9718(char *m) {
    char *d = *(char **)(m + 0x78);
    unsigned char *g;
    unsigned char *p;
    int c;
    int one;
    func_001F9908((int *)(d + 4));
    if (*(unsigned char *)(m + 0x20) == 0) {
        g = D_0013E633 + 0xE1D;
        if (func_001FA850(*(float *)(m + 0x48), func_L00_001FF860(*(float *)(g + 0x80) - *(float *)(m + 0x10), *(float *)(g + 0x84) - *(float *)(m + 0x14))) < 1.5707964f) {
            if (func_001F9D48(m + 0x10, g + 0x80) < 4.0f) {
                if (*(int *)(d + 4) == 0) {
                    if (D_L10_0015F6A8 != 2) {
                        func_L00_002676A0(m, 1);
                        p = D_0013D355 + 0x13B;
                        if (p[4] == 0) {
                            if (D_0015EE98 > 4000) {
                                c = func_00215F80(1, 0x271E) != 0;
                                if ((*(int *)(D_0013A5E0 + 0x2604) & 0x10) && c) {
                                    one = 1;
                                    p[4] = one;
                                    *(int *)(g + 0x22A8) = 5;
                                    D_0015EE98 = D_0015EE98 - 4000;
                                    *(int *)&D_0015EEA0 = 5;
                                    func_L00_00299B68(0);
                                    m[0x20] = one;
                                }
                            } else {
                                func_00215F80(1, 0x2720);
                            }
                        } else if (p[5] == 0) {
                            if (*(int *)(d + 4) == 0) {
                                if (D_0015EE98 > 30000) {
                                    c = func_00215F80(1, 0x271F) != 0;
                                    if ((*(int *)(D_0013A5E0 + 0x2604) & 0x10) && c) {
                                        one = 1;
                                        p[5] = one;
                                        *(int *)(g + 0x22A8) = 8;
                                        D_0015EE98 = D_0015EE98 - 30000;
                                        *(int *)&D_0015EEA0 = 8;
                                        func_L00_00299B68(1);
                                        m[0x20] = one;
                                    }
                                } else {
                                    func_00215F80(1, 0x2721);
                                }
                            }
                        }
                    }
                }
            }
        }
    } else if (*(unsigned char *)(m + 0x20) == 1) {
        if (D_L10_0015F6A8 != 2) {
            func_0020BFC8(0, -1);
            func_L00_00264DB8(D_0013D355[0x140] ? 0x271D : 0x271C, -1);
            *(int *)&D_L10_0015F720 = 0xB4;
            *(int *)(d + 4) = func_001F9850(0xF0);
            m[0x20] = 0;
        }
    }
}
