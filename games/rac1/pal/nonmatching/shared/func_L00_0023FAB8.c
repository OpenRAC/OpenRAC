/* NON_MATCHING func_L00_0023FAB8 -- src/overlays/shared/lights_0023FA70.c
 * Best so far: BYTES 16/184 (91.3% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Light-table loader: stores p+16 and count (p[0]) to D_L00_0015F53C/F538, the table pointer p+16+count*32 to D_
 *   Budget spent at p3/p4/p9 (BYTES 16-20/184, same size; all three globals `int ... MACRO_ADDR`, separate counter
 */
extern int D_L00_0015F53C MACRO_ADDR;
extern int D_L00_0015F538 MACRO_ADDR;
extern int D_L00_0015F540 MACRO_ADDR;

// Relocates a light table: stores header fields and rebases its pointer tables.
void func_L00_0023FAB8(int *p) {
    int i, j;
    int n;
    n = p[0];
    p += 4;
    D_L00_0015F53C = (int)p;
    D_L00_0015F538 = n;
    D_L00_0015F540 = (int)(p + n * 8);
    for (i = 0; i < D_L00_0015F538; i++) {
        ((int *)D_L00_0015F540)[i] += D_L00_0015F540;
    }
    for (j = 0; j < D_L00_0015F538; j++) {
        int *q = ((int **)D_L00_0015F540)[j];
        q[1] += D_L00_0015F540;
    }
}
