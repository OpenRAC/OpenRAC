/* NON_MATCHING func_L01_00317408 -- src/overlays/shared/vendor_002F7700.c
 * Best so far: BYTES 31/132 (76.5% of the bytes match), checked 2026-10-07.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Stop reason: three or more variants in a row compile to the same bytes
 *   (p3-p5, then p6/p8). That is a scheduler tie at the head, which wording did
 *   not move. Known wall in LEVERS terms: none of the listed walls. The
 *   scheduler and register choice are the open question. A later idea would be
 *   to test whether retail's lui(g)-first order can be forced by a different
 *   first use of g, but that was not tried.
 *   Not run: `-mno-split-addresses` (WORKER step 6). The difference is the
 *   schedule, not `%hi` handling, so it doesn't apply here.
 */
/* ExitCamera_3: when the camera record matches, sets the moby's state short to 3 or 5.
 * The early exits and the store-3 path share one trailing return (retail's func_001E9768 piece). */
extern char *D_L01_0015F050;
extern char *D_L01_0015F7EC MACRO_ADDR;

void func_L01_00317408(char *m) {
    char *g = D_0013E633 + 0xE1D;
    int cam = *(int *)(g + 0x2284);
    char *q = *(char **)(D_L01_0015F050 + (*(short *)(m + 0x84) << 5) + 0x1C);
    if (cam == *(short *)(m + 0x86)) {
        int v = *(int *)(q + 0x24);
        if (v < 0) goto out;
        if (*(int *)(g + 0x560) == *(int *)(D_L01_0015F7EC + (v << 5) + 0x10)) {
            if (*(int *)(g + 0x570) == 0) goto out;
        }
    }
    if (*(int *)(q + 0x30) == 3) {
        *(short *)(m + 0x7E) = 5;
        return;
    }
    *(short *)(m + 0x7E) = 3;
out:
    return;
}
