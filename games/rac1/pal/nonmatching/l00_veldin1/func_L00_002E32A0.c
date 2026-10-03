/* NON_MATCHING func_L00_002E32A0 -- src/overlays/l00_veldin1/vendor_002DB278.c
 * Best so far: SIZE ours 300 / retail 308, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   UpdateMoby_1471: switch on moby[0x20]; state 0 zeroes two 3-int arrays (D_L00_00161CC8, D_L00_00161D58), sets 
 *   Structure (switch, unsigned char state, separate loop counter for the zeroing loop, store order) matches; left
 *   Register allocation priority of the loop counter vs the two array pointers; 11 wordings (pointers, index, loca
 */
extern int D_L00_00161D58[3];
extern int D_L00_00161CC8[3];
extern void func_L00_002E35A8(int);
extern void func_L00_002E3700(int, int);
extern void func_L00_002E3FA0(void);
extern void func_001F49B0(void (*)(void), void *);

/* update: clear two slot arrays, then service the pending slots */
void func_L00_002E32A0(unsigned char *moby) {
    int i;
    int n;
    if (moby[0x20] == 0) {
        for (i = 0; i < 3; i++) {
            D_L00_00161CC8[i] = 0;
            D_L00_00161D58[i] = 0;
        }
        moby[0x30] = 0xFF;
        moby[0x20] = 1;
    } else if (moby[0x20] == 1) {
        n = 0;
        for (i = 0; i < 3; i++) {
            int v = D_L00_00161CC8[i];
            if (v != 1) {
                if (v == 4) {
                    func_L00_002E35A8(i);
                } else if (v != 0) {
                    n++;
                    func_L00_002E3700(D_L00_00161D58[i], i);
                    D_L00_00161CC8[i] = 4;
                }
            }
        }
        if (n != 0) {
            func_001F49B0(func_L00_002E3FA0, moby);
        }
    }
}
