/* NON_MATCHING func_L04_0024D4A8 -- src/overlays/l04_eudora/mobyutil_0024D4A8.c
 * Best so far: SIZE ours 152 / retail 160, checked 2026-10-09.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   functions.tsv decision for the lead; no candidate can give EXACT against
 *   the current 140-byte symbol.
 *   - The dead `int dead = i % n;` is a construct to get the extra trap. It is
 *   plain C with no barrier, but it has no source-level meaning. A more natural
 *   second `%` that leaves one `div` was not found in this round.
 *   New wall (for LEVERS.md, "Known walls"): a function whose loop tail is a
 *   separate catalogue symbol that its own branch jumps into. Report the split
 *   and stop; the body itself matches.
 */
/* Tests the point against each edge of a polygon (poly holds n vec4 points):
   returns (edge index + 1) of the first edge whose cross product with the
   point is positive, else 0. The loop tail sits in func_L04_0024D534 in
   retail, so this one symbol is longer than retail's 140 bytes. */
int func_L04_0024D4A8(float *p, float *poly, int n) {
    int i;

    for (i = 0; i < n; i++) {
        int k = i + 1;
        float *b = poly + (k % n) * 4;
        float *a = poly + i * 4;
        float cross = (b[0] - a[0]) * (p[1] - a[1]) - (b[1] - a[1]) * (p[0] - a[0]);
        if (0.0f < cross) return k;
    }
    return 0;
}
