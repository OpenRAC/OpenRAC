/* NON_MATCHING func_L00_00235CA0 -- src/overlays/shared/hud_00235960.c
 * Best so far: SIZE ours 372 / retail 380, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   func_L00_00235CA0 (hud table rebase): subtracts segment start offset (idx-th entry at base+0x74) from entries 
 *   Best candidate p7.c (p6.c same bytes): 372 vs retail 380. Structure, jump threading (if/else for start), fresh
 *   Remaining: retail has a delay-slot nop before loop 2's preheader (copy+lui) and recomputes `addiu $3,$13,%lo(D
 */
extern char D_L00_0017E5D8[];

/* Rebases the animation segment IDX in the hud tables: zeroes the first segment's flags, subtracts its start offset from each entry and sets their top bit. */
void func_L00_00235CA0(int idx) {
    char *d = D_L00_0017E5D8;
    int sub = *(int *)(*(char **)(d + 0x18) + idx * 4 + 0x74);
    int i, end, off = 0, start;
    char *e, *e2, *g;

    if (idx == 0) {
        for (i = 0; i < *(int *)(*(char **)(d + 0x18) + 0x34); i++) {
            *(short *)(*(char **)(d + 0x24) + i * 8 + 4) = 0;
        }
    }
    if (idx != 0) {
        char *d2 = D_L00_0017E5D8;
        start = *(int *)(*(char **)(d2 + 0x18) + (idx - 1) * 4 + 0x14);
        off = idx * 4;
    } else {
        start = 0;
    }
    i = start;
    e = D_L00_0017E5D8;
    end = *(int *)(*(char **)(e + 0x18) + off + 0x14);
    for (; i < end; i++) {
        char *f = D_L00_0017E5D8;
        *(int *)(*(char **)(f + 0x28) + i * 8) -= sub;
        *(int *)(*(char **)(f + 0x28) + i * 8) |= 0x80000000;
    }
    if (idx != 0) {
        char *d4 = D_L00_0017E5D8;
        start = *(int *)(*(char **)(d4 + 0x18) + (idx - 1) * 4 + 0x34);
    } else {
        start = 0;
    }
    i = start;
    e2 = D_L00_0017E5D8;
    end = *(int *)(*(char **)(e2 + 0x18) + off + 0x34);
    for (; i < end; i++) {
        char *f = D_L00_0017E5D8;
        *(int *)(*(char **)(f + 0x24) + i * 8) -= sub;
        *(int *)(*(char **)(f + 0x24) + i * 8) |= 0x80000000;
    }
    g = D_L00_0017E5D8;
    *(int *)(*(char **)(g + 0x18) + off + 0x74) = 0;
}
