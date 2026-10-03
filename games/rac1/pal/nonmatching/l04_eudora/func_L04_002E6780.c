/* NON_MATCHING func_L04_002E6780 -- src/overlays/l04_eudora/vendor_002CB800.c
 * Best so far: SIZE ours 148 / retail 156, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L04_002E6780 (UpdateMoby_1549): state 0 sets state 1 and m[0x30]=0xFF; state 1, when D_L04_0015F6A8 == 2 
 *   Best p2.c (144 of 156 bytes): idx logic (xori/move/movn from the 2 in $4) and the +0x178 folded into the lw ar
 *   Unknown what source gives two separate lo_sums from one hi (tried: char[]/int[] decl, shared local b at two sc
 */
extern int D_L04_0015F6A8;
extern char D_L04_0016CA60[];
extern void func_L00_00264870(int);

// Update: state 0 initialises; state 1 fires a helper with an entry chosen by a global value.
void func_L04_002E6780(char *m) {
    switch (*(unsigned char *)(m + 0x20)) {
    case 0:
        m[0x20] = 1;
        m[0x30] = 0xFF;
        break;
    case 1:
        if (D_L04_0015F6A8 == 2) {
            char *b = D_L04_0016CA60;
            unsigned int v = *(unsigned int *)(b + 0x30);
            if (v < 2) {
                int idx;
                idx = v == 0 ? 3 : (v == 1 ? 2 : 0);
                func_L00_00264870(*(int *)(b + idx * 4 + 0x178));
            }
        }
        break;
    }
}
