/* NON_MATCHING func_L05_0031AA20 -- src/overlays/shared/vendor_002CF2C0.c
 * Best so far: SIZE ours 136 / retail 132, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Spawns an object for slot d[0xB6], copies two 16-byte vectors from a level table (index<<7 + D_L05_001601AC) i
 *   Best is p6.c/p8.c/p9.c (BYTES 11/132): all instructions right except the first block's schedule: retail puts a
 *   Local for r+0x10, g declared before/after the index: same bytes. Base-first addu came from a local `off`; `&g[
 */
extern char *func_L05_0031AAA8(void *, int);
extern void func_L05_0031A718(void *);
extern char *D_L05_001601AC;

/* Spawns a child for the current slot, copies the slot's two vectors into it, then advances the slot. */
void func_L05_0031AA20(char *m) {
    char *d = *(char **)(m + 0x78);
    char *r = func_L05_0031AAA8(m, *(short *)(d + 0xB6));
    if (r != 0) {
        int k = *(int *)(d + *(short *)(d + 0xB4) * 4 + 0x80) << 7;
        qcopy(r + 0x10, D_L05_001601AC + k + 0x30);
        qcopy(r + 0x40, D_L05_001601AC + k + 0x70);
        func_L05_0031A718(r);
    }
    *(short *)(d + 0xB4) = *(unsigned short *)(d + 0xB6);
}
