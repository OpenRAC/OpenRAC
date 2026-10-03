/* NON_MATCHING func_L02_001FC308 -- src/overlays/shared/camera_001FC308.c
 * Best so far: SIZE ours 100 / retail 104, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Adds a value to a 16-entry int list (count D_L02_0015F04C, array D_L02_00169670) unless already present.
 *   Best shape is p1/p7 (MACRO_ADDR count, for loop), 44/104 bytes wrong: retail keeps the count in two registers 
 *   Could not find a source form that makes gcc keep the extra copy (tried n=c in/out of the block, n++, do-while,
 */
extern int D_L02_0015F04C;
extern int D_L02_00169670[];

/* Adds a value to a 16-entry list unless it is already present. */
void func_L02_001FC308(int v) {
    int c = D_L02_0015F04C;
    int n;
    int i;
    if (c < 16) {
        n = c;
        for (i = 0; i < c; i++) {
            if (v == D_L02_00169670[i]) return;
        }
        D_L02_00169670[n] = v;
        D_L02_0015F04C = n + 1;
    }
}
