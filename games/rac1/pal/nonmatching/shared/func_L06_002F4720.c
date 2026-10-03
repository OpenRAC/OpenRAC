/* NON_MATCHING func_L06_002F4720 -- src/overlays/shared/vendor_002D9548.c
 * Best so far: BYTES 9/244 (96.3% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Walks a candidate list for a moby: if data+0x30C != -1 it iterates with func_L00_0025A208/2A2F0 (cursor in st[
 *   p2 reaches 9/244 bytes: everything matches except the scheduling of `sw $zero,4($sp)` in the prologue (retail:
 *   Loops are `if (v) do { ... } while (v)` with v reloaded each iteration. Would need a wording that delays the r
 */
extern int func_L00_0025A208(int *, int, int, int);
extern int func_L06_002F4058(char *, char *, int, char *, int *);
extern int func_L00_0025A2F0(int *, int, int, int);
extern int D_L06_001AC500[];

// Tries each candidate target for a moby until one is accepted; returns the result.
int func_L06_002F4720(char *moby, char *arg) {
    int cur;
    int out = 0;
    char *data = *(char **)(moby + 0x78);
    if (*(int *)(data + 0x30C) != -1) {
        func_L00_0025A208(&cur, *(int *)(data + 0x30C), 0, 0);
        if (cur != 0) {
            do {
                if (func_L06_002F4058(moby, data, cur, arg, &out) != 0) break;
                func_L00_0025A2F0(&cur, cur, 0, 0);
            } while (cur != 0);
        }
    } else {
        int *p = D_L06_001AC500;
        int v = *p;
        if (v != 0) {
            do {
                if (func_L06_002F4058(moby, data, v, arg, &out) != 0) break;
                p++;
                v = *p;
            } while (v != 0);
        }
    }
    return out;
}
