/* NON_MATCHING func_L14_00304E68 -- src/overlays/shared/vendor_002B2A28.c
 * Best so far: SIZE ours 344 / retail 336, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L14_00304E68 (UpdateMoby_1331): state 0 clears two 3-int tables (D_L14_00162218 states, D_L14_001622A8 mo
 *   p5.c: layout, branches and every instruction match; 69 bytes differ only by register numbers (retail m=$s4, cn
 *   Declaration order does not move it (p5/p6/p7 identical); an allocator priority tie that rewording did not move
 */
extern void func_L00_002E35A8(int);
extern void func_L00_002E3700(int, int);
extern void func_L00_002E3FA0(void);
extern void func_001F49B0(void *, void *);
extern int D_L14_001622A8[];
extern int D_L14_00162218[];

/* update for the three-slot spawner object */
void func_L14_00304E68(char *m) {
    int i, cnt;
    switch (m[0x20]) {
    case 0:
        for (i = 2; i >= 0; i--) {
            D_L14_00162218[i] = 0;
            D_L14_001622A8[i] = 0;
        }
        m[0x30] = 0xFF;
        m[0x20] = 1;
        break;
    case 1:
        cnt = 0;
        for (i = 0; i < 3; i++) {
            int s = D_L14_00162218[i];
            if (s == 1) {
                if (((char *)D_L14_001622A8[i])[0x20] < 0) {
                    D_L14_00162218[i] = 0;
                    D_L14_001622A8[i] = 0;
                }
            } else if (s == 4) {
                func_L00_002E35A8(i);
            } else if (s != 0) {
                func_L00_002E3700(D_L14_001622A8[i], i);
                cnt++;
                D_L14_00162218[i] = 4;
            }
        }
        if (cnt != 0) func_001F49B0(func_L00_002E3FA0, m);
        break;
    }
}
