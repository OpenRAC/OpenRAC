/* NON_MATCHING func_L02_002DC2C0 -- src/overlays/shared/vendor_002A5218.c
 * Best so far: BYTES 18/208 (91.3% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Draws 4 quads: copies a 32-byte entry of D_L02_001D35E0 (struct of long[4], gives ld/sd) to the stack, fills t
 *   Frame, loop and copy match (p4+: size 208). Only the scheduling of the 9 stores/5 gp loads differs (retail ord
 *   Would need a source store order/local declaration that steers the scheduler; not found in 10 runs.
 */
typedef struct { long f[4]; } V8;
extern V8 D_L02_001D35E0[4];
extern short D_L02_00161C24;
extern short D_L02_00161B44;
extern short D_L02_00161B48;
extern short D_L02_00161B3C;
extern short D_L02_00161B40;
extern short D_L02_00161BAC;
extern int func_001F4868(int);
extern void func_L02_0020BF88(void *, void *, void *, int, int);

// Draws four quads from a table using shared colour/size globals.
void func_L02_002DC2C0(void) {
    int i;
    for (i = 0; i < 4; i++) {
        V8 a;
        int c[4];
        int b[4];
        b[2] = *(int *)&D_L02_00161B44;
        b[3] = *(int *)&D_L02_00161B48;
        c[0] = *(int *)&D_L02_00161C24;
        c[3] = *(int *)&D_L02_00161C24;
        c[2] = *(int *)&D_L02_00161C24;
        c[1] = *(int *)&D_L02_00161C24;
        b[0] = *(int *)&D_L02_00161B3C;
        b[1] = *(int *)&D_L02_00161B40;
        a = D_L02_001D35E0[i];
        func_L02_0020BF88(&a, b, c, func_001F4868(*(int *)&D_L02_00161BAC + 0x28), 1);
    }
}
