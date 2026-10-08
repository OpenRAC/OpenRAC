/* NON_MATCHING func_L00_002122A0 -- src/overlays/shared/help_0020CDF0.c
 * Best so far: BYTES 7/268 (97.4% of the bytes match), checked 2026-10-05.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Clamps/scales the float at D_0013E633+0xE1D+0x190 depending on mode (+0x2084 == 0x3F or 0x73), using D_0015EE6
 *   Best p4 (9/268 bytes): structure, loads, branches all right; only $f register numbering differs (block 1: 3.5 
 *   Unblock: a wording that changes float pseudo numbering; ternary, local x, *=, operand swaps tried.
 *   q27 s12: best p13.c BYTES 7/268 (6 runs). Fixed the call signature (float,int); function-scope t with D*t made
 *   Left: float regs only: block 1 (x and 3.5 const f1/f2 swapped), block 2 (t in f1 vs retail f0, D in f0 vs f1).
 */
extern void func_L00_00211F80(float, int);
extern float D_0015EE6C MACRO_ADDR;
extern char D_0013E633[];
extern float D_L00_0017BEB8[];

/* Adjusts the float at +0x190 of the global block according to the mode value at +0x2084. */
void func_L00_002122A0(void) {
    char *g;
    char *h;
    float t;
    float lim;
    func_L00_00211F80(1.0f, 0);
    g = D_0013E633 + 0xE1D;
    if (*(int *)(g + 0x2084) == 0x3F) {
        lim = D_0015EE6C + D_0015EE6C;
        *(float *)(g + 0x190) = D_0015EE6C * 3.5f * *(float *)(g + 0x190);
        if (*(float *)(g + 0x190) < lim) {
            *(float *)(g + 0x190) = lim;
        }
        return;
    }
    if (0.0f < *(float *)(g + 0x190)) {
        if (*(float *)(g + 0x190) < D_L00_0017BEB8[3]) {
            t = D_L00_0017BEB8[2];
        } else {
            t = D_L00_0017BEB8[6];
        }
        *(float *)(g + 0x190) = D_0015EE6C * t;
    }
    h = D_0013E633 + 0xE1D;
    if (*(int *)(h + 0x2084) == 0x73) {
        *(float *)(h + 0x190) = *(float *)(h + 0x190) * 0.8f;
        lim = D_0015EE6C * 2.5f;
        if (*(float *)(h + 0x190) < lim) {
            *(float *)(h + 0x190) = lim;
        }
    }
}
