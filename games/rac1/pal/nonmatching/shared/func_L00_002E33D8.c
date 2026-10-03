/* NON_MATCHING func_L00_002E33D8 -- src/overlays/shared/vendor_002E1660.c
 * Best so far: SIZE ours 284 / retail 276, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Slot allocator for a three-entry parallel-array table (D_L00_00161D58 holds the key): returns 1 if the key (a0
 *   Control flow and instruction count of the loops match, but the allocator differs: retail keeps a0..a2 in $4-$6
 *   Tried: return shapes, separate index vars, reversed store order, operand swap, continue form, array sizes, inl
 */
extern int D_L00_00161D58[3];
extern int D_L00_00161CC8[3];
extern int D_L00_00161D48[3];
extern int D_L00_00161D38[3];
extern int D_L00_00161D28[3];
extern int D_L00_00161D18[3];
extern int D_L00_00161D08[3];
extern int D_L00_00161CF8[3];
extern int D_L00_00161CE8[3];
extern int D_L00_00161CD8[3];

/* find or allocate a slot in a three-entry table; returns 1 on success */
int func_L00_002E33D8(int a, int b, int c) {
    int i;
    for (i = 0; i < 3; i++) {
        if (a == D_L00_00161D58[i]) return 1;
    }
    for (i = 0; i < 3; i++) {
        if (D_L00_00161D58[i] == 0) {
            D_L00_00161D58[i] = a;
            D_L00_00161D18[i] = -1;
            D_L00_00161D28[i] = b;
            D_L00_00161D38[i] = c;
            D_L00_00161CC8[i] = 1;
            D_L00_00161CD8[i] = 0;
            D_L00_00161CE8[i] = 0;
            D_L00_00161CF8[i] = 0;
            D_L00_00161D08[i] = -1;
            D_L00_00161D48[i] = 0;
            return 1;
        }
    }
    return 0;
}
