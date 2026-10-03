/* NON_MATCHING func_L00_002420C0 -- src/overlays/shared/loaders_00240398.c
 * Best so far: BYTES 11/96 (88.5% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   CollInit: relocates two header offsets to pointers (global at lui D_L00_00173F40, gp global at 0x15F6F8), then
 *   Everything matches except the store to the gp global: retail has it in the blez delay slot after `lw count; da
 *   Would unblock: knowing how the gp store is declared in retail (likely a type giving no alias with the count lo
 */
extern int D_L00_00173F40[];
extern short D_L00_0015F6F8;

// Relocates the collision header's offsets into pointers.
void func_L00_002420C0(char *p) {
    const int *q;
    int i;
    int *e;

    if (*(int *)p != 0) {
        *D_L00_00173F40 = (int)(p + *(int *)p);
    }
    if (*(int *)(p + 4) != 0) {
        q = (const int *)(p + *(int *)(p + 4));
        *(const int **)&D_L00_0015F6F8 = q;
        e = (int *)q + 7;
        for (i = 0; i < *q; i++) {
            *e = (int)q + *e;
            e += 4;
        }
    }
}
