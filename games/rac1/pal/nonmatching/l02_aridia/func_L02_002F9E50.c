/* NON_MATCHING func_L02_002F9E50 -- src/overlays/l02_aridia/vendor_002E21F8.c
 * Best so far: BYTES 8/132 (93.9% of the bytes match), checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 */
extern char D_L02_00167300_a[] __asm__("D_L02_00167300");
extern void func_L00_001ED9B0_u(void *, void *) __asm__("func_L00_001ED9B0");

void func_L02_002F9E50(int kind, int val) {
    char *g = D_L02_00167300_a;
    char *o = *(char **)(g + 0x180);
    char *d = *(char **)(o + 0x70);
    char *e = d + 0xE0;
    o[0x88] = kind;
    switch (kind) {
    case 3:
        qcopy(d + 0x110, D_0013E633 + 0xE9D);
        func_L00_001ED9B0_u(d + 0xFC, g + 0x140);
    case 2:
        *(int *)(e + 4) = val;
        *(int *)(d + 0xE0) = val;
        break;
    }
}
