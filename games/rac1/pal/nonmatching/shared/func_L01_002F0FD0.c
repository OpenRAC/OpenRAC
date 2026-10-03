/* NON_MATCHING func_L01_002F0FD0 -- src/overlays/shared/vendor_002B90A8.c
 * Best so far: SIZE ours 100 / retail 104, checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Scans an int array downward from index n-1 for the first slot below the global D_L01_0015F6B0, stores global+a
 *   Budget spent (10 runs). Best shapes: 92 bytes (p0/p9) merge the two stores into one shared sw; retail keeps th
 *   Unblock: a wording that keeps the first-arm store un-merged while the loop still recomputes sll+addu each pass
 */
extern int D_L01_0015F6B0 MACRO_ADDR;

/* Stores the global plus an offset into the last array slot below it, scanning down. */
int func_L01_002F0FD0(int *arr, int n, int add) {
    int v;
    n--;
    if (n >= 0) {
        int *p;
        v = D_L01_0015F6B0;
        p = arr + n;
        if (*p < v) {
            *p = v + add;
            return n;
        }
        n--;
        while (n >= 0) {
            p = arr + n;
            if (*p < v) {
                *p = v + add;
                break;
            }
            n--;
        }
    }
    return n;
}
