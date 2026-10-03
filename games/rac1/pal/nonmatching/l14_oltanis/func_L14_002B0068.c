/* NON_MATCHING func_L14_002B0068 -- src/overlays/l14_oltanis/vendor_002ACCC0.c
 * Best so far: SIZE ours 260 / retail 252, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Fills a 20-entry id table (D_L14_001D8980) with -1, then for each of the owner's count entries calls func_L14_
 *   p3/p4 are the same size (252) with everything right except the repeated `lui`: retail recomputes hi(D_L14_001D
 *   Looks like a per-function flag / CSE-path difference (repeated lui for one symbol); reworded 6 ways (array vs 
 */
extern int D_L14_001D8980[20];
extern void func_L14_002AEF88(char *);
extern void func_L14_002AEC58(char *);

/* Collects the ids of the owned mobys into a table and runs their handlers. */
void func_L14_002B0068(char *moby) {
    char *data = *(char **)(moby + 0x78);
    int i;
    int *p;
    int *list;
    int *q = D_L14_001D8980;
    for (i = 19; i >= 0; i--) {
        q[i] = -1;
    }
    list = D_L14_001D8980;
    p = list;
    for (i = 0; i < *(short *)(data + 0x20A); i++) {
        char *m = func_L14_002B0168(((unsigned char *)moby)[0x21], list);
        char *md;
        *p++ = *(short *)(m + 0xB2);
        md = *(char **)(m + 0x78);
        if (*(int *)(md + 0x20C) & 8) {
            func_L14_002AEF88(m);
        }
        if (*(int *)(md + 0x20C) & 2) {
            func_L14_002AEC58(m);
        }
    }
}
