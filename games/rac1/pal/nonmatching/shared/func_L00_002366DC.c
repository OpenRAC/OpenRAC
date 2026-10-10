/* NON_MATCHING func_L00_002366DC -- src/overlays/shared/hud_00235960.c
 * Best so far: BYTES 3/48 (93.8% of the bytes match), checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Fragment: starts with slti on $9, a register never set inside the function (and uses $5 and $4 as if mid-funct
 *   - hq2 s11: `if (n < 12) p[0x5C] = 12; p[0x58] += (a + 1) * 14;` (6 args: $4..$9). Plain p0 is 44/48: retail ha
 *   - A volatile read-modify-write of 0x58 (p1, p3, p6) fixes the store order (3/48 bytes), but then the slti resu
 *   - Unblock: a form where the compare temp is fresh while the 0x58 store is not a delay-slot filler (the allocat
 */
// Adds (a + 1) * 14 to the field at 0x58 and sets 0x5C to 12 while n is below 12.
void func_L00_002366DC(int a, char *p, int b, int c, int d, int n) {
    int *q = (int *)(p + 0x58);
    if (n < 12) {
        *(int *)(p + 0x5C) = 12;
    }
    *(volatile int *)q = *q + (a + 1) * 14;
}
