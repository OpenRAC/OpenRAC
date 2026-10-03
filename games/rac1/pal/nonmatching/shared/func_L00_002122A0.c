/* NON_MATCHING func_L00_002122A0 -- src/overlays/shared/help_0020CDF0.c
 * Best so far: BYTES 223/268 (16.8% of the bytes match), checked 2026-10-03.
 * Not built into anything: the retail assembly stays in the source file
 * until a candidate is EXACT (docs/NONMATCHING.md). Start from this one.
 * What the last attempts found:
 *   Clamps/scales the float at D_0013E633+0xE1D+0x190 depending on mode (+0x2084 == 0x3F or 0x73), using D_0015EE6
 *   Best p4 (9/268 bytes): structure, loads, branches all right; only $f register numbering differs (block 1: 3.5 
 *   Unblock: a wording that changes float pseudo numbering; ternary, local x, *=, operand swaps tried.
 */
extern float D_0015EE6C MACRO_ADDR;
extern char D_0013E633[];
extern float D_L00_0017BEB8[];

/* Adjusts the float at +0x190 of the global block according to the mode value at +0x2084. */
void func_L00_002122A0(void) {
    char *g;
    char *h;
    float lim;
    float x;
    func_L00_00211F80(0, 1.0f);
    g = D_0013E633 + 0xE1D;
    if (*(int *)(g + 0x2084) == 0x3F) {
        lim = D_0015EE6C + D_0015EE6C;
        *(float *)(g + 0x190) = D_0015EE6C * 3.5f * *(float *)(g + 0x190);
        if (*(float *)(g + 0x190) < lim) {
            *(float *)(g + 0x190) = lim;
        }
        return;
    }
    x = *(float *)(g + 0x190);
    if (0.0f < x) {
        if (x < D_L00_0017BEB8[3]) {
            x = D_L00_0017BEB8[2];
        } else {
            x = D_L00_0017BEB8[6];
        }
        *(float *)(g + 0x190) = x * D_0015EE6C;
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
